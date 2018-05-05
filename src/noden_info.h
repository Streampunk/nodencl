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

#ifndef NODEN_INFO_H
#define NODEN_INFO_H

#ifdef __APPLE__
    #include "OpenCL/opencl.h"
#else
    #include "CL/cl.h"
#endif
#include <vector>
#include "node_api.h"

cl_int getPlatformIds(std::vector<cl_platform_id> &ids);
cl_int getDeviceIds(cl_uint platformId, std::vector<cl_device_id> &ids);
napi_value getPlatformInfo(napi_env env, napi_callback_info info);
napi_value findFirstGPU(napi_env env, napi_callback_info info);
napi_status getDeviceParamString(napi_env env, cl_device_id deviceId,
  cl_device_info param, napi_value* result);
napi_status getDeviceParamBool(napi_env env, cl_device_id deviceId,
  cl_device_info param, napi_value* result);
napi_status getDeviceParamUint(napi_env env, cl_device_id deviceId,
  cl_device_info param, napi_value* result);
napi_status getDeviceParamUlong(napi_env env, cl_device_id deviceId,
  cl_device_info param, napi_value* result);
napi_status getDeviceParamSizet(napi_env env, cl_device_id deviceId,
  cl_device_info param, napi_value* result);
napi_status getDeviceParamEnum(napi_env env, cl_device_id deviceId,
  cl_device_info param, napi_value* result);
napi_status getDeviceParamBitfield(napi_env env, cl_device_id deviceId,
  cl_device_info param, napi_value* result);
napi_status getDeviceParamSizetArray(napi_env env, cl_device_id deviceId,
  cl_device_info param, napi_value* result);
napi_status getDeviceParamEnumArray(napi_env env, cl_device_id deviceId,
  cl_device_info param, napi_value* result);

typedef napi_status (*getParamFunc)(napi_env, cl_device_id, cl_device_info, napi_value*);

struct deviceParam {
  cl_device_info deviceInfo;
  char* name;
  getParamFunc getParam;
};

const deviceParam deviceParams[] = {
  { CL_DEVICE_AVAILABLE, "available", getDeviceParamBool },
  { CL_DEVICE_ADDRESS_BITS, "addressBits", getDeviceParamUint },
  { CL_DEVICE_BUILT_IN_KERNELS, "builtInKernels", getDeviceParamString },
  { CL_DEVICE_ENDIAN_LITTLE, "endianLittle", getDeviceParamBool },
  { CL_DEVICE_ERROR_CORRECTION_SUPPORT, "errorCorrectionSupport", getDeviceParamBool },
  { CL_DEVICE_EXTENSIONS, "extensions", getDeviceParamString },
  { CL_DEVICE_GLOBAL_MEM_CACHE_SIZE, "globalMemCacheSize", getDeviceParamUlong },
  { CL_DEVICE_GLOBAL_MEM_CACHELINE_SIZE, "globalMemCachelineSize", getDeviceParamUint },
  { CL_DEVICE_GLOBAL_MEM_SIZE, "globalMemSize", getDeviceParamUlong },
  { CL_DEVICE_GLOBAL_VARIABLE_PREFERRED_TOTAL_SIZE, "globalVariablePreferredTotalSize", getDeviceParamSizet },
  { CL_DEVICE_IMAGE2D_MAX_HEIGHT, "image2DMaxHeight", getDeviceParamSizet },
  { CL_DEVICE_IMAGE2D_MAX_WIDTH, "image2DMaxWidth", getDeviceParamSizet },
  { CL_DEVICE_IMAGE3D_MAX_DEPTH, "image3DMaxDepth", getDeviceParamSizet },
  { CL_DEVICE_IMAGE3D_MAX_HEIGHT, "image3DMaxHeight", getDeviceParamSizet },
  { CL_DEVICE_IMAGE3D_MAX_WIDTH, "image3DMaxWidth", getDeviceParamSizet },
  { CL_DEVICE_IMAGE_BASE_ADDRESS_ALIGNMENT, "imageBaseAddressAlignment", getDeviceParamUint },
  { CL_DEVICE_IMAGE_MAX_ARRAY_SIZE, "imageMaxArraySize", getDeviceParamSizet },
  { CL_DEVICE_IMAGE_MAX_BUFFER_SIZE, "imageMaxBufferSize", getDeviceParamSizet },
  { CL_DEVICE_IMAGE_PITCH_ALIGNMENT, "imagePitchAlignment", getDeviceParamUint },
  { CL_DEVICE_IMAGE_SUPPORT, "imageSupport", getDeviceParamBool },
  { CL_DEVICE_LINKER_AVAILABLE, "linkedAvailable", getDeviceParamBool },
  { CL_DEVICE_LOCAL_MEM_SIZE, "localMemSize", getDeviceParamUlong },
  { CL_DEVICE_MAX_CLOCK_FREQUENCY, "maxClockFrequency", getDeviceParamUint },
  { CL_DEVICE_MAX_COMPUTE_UNITS, "maxComputeUnits", getDeviceParamUint },
  { CL_DEVICE_MAX_CONSTANT_ARGS, "maxConstantArgs", getDeviceParamUint },
  { CL_DEVICE_MAX_CONSTANT_BUFFER_SIZE, "maxConstantBufferSize", getDeviceParamUlong },
  { CL_DEVICE_MAX_GLOBAL_VARIABLE_SIZE, "maxGlobalVariableSize", getDeviceParamSizet },
  { CL_DEVICE_MAX_MEM_ALLOC_SIZE, "maxMemAllocSize", getDeviceParamUlong },
  { CL_DEVICE_MAX_ON_DEVICE_EVENTS, "maxOnDeviceEvents", getDeviceParamUint },
  { CL_DEVICE_MAX_ON_DEVICE_QUEUES, "maxOnDeviceQueues", getDeviceParamUint },
  { CL_DEVICE_MAX_PARAMETER_SIZE, "maxParameterSize", getDeviceParamSizet },
  { CL_DEVICE_MAX_PIPE_ARGS, "maxPipeArgs", getDeviceParamUint },
  { CL_DEVICE_MAX_READ_IMAGE_ARGS, "maxReadImageArgs", getDeviceParamUint },
  { CL_DEVICE_MAX_READ_WRITE_IMAGE_ARGS, "maxReadWriteImageArgs", getDeviceParamUint },
  { CL_DEVICE_MAX_SAMPLERS, "maxSamplers", getDeviceParamUint },
  { CL_DEVICE_MAX_WORK_GROUP_SIZE, "maxWorkGroupSize", getDeviceParamSizet },
  { CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS, "maxWorkItemDimensions", getDeviceParamUint },
  { CL_DEVICE_MAX_WRITE_IMAGE_ARGS, "maxWriteImageArgs", getDeviceParamUint },
  { CL_DEVICE_MEM_BASE_ADDR_ALIGN, "memBaseAddrAlign", getDeviceParamUint },
  { CL_DEVICE_OPENCL_C_VERSION, "openclCVersion", getDeviceParamString },
  { CL_DEVICE_PARTITION_MAX_SUB_DEVICES, "partitionMaxSubDevices", getDeviceParamUint },
  { CL_DEVICE_PIPE_MAX_ACTIVE_RESERVATIONS, "pipeMaxActiveReservation", getDeviceParamUint },
  { CL_DEVICE_PIPE_MAX_PACKET_SIZE, "pipeMaxPacketSize", getDeviceParamUint },
  { CL_DEVICE_PREFERRED_GLOBAL_ATOMIC_ALIGNMENT, "preferredGlobalAtomicAlignment", getDeviceParamUint },
  { CL_DEVICE_PREFERRED_INTEROP_USER_SYNC, "preferredInteropUserSync", getDeviceParamBool },
  { CL_DEVICE_PREFERRED_LOCAL_ATOMIC_ALIGNMENT, "preferredGlobalAtomicAlignment", getDeviceParamUint },
  { CL_DEVICE_PREFERRED_PLATFORM_ATOMIC_ALIGNMENT, "preferredPlatformAtomicAlignment", getDeviceParamUint },
  { CL_DEVICE_PRINTF_BUFFER_SIZE, "printfBufferSize", getDeviceParamSizet },
  { CL_DEVICE_PROFILE, "profile", getDeviceParamString },
  { CL_DEVICE_PROFILING_TIMER_RESOLUTION, "profilingTimerResolution", getDeviceParamSizet },
  { CL_DEVICE_QUEUE_ON_DEVICE_MAX_SIZE, "queueOnDeviceMaxSize", getDeviceParamUint },
  { CL_DEVICE_QUEUE_ON_DEVICE_PREFERRED_SIZE, "queueOnDevicePreferredSize", getDeviceParamUint },
  { CL_DEVICE_REFERENCE_COUNT, "referenceCount", getDeviceParamUint },
  { CL_DEVICE_VENDOR, "vendor", getDeviceParamString },
  { CL_DEVICE_VENDOR_ID, "vendorId", getDeviceParamUint },
  { CL_DEVICE_VERSION, "version", getDeviceParamString },
  { CL_DRIVER_VERSION, "driverVersion", getDeviceParamString },
  { CL_DEVICE_GLOBAL_MEM_CACHE_TYPE, "globalMemCacheType", getDeviceParamEnum },
  { CL_DEVICE_DOUBLE_FP_CONFIG, "doubleFPConfig", getDeviceParamBitfield },
  { CL_DEVICE_EXECUTION_CAPABILITIES, "executionCapabilities", getDeviceParamBitfield },
  { CL_DEVICE_LOCAL_MEM_TYPE, "localMemType", getDeviceParamEnum },
  { CL_DEVICE_PARTITION_AFFINITY_DOMAIN, "partitionAffinityDomain", getDeviceParamBitfield },
  { CL_DEVICE_QUEUE_ON_DEVICE_PROPERTIES, "queueOnDeviceProperties", getDeviceParamBitfield },
  { CL_DEVICE_QUEUE_ON_HOST_PROPERTIES, "queueOnHostProperties", getDeviceParamBitfield },
  { CL_DEVICE_SINGLE_FP_CONFIG, "singleFPConfig", getDeviceParamBitfield },
  { CL_DEVICE_SVM_CAPABILITIES, "svmCapabilities", getDeviceParamBitfield },
  { CL_DEVICE_NATIVE_VECTOR_WIDTH_CHAR, "nativeVectorWidthChar", getDeviceParamUint },
  { CL_DEVICE_NATIVE_VECTOR_WIDTH_SHORT, "nativeVectorWidthShort", getDeviceParamUint },
  { CL_DEVICE_NATIVE_VECTOR_WIDTH_INT, "nativeVectorWidthInt", getDeviceParamUint },
  { CL_DEVICE_NATIVE_VECTOR_WIDTH_LONG, "nativeVectorWidthLong", getDeviceParamUint },
  { CL_DEVICE_NATIVE_VECTOR_WIDTH_FLOAT, "nativeVectorWidthFloat", getDeviceParamUint },
  { CL_DEVICE_NATIVE_VECTOR_WIDTH_DOUBLE, "nativeVectorWidthDouble", getDeviceParamUint },
  { CL_DEVICE_NATIVE_VECTOR_WIDTH_HALF, "nativeVectorWidthHalf", getDeviceParamUint },
  { CL_DEVICE_PREFERRED_VECTOR_WIDTH_CHAR, "preferredVectorWidthChar", getDeviceParamUint },
  { CL_DEVICE_PREFERRED_VECTOR_WIDTH_SHORT, "preferredVectorWidthShort", getDeviceParamUint },
  { CL_DEVICE_PREFERRED_VECTOR_WIDTH_INT, "preferredVectorWidthInt", getDeviceParamUint },
  { CL_DEVICE_PREFERRED_VECTOR_WIDTH_LONG, "preferredVectorWidthLong", getDeviceParamUint },
  { CL_DEVICE_PREFERRED_VECTOR_WIDTH_FLOAT, "preferredVectorWidthFloat", getDeviceParamUint },
  { CL_DEVICE_PREFERRED_VECTOR_WIDTH_DOUBLE, "preferredVectorWidthDouble", getDeviceParamUint },
  { CL_DEVICE_PREFERRED_VECTOR_WIDTH_HALF, "preferredVectorWidthHalf", getDeviceParamUint },
  { CL_DEVICE_MAX_WORK_ITEM_SIZES, "maxWorkItemSizes", getDeviceParamSizetArray },
  { CL_DEVICE_PARTITION_PROPERTIES, "partitionProperties", getDeviceParamEnumArray },
  { CL_DEVICE_NAME, "name", getDeviceParamString },
  { CL_DEVICE_TYPE, "type", getDeviceParamBitfield }
};

/* Device properties not yet supported and why:

- CL_DEVICE_PARTITION_TYPE - an array of enums - relates to subdevices - complex
- CL_DEVICE_PLATFORM - returns a pointer - don't want to expose in JS land
*/
const uint32_t deviceParamCount = 142 - 58;

// Field not supported on pre 2.0
#define INVALID_CHECK if (error == CL_INVALID_VALUE) {\
  status = napi_get_undefined(env, result); \
  return status; \
}

#endif /* NODEN_INFO_H */
