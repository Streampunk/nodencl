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

const testBuffer = `
  __kernel void test(__global uint4* restrict input,
                     __global uint4* restrict output) {
    uint off = (get_group_id(0) * get_local_size(0) + get_local_id(0)) * 4;
    for (uint i=0; i<4; ++i) {
      output[off] = input[off];
      off++;
    }
  }
`;

const testImage = `
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

function createContext(description, properties, cb) {
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

const pi = 0;
const di = 0;
createContext('Create program with buffer parameters', { platformIndex: pi, deviceIndex: di }, async (t, clContext) => {
  const workItemsPerGroup = 64;
  const globalWorkItems = 4096;
  const testProgram = await clContext.createProgram(testBuffer, {
    name: 'test',
    globalWorkItems: globalWorkItems,
    workItemsPerGroup: workItemsPerGroup
  });
  t.deepEqual(testBuffer, testProgram.kernelSource, 'has the correct program source');
  t.ok(Object.prototype.hasOwnProperty.call(testProgram, 'run'), 'has run function');
});

createContext('Create program with no kernel name parameter', { platformIndex: pi, deviceIndex: di }, async (t, clContext) => {
  const workItemsPerGroup = 64;
  const globalWorkItems = 4096;
  const testProgram = await clContext.createProgram(testBuffer, {
    globalWorkItems: globalWorkItems,
    workItemsPerGroup: workItemsPerGroup
  });
  t.deepEqual(testBuffer, testProgram.kernelSource, 'has the correct program source');
  t.ok(Object.prototype.hasOwnProperty.call(testProgram, 'run'), 'has run function');
});

createContext('Create program with no globalWorkItems parameter', { platformIndex: pi, deviceIndex: di }, async (t, clContext) => {
  const workItemsPerGroup = 64;
  try {
    await clContext.createProgram(testBuffer, {
      name: 'test',
      workItemsPerGroup: workItemsPerGroup
    });
    t.fail('no globalWorkItems parameter should give error');
  } catch (err) {
    t.pass(`no globalWorkItems parameter produces ${err}`);
  }
});

createContext('Create program with image parameters', { platformIndex: pi, deviceIndex: di }, async (t, clContext) => {
  const width = 256;
  const height = 16;
  const testProgram = await clContext.createProgram(testImage, {
    name: 'test',
    globalWorkItems: Uint32Array.from([ width, height ])
  });
  t.deepEqual(testImage, testProgram.kernelSource, 'has the correct program source');
  t.ok(Object.prototype.hasOwnProperty.call(testProgram, 'run'), 'has run function');
});
