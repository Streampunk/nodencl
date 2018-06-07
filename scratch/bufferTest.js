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

function ycbcr2rgbMatrix(matrix, colourspace, numBits) {
  let kR, kG, kB;
  switch (colourspace) {
  default:
  case '709':  // https://www.itu.int/dms_pubrec/itu-r/rec/bt/R-REC-BT.709-6-201506-I!!PDF-E.pdf
    kR = 0.2126;
    kB = 0.0722;
    break;
  case '2020': // https://www.itu.int/dms_pubrec/itu-r/rec/bt/R-REC-BT.2020-2-201510-I!!PDF-E.pdf
    kR = 0.2627;
    kB = 0.0593;
    break;
  case '601':  // https://www.itu.int/dms_pubrec/itu-r/rec/bt/R-REC-BT.601-7-201103-I!!PDF-E.pdf
    kR = 0.299;
    kB = 0.114;
    break;
  }
  kG = 1.0 - kR - kB;

  const kLumaBlack = 16.0 << (numBits - 8);
  const kLumaWhite = 235.0 << (numBits - 8);
  const kChromaNull = 128.0 << (numBits - 8);
  const kLumaRange = kLumaWhite - kLumaBlack;
  const kChrRange = 224 << (numBits - 8);

  const Yr = 1.0 / kLumaRange;
  const Ur = 0.0;
  const Vr = (1.0 - kR) / kChrRange / 2;
  const Or = - kLumaBlack * Yr - kChromaNull * Ur - kChromaNull * Vr;

  const Yg = 1.0 / kLumaRange;
  const Ug = -(1.0 - kB) * kB / kG / kChrRange / 2;
  const Vg = -(1.0 - kR) * kR / kG / kChrRange / 2;
  const Og = - kLumaBlack * Yg - kChromaNull * Ug - kChromaNull * Vg;

  const Yb = 1.0 / kLumaRange;
  const Ub = (1.0 - kB) / kChrRange / 2;
  const Vb = 0.0;
  const Ob = - kLumaBlack * Yb - kChromaNull * Ub - kChromaNull * Vb;

  const matrixArray = [
    Yr, Ur, Vr, Or,
    Yg, Ug, Vg, Og,
    Yb, Ub, Vb, Ob,
    0.0, 0.0, 0.0, 1.0];

  const matrixTypedArray = Float32Array.from(matrixArray);
  Buffer.from(matrixTypedArray.buffer).copy(matrix);
}

const kernel = `__kernel void 
  convert(__global uint4* input,
          __global float4* output,
          __private unsigned int width,
          __constant float4* matrix) {
    uint item = get_global_id(0);
    bool lastItemOnLine = get_local_id(0) == get_local_size(0) - 1;

    // 48 pixels per workItem = 8 input uint4s per work item
    uint numPixels = lastItemOnLine ? width % 48 : 48;
    uint numLoops = numPixels / 6;
    uint remain = width % 6;

    uint inOff = 8 * item;

    // 48 pixels per workItem = 48 output float4s per work item
    uint outOff = 48 * item;

    if (48 != numPixels) {
      // clear the output buffer for the last item, partially overwritten below
      uint clearOff = outOff;
      for (uint i=0; i<48; ++i)
        output[clearOff++] = (float4)(0.0, 0.0, 0.0, 0.0);
    }

    float4 matR = matrix[0];
    float4 matG = matrix[1];
    float4 matB = matrix[2];
    float4 matA = matrix[3];

    for (uint i=0; i<numLoops; ++i) {
      uint4 w = input[inOff];

      uint4 yuva[6];
      yuva[0] = (uint4)((w.s0 >> 10) & 0x3ff, w.s0 & 0x3ff, (w.s0 >> 20) & 0x3ff, 0);
      yuva[1] = (uint4)(w.s1 & 0x3ff, yuva[0].s1, yuva[0].s2, 0);
      yuva[2] = (uint4)((w.s1 >> 20) & 0x3ff, (w.s1 >> 10) & 0x3ff, w.s2 & 0x3ff, 0);
      yuva[3] = (uint4)((w.s2 >> 10) & 0x3ff, yuva[2].s1, yuva[2].s2, 0);
      yuva[4] = (uint4)(w.s3 & 0x3ff, (w.s2 >> 20) & 0x3ff, (w.s3 >> 10) & 0x3ff, 0);
      yuva[5] = (uint4)((w.s3 >> 20) & 0x3ff, yuva[4].s1, yuva[4].s2, 0);

      for (uint i=0; i<6; ++i) {
        float4 rgba;
        rgba.s0 = matR.s0 * (float)yuva[i].s0 + matR.s1 * (float)yuva[i].s1 + matR.s2 * (float)yuva[i].s2 + matR.s3;
        rgba.s1 = matG.s0 * (float)yuva[i].s0 + matG.s1 * (float)yuva[i].s1 + matG.s2 * (float)yuva[i].s2 + matG.s3;
        rgba.s2 = matB.s0 * (float)yuva[i].s0 + matB.s1 * (float)yuva[i].s1 + matB.s2 * (float)yuva[i].s2 + matB.s3;
        rgba.s3 = 0.0f;
        output[outOff+i] = rgba;
      }

      inOff++;
      outOff+=6;
    }

    if (remain > 0) {
      uint4 w = input[inOff];

      uint4 yuva[4];
      yuva[0] = (uint4)((w.s0 >> 10) & 0x3ff, w.s0 & 0x3ff, (w.s0 >> 20) & 0x3ff, 0);
      yuva[1] = (uint4)(w.s1 & 0x3ff, yuva[0].s1, yuva[0].s2, 0);

      if (4 == remain) {
        yuva[2] = (uint4)((w.s1 >> 20) & 0x3ff, (w.s1 >> 10) & 0x3ff, w.s2 & 0x3ff, 0);
        yuva[3] = (uint4)((w.s2 >> 10) & 0x3ff, yuva[2].s1, yuva[2].s2, 0);
      }

      for (uint i=0; i<remain; ++i) {
        float4 rgba = (float4)(0, 0, 0, 0);
        rgba.s0 = matR.s0 * (float)yuva[i].s0 + matR.s1 * (float)yuva[i].s1 + matR.s2 * (float)yuva[i].s2 + matR.s3;
        rgba.s1 = matG.s0 * (float)yuva[i].s0 + matG.s1 * (float)yuva[i].s1 + matG.s2 * (float)yuva[i].s2 + matG.s3;
        rgba.s2 = matB.s0 * (float)yuva[i].s0 + matB.s1 * (float)yuva[i].s1 + matB.s2 * (float)yuva[i].s2 + matB.s3;
        rgba.s3 = 0.0f;
        output[outOff+i] = rgba;
      }
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
  const Y=0x040;
  const Cb=0x200;
  const Cr=0x200;
  let yOff = 0;
  for (let y=0; y<height; ++y) {
    let xOff = 0;
    for (let x=0; x<(width-width%6)/6; ++x) {
      buf.writeUInt32LE((Cr<<20) | (Y<<10) | Cb, yOff + xOff);
      buf.writeUInt32LE((Y<<20) | (Cb<<10) | Y, yOff + xOff + 4);
      buf.writeUInt32LE((Cb<<20) | (Y<<10) | Cr, yOff + xOff + 8);
      buf.writeUInt32LE((Y<<20) | (Cr<<10) | Y, yOff + xOff + 12);
      xOff += 16;
    }

    const remain = width%6;
    if (remain) {
      buf.writeUInt32LE((Cr<<20) | (Y<<10) | Cb, yOff + xOff);
      if (2 === remain) {
        buf.writeUInt32LE(Y, yOff + xOff + 4);
      } else if (4 === remain) {      
        buf.writeUInt32LE((Y<<20) | (Cb<<10) | Y, yOff + xOff + 4);
        buf.writeUInt32LE((Y<<10) | Cr, yOff + xOff + 8);
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
  const numBytesV210 = getV210PitchBytes(width) * height;
  const numBytesRGBA = width * height * 4 * 4;

  // process one image line per work group, 48 pixels per work item
  const workItemsPerGroup = (width + 47 - ((width - 1) % 48)) / 48;
  const globalWorkItems = workItemsPerGroup * height;

  let program = await addon.createProgram(kernel, {
    platformIndex: platformIndex, 
    deviceIndex: deviceIndex,
    globalWorkItems: globalWorkItems,
    workItemsPerGroup: workItemsPerGroup
  });

  let [i, o, m] = await Promise.all([
    program.createBuffer(numBytesV210, 'in', 'coarse'),
    program.createBuffer(numBytesRGBA, 'out', 'coarse'),
    program.createBuffer(64, 'in', 'param')
  ]);

  o.fill(0);
  fillV210Buf(i, width, height);
  dumpV210Buf(i, width, 4);

  ycbcr2rgbMatrix(m, '709', 10);

  for (let x = 0; x < 10; x++) {
    let timings = await program.run({input: i, output: o, width: width, matrix: m});
    console.log(`${timings.dataToKernel}, ${timings.kernelExec}, ${timings.dataFromKernel}, ${timings.totalTime}`);
    // [o, i] = [i, o];
  }

  for (let y=0; y<4; ++y) {
    const off = y*width*4*4;
    console.log(o.readFloatLE(off+0).toFixed(4), o.readFloatLE(off+4).toFixed(4), o.readFloatLE(off+8).toFixed(4), o.readFloatLE(off+12).toFixed(4));
  }

  return [i, o];
}
noden().then(([i, o]) => { console.log(i.creationTime, o.creationTime); }, console.error);
