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

const addon = require('bindings')('nodencl');
const SegfaultHandler = require('segfault-handler');
SegfaultHandler.registerHandler("crash.log"); // With no argument, SegfaultHandler will generate a generic log file name

const kernel = `__kernel void square(
    __global unsigned char* input,
    __global unsigned char* output,
    const unsigned int count) {

    int i = get_global_id(0);
    if (i < count)
        output[i] = input[i] + 1;
}`;

console.log(addon.getPlatformNames());
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
