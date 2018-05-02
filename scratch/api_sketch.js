createProgram(0, 1, kernel) => Promise => CLProgram on platform 0, device 1
createProgram(kernel) => Promise => CLProgram on first GPU detected

CLProgram {
  platformIndex: 0,
  deviceIndex: 1,
  deviceType: "GPU",
  kernelSource: "...",
  svmCapabilities: 11,
  context: [external],
  commands: [external],
  kernel: [external],
  program: [external],
  kernelWorkGroupSize: 256,
  createBuffer(size, type), // type is copy, svm coarse, svm fine
  run(inputs, outputs) // either values or arrays
};

createBuffer(size) => Promise => a (mapped) buffer and parameters

length is the JS size
clSize is the acutal buffer size scaled to work group size.
type is whether this buffer needs a copy, coarse mapping or map-free fine grained sharing.
mapped is the current mapping status. Ideally, an unmapped buffer should not be written or read.

run(inputs, outputs) => Promise => completes empty on CL finished, buffers copied or mapped

Other features ....

- Use typed arrays so larger ints and floats can be manipulated
- Consider an OpenCL diag function like nVidia's diagnostic
