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

const colParams = {
  '601-625': { // https://www.itu.int/dms_pubrec/itu-r/rec/bt/R-REC-BT.601-7-201103-I!!PDF-E.pdf
    kR: 0.299, kB: 0.114,
    rx: 0.640, ry: 0.330,
    gx: 0.290, gy: 0.600,
    bx: 0.150, by: 0.060,
    wx: 0.3127, wy: 0.3290,
    alpha: 1.099,
    beta: 0.018,
    gamma: 0.45,
    delta: 4.5
  },
  '601_525': { // https://www.itu.int/dms_pubrec/itu-r/rec/bt/R-REC-BT.601-7-201103-I!!PDF-E.pdf
    kR: 0.299, kB: 0.114,
    rx: 0.630, ry: 0.340,
    gx: 0.310, gy: 0.595,
    bx: 0.155, by: 0.070,
    wx: 0.3127, wy: 0.3290,
    alpha: 1.099,
    beta: 0.018,
    gamma: 0.45,
    delta: 4.5
  },
  '709': { // https://www.itu.int/dms_pubrec/itu-r/rec/bt/R-REC-BT.709-6-201506-I!!PDF-E.pdf
    kR: 0.2126, kB: 0.0722,
    rx: 0.640, ry: 0.330,
    gx: 0.300, gy: 0.600,
    bx: 0.150, by: 0.060,
    wx: 0.3127, wy: 0.3290,
    alpha: 1.099,
    beta: 0.018,
    gamma: 0.45,
    delta: 4.5
  },
  '2020': { // https://www.itu.int/dms_pubrec/itu-r/rec/bt/R-REC-BT.2020-2-201510-I!!PDF-E.pdf
    kR: 0.2627, kB: 0.0593,
    rx: 0.708, ry: 0.292,
    gx: 0.170, gy: 0.797,
    bx: 0.131, by: 0.046,
    wx: 0.3127, wy: 0.3290,
    alpha: 1.099,
    beta: 0.018,
    gamma: 0.45,
    delta: 4.5
  }
};

function gammaToLinearLUT(lut, colSpace) {
  if (!Object.prototype.hasOwnProperty(colParams, colSpace)) {
    console.error(`Unrecognised colourspace ${colSpace} - defaulting to BT.709`);
    colSpace = '709';
  }
  const alpha = colParams[colSpace].alpha;
  const delta = colParams[colSpace].delta;
  const beta = colParams[colSpace].beta * delta;
  const gamma = colParams[colSpace].gamma;
  const numEntries = 2**16;
  const lutArr = new Float32Array(numEntries);
  lutArr.fill(1.0);

  for (let i=0; i<numEntries; ++i) {
    const fi = i / (numEntries-1);
    if (fi < beta)
      lutArr[i] = fi / delta;
    else
      lutArr[i] = ((fi + (alpha-1)) / alpha)**(1/gamma);
  }
  Buffer.from(lutArr.buffer).copy(lut);
}

function linearToGammaLUT(lut, colSpace) {
  if (!Object.prototype.hasOwnProperty(colParams, colSpace)) {
    console.error(`Unrecognised colourspace ${colSpace} - defaulting to BT.709`);
    colSpace = '709';
  }
  const alpha = colParams[colSpace].alpha;
  const beta = colParams[colSpace].beta;
  const gamma = colParams[colSpace].gamma;
  const delta = colParams[colSpace].delta;
  const numEntries = 2**16;
  const lutArr = new Float32Array(numEntries);
  lutArr.fill(1.0);
  for (let i=0; i<numEntries; ++i) {
    const fi = i / (numEntries-1);
    if (fi < beta)
      lutArr[i] = fi * delta;
    else
      lutArr[i] = alpha * (fi**gamma) - (alpha-1);
  }
  Buffer.from(lutArr.buffer).copy(lut);
}

function ycbcr2rgbMatrix(matrix, colSpace, numBits) {
  if (!Object.prototype.hasOwnProperty(colParams, colSpace)) {
    console.error(`Unrecognised colourspace ${colSpace} - defaulting to BT.709`);
    colSpace = '709';
  }
  const kR = colParams[colSpace].kR;
  const kB = colParams[colSpace].kB;
  const kG = 1.0 - kR - kB;

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

function rgb2ycbcrMatrix(matrix, colSpace, numBits) {
  if (!colParams.hasOwnProperty(colSpace)) {
    console.error(`Unrecognised colourspace ${colSpace} - defaulting to BT.709`);
    colSpace = '709';
  }
  const kR = colParams[colSpace].kR;
  const kB = colParams[colSpace].kB;
  const kG = 1.0 - kR - kB;

  const kLumaBlack = 16.0 << (numBits - 8);
  const kLumaWhite = 235.0 << (numBits - 8);
  const kChromaNull = 128.0 << (numBits - 8);
  const kLumaRange = kLumaWhite - kLumaBlack;
  const kChrRange = 224 << (numBits - 8);

  const Ry = kR * kLumaRange;
  const Gy = kG * kLumaRange;
  const By = kB * kLumaRange;
  const Oy = kLumaBlack;

  const Ru = - kR / (1 - kB) / 2 * kChrRange;
  const Gu = - kG / (1 - kB) / 2 * kChrRange;
  const Bu = (1 - kB) / (1 - kB) / 2 * kChrRange;
  const Ou = kChromaNull;

  const Rv = (1 - kR) / (1 - kR) / 2 * kChrRange;
  const Gv = - kG / (1 - kR) / 2 * kChrRange;
  const Bv = - kB / (1 - kR) / 2 * kChrRange;
  const Ov = kChromaNull;

  const matrixArray = [
    Ry, Gy, By, Oy,
    Ru, Gu, Bu, Ou,
    Rv, Gv, Bv, Ov,
    0.0, 0.0, 0.0, 1.0];
  
  const matrixTypedArray = Float32Array.from(matrixArray);
  Buffer.from(matrixTypedArray.buffer).copy(matrix);
}

const readV210 = `__kernel void
  readV210(__global uint4* input,
           __global float4* output,
           __private unsigned int width,
           __constant float4* matrix,
           __global float* gammaToLinearLUT) {
    uint item = get_global_id(0);
    bool lastItemOnLine = get_local_id(0) == get_local_size(0) - 1;

    // 48 pixels per workItem = 8 input uint4s per work item
    uint numPixels = lastItemOnLine && (0 != width % 48) ? width % 48 : 48;
    uint numLoops = numPixels / 6;
    uint remain = numPixels % 6;

    uint inOff = 8 * item;
    uint outOff = width * get_group_id(0) + get_local_id(0) * 48;

    float4 matR = matrix[0];
    float4 matG = matrix[1];
    float4 matB = matrix[2];
    float4 matA = matrix[3];

    for (uint i=0; i<numLoops; ++i) {
      uint4 w = input[inOff];

      ushort4 yuva[6];
      yuva[0] = (ushort4)((w.s0 >> 10) & 0x3ff, w.s0 & 0x3ff, (w.s0 >> 20) & 0x3ff, 1);
      yuva[1] = (ushort4)(w.s1 & 0x3ff, yuva[0].s1, yuva[0].s2, 1);
      yuva[2] = (ushort4)((w.s1 >> 20) & 0x3ff, (w.s1 >> 10) & 0x3ff, w.s2 & 0x3ff, 1);
      yuva[3] = (ushort4)((w.s2 >> 10) & 0x3ff, yuva[2].s1, yuva[2].s2, 1);
      yuva[4] = (ushort4)(w.s3 & 0x3ff, (w.s2 >> 20) & 0x3ff, (w.s3 >> 10) & 0x3ff, 1);
      yuva[5] = (ushort4)((w.s3 >> 20) & 0x3ff, yuva[4].s1, yuva[4].s2, 1);

      for (uint p=0; p<6; ++p) {
        float4 yuva_f = convert_float4(yuva[p]);
        float4 rgba;
        rgba.s0 = gammaToLinearLUT[convert_ushort_sat_rte(dot(matR, yuva_f) * 65535.0f)];
        rgba.s1 = gammaToLinearLUT[convert_ushort_sat_rte(dot(matG, yuva_f) * 65535.0f)];
        rgba.s2 = gammaToLinearLUT[convert_ushort_sat_rte(dot(matB, yuva_f) * 65535.0f)];
        rgba.s3 = gammaToLinearLUT[convert_ushort_sat_rte(dot(matA, yuva_f) * 65535.0f)];
        output[outOff+p] = rgba;
      }

      inOff++;
      outOff+=6;
    }

    if (remain > 0) {
      uint4 w = input[inOff];

      ushort4 yuva[4];
      yuva[0] = (ushort4)((w.s0 >> 10) & 0x3ff, w.s0 & 0x3ff, (w.s0 >> 20) & 0x3ff, 0);
      yuva[1] = (ushort4)(w.s1 & 0x3ff, yuva[0].s1, yuva[0].s2, 0);

      if (4 == remain) {
        yuva[2] = (ushort4)((w.s1 >> 20) & 0x3ff, (w.s1 >> 10) & 0x3ff, w.s2 & 0x3ff, 0);
        yuva[3] = (ushort4)((w.s2 >> 10) & 0x3ff, yuva[2].s1, yuva[2].s2, 0);
      }

      for (uint p=0; p<remain; ++p) {
        float4 yuva_f = convert_float4(yuva[p]);
        float4 rgba;
        rgba.s0 = gammaToLinearLUT[convert_ushort_sat_rte(dot(matR, yuva_f) * 65535.0f)];
        rgba.s1 = gammaToLinearLUT[convert_ushort_sat_rte(dot(matG, yuva_f) * 65535.0f)];
        rgba.s2 = gammaToLinearLUT[convert_ushort_sat_rte(dot(matB, yuva_f) * 65535.0f)];
        rgba.s3 = gammaToLinearLUT[convert_ushort_sat_rte(dot(matA, yuva_f) * 65535.0f)];
        output[outOff+p] = rgba;
      }
    }
  }
`;

const writeV210 = `__kernel void
  writeV210(__global float4* input,
            __global uint4* output,
            __private unsigned int width,
            __constant float4* matrix,
            __global float* linearToGammaLUT) {
    uint item = get_global_id(0);
    bool lastItemOnLine = get_local_id(0) == get_local_size(0) - 1;

    // 48 pixels per workItem = 8 output uint4s per work item
    uint numPixels = lastItemOnLine && (0 != width % 48) ? width % 48 : 48;
    uint numLoops = numPixels / 6;
    uint remain = numPixels % 6;

    uint inOff = width * get_group_id(0) + get_local_id(0) * 48;
    uint outOff = 8 * item;

    if (48 != numPixels) {
      // clear the output buffer for the last item, partially overwritten below
      uint clearOff = outOff;
      for (uint i=0; i<8; ++i)
        output[clearOff++] = (uint4)(0, 0, 0, 0);
    }

    float4 matY = matrix[0];
    float4 matU = matrix[1];
    float4 matV = matrix[2];
    float4 matA = matrix[3];

    for (uint i=0; i<numLoops; ++i) {
      ushort4 yuva[6];

      for (uint p=0; p<6; ++p) {
        float4 rgba_l = input[inOff+p];
        float4 rgba;
        rgba.s0 = linearToGammaLUT[convert_ushort_sat_rte(rgba_l.s0 * 65535.0f)];
        rgba.s1 = linearToGammaLUT[convert_ushort_sat_rte(rgba_l.s1 * 65535.0f)];
        rgba.s2 = linearToGammaLUT[convert_ushort_sat_rte(rgba_l.s2 * 65535.0f)];
        rgba.s3 = linearToGammaLUT[convert_ushort_sat_rte(rgba_l.s3 * 65535.0f)];

        yuva[p].s0 = convert_ushort_sat_rte(dot(matY, rgba));
        yuva[p].s1 = convert_ushort_sat_rte(dot(matU, rgba));
        yuva[p].s2 = convert_ushort_sat_rte(dot(matV, rgba));
        yuva[p].s3 = convert_ushort_sat_rte(dot(matA, rgba));
      }

      uint4 w;
      w.s0 = yuva[0].s2 << 20 | yuva[0].s0 << 10 | yuva[0].s1;
      w.s1 = yuva[2].s0 << 20 | yuva[2].s1 << 10 | yuva[1].s0;
      w.s2 = yuva[4].s1 << 20 | yuva[3].s0 << 10 | yuva[2].s2;
      w.s3 = yuva[5].s0 << 20 | yuva[4].s2 << 10 | yuva[4].s0;
      output[outOff] = w;

      inOff+=6;
      outOff++;
    }

    if (remain > 0) {
      uint4 w = (uint4)(0, 0, 0, 0);

      ushort4 yuva[4];
      for (uint p=0; p<remain; ++p) {
        float4 rgba_l = input[inOff+p];
        float4 rgba;
        rgba.s0 = linearToGammaLUT[convert_ushort_sat_rtz(rgba_l.s0 * 65535.0f)];
        rgba.s1 = linearToGammaLUT[convert_ushort_sat_rtz(rgba_l.s1 * 65535.0f)];
        rgba.s2 = linearToGammaLUT[convert_ushort_sat_rtz(rgba_l.s2 * 65535.0f)];
        rgba.s3 = linearToGammaLUT[convert_ushort_sat_rtz(rgba_l.s3 * 65535.0f)];

        yuva[p].s0 = convert_ushort_sat(round(dot(matY, rgba)));
        yuva[p].s1 = convert_ushort_sat(round(dot(matU, rgba)));
        yuva[p].s2 = convert_ushort_sat(round(dot(matV, rgba)));
        yuva[p].s3 = convert_ushort_sat(round(dot(matA, rgba)));
      }

      w.s0 = yuva[0].s2 << 20 | yuva[0].s0 << 10 | yuva[0].s1;
      if (2 == remain) {
        w.s1 = yuva[1].s0;
      } else if (4 == remain) {
        w.s1 = yuva[2].s0 << 20 | yuva[2].s1 << 10 | yuva[1].s0;
        w.s2 = yuva[3].s0 << 10 | yuva[2].s2;
      }
      output[outOff] = w;
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
  let Y=64;
  const Cb=512;
  const Cr=512;
  let yOff = 0;
  for (let y=0; y<height; ++y) {
    let xOff = 0;
    for (let x=0; x<(width-width%6)/6; ++x) {
      buf.writeUInt32LE((Cr<<20) | (Y<<10) | Cb, yOff + xOff);
      buf.writeUInt32LE((Y<<20) | (Cb<<10) | Y, yOff + xOff + 4);
      buf.writeUInt32LE((Cb<<20) | (Y<<10) | Cr, yOff + xOff + 8);
      buf.writeUInt32LE((Y<<20) | (Cr<<10) | Y, yOff + xOff + 12);
      xOff += 16;
      Y = (940==Y)?64:++Y;
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
  function getHex(off) { return buf.readUInt32LE(lineOff + off).toString(16); }

  const pitch = getV210PitchBytes(width);
  for (let l = 0; l < numLines; ++l) {
    lineOff = l * pitch;
    console.log(`Line ${l}: ${getHex(0)}, ${getHex(4)}, ${getHex(8)}, ${getHex(12)} ... ${getHex(128+0)}, ${getHex(128+4)}, ${getHex(128+8)}, ${getHex(128+12)}`);
  }
}

function diff32(buf1, buf2, width, height) {
  const pitch = getV210PitchBytes(width);
  let diffs = 0;
  for (let y=0; y<height; ++y) {
    for (let x=0; x<pitch/4; ++x) {
      const val1 = buf1.readUInt32LE(y*pitch + x*4);
      const val2 = buf2.readUInt32LE(y*pitch + x*4);
      if (val1 != val2) {
        if (diffs++ < 100)
          console.log(`Diff: (${x},${y}): ${val1.toString(16)} != ${val2.toString(16)}`);
      }
    }
  }
  if (0 === diffs)
    console.log('Buffers are identical');
}

async function noden() {
  const platformIndex = 0;
  const deviceIndex = 0;
  const context = new addon.clContext({
    platformIndex: platformIndex, 
    deviceIndex: deviceIndex
  });
  const platformInfo = context.getPlatformInfo();
  // console.log(JSON.stringify(platformInfo, null, 2));
  console.log(platformInfo.vendor, platformInfo.devices[deviceIndex].type);

  const width = 1920;
  const height = 1080;
  const numBytesV210 = getV210PitchBytes(width) * height;
  const numBytesRGBA = width * height * 4 * 4;

  // process one image line per work group, 48 pixels per work item
  const workItemsPerGroup = (width + 47 - ((width - 1) % 48)) / 48;
  const globalWorkItems = workItemsPerGroup * height;

  const readV210program = await context.createProgram(readV210, {
    globalWorkItems: globalWorkItems,
    workItemsPerGroup: workItemsPerGroup
  });

  const writeV210program = await context.createProgram(writeV210, {
    globalWorkItems: globalWorkItems,
    workItemsPerGroup: workItemsPerGroup
  });

  let [i1, o1, m1, g1] = await Promise.all([
    context.createBuffer(numBytesV210, 'readonly', 'coarse'),
    context.createBuffer(numBytesRGBA, 'readwrite', 'coarse'),
    context.createBuffer(64, 'readonly', 'none'),
    context.createBuffer(2**16 * 4, 'readonly', 'none')
  ]);

  await i1.hostAccess('writeonly');
  fillV210Buf(i1, width, height);
  dumpV210Buf(i1, width, 4);

  await m1.hostAccess('writeonly');
  ycbcr2rgbMatrix(m1, '709', 10);
  await g1.hostAccess('writeonly');
  gammaToLinearLUT(g1, '709');

  for (let x = 0; x < 10; x++) {
    let timings1 = await readV210program.run({input: i1, output: o1, width: width, matrix: m1, gammaToLinearLUT: g1});
    console.log(`${timings1.dataToKernel}, ${timings1.kernelExec}, ${timings1.dataFromKernel}, ${timings1.totalTime}`);
    // [o, i] = [i, o];
  }

  await o1.hostAccess('readonly');
  for (let y=0; y<4; ++y) {
    const off = y*width*4*4;
    console.log(`Line ${y}: ${o1.readFloatLE(off+0).toFixed(4)}, ${o1.readFloatLE(off+4).toFixed(4)}, ${o1.readFloatLE(off+8).toFixed(4)}, ${o1.readFloatLE(off+12).toFixed(4)}, ${o1.readFloatLE(off+16).toFixed(4)}, ${o1.readFloatLE(off+20).toFixed(4)}, ${o1.readFloatLE(off+24).toFixed(4)}, ${o1.readFloatLE(off+28).toFixed(4)}`);
  }

  let [o2, m2, g2] = await Promise.all([
    context.createBuffer(numBytesV210, 'writeonly', 'coarse'),
    context.createBuffer(64, 'readonly', 'none'),
    context.createBuffer(2**16 * 4, 'readonly', 'none')
  ]);

  await m2.hostAccess('writeonly');
  rgb2ycbcrMatrix(m2, '709', 10);
  await g2.hostAccess('writeonly');
  linearToGammaLUT(g2, '709');

  for (let x = 0; x < 10; x++) {
    let timings2 = await writeV210program.run({input: o1, output: o2, width: width, matrix: m2, linearToGammaLUT: g2});
    console.log(`${timings2.dataToKernel}, ${timings2.kernelExec}, ${timings2.dataFromKernel}, ${timings2.totalTime}`);
    // [o, i] = [i, o];
  }

  await o2.hostAccess('readonly');
  dumpV210Buf(o2, width, 4);

  await i1.hostAccess('readonly');
  console.log('Compare returned', i1.compare(o2));
  diff32(i1, o2, width, height);

  return [i1, o2];
}
noden()
  .then(([i, o]) => { return [i.creationTime, o.creationTime]; })
  .then(([ict, oct]) => { if (global.gc) global.gc(); console.log(ict, oct); })
  .catch( console.error );
// .then(([i, o]) => { console.log(i.creationTime, o.creationTime); return }, console.error)
// .then(() => { if (global.gc()) global.gc(); });
