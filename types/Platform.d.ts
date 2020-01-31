/* Copyright 2020 Streampunk Media Ltd.

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

export interface OpenCLDevice {
  /** True if the device is available and false otherwise. A device is considered to be available
   * if the device can be expected to successfully execute commands enqueued to the device.
   */
  readonly available: boolean
  /**
   * The default compute device address space size of the global address space specified
   * as an unsigned integer value in bits. Currently supported values are 32 or 64 bits.
   */
  readonly addressBits: number
  /**
   * A semi-colon separated list of built-in kernels supported by the device. An empty
   * string is returned if no built-in kernels are supported by the device.
   */
  readonly builtInKernels: string
  /**
   * False if the implementation does not have a compiler available to compile the 
   * program source. True if the compiler is available.
   */
  readonly compilerAvailable: boolean,
  /** True if the OpenCL device is a little endian device and false otherwise. */
  readonly endianLittle: boolean
  /** True if the device implements error correction for all accesses to compute device memory
   * (global and constant). False if the device does not implement such error correction.
   */
  readonly errorCorrectionSupport: boolean
  /**
   * Returns a space separated list of extension names (the extension names themselves do
   * not contain any spaces) supported by the device. The list of extension names returned
   * can be vendor supported extension names and one or more of the following Khronos approved
   * extension names:
   * - cl_khr_int64_base_atomics
   * - cl_khr_int64_extended_atomics
   * - cl_khr_fp16
   * - cl_khr_gl_sharing
   * - cl_khr_gl_event
   * - cl_khr_d3d10_sharing
   * - cl_khr_dx9_media_sharing
   * - cl_khr_d3d11_sharing
   * - cl_khr_gl_depth_images
   * - cl_khr_gl_msaa_sharing
   * - cl_khr_initialize_memory
   * - cl_khr_terminate_context
   * - cl_khr_spir
   * - cl_khr_srgb_image_writes
   * 
   * The following approved Khronos extension names must be returned by all devices that support OpenCL C 2.0:
   * - cl_khr_byte_addressable_store
   * - cl_khr_fp64 (for backward compatibility if double precision is supported)
   * - cl_khr_3d_image_writes
   * - cl_khr_image2d_from_buffer
   * - cl_khr_depth_images
   * 
   * Please refer to the OpenCL 2.0 Extension Specification for a detailed description of these extensions.
   */
  readonly extensions: string
  /** Size of global memory cache in bytes. */
  readonly globalMemCacheSize: number
  /** Size of global memory cache line in bytes. */
  readonly globalMemCachelineSize: number
  /** Size of global device memory in bytes. */
  readonly globalMemSize: number
  /** Maximum preferred total size, in bytes, of all program variables in the global address space.
   * This is a performance hint. An implementation may place such variables in storage with optimized
   * device access. This query returns the capacity of such storage. The minimum value is 0.
   */
  readonly globalVariablePreferredTotalSize: number | undefined
  /** Max height of 2D image in pixels. The minimum value is 16384 if OpenCLDevice.imageSupport is true. */
  readonly image2DMaxHeight: number
  /**
   * Max width of 2D image or 1D image not created from a buffer object in pixels. The minimum value
   * is 16384 if OpenCLDevice.imageSupport is true.
   */
  readonly image2DMaxWidth: number
  /** Max depth of 3D image in pixels. The minimum value is 2048 if OpenCLDevice.imageSupport is true. */
  readonly image3DMaxDepth: number
  /** Max height of 3D image in pixels. The minimum value is 2048 if OpenCLDevice.imageSupport is true. */
  readonly image3DMaxHeight: number
  /** Max width of 3D image in pixels. The minimum value is 2048 if OpenCLDevice.imageSupport is true. */
  readonly image3DMaxWidth: number
  /**
   * This query should be used when a 2D image is created from a buffer which was created using
   * CL_MEM_USE_HOST_PTR. The value returned must be a power of 2. This query specifies the minimum
   * alignment in pixels of the host_ptr specified to clCreateBuffer.
   * If the device does not support images, this value must be 0.
   */
  readonly imageBaseAddressAlignment: number | undefined,
  /** Max number of images in a 1D or 2D image array. The minimum value is 2048 if OpenCLDevice.imageSupport is true. */
  readonly imageMaxArraySize: number
  /** 
   * Max number of pixels for a 1D image created from a buffer object.
   * The minimum value is 65536 if OpenCLDevice.imageSupport is true. */
  readonly imageMaxBufferSize: number
  /**
   * The row pitch alignment size in pixels for 2D images created from a buffer. The value 
   * returned must be a power of 2. If the device does not support images, this value must be 0.
   */
  readonly imagePitchAlignment: number | undefined,
  /** True if images are supported by the OpenCL device and false otherwise. */
  readonly imageSupport: boolean,
  /**
   * False if the implementation does not have a linker available. True if the linker is available.
   * This can be false for the embedded platform profile only.
   * This must be true if compilerAvailable is true.
   */
  readonly linkerAvailable: boolean,
  /** Size of local memory region in bytes. The minimum value is 32 KB for devices that are not of type 'CL_DEVICE_TYPE_CUSTOM'. */
  readonly localMemSize: number
  /** Maximum configured clock frequency of the device in MHz. */
  readonly maxClockFrequency: number
  /**
   * The number of parallel compute units on the OpenCL device. A work-group executes on a single compute unit.
   * The minimum value is 1.
   */
  readonly maxComputeUnits: number
  /**
   * Max number of arguments declared with the __constant qualifier in a kernel. The minimum value is 8
   * for devices that are not of type 'CL_DEVICE_TYPE_CUSTOM'.
   */
  readonly maxConstantArgs: number
  /**
   * Max size in bytes of a constant buffer allocation. The minimum value is 64 KB for devices that
   * are not of type 'CL_DEVICE_TYPE_CUSTOM'.
   */
  readonly maxConstantBufferSize: number
  /**
   * The maximum number of bytes of storage that may be allocated for any single variable in program
   * scope or inside a function in OpenCL C declared in the global address space.
   */
  readonly maxGlobalVariableSize: number | undefined,
  /**
   * Max size of memory object allocation in bytes. The minimum value is
   * max(min(1024*1024*1024, 1/4th of globalMemSize), 32*1024*1024)
   * for devices that are not of type 'CL_DEVICE_TYPE_CUSTOM'.
   */
  readonly maxMemAllocSize: number
  /**
   * The maximum number of events in use by a device queue. These refer to events returned by the
   * enqueue_ built-in functions to a device queue or user events returned by the create_user_event
   * built-in function that have not been released. The minimum value is 1024.
   */
  readonly maxOnDeviceEvents: number
  /** The maximum number of device queues that can be created per context. The minimum value is 1. */
  readonly maxOnDeviceQueues: number
  /**
   * Max size in bytes of all arguments that can be passed to a kernel. The minimum value is 1024
   * for devices that are not of type 'CL_DEVICE_TYPE_CUSTOM'. For this minimum value, only a
   * maximum of 128 arguments can be passed to a kernel.
   */
  readonly maxParameterSize: number
  /** The maximum number of pipe objects that can be passed as arguments to a kernel. The minimum value is 16. */
  readonly maxPipeArgs: number | undefined,
  /**
   * Max number of image objects arguments of a kernel declared with the read_only qualifier.
   * The minimum value is 128 if imageSupport is true.
   */
  readonly maxReadImageArgs: number
  /**
   * Max number of image objects arguments of a kernel declared with the write_only or read_write qualifier.
   * The minimum value is 64 if imageSupport is true.
   */
  readonly maxReadWriteImageArgs: number | undefined,
  /** Maximum number of samplers that can be used in a kernel. The minimum value is 16 if imageSupport is true. */
  readonly maxSamplers: number
  /**
   * Maximum number of work-items in a work-group that a device is capable of executing on a single compute unit,
   * for any given kernel-instance running on the device. The minimum value is 1.
   */
  readonly maxWorkGroupSize: number
  /**
   * Maximum dimensions that specify the global and local work-item IDs used by the data parallel execution model.
   * The minimum value is 3 for devices that are not of type 'CL_DEVICE_TYPE_CUSTOM'.
   */
  readonly maxWorkItemDimensions: number
  /**
   * Max number of image objects arguments of a kernel declared with the write_only qualifier. 
   * The minimum value is 64 if imageSupport is true.
   */
  readonly maxWriteImageArgs: number
  /**
   * Alignment requirement (in bits) for subbuffer offsets. The minimum value is the
   * size (in bits) of the largest OpenCL built-in data type supported by the device
   * (long16 in FULL profile, long16 or int16 in EMBEDDED profile) for devices that are not of
   * type 'CL_DEVICE_TYPE_CUSTOM'.
   */
  readonly memBaseAddrAlign: number
  /**
   * OpenCL C version string. Returns the highest OpenCL C version supported by the compiler for
   * this device that is not of type 'CL_DEVICE_TYPE_CUSTOM'. This version string has the following format:
   * - OpenCL<space>C<space><major_version.minor_version><space><vendorspecific information>
   */
  readonly openclCVersion: string
  /**
   * The maximum number of subdevices that can be created when a device is partitioned.
   * The value returned cannot exceed maxComputeUnits.
   */
  readonly partitionMaxSubDevices: number
  /**
   * The maximum number of reservations that can be active for a pipe per work-item in a kernel.
   * A work-group reservation is counted as one reservation per work-item. The minimum value is 1.
   */
  readonly pipeMaxActiveReservation: number,
  /** The maximum size of pipe packet in bytes. The minimum value is 1024 bytes. */
  readonly pipeMaxPacketSize: number,
  /**
   * The value representing the preferred alignment in bytes for OpenCL 2.0 atomic types to global memory.
   * This query can return 0 which indicates that the preferred alignment is aligned to the natural size of the type.
   */
  readonly preferredGlobalAtomicAlignment: number,
  /**
   * True if the device’s preference is for the user to be responsible for synchronization,
   * when sharing memory objects between OpenCL and other APIs such as DirectX,
   * false if the device / implementation has a performant path for performing synchronization of memory
   * object shared between OpenCL and other APIs such as DirectX.
   */
  readonly preferredInteropUserSync: boolean
  /**
   * The value representing the preferred alignment in bytes for OpenCL 2.0 fine-grained SVM atomic types.
   * This query can return 0 which indicates that the preferred alignment is aligned to the natural size of the type.
   */
  readonly preferredPlatformAtomicAlignment: number,
  /**
   * Maximum size in bytes of the internal buffer that holds the output of printf calls from a kernel.
   * The minimum value for the FULL profile is 1 MB.
   */
  readonly printfBufferSize: number
  /**
   * OpenCL profile string. Returns the profile name supported by the device.
   * The profile name returned can be one of the following strings:
   *  - 'FULL_PROFILE' – if the device supports the OpenCL specification (functionality defined as
   * part of the core specification and does not require any extensions to be supported).
   *  - 'EMBEDDED_PROFILE' - if the device supports the OpenCL embedded profile.
   */
  readonly profile: string
  /** Describes the resolution of device timer. This is measured in nanoseconds */
  readonly profilingTimerResolution: number
  /**
   * The max. size of the device queue in bytes. The minimum value is 256 KB for the full profile
   * and 64 KB for the embedded profile.
   */
  readonly queueOnDeviceMaxSize: number
  /**
   * The size of the device queue in bytes preferred by the implementation. Applications should use
   * this size for the device queue to ensure good performance. The minimum value is 16 KB.
   */
  readonly queueOnDevicePreferredSize: number
  /** The device reference count. If the device is a root-level device, a reference count of one is returned. */
  readonly referenceCount: number
  /** Vendor name string. */
  readonly vendor: string
  /** A unique device vendor identifier. An example of a unique device identifier could be the PCIe ID. */
  readonly vendorId: number
  /**
   * OpenCL version string. Returns the OpenCL version supported by the device.
   * This version string has the following format:
   * - OpenCL<space><major_version.minor_version><space><vendor-specificinformation>.
   */
  readonly version: string
  /** OpenCL software driver version string. Follows a vendor-specific format. */
  readonly driverVersion: string
  /**
   * Type of global memory cache supported. Valid values are:
   * 'CL_NONE', 'CL_READ_ONLY_CACHE' and 'CL_READ_WRITE_CACHE'.
   */
  readonly globalMemCacheType: string
  /** 
   * Describes double precision floating-point capability of the OpenCL device. This is an array
   * that includes one or more of the following values:
   * - 'CL_FP_DENORM' – denorms are supported
   * - 'CL_FP_INF_NAN' – INF and NaNs are supported.
   * - 'CL_FP_ROUND_TO_NEAREST' – round to nearest even rounding mode supported.
   * - 'CL_FP_ROUND_TO_ZERO' – round to zero rounding mode supported.
   * - 'CL_FP_ROUND_TO_INF' – round to positive and negative infinity rounding modes supported.
   * - 'CP_FP_FMA – IEEE754-2008' fused multiply-add is supported.
   * - 'CL_FP_SOFT_FLOAT' – Basic floating-point operations (such as addition, subtraction, multiplication) are implemented in software.
   * 
   * Double precision is an optional feature so the mandated minimum double precision floating-point
   * capability is 0. If double precision is supported by the device, then the minimum double precision
   * floating-point capability must be:
   * [ 'CL_FP_FMA', 'CL_FP_ROUND_TO_NEAREST', 'CL_FP_INF_NAN', 'CL_FP_DENORM' ].
   */
  readonly doubleFPConfig: Array<string>
  /**
   * Describes the execution capabilities of the device. This is an array that includes
   * one or more of the following values:
   * - 'CL_EXEC_KERNEL' – The OpenCL device can execute OpenCL kernels.
   * - 'CL_EXEC_NATIVE_KERNEL' – The OpenCL device can execute native kernels.
   * 
   * The mandated minimum capability is: 'CL_EXEC_KERNEL'.
   */
  readonly executionCapabilities: Array<string>
  /**
   * Type of local memory supported. This can be set to CL_LOCAL implying dedicated local memory
   * storage such as SRAM, or CL_GLOBAL.
   * For custom devices, CL_NONE can also be returned indicating no local memory support.
   */
  readonly localMemType: string
  /**
   * Returns the list of supported affinity domains for partitioning the device using
   * CL_DEVICE_PARTITION_BY_AFFINITY_DOMAIN. This is an array that includes one or
   * more of the following values:
   * - 'CL_DEVICE_AFFINITY_DOMAIN_NUMA'
   * - 'CL_DEVICE_AFFINITY_DOMAIN_L4_CACHE'
   * - 'CL_DEVICE_AFFINITY_DOMAIN_L3_CACHE' 
   * - 'CL_DEVICE_AFFINITY_DOMAIN_L2_CACHE'
   * - 'CL_DEVICE_AFFINITY_DOMAIN_L1_CACHE'
   * - 'CL_DEVICE_AFFINITY_DOMAIN_NEXT_PARTITIONABLE'
   * 
   * If the device does not support any affinity domains, an empty array will be returned.
   */
  readonly partitionAffinityDomain: Array<string>
  /**
   * Describes the on device command-queue properties supported by the device. This is
   * an array that includes one or more of the following values:
   * - 'CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE'
   * - 'CL_QUEUE_PROFILING_ENABLE'
   * 
   * The mandated minimum capability is: [ 'CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE',
   * 'CL_QUEUE_PROFILING_ENABLE' ].
   */
  readonly queueOnDeviceProperties: Array<string>
  /**
   * Describes the on host command-queue properties supported by the device. This is
   * an array that includes one or more of the following values:
   * - 'CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE'
   * - 'CL_QUEUE_PROFILING_ENABLE'
   * 
   * The mandated minimum capability is: [ 'CL_QUEUE_PROFILING_ENABLE' ].
   */
  readonly queueOnHostProperties: Array<string>
  /**
   * Describes single precision floating-point capability of the device. This is an array that
   * describes one or more of the following values:
   * - 'CL_FP_DENORM' – denorms are supported.
   * - 'CL_FP_INF_NAN' – INF and quiet NaNs are supported.
   * - 'CL_FP_ROUND_TO_NEAREST' – round to nearest even rounding mode supported.
   * - 'CL_FP_ROUND_TO_ZERO' – round to zero rounding mode supported.
   * - 'CL_FP_ROUND_TO_INF' – round to positive and negative infinity rounding modes supported.
   * - 'CL_FP_FMA – IEEE754-2008' fused multiplyadd is supported.
   * - 'CL_FP_CORRECTLY_ROUNDED_DIVIDE_SQRT' – divide and sqrt are correctly rounded as defined by the IEEE754 specification.
   * - 'CL_FP_SOFT_FLOAT' – Basic floating-point operations (such as addition, subtraction, multiplication) are implemented in software.
   * 
   * For the full profile, the mandated minimum floating-point capability for devices that are not of type
   * 'CL_DEVICE_TYPE_CUSTOM' is: [ 'CL_FP_ROUND_TO_NEAREST', 'CL_FP_INF_NAN' ].
   */
  readonly singleFPConfig: Array<string>
  /**
   * Describes the various shared virtual memory (a.k.a. SVM) memory allocation types the device supports.
   * Coarse-grain SVM allocations are required to be supported by all OpenCL 2.0 devices. This is an array
   * that describes a combination of the following values:
   * - 'CL_DEVICE_SVM_COARSE_GRAIN_BUFFER' – Support for coarse-grain buffer sharing using clSVMAlloc.
   * Memory consistency is guaranteed at synchronization points and the host must use calls to
   * clEnqueueMapBuffer and clEnqueueUnmapMemObject.
   * - 'CL_DEVICE_SVM_FINE_GRAIN_BUFFER' – Support for fine-grain buffer sharing using clSVMAlloc.
   * Memory consistency is guaranteed at synchronization points without need for clEnqueueMapBuffer and
   * clEnqueueUnmapMemObject.
   * - 'CL_DEVICE_SVM_FINE_GRAIN_SYSTEM' – Support for sharing the host’s entire virtual memory including
   * memory allocated using malloc. Memory consistency is guaranteed at synchronization points.
   * - 'CL_DEVICE_SVM_ATOMICS' – Support for the OpenCL 2.0 atomic operations that provide memory consistency
   * across the host and all OpenCL devices supporting fine-grain SVM allocations.
   * 
   * The mandated minimum capability is [ 'CL_DEVICE_SVM_COARSE_GRAIN_BUFFER' ]
   */
  readonly svmCapabilities: Array<string>,
  /**
   * Returns the native ISA vector width for builtin _char_ types. The vector width is defined as the
   * number of scalar elements that can be stored in the vector.
   */
  readonly nativeVectorWidthChar: number
  /**
   * Returns the native ISA vector width for builtin _short_ types. The vector width is defined as the
   * number of scalar elements that can be stored in the vector.
   */
  readonly nativeVectorWidthShort: number
  /**
   * Returns the native ISA vector width for builtin _int_ types. The vector width is defined as the
   * number of scalar elements that can be stored in the vector.
   */
  readonly nativeVectorWidthInt: number
  /**
   * Returns the native ISA vector width for builtin _long_ types. The vector width is defined as the
   * number of scalar elements that can be stored in the vector.
   */
  readonly nativeVectorWidthLong: number
  /**
   * Returns the native ISA vector width for builtin _float_ types. The vector width is defined as the
   * number of scalar elements that can be stored in the vector.
   */
  readonly nativeVectorWidthFloat: number
  /**
   * Returns the native ISA vector width for builtin _double_ types. The vector width is defined as the
   * number of scalar elements that can be stored in the vector.
   * If double precision is not supported, must be 0.
   */
  readonly nativeVectorWidthDouble: number
  /**
   * Returns the native ISA vector width for builtin _half_ types. The vector width is defined as the
   * number of scalar elements that can be stored in the vector.
   * If cl_khr_fp16 extension is not supported, must be 0.
   */
  readonly nativeVectorWidthHalf: number
  /**
   * Preferred native vector width size for builtin _char_ types that can be put into vectors.
   * The vector width is defined as the number of scalar elements that can be stored in the vector.
   */
  readonly preferredVectorWidthChar: number
  /**
   * Preferred native vector width size for builtin _short_ types that can be put into vectors.
   * The vector width is defined as the number of scalar elements that can be stored in the vector.
   */
  readonly preferredVectorWidthShort: number
  /**
   * Preferred native vector width size for builtin _int_ types that can be put into vectors.
   * The vector width is defined as the number of scalar elements that can be stored in the vector.
   */
  readonly preferredVectorWidthInt: number
  /**
   * Preferred native vector width size for builtin _long_ types that can be put into vectors.
   * The vector width is defined as the number of scalar elements that can be stored in the vector.
   */
  readonly preferredVectorWidthLong: number
  /**
   * Preferred native vector width size for builtin _float_ types that can be put into vectors.
   * The vector width is defined as the number of scalar elements that can be stored in the vector.
   */
  readonly preferredVectorWidthFloat: number
  /**
   * Preferred native vector width size for builtin _double_ types that can be put into vectors.
   * The vector width is defined as the number of scalar elements that can be stored in the vector.
   * If double precision is not supported, must be 0.
   */
  readonly preferredVectorWidthDouble: number
  /**
   * Preferred native vector width size for builtin _half_ types that can be put into vectors.
   * The vector width is defined as the number of scalar elements that can be stored in the vector.
   * If cl_khr_fp16 extension is not supported, must be 0.
   */
  readonly preferredVectorWidthHalf: number
  /**
   * Maximum number of work-items that can be specified in each dimension of the work-group to clEnqueueNDRangeKernel.
   * Returns an array of n size_t entries, where n is the value returned by maxWorkItemDimensions.
   * The minimum value is [ 1, 1, 1 ] for devices that are not of type 'CL_DEVICE_TYPE_CUSTOM'.
   */
  readonly maxWorkItemSizes: Array<number>
  /**
   * Returns the list of partition types supported by device. This is an array of
   * cl_device_partition_property values drawn from the following list:
   * - 'CL_DEVICE_PARTITION_EQUALLY'
   * - 'CL_DEVICE_PARTITION_BY_COUNTS'
   * - 'CL_DEVICE_PARTITION_BY_AFFINITY_DOMAIN'
   * 
   * If the device cannot be partitioned (i.e. there is no partitioning scheme supported by the device
   * that will return at least two subdevices), a value of 0 will be returned.
   */
  readonly partitionProperties: Array<string>
  /** Device name string. */
  readonly name: string
  /**
   * The OpenCL device type. Currently supported values are:
   * - 'CL_DEVICE_TYPE_CPU'
   * - 'CL_DEVICE_TYPE_GPU'
   * - 'CL_DEVICE_TYPE_ACCELERATOR'
   * - 'CL_DEVICE_TYPE_DEFAULT', a combination of the above types.
   * - 'CL_DEVICE_TYPE_CUSTOM'
   */
  readonly type: Array<string>
  /** The platform index associated with this device. */
  readonly platformIndex: number
  /** The device index in the array of devices for the platform associated with this device. */
  readonly deviceIndex: number
}

export interface OpenCLPlatform {
  /**
   * OpenCL profile string. Returns the profile name supported by the implementation.
   * The profile name returned can be one of the following strings:
   * - FULL_PROFILE – if the implementation supports the OpenCL specification (functionality defined as
   * part of the core specification and does not require any extensions to be supported).
   * - EMBEDDED_PROFILE - if the implementation supports the OpenCL embedded profile. The embedded profile
   * is defined to be a subset for each version of OpenCL. 
   */
  readonly profile: string
  /**
   * OpenCL version string. Returns the OpenCL version supported by the implementation.
   * This version string has the following format:
   * - OpenCL<space><major_version.minor_version><space><platform-specificinformation>.
   */
  readonly version: string
  /** Platform name string. */
  readonly name: string
  /** Platform vendor string. */
  readonly vendor: string
  /**
   * Returns a space separated list of extension names (the extension names themselves do not
   * contain any spaces) supported by the platform. Each extension that is supported by all
   * devices associated with this platform must be reported here.
   */
  readonly extensions: string
  /** Returns the resolution of the host timer in nanoseconds as used by clGetDeviceAndHostTimer. */
  readonly hostTimerResolution: number
  /** Array of devices available on this platform */
  readonly devices: Array<OpenCLDevice>
}

/**
 * [Enumerate](https://github.com/Streampunk/nodencl#discovering-the-available-platforms) all the OpenCL platforms and devices that are available
 * @returns Array of platforms, each with an array of devices
 */
export function getPlatformInfo(): ReadonlyArray<OpenCLPlatform>
