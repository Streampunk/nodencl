const addon = require('bindings')('nodencl');
const SegfaultHandler = require('segfault-handler');
SegfaultHandler.registerHandler("crash.log"); // With no argument, SegfaultHandler will generate a generic log file name

const kernel = `__kernel void square(
    __global unsigned char* input,
    __global unsigned char* output,
    const unsigned int count) {

    int i = get_global_id(0);
    if (i < count)
        output[i] = input[i] * input[i];
}`;

console.log(addon.getPlatformNames());
console.log(addon.getDeviceNames(0));
console.log(addon.getDeviceNames(1));

console.log("Building a program");
let startTime = process.hrtime();
let clProgram = addon.buildAProgram(0, 0, kernel);
console.log("Program completed in", process.hrtime(startTime), "with", clProgram);

startTime = process.hrtime();
let result = addon.runProgram(clProgram, new Buffer(65536 * 100));
console.log("Program executed in", process.hrtime(startTime), "with", result.length);
