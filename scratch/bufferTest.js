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
const SegfaultHandler = require('segfault-handler');
SegfaultHandler.registerHandler("crash.log"); // With no argument, SegfaultHandler will generate a generic log file name

const kernel = `__kernel void 
  convert(__global uint4* input,
          __global uint4* output) {
    int item = get_global_id(0);

    // 48 pixels per workItem = 128 input bytes per work item = 8 input uint4s per work item
    int inOff = 8 * item;

    // output the same for now
    int outOff = 8 * item;

    for (int i=0; i<8; ++i) {
      uint4 w = input[inOff];

      ushort Cb0 =  w.s0        & 0x3ff;
      ushort Y0  = (w.s0 >> 10) & 0x3ff;
      ushort Cr0 = (w.s0 >> 20) & 0x3ff;
      ushort Y1  =  w.s1        & 0x3ff;

      ushort Cb2 = (w.s1 >> 10) & 0x3ff;
      ushort Y2  = (w.s1 >> 20) & 0x3ff;
      ushort Cr2 =  w.s2        & 0x3ff;
      ushort Y3  = (w.s2 >> 10) & 0x3ff;

      ushort Cb4 = (w.s2 >> 20) & 0x3ff;
      ushort Y4  =  w.s3        & 0x3ff;
      ushort Cr4 = (w.s3 >> 10) & 0x3ff;
      ushort Y5  = (w.s3 >> 20) & 0x3ff;

      uint4 r;
      r.s0 = Cr0 << 20 | Y0  << 10 | Cb0;
      r.s1 = Y2  << 20 | Cb2 << 10 | Y1;
      r.s2 = Cb4 << 20 | Y3  << 10 | Cr2;
      r.s3 = Y5  << 20 | Cr4 << 10 | Y4;
      output[outOff] = r;

      inOff++;
      outOff++;
    }
  }
`;

function getV210PitchBytes(width) {
  const pitchWidth = width + 47 - ((width - 1) % 48);
  return pitchWidth * 8 / 3;
}

function fillV210Buf(buf, width, height) {
  const pitchBytes = getV210PitchBytes(width);
  buf.fill(0);
  let yOff = 0;
  for (let y=0; y<height; ++y) {
    let xOff = 0;
    for (let x=0; x<(width-width%6)/6; ++x) {
      buf.writeUInt32LE((0x200<<20) | (0x040<<10) | 0x200, yOff + xOff);
      buf.writeUInt32LE((0x040<<20) | (0x200<<10) | 0x040, yOff + xOff + 4);
      buf.writeUInt32LE((0x200<<20) | (0x040<<10) | 0x200, yOff + xOff + 8);
      buf.writeUInt32LE((0x040<<20) | (0x200<<10) | 0x040, yOff + xOff + 12);
      xOff += 16;
    }

    const remain = width%6;
    if (remain) {
      buf.writeUInt32LE((0x200<<20) | (0x040<<10) | 0x200, yOff + xOff);
      if (2 === remain) {
        buf.writeUInt32LE(0x040, yOff + xOff + 4);
      } else if (4 === remain) {      
        buf.writeUInt32LE((0x040<<20) | (0x200<<10) | 0x040, yOff + xOff + 4);
        buf.writeUInt32LE((0x040<<10) | 0x200, yOff + xOff + 8);
      }
    }
    yOff += pitchBytes;
  }   
  return buf;
}

function dumpV210Buf(buf, width, numLines) {
  let lineOff = 0;
  function getHex(off) { return buf.readUInt32LE(lineOff + off).toString(16) }

  const pitch = getV210PitchBytes(width);
  for (let l = 0; l < numLines; ++l) {
    lineOff = l * pitch;
    console.log(`Line ${l}: ${getHex(0)}, ${getHex(4)}, ${getHex(8)}, ${getHex(12)}, ${getHex(16)}, ${getHex(20)}, ${getHex(24)}, ${getHex(28)}`);
  }
}

async function noden() {
  const platformIndex = 0;
  const deviceIndex = 0;
  const platformInfo = addon.getPlatformInfo()[platformIndex];
  console.log(platformInfo.vendor, platformInfo.devices[deviceIndex].type);

  const width = 1920;
  const height = 1080;
  const numBytes = getV210PitchBytes(width) * height;

  // process one image line per work group, 48 pixels per work item
  const workItemsPerGroup = (width + 47 - ((width - 1) % 48)) / 48;
  const globalWorkItems = workItemsPerGroup * height;

  let program = await addon.createProgram(kernel, {
    platformIndex: platformIndex, 
    deviceIndex: deviceIndex,
    globalWorkItems: globalWorkItems,
    workItemsPerGroup: workItemsPerGroup
    });
  let [i, o] = await Promise.all([
    program.createBuffer(numBytes, 'coarse'),
    program.createBuffer(numBytes, 'coarse')
  ]);

  o.fill(0);
  fillV210Buf(i, width, height);
  dumpV210Buf(i, width, 4);
  // console.log(i);
  for (let x = 0; x < 10; x++) {
    let timings = await program.run(i, o);
    console.log(`${timings.dataToKernel}, ${timings.kernelExec}, ${timings.dataFromKernel}, ${timings.totalTime}`);
    [o, i] = [i, o];
  }

  dumpV210Buf(o, width, 4);
  // console.log(o);
  console.log('Compare result:', i.compare(o));
  return [i, o];
}
noden().then(([i, o]) => { console.log(i.creationTime, o.creationTime); }, console.error);
