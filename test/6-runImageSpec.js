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

const pi = 0;
const di = 0;
const properties = { platformIndex: pi, deviceIndex: di };
function createContext(description, cb) {
  tape(description, async t => {
    const clContext = new addon.clContext(properties);
    try {
      await cb(t, clContext);
      clContext.close(t.end);
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

createContext('Run OpenCL program with image parameters', async (t, clContext) => {
  const testProgram = await createProgram(clContext, testKernel);
  const srcBuf = Buffer.alloc(numBytes);
  for (let i=0; i<numBytes; i+=4)
    srcBuf.writeFloatLE(i/numBytes, i);
  console.log(srcBuf);

  const bufIn = await clContext.createBuffer(numBytes, 'readwrite', 'none');
  await bufIn.hostAccess('writeonly', srcBuf);
  const bufOut = await clContext.createBuffer(numBytes, 'readwrite', 'none');

  await testProgram.run({ input: bufIn, output: bufOut });
  await bufOut.hostAccess('readonly');
  t.deepEqual(bufOut, bufIn, 'program produced expected result');
});
