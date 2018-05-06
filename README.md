# nodencl
Experimental library for executing Open-CL kernels over Node.JS buffers. The aim of this library is to expose GPU processing capability into Node.JS in the simplest and most efficient way possible, with OpenCL programs represented as Javascript strings and buffers using shared virtual memory wherever possible.

Note that this version is Windows 64-bit only. Future versions will support all platforms.

Next step is to convert this synchronous library into a modular set of asynchronous native promises.
