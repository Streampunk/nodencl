/* Copyright 2018 Streampunk Media Ltd.

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
*/

const addon = require('../index.js');
const v210_io = require('./v210_io.js');

const SegfaultHandler = require('segfault-handler');
SegfaultHandler.registerHandler("crash.log"); // With no argument, SegfaultHandler will generate a generic log file name

function dumpFloatBuf(buf, width, numLines) {
  for (let y=0; y<numLines; ++y) {
    const off = y*width*4*4;
    console.log(`Line ${y}: ${buf.readFloatLE(off+0).toFixed(4)}, ${buf.readFloatLE(off+4).toFixed(4)}, ${buf.readFloatLE(off+8).toFixed(4)}, ${buf.readFloatLE(off+12).toFixed(4)}, ${buf.readFloatLE(off+16).toFixed(4)}, ${buf.readFloatLE(off+20).toFixed(4)}, ${buf.readFloatLE(off+24).toFixed(4)}, ${buf.readFloatLE(off+28).toFixed(4)}`);
  }
}

async function noden() {
  const platformIndex = 1;
  const deviceIndex = 0;
  const platformInfo = addon.getPlatformInfo()[platformIndex];
  // console.log(JSON.stringify(platformInfo, null, 2));
  console.log(platformInfo.vendor, platformInfo.devices[deviceIndex].type);

  const context = await addon.createContext({
    platformIndex: platformIndex, 
    deviceIndex: deviceIndex
  });

  const colSpecRead = '709';
  const colSpecWrite = '709';
  const width = 1920;
  const height = 1080;

  const v210Reader = await new v210_io.reader(context, width, height, colSpecRead, colSpecWrite);
  await v210Reader.init();

  const v210Writer = await new v210_io.writer(context, width, height, colSpecWrite);
  await v210Writer.init();

  const numBytesV210 = v210_io.getPitchBytes(width) * height;
  const v210Src = await context.createBuffer(numBytesV210, 'readonly', 'coarse');

  const numBytesRGBA = width * height * 4 * 4;
  const rgbaDst = await context.createBuffer(numBytesRGBA, 'readwrite', 'coarse');
  
  const v210Dst = await context.createBuffer(numBytesV210, 'writeonly', 'coarse');

  let v210SrcBuf = await v210Src.hostAccess('writeonly');
  v210_io.fillBuf(v210SrcBuf, width, height);
  v210_io.dumpBuf(v210SrcBuf, width, 4);
  v210SrcBuf.release();

  let timings = await v210Reader.fromV210(v210Src, rgbaDst);
  console.log(`${timings.dataToKernel}, ${timings.kernelExec}, ${timings.dataFromKernel}, ${timings.totalTime}`);

  let rgbaDstBuf = await rgbaDst.hostAccess('readonly');
  dumpFloatBuf(rgbaDstBuf, width, 4);
  rgbaDstBuf.release();

  timings = await v210Writer.toV210(rgbaDst, v210Dst);
  console.log(`${timings.dataToKernel}, ${timings.kernelExec}, ${timings.dataFromKernel}, ${timings.totalTime}`);

  let v210DstBuf = await v210Dst.hostAccess('readonly');
  v210_io.dumpBuf(v210DstBuf, width, 4);

  v210SrcBuf = await v210Src.hostAccess('readonly');
  console.log('Compare returned', v210SrcBuf.compare(v210DstBuf));

  v210SrcBuf.release();
  v210DstBuf.release();

  return [v210Src, v210Dst];
}
noden()
  .then(([i, o]) => { return [i.creationTime, o.creationTime] })
  .then(([ict, oct]) => { if (global.gc) global.gc(); console.log(ict, oct); })
  .catch( console.error );
