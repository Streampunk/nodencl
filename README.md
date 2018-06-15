# nodencl
Experimental library for executing [OpenCL](https://www.khronos.org/opencl/) kernels over [Node.JS](https://nodejs.org/) [buffers](https://nodejs.org/dist/latest-v8.x/docs/api/buffer.html). The aim of this library is to expose GPU processing capability into Node.JS in the simplest and most efficient way possible, with OpenCL programs represented as Javascript strings and buffers using shared virtual memory wherever possible.

The initial purpose of this library was to test data movement speeds for real time (or faster) uncompressed video. The aim of this library is to support the development of GPU accelerated functions for processing 10-bit and higher professional HD and UHD video formats.

### Approach

The OpenCL API is rich and has many features and two significant versions, so far. Only some of the features are supported on any particular platform and/or by an OpenCL version. This gives two basic choices when trying to provide access to OpenCL capabilities:

1. Expose every function call of the OpenCL API into Javascript, as with [nooocl](https://www.npmjs.com/package/nooocl) and [node-opencl](https://www.npmjs.com/package/nooocl), requiring the Javascript developer to understand every detail of both OpenCL and how it is mapped into Javascript.
2. Create a layer above the OpenCL API, exposing key aspects of OpenCL capability into Javascript, using familiar idioms and automating low-level aspects wherever possible.

The second approach is adopted for nodencl (_Node and OpenCL_), with promises facilitating OpenCL execution to take place asynchronously on a separate thread. The library helps the user to choose an appropriate data movement paradigm, choosing shared virtual memory over memory copying where available.

### Native bindings

Development of this binding has been made simpler thanks to the experimental [N-API](https://nodejs.org/dist/latest-v8.x/docs/api/n-api.html) for the development of C++ addons for Node.JS. In particular, the new support for implementing promises exposed into C/C++. Note that N-API will become a core part of Node.JS in version 10 LTS available from October 2018. Until then, a warning about this features being experimental will be printed.

### Supported platform

Note that this version is Windows 64-bit only and has been developed against the Intel OpenCL SDK. Future versions will support Linux and Mac.

## Installation

### Prerequisites

1. Install [Node.JS](https://nodejs.org/) LTS for your platform.
2. Install the C++ build tools [node-gyp](https://github.com/nodejs/node-gyp) by following the [installation instructions](https://github.com/nodejs/node-gyp#installation).

### Using nodencl as a dependency

Install nodencl into your project by running the npm install command in its root folder:

    npm install nodencl

Note that the `--save` option is now the default.

### Building nodencl

1. Clone this project with `git clone https://github.com/Streampunk/nodencl.git`
2. Enter the project folder with `cd nodencl`
3. Run `npm install`

## Using nodencl

### Discovering the available platforms

To find out details of the OpenCL platforms and devices available, try:

```Javascript
const nodencl = require('nodencl');
let platforms = nodencl.getPlatformInfo();
console.log(JSON.stringify(platforms, null, 2));
```

The variable `platforms` contains an array of supported OpenCL platforms that are typically distinguished by their version number. Each platform has one or more devices. The output of the above script contains a lot of detail, with an example output shown below (many properties omitted).

```JSON
[
  {
    "profile": "FULL_PROFILE",
    "version": "OpenCL 2.1 ",
    "name": "Experimental OpenCL 2.1 CPU Only Platform",
    "vendor": "Intel(R) Corporation",
    "devices": [ ... ]
  },
  {
    "profile": "FULL_PROFILE",
    "version": "OpenCL 2.0 ",
    "name": "Intel(R) OpenCL",
    "vendor": "Intel(R) Corporation",
    "devices": [
      {
        "available": true,
        "endianLittle": true,
        "globalMemCacheSize": 524288,
        "globalMemCachelineSize": 64,
        "globalMemSize": 3368747008,
        "vendor": "Intel(R) Corporation",
        "version": "OpenCL 2.0 ",
        "driverVersion": "21.20.16.4727",
        "globalMemCacheType": "CL_READ_WRITE_CACHE",
        "svmCapabilities": [
          "CL_DEVICE_SVM_COARSE_GRAIN_BUFFER",
          "CL_DEVICE_SVM_FINE_GRAIN_BUFFER",
          "CL_DEVICE_SVM_ATOMICS"
        ],
        "name": "Intel(R) HD Graphics 520",
        "type": [
          "CL_DEVICE_TYPE_GPU"
        ],
        "platformIndex": 1,
        "deviceIndex": 0
      },
      {
        "name": "Intel(R) Core(TM) i7-6500U CPU @ 2.50GHz",
        "type": [
          "CL_DEVICE_TYPE_CPU"
        ],
        "platformIndex": 1,
        "deviceIndex": 1
      }
    ]
  }
]
```

Consider filtering the full output for the properties that you are interested in.

### Creating a program

Define an OpenCL kernel as a Javascript string. The first function in the script will be used as the executable kernel unless a specific function name is given as an option. For example:

```Javascript
const kernel = `__kernel void square(
    __global unsigned char* input,
    __global unsigned char* output) {

    int i = get_global_id(0);
    output[i] = input[i] - i % 7;
}`;
```

Create an OpenCL context using the `nodencl.createContext()` function. This returns a promise that resolves to an OpenCL _context_ which includes a single command queue for supporting OpenCL programs and memory buffers on a single platform and device.

```Javascript
let contextPromise = nodencl.createContext();
contextPromise.then(context => ..., console.error);
```
The object returned by the method contains the native OpenCL structures required to support creating programs, kernels and memory buffers. In this default mode with no options, the first device on the system that is of type GPU is selected. This can also be determined by calling the `nodencl.findFirstGPU()` function.

To select a specific device options can be provided as an argument to `createContext()`. The options must include a numerical `platformIndex` and `deviceIndex` that are the indexes into the array of platform details, then the `devices` property of each platform, as returned by `getPlatformInfo()`. For example:

```Javascript
let contextPromise = nodencl.createContext(kernel,
  { platformIndex: 1, deviceIndex: 0 });
contextPromise.then(context => ..., console.error);
```

Create an OpenCL program using the `context.createProgram()` method of the context object returned by `nodencl.createContext()`. This returns a promise that resolves to an OpenCL compiled kernel for an OpenCL _program_ as a program object.

```Javascript
let progPromise = context.createProgram(kernel);
progPromise.then(program => ..., console.error);
```

The object returned by the method contains the native OpenCL structures required to execute the kernel and pass data to and from. To explicitly name the function in the kernel code that is the entry point for the kernel (e.g. `square` in the example above), options can be provided as the second argument to `createProgram()`. The `name` property gives the name of the kernel entry point function. For example:

```Javascript
let progPromise = context.createProgram(kernel,
  { name: 'square' });
progPromise.then(program => ..., console.error);
```

### Creating data buffers

OpenCL buffers allow data to be exchanged between the Node.JS program and the execution context of the Open CL kernel, managing either the transfer of data between system RAM and graphics RAM or the sharing of virtual memory between devices. Once created, the data buffer is wrapped in a Node.JS Buffer object that can be used like any other. When the OpenCL program is executed, nodencl takes care of passing the data to and from kernel device.

To create a data buffer, use the `context.createBuffer()` method of the context object returned by `nodencl.createContext()`. This returns a promise that resolves to a buffer of the requested size and type. For example, within the body of an ES6 _async_ function:

```Javascript
let inputBuffer = await context.createBuffer(65536, 'in');
let outputBuffer = await context.createBuffer(9000, 'out', 'fine');
```

... or to execute in parallel ...

```Javascript
let [inputBuffer, outuputBuffer] = await Promise.all([
  context.createBuffer(65536, 'in'),
  context.createBuffer(9000, 'out', 'fine')
]);
```

The first argument is the size of the buffer in bytes. Internally, a slightly larger buffer may be allocated of a size divisible by the kernel workgroup size, although that detail is not exposed into Javascript.

The second argument describes the direction of the buffer with respect to the kernel function - either 'in' for input or 'out' for output.

The third optional argument determines the type of memory used for the buffer: '`none`' for no shared virtual memory, '`coarse`' for coarse-grained shared virtual memory (where supported), '`fine`' for fine-grained shared virtual memory (where supported). When this argument is not present, the default value is the expected-to-be-fastest kind of memory supported by the device.

Note that further development of the API is intended to add support for Javascript typed arrays.

### Execute the kernel

To run the kernel having created a program object, created the input and output data buffers and set the values of the input buffer as required, call the program object's `program.run()` method. The argument is an object with key names that must match the kernel parameter names and values whose type is compatible with those of the kernel program. This returns a promise that resolves to an object containing timing measurements for the execution. For example, in the body if an ES6 _async_ function:

```Javascript
let execTimings = await program.run({input: inputBuffer, output: outputBuffer});
console.log(JSON.stringify(execTimings, null, 2));
```

An example output of the above code fragment is:

```JSON
{
  "totalTime": 1466,
  "dataToKernel": 605,
  "kernelExec": 202,
  "dataFromKernel": 659
}
```

The results are measurements in microseconds for the total time (`totalTime`) taken to run the program, the time taken to move data to the kernel (`dataToKernel`), the time taken to execute the kernel (`kernelExec`) and the time taken to make the result available in system memory (`dataFromKernel`). On resolution of the promise, the output buffer will contain the result of the execution.

Note that the execution model is currently limited to a single input and single output buffer. The intention is to add support for input and output objects with names parameters that can be matched with the function parameters of the kernel entry point function. Note also that the size of the input buffer is passed as the `count` parameter into the kernel.

### Measuring performance

A example test script that moves blocks of memory of a given size to and from the system memory and GPU memory, executing an example kernel process between, is provided as script [`measureWriteExecRead.js`](scratch/measureWriteExecRead.js). To run the script:

    node scratch/measureWriteExecRead.js <buffer_size> <svm_type>

The `buffer_size` is a number of bytes to simulate. The `svm_type` is one of: `none` for no shared virtual memory, `coarse` for coarse-grained shared virtual memory (where supported), `fine` for fine-grained shared virtual memory (where supported). Some results of running this script for common video payload sizes are available in the [`results`](results/) folder.

## Status, support and further development

Contributions can be made via pull requests and will be considered by the author on their merits. Enhancement requests and bug reports should be raised as github issues. For support, please contact [Streampunk Media](http://www.streampunk.media/).

Next steps include:

* a flexible approach to attaching multiple kernel input parameters;
* support for numerical data types other than Uint8 via Javascript typed arrays;
* consideration of how to use OpenCL pipelines;
* control of how work is split up into threads;
* adding support for OpenCL SDKs from nVidia and AMD;
* adding support for linux and Mac platforms.

## License

This software is released under the Apache 2.0 license. Copyright 2018 Streampunk Media Ltd.
