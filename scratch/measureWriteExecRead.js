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

const kernel = `__kernel void square(
    __global unsigned char* input,
    __global unsigned char* output) {

    int i = get_global_id(0);
    output[i] = input[i] - i % 7;
}`;

async function noden() {
  const platformIndex = 0;
  const deviceIndex = 0;
  const context = new addon.clContext({
    platformIndex: platformIndex, 
    deviceIndex: deviceIndex
  });
  await context.initialise();

  let program = await context.createProgram(kernel, {globalWorkItems: +process.argv[2]});
  let [i, o] = await Promise.all([
    context.createBuffer(+process.argv[2], 'readwrite', process.argv[3]),
    context.createBuffer(+process.argv[2], 'readwrite', process.argv[3])
  ]);
  i.fill(42);
  console.log(i);
  for ( let x = 0 ; x < 1000 ; x++ ) {
    let timings = await program.run({ input: i, output: o });
    console.log(`${timings.dataToKernel}, ${timings.kernelExec}, ${timings.dataFromKernel}, ${timings.totalTime}`);
    [o, i] = [i, o];
  }
  console.log(o);
  return [i, o];
}

noden()
  .then(([i, o]) => { console.log(i.creationTime, o.creationTime); })
  .catch(console.error);

/* let b = Buffer.alloc(65536*100);
b.fill(42);
let start = process.hrtime();
for ( let c = 0 ; c < 10 ; c++ ) {
for ( let x = 0 ; x < b.length ; x++) {
  b[x] = b[x] * b[x];
}
}
console.log(process.hrtime(start)); */

/* console.log(addon.getPlatformNames());
console.log(addon.getDeviceNames(0));
console.log(addon.getDeviceNames(1));

console.log("Building a program");
let startTime = process.hrtime();
let clProgram = addon.buildAProgram(1, 0, kernel);
console.log("Program completed in", process.hrtime(startTime), "with", clProgram);

let b = addon.createSVMBuffer(clProgram, 65536 * 100);
b[0] = 2;
for ( let x = 0 ; x < 1000 ; x++ ) {
startTime = process.hrtime();
let result = addon.runProgramSVM(clProgram, b);
console.log("Program executed in", process.hrtime(startTime), "with", result.length, "value", b[0]);
}
*/
