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
const tape = require('tape');

let pi = 0;
let di = 0;
// Find first CPU or GPU device
const clDeviceTypes = [ 'CL_DEVICE_TYPE_CPU', 'CL_DEVICE_TYPE_GPU'];
const platformInfo = addon.getPlatformInfo();
platformInfo.some((platform, p) => platform.devices.find((device, d) => {
  if (clDeviceTypes.indexOf(device.type[0]) >= 0) {
    pi = p;
    di = d;
    return true;
  } else return false;
}));

const properties = { platformIndex: pi, deviceIndex: di };
function createContext(description, cb) {
  tape(description, async t => {
    const clContext = new addon.clContext(properties);
    try {
      await clContext.initialise();
      await cb(t, clContext);
      await clContext.close(t.end);
    } catch (err) {
      t.fail(err);
      t.end();
    }
  });
}

const testKernel = `
__constant sampler_t sampler =
      CLK_NORMALIZED_COORDS_FALSE
    | CLK_ADDRESS_CLAMP_TO_EDGE
    | CLK_FILTER_NEAREST;

__kernel void
  test(__read_only image2d_t input,
       __write_only image2d_t output) {

    int x = get_global_id(0);
    int y = get_global_id(1);
    float4 in = read_imagef(input, sampler, (int2)(x,y));
    write_imagef(output, (int2)(x,y), in);
  }
`;

const width = 1024;
const height = 64;
const numBytes = width * height * 4 * 4; // rgba-f32
const createProgram = function(clContext, kernel) {
  return clContext.createProgram(kernel, {
    name: 'test',
    globalWorkItems: Uint32Array.from([ width, height ])
  });
};

const bufDirs = [ ['readonly', 'writeonly'], ['readonly', 'readwrite'], ['readwrite', 'writeonly'], ['readwrite', 'readwrite'] ];
const svmTypes = [];

platformInfo.forEach((platform, pi) => {
  svmTypes.push([]);
  svmTypes[pi].push([]);
  platform.devices.forEach((device, di) => {
    svmTypes[pi][di].push('none');
    if (device.svmCapabilities.includes('CL_DEVICE_SVM_COARSE_GRAIN_BUFFER'))
      svmTypes[pi][di].push('coarse');
    if (device.svmCapabilities.includes('CL_DEVICE_SVM_FINE_GRAIN_BUFFER'))
      svmTypes[pi][di].push('fine');
  });
});

for (let d=0; d<bufDirs.length; ++d) {
  svmTypes[pi][di].forEach((svm) => {
    const dirs = bufDirs[d];
    createContext(`Run OpenCL program with image parameters, ${dirs[0]}->${dirs[1]}, SVM type ${svm}`, async (t, clContext) => {
      const testProgram = await createProgram(clContext, testKernel);
      const srcBuf = Buffer.alloc(numBytes);
      for (let i=0; i<numBytes; i+=4)
        srcBuf.writeFloatLE(i/numBytes, i);

      const bufIn = await clContext.createBuffer(numBytes, dirs[0], svm, { width: width, height: height });
      await bufIn.hostAccess('writeonly', srcBuf);
      const bufOut = await clContext.createBuffer(numBytes, dirs[1], svm, { width: width, height: height });

      await testProgram.run({ input: bufIn, output: bufOut });
      await bufOut.hostAccess('readonly');
      t.deepEqual(bufOut, srcBuf, 'program produced expected result');
    });
  });
}
