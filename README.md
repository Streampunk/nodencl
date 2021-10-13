# nodencl
Experimental library for executing [OpenCL](https://www.khronos.org/opencl/) kernels over [Node.JS](https://nodejs.org/) [buffers](https://nodejs.org/dist/latest-v8.x/docs/api/buffer.html). The aim of this library is to expose GPU processing capability into Node.JS in the simplest and most efficient way possible, with OpenCL programs represented as Javascript strings and buffers using shared virtual memory wherever possible.

The aim of this library is to support the development of GPU accelerated functions for processing 10-bit and higher professional HD and UHD video formats.

### Approach

The OpenCL API is rich and has many features and two significant versions, so far. Only some of the features are supported on any particular platform and/or by an OpenCL version. This gives two basic choices when trying to provide access to OpenCL capabilities:

1. Expose every function call of the OpenCL API into Javascript, as with [nooocl](https://www.npmjs.com/package/nooocl) and [node-opencl](https://www.npmjs.com/package/nooocl), requiring the Javascript developer to understand every detail of both OpenCL and how it is mapped into Javascript.
2. Create a layer above the OpenCL API, exposing key aspects of OpenCL capability into Javascript, using familiar idioms and automating low-level aspects wherever possible.

The second approach is adopted for nodencl (_Node and OpenCL_), with promises facilitating OpenCL execution to take place asynchronously on a separate thread. The library helps the user to choose an appropriate data movement paradigm, choosing shared virtual memory over memory copying where available.

### Native bindings

Development of this binding has been made simpler thanks to [N-API](https://nodejs.org/dist/latest-v8.x/docs/api/n-api.html) for the development of C++ addons for Node.JS. In particular, the support for implementing promises exposed into C/C++. Note that N-API is a core part of Node.JS in version 10 LTS and later. In earler Node.JS versions a warning about this feature being experimental will be printed.

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
Support for kernel parameters currently includes all scalar types, buffer pointers including vector types and image2d types.

Create an OpenCL context by creating an instance of the clContext object:
```Javascript
const context = new addon.clContext();
```
This object wraps a member that is a promise that resolves to an OpenCL _context_ which includes a single command queue for supporting OpenCL programs and memory buffers on a single platform and device.

The resolved object contains the native OpenCL structures required to support creating programs, kernels and memory buffers. In this default mode with no options, the first device on the system that is of type GPU is selected.

To select a specific device options can be provided as an argument to the clContext constructor. The options must include a numerical `platformIndex` and `deviceIndex` that are the indexes into the array of platform details, then the `devices` property of each platform, as returned by `getPlatformInfo()`. The optional `overlapping` property is used to control overlapping of copies and kernel execution - see below. For example:

```Javascript
const context = new addon.clContext(
  { platformIndex: 1, deviceIndex: 0, overlapping: false });
```

An optional second parameter allows the provision of a custom logging object with log, warn and error methods. By default the console object will be used.

Retrieve the platform info as above for the specific platform selected using `context.getPlatformInfo()`;

Create an OpenCL program using the `context.createProgram()` method of the context object. This returns a promise that resolves to an OpenCL compiled kernel for an OpenCL _program_ as a program object.

```Javascript
await context.initialise();
```

After creating the context, initialize the context.

```Javascript
const progPromise = context.createProgram(kernel);
progPromise.then(program => ..., console.error);
```

The object returned by the method contains the native OpenCL structures required to execute the kernel and pass data to and from. To explicitly name the function in the kernel code that is the entry point for the kernel (e.g. `square` in the example above), options can be provided as the second argument to `createProgram()`. The `name` property gives the name of the kernel entry point function, the `globalWorkItems` and `workItemsPerGroup` properties affect the number and size of the kernel executions. 

For example:

```Javascript
const workItemsPerGroup = width / 16;
const globalWorkItems = workItemsPerGroup * height;
const progPromise = context.createProgram(kernel,
  { name: 'square',
    globalWorkItems: globalWorkItems,
    workItemsPerGroup: workItemsPerGroup
  }
);
progPromise.then(program => ..., console.error);
```

### Creating data buffers

OpenCL buffers allow data to be exchanged between the Node.JS program and the execution context of the Open CL kernel, managing either the transfer of data between system RAM and graphics RAM or the sharing of virtual memory between devices. Once created, the data buffer is wrapped in a Node.JS Buffer object that can be used like any other. When the OpenCL program is executed, nodencl takes care of passing the data to and from the kernel device.

To create a data buffer, use the `context.createBuffer()` method of the context object. This returns a promise that resolves to a buffer of the requested size and type. For example, within the body of an ES6 _async_ function:

```Javascript
let input = await context.createBuffer(65536, 'readonly');
let output = await context.createBuffer(9000, 'writeonly', 'fine');
```

... or to execute in parallel ...

```Javascript
let [input, output] = await Promise.all([
  context.createBuffer(65536, 'readonly'),
  context.createBuffer(9000, 'writeonly', 'fine', { width: width, height: height }, 'myOut')
]);
```

The first argument is normally the size of the desired buffer in bytes. Passing in an allocated buffer is supported in a special case - please see below for the details of this optimisation.

The second argument describes the intended use of the buffer with respect to execution of kernel functions - either 'readonly' for input parameters, 'writeonly' for output parameters or 'readwrite' if the buffer will be used in both directions.

The third optional argument determines the type of memory used for the buffer: '`none`' for no shared virtual memory, '`coarse`' for coarse-grained shared virtual memory (where supported), '`fine`' for fine-grained shared virtual memory (where supported). When this argument is not present, the default value is the expected-to-be-fastest kind of memory supported by the device.

The fourth optional argument is required if a buffer is to be used as input or output as an image type in a kernel - eg image_2d_t. This argument is an object that is used to provide the image dimensions with properties `width`, `height` and `depth` as required.

The fifth optional argument is a string that allows allocations to have an owner name associated with them. This can be helpful in logging and enables resource management as follows.

Graphics RAM is a limited resource. To help manage this nodencl includes a resource management system that allows buffer allocations to be referenced and released. When a buffer is created with an owner, it is marked as 'reserved'. The buffer provides two methods '`addRef()`' and '`release()`' that are used to control the buffer lifetime.

`buffer.addRef()` should be called before the buffer is passed as a parameter to a kernel function, `buffer.release()` should be called when the buffer (and its contents) are no longer required. When `release` is called if there are no outstanding references (from `addRef`) then the buffer will no longer be marked as reserved. This means that when a caller requests to create a new buffer with the same attributes they can be returned the unreserved existing buffer.

Once a buffer has been unreserved, it becomes a candidate to be freed if graphics memory is running short. Callers should not attempt to `addRef` a buffer that has already been unreserved.

If an owner name has been used for buffer allocations then the `context.releaseBuffers(owner)` function can be used to completely free all allocations with a particular owner name.

### Host access to data buffers

In order to allow normal host access to the buffer for read and write operations in Javascript, use the `buffer.hostAccess()` method of the buffer object. This returns a promise that resolves when host access is available. For example:

```Javascript
await input.hostAccess();
await input.hostAccess('writeonly', srcBuf);
```
The optional first argument describes the intended host access required: '`readwrite`' is the default if no parameter is provided, '`writeonly`' to be able to fill the buffer, '`readonly`' to be able to read from the buffer and '`none`' to indicate that hostAccess is no longer required.

The optional second argument allows a source buffer to be passed to the asynchronous thread and be copied into the buffer object, the promise resolving once the copy is complete. The first argument is required when using this option.

The `buffer.hostAccess()` method initiates transfers between host and device memory when required, for example requesting `readonly` access to a buffer after running a kernel that writes to it will enqueue a copy from device to host memory.

Note that further development of the API is intended to add support for Javascript typed arrays.

### Execute the kernel

To run the kernel having created a program object, created the input and output data buffers and set the values of the input buffer as required, call the program object's `program.run()` method. The argument is an object with key names that must match the kernel parameter names and values whose type is compatible with those of the kernel program. This returns a promise that resolves to an object containing timing measurements for the execution. For example, in the body if an ES6 _async_ function:

```Javascript
let execTimings = await program.run({input: input, output: output});
console.log(JSON.stringify(execTimings, null, 2));
```

### Overlapping

When overlapping is enabled at context creation, the `buffer.hostAccess()` and `program.run()` methods each take a second parameter and return a promise that resolves when the requested work has been enqueued, not completed. This allows overlapping of buffer loading, kernel running and buffer unloading.

In order to progress correctly only when each step is complete, it is necessary to wait on the relevant queue to complete, using the `context.waitFinish()` method. This is shown in-line for simplicity:

```Javascript
// loading - enqueue copy from host to device
await input.hostAccess('writeonly', context.queue.load, buf); // copies from source buffer to OpenCL host buffer
await input.hostAccess('none', context.queue.load); // copies from OpenCL host buffer to device buffer
await context.waitFinish(context.queue.load);

// processing - on-device kernel operations only
await program.run({input: input, output: output}, context.queue.process);
await context.waitFinish(context.queue.process);

// unloading - enqueue copy from device to host
await output.hostAccess('readonly', context.queue.unload);
await context.waitFinish(context.queue.unload);
// OpenCL host buffer can now be accessed and copied
```

The diagram below shows the intended overlapped flow for a sequence of frames, with the asterisks indicating the completion of the `context.waitFinish()` call.

    load queue:   |---Load 0---|*|---Load 1---|*     |---Load 2---|*
    process queue:               |----Process 0----|*|----Process 1----|*|----Process 2----|*
    unload queue:                                    |--Unload 0--|*     |--Unload 1--|*     |--Unload 2--|*

The overlapping relies on hardware in the GPU that allows DMA transfers to be setup for host to device and device to host copies. Some GPUs have hardware to allow two copies to proceed at the same time allowing full overlap of load, process and unload.

### Cleaning up

When finished with the context object, it should be closed in order to ensure all allocations are freed:
```Javascript
await clContext.close(done => {  });
```
The callback will be called when all allocations have been freed. If owner names and the `releaseBuffers` method detailed above are not used then a warning message will be emitted that a timeout has been used to free the allocations.

### Testing

An example output of the above kernel code fragment is:

```JSON
{
  "totalTime": 1466,
  "dataToKernel": 605,
  "kernelExec": 202,
  "dataFromKernel": 659
}
```

The results are measurements in microseconds for the total time (`totalTime`) taken to run the program, the time taken to move data to the kernel (`dataToKernel`), the time taken to execute the kernel (`kernelExec`) and the time taken to make the result available in system memory (`dataFromKernel`). On resolution of the promise, the output buffer will contain the result of the execution.

### Measuring performance

An example test script that moves blocks of memory of a given size to and from the system memory and GPU memory, executing an example kernel process between, is provided as script [`measureWriteExecRead.js`](scratch/measureWriteExecRead.js). To run the script:

    node scratch/measureWriteExecRead.js <buffer_size> <svm_type>

The `buffer_size` is a number of bytes to simulate. The `svm_type` is one of: `none` for no shared virtual memory, `coarse` for coarse-grained shared virtual memory (where supported), `fine` for fine-grained shared virtual memory (where supported). Some results of running this script for common video payload sizes are available in the [`results`](results/) folder.

## Status, support and further development

Contributions can be made via pull requests and will be considered by the author on their merits. Enhancement requests and bug reports should be raised as github issues. For support, please contact [Streampunk Media](http://www.streampunk.media/).

Next steps include:

* support for Javascript typed arrays;
* consideration of whether and how to use OpenCL pipelines;
* control of how work is split up into threads;
* adding support for OpenCL SDKs from nVidia and AMD;
* adding support for linux and Mac platforms.

## License

This software is released under the Apache 2.0 license. Copyright 2018 Streampunk Media Ltd.
