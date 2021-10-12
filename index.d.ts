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

import { OpenCLPlatform } from "./types/Platform"
export * from "./types/Platform"

export type BufDir = 'readonly' | 'writeonly' | 'readwrite'
export type BufSVMType = 'none' | 'coarse' | 'fine'
export type ImageDims = { width: number, height: number, depth?: number }

/** Internal structure for managing allocated buffers */
export interface ContextBuffer {
	/** The data direction for the buffer with respect to execution of kernel functions */
	readonly bufDir: BufDir
	/** The type of Shared Virtual Memory in use for the buffer */
	readonly bufType: BufSVMType
	/** The dimension to be used if the buffer is to be used as an image type for a kernel */
	readonly imageDims?: ImageDims

	// Internal parameters
	readonly reserved: boolean
	readonly owner: string
	readonly index: number
	readonly refs: number
}

/** Internal structure for a buffer */
interface OpenCLBufferInternals {
  /** The data direction for the buffer with respect to execution of kernel functions */
	readonly bufDir: BufDir
  /** The type of Shared Virtual Memory in use for the buffer */
	readonly bufType: BufSVMType
  /** The dimension to be used if the buffer is to be used as an image type for a kernel */
	readonly imageDims: ImageDims
  /** The allocated buffer size */
	readonly numBytes: number
  /** The time taken to perform the allocation of OpenCL memory for this OpenCLBuffer */
	readonly creationTime: number
	/** Field to carry a load time timestamp */
	loadstamp: number
	/** Field to carry a frame timestamp */
	timestamp: number

	// Internal parameters
	readonly numQueues: number
	readonly reserved: boolean
	readonly owner: string | undefined
	readonly index: number
	readonly refs: number
}

/** Functions that operate on OpenCLBuffer objects */
interface OpenCLBufferFunctions {
	/** Allow normal [host access](https://github.com/Streampunk/nodencl#host-access-to-data-buffers) to the buffer for read and write operations in Javascript.
	 * @param bufDir the host data direction for which the access is required, will default to `readwrite`.
	 * @returns a promise that resolves when host access is available.
	 */
	hostAccess(bufDir?: BufDir): Promise<undefined>
	/** Allow normal [host access](https://github.com/Streampunk/nodencl#host-access-to-data-buffers) to the buffer for read and write operations in Javascript.
	 * @param bufDir the host data direction for which the access is required.
	 * @param sourceBuf Allows a source buffer to be passed to the asynchronous thread and be
	 * copied into the buffer object. Requires that the bufDir is not `readonly`.
	 * @returns a promise that resolves when any source copy is complete and host access is available.
	 */
	hostAccess(bufDir: BufDir | 'none', sourceBuf: Buffer): Promise<undefined>
	/**
	 * Allow normal [host access](https://github.com/Streampunk/nodencl#host-access-to-data-buffers) to the buffer for read and write operations in Javascript,
	 * with [overlapping](https://github.com/Streampunk/nodencl#overlapping) support.
	 * @param bufDir the host data direction for which the access is required.
	 * @param queueNum the CommandQueue to use for this operation when overlapping is enabled.
	 * Typically will be `context.queue.load` or `context.queue.unload`.
	 * @param sourceBuf an optional Buffer object to be used as source data when the bufDir is not readonly
	 * @returns a promise that resolves when any source copy is complete and host access is available.
	 */
	hostAccess(bufDir: BufDir | 'none', queueNum: number, sourceBuf?: Buffer): Promise<undefined>
	/** Free any allocated OpenCL memory associated with this OpenCLBuffer object */
	freeAllocation(): undefined

	/** Increment the reference count of this OpenCLBuffer object to keep it in the buffer cache */
	addRef() : void
	/**
	 * Decrement the reference count of this OpenCLBuffer object.
	 * If the reference count becomes zero allow the allocation to be re-used
	 */
	release() : void
}
/** OpenCLBuffer object is a NodeJS Buffer object with extra parameters and functions */
export type OpenCLBuffer = Buffer & OpenCLBufferFunctions & OpenCLBufferInternals

/**
 * Parameters for the Program run function. Parameter names must match the parameter names of the chosen
 * kernel program
 */
export interface KernelParams {
	[key: string]: unknown
}

export type CommandQueues = { load: number, process: number, unload: number }

/**
 * Timings object returned by the Program run function. These timings are only associated with work
 * done during exection of the run function. Other data transfers using hostAccess calls will be
 * required and those timings are not included.
 */
export interface RunTimings {
	/** Time taken in transferring data from host memory to GPU memory */
	readonly dataToKernel: number
	/** Time taken in processing the kernel */
	readonly kernelExec: number
	/** Time taken in transferring data from GPU memory to host memory - only used during development */
	readonly dataFromKernel: number
  /** Total time taken during transfers and processing */
	readonly totalTime: number
}

export interface OpenCLProgram {
	/** The OpenCL kernel is held as a Javascript string */
	readonly kernelSource: string
	/** The number of CommandQueues configured - will be > 1 if overlapping is enabled */
	readonly numQueues: number
  /** The time taken to build the kernelSource for the selected program */
	readonly buildTime: number
  /**
	 * [Run](https://github.com/Streampunk/nodencl#execute-the-kernel) the program with the provided parameters
	 * Prefer clContext.runProgram if using the buffer cache
	 * @param params an object with keys that match the selected kernel parameter names and
	 * data types that match the selected kernel parameters
	 * @param queueNum the CommandQueue to be used to run the program. Typically will be `context.queue.process`
	 * @returns Promise that resolves to a RunTimings object on success
	 */
	run(params: KernelParams, queueNum?: number): Promise<RunTimings>
}

/** Object to hold a context for a selected OpenCL platform and device */
export class clContext {
	/**
	 * Create a new clContext object with the default OpenCL platform and device
	 * @param logger Allows override of the default console.log etc functions
	 */
	constructor(logger?: { log?: Function, warn?: Function, error?: Function })
	/**
	 * Create a new clContext object
	 * @param params Selection of the particular OpenCL platform and device for this context
	 * @param logger Allows override of the default console.log etc functions
	 */
	constructor(
		params: {
			/** Select the OpenCL platform for this context */
			platformIndex: number
			/** Select the OpenCL device for this context and platform */
			deviceIndex: number
			/** Enable [overlapping](https://github.com/Streampunk/nodencl#overlapping) of data transfers and running kernels */
			overlapping?: boolean
		},
		logger?: { log?: Function, warn?: Function, error?: Function }
	)

	// Internal parameters
	readonly params: { platformIndex: number, deviceIndex: number, overlapping: boolean }
	readonly logger: { log: Function, warn: Function, error: Function }
	readonly buffers: ReadonlyArray<ContextBuffer>
	readonly bufIndex: number
	readonly queue: CommandQueues
	readonly context: {	svmCaps: number, platformIndex: number, deviceIndex: number, numQueues: number }

	/**
	 * Initialise the context object on the hardware
	 * @returns Promise that resolves to void when the context is initialised
	 */
  initialise(): Promise<void>

	/**
	 * Get all the OpenCL details for the selected platform for this context
	 * @returns Promise that resolves to an OpenCLPlatform object describing the details of the selected platform
	 */
	getPlatformInfo(): OpenCLPlatform

  /**
	 * Create an OpenCL [program](https://github.com/Streampunk/nodencl#creating-a-program) from a string containing an OpenCL kernel
	 * @param kernel Javascript string representing an OpenCL kernel
	 * @param options Parameters to set up the required kernel operation
	 * @returns Promise that resolves to an OpenCL OpenCLProgram object holding the compiled kernel
	 */
	createProgram(
		kernel: string,
		options: {
			/** Selects a particular kernel program from the kernel string. Defaults to using the first */
			name?: string
			/** The total number of work-items in each dimension that will execute the kernel function */
			globalWorkItems: number | Uint32Array
      /** The number of work-items that make up a work-group that will execute the kernel function */
			workItemsPerGroup?: number
		}
	): Promise<OpenCLProgram>

  /**
	 * Create an OpenCL [buffer](https://github.com/Streampunk/nodencl#creating-data-buffers) for use by OpenCL programs
	 * @param numBytes The size of the desired buffer in bytes
	 * @param bufDir The data direction for the buffer with respect to execution of kernel functions
	 * @param bufType The type of Shared Virtual Memory to be used for the buffer
	 * @param imageDims The image dimensions to be used if this buffer is to be used as a kernel image type parameter
	 * @param owner Name that can be helpful in logging and enables resource management via a cache
	 * @returns Promise that resolves to an OpenCLBuffer object holding OpenCL memory allocations
	 */
	createBuffer(
		numBytes: number,
		bufDir: BufDir,
		bufType: BufSVMType,
		imageDims?: ImageDims,
		owner?: string
	): Promise<OpenCLBuffer>

  /** Log any buffer allocations that have had the owner parameter set */
	logBuffers(): null

  /**
	 * Release any buffer allocations that have had the selected owner parameter set
	 * @param owner String matching the owner name set during buffer creation
	 */
	releaseBuffers(owner: string): null

	/**
	 * [Run](https://github.com/Streampunk/nodencl#execute-the-kernel) the program with the provided parameters
	 * Prefer this function rather than program.run if using the buffer cache
	 * @param program The OpenCLProgram object that holds the compiled kernel
	 * @param params an object with keys that match the selected kernel parameter names and
	 * data types that match the selected kernel parameters
	 * @param queueNum The queue to run the command on
	 * @returns Promise that resolves to a RunTimings object on success
	 *
	 * Note: This function does not support overlapping - use the program.run function if required
	 */
	runProgram(
		program: OpenCLProgram,
		params: KernelParams,
		queueNum?: number
	): Promise<RunTimings>

	/**
	 * Wait for the selected queue to complete - only required when overlapping is enabled
	 * @param queueNum The CommandQueue to wait for
	 */
	waitFinish(queueNum?: number): Promise<undefined>

	/**
	 * [Close](https://github.com/Streampunk/nodencl#cleaning-up) the context in order to ensure that all allocations are freed
	 * @param Function that will be called when the allocations have been freed
	 */
	close(done: Function): null
}
