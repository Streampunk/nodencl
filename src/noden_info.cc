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

#include "noden_info.h"
#include "noden_util.h"
#include <inttypes.h>

const char* getDeviceMemCacheType(uint32_t value) {
  switch (value) {
    case 0: return "CL_NONE";
    case 1: return "CL_READ_ONLY_CACHE";
    case 2: return "CL_READ_WRITE_CACHE";
    default: return "NODENCL_VALUE_UNKNOWN";
  }
}

const char* getDeviceLocalMemType(uint32_t value) {
  switch (value) {
    case 0: return "CL_NONE";
    case 1: return "CL_LOCAL";
    case 2: return "CL_GLOBAL";
    default: return "NODENCL_VALUE_UNKNOWN";
  }
}

const char* getDevicePartitionProps(uint32_t value) {
  switch (value) {
    case 0x1086: return "CL_DEVICE_PARTITION_EQUALLY";
    case 0x1087: return "CL_DEVICE_PARTITION_BY_COUNTS";
    case 0x0: return "CL_DEVICE_PARTITION_BY_COUNTS_LIST_END";
    case 0x1088: return "CL_DEVICE_PARTITION_BY_AFFINITY_DOMAIN";
    case 0x4050: return "CL_DEVICE_PARTITION_EQUALLY_EXT";
    case 0x4051: return "CL_DEVICE_PARTITION_BY_COUNTS_EXT";
    case 0x4052: return "CL_DEVICE_PARTITION_BY_NAMES_EXT";
    case 0x4053: return "CL_DEVICE_PARTITION_BY_AFFINITY_DOMAIN_EXT";
    default: return "NODENCL_VALUE_UNKNOWN";
  }
}

const char* getDeviceFPConfig(int64_t value) {
  switch (value) {
    case (1 << 0): return "CL_FP_DENORM";
    case (1 << 1): return "CL_FP_INF_NAN";
    case (1 << 2): return "CL_FP_ROUND_TO_NEAREST";
    case (1 << 3): return "CL_FP_ROUND_TO_ZERO";
    case (1 << 4): return "CL_FP_ROUND_TO_INF";
    case (1 << 5): return "CL_FP_FMA";
    case (1 << 6): return "CL_FP_SOFT_FLOAT";
    case (1 << 7): return "CL_FP_CORRECTLY_ROUNDED_DIVIDE_SQRT";
    default: return nullptr;
  }
}

const char* getDeviceExecCaps(int64_t value) {
  switch (value) {
    case (1 << 0): return "CL_EXEC_KERNEL";
    case (1 << 1): return "CL_EXEC_NATIVE_KERNEL";
    default: return nullptr;
  }
}

const char* getDevicePartitionAffinityDomain(int64_t value) {
  switch (value) {
    case (1 << 0): return "CL_DEVICE_AFFINITY_DOMAIN_NUMA";
    case (1 << 1): return "CL_DEVICE_AFFINITY_DOMAIN_L4_CACHE";
    case (1 << 2): return "CL_DEVICE_AFFINITY_DOMAIN_L3_CACHE";
    case (1 << 3): return "CL_DEVICE_AFFINITY_DOMAIN_L2_CACHE";
    case (1 << 4): return "CL_DEVICE_AFFINITY_DOMAIN_L1_CACHE";
    case (1 << 5): return "CL_DEVICE_AFFINITY_DOMAIN_NEXT_PARTITIONABLE";
    default: return nullptr;
  }
}

const char* getDeviceCommandQProps(int64_t value) {
  switch (value) {
    case (1 << 0): return "CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE";
    case (1 << 1): return "CL_QUEUE_PROFILING_ENABLE";
    case (1 << 2): return "CL_QUEUE_ON_DEVICE";
    case (1 << 3): return "CL_QUEUE_ON_DEVICE_DEFAULT";
    default: return nullptr;
  }
}

const char* getDeviceSvmCapabilities(int64_t value) {
  switch (value) {
    case (1 << 0): return "CL_DEVICE_SVM_COARSE_GRAIN_BUFFER";
    case (1 << 1): return "CL_DEVICE_SVM_FINE_GRAIN_BUFFER";
    case (1 << 2): return "CL_DEVICE_SVM_FINE_GRAIN_SYSTEM";
    case (1 << 3): return "CL_DEVICE_SVM_ATOMICS";
    default: return nullptr;
  }
}

const char* getDeviceType(int64_t value) {
  switch (value) {
    case (1 << 0): return "CL_DEVICE_TYPE_DEFAULT";
    case (1 << 1): return "CL_DEVICE_TYPE_CPU";
    case (1 << 2): return "CL_DEVICE_TYPE_GPU";
    case (1 << 3): return "CL_DEVICE_TYPE_ACCELERATOR";
    case (1 << 4): return "CL_DEVICE_TYPE_CUSTOM";
    // Note not supporting CL_DEVICE_TYPE_ALL
    default: return nullptr;
  }
}

const char* getDeviceEnumLiteral(cl_device_info info, uint32_t value) {
  switch (info) {
    case CL_DEVICE_GLOBAL_MEM_CACHE_TYPE: return getDeviceMemCacheType(value);
    case CL_DEVICE_LOCAL_MEM_TYPE: return getDeviceLocalMemType(value);
    case CL_DEVICE_PARTITION_PROPERTIES: return getDevicePartitionProps(value);
    default: return "NODENCL_ENUM_TYPE_UNKNOWN";
  }
}

const char* getDeviceBitfieldLiteral(cl_device_info info, int64_t value) {
  switch (info) {
    case CL_DEVICE_DOUBLE_FP_CONFIG: return getDeviceFPConfig(value);
    case CL_DEVICE_EXECUTION_CAPABILITIES: return getDeviceExecCaps(value);
    case CL_DEVICE_PARTITION_AFFINITY_DOMAIN: return getDevicePartitionAffinityDomain(value);
    case CL_DEVICE_QUEUE_ON_DEVICE_PROPERTIES: return getDeviceCommandQProps(value);
    case CL_DEVICE_QUEUE_ON_HOST_PROPERTIES: return getDeviceCommandQProps(value);
    case CL_DEVICE_SINGLE_FP_CONFIG: return getDeviceFPConfig(value);
    case CL_DEVICE_SVM_CAPABILITIES: return getDeviceSvmCapabilities(value);
    case CL_DEVICE_TYPE: return getDeviceType(value);
    default: return nullptr;
  }
}

cl_int getPlatformIds(std::vector<cl_platform_id> &ids) {
  cl_int error;
  cl_uint platformIdCount = 0;
  error = clGetPlatformIDs(0, nullptr, &platformIdCount);
  PASS_CL_ERROR;

  ids.resize(platformIdCount);
  error = clGetPlatformIDs(platformIdCount, ids.data(), nullptr);
  PASS_CL_ERROR;

  return CL_SUCCESS;
}

cl_int getDeviceIds(cl_int platformId, std::vector<cl_device_id> &ids) {
  cl_int error;
  std::vector<cl_platform_id> platformIds;
  error = getPlatformIds(platformIds);
  PASS_CL_ERROR;

  if (platformId < 0 || platformId >= (cl_int)platformIds.size()) {
    return CL_INVALID_VALUE;
  }

  cl_uint deviceIdCount = 0;
  error = clGetDeviceIDs (platformIds[platformId], CL_DEVICE_TYPE_ALL, 0, nullptr,
    &deviceIdCount);
  PASS_CL_ERROR;
  ids.resize(deviceIdCount);
  error = clGetDeviceIDs (platformIds[platformId], CL_DEVICE_TYPE_ALL, deviceIdCount,
    ids.data(), nullptr);
  PASS_CL_ERROR;

  return CL_SUCCESS;
}

napi_status getPlatformParamString(napi_env env, cl_platform_id platformId,
  cl_platform_info param, napi_value* result) {

  cl_int error;
  napi_status status;
  size_t paramSize;
  error = clGetPlatformInfo(platformId, param, 0, nullptr, &paramSize);
  INVALID_CHECK;
  THROW_CL_ERROR;

  char* paramString = (char *) malloc(sizeof(char) * paramSize);
  error = clGetPlatformInfo(platformId, param, paramSize, paramString, &paramSize);
  THROW_CL_ERROR;

  status = napi_create_string_utf8(env, paramString, NAPI_AUTO_LENGTH, result);
  free(paramString);
  return status;
}

napi_status getPlatformParamUlong(napi_env env, cl_platform_id platformId,
  cl_platform_info param, napi_value* result) {

    cl_int error;
    napi_status status;
    cl_ulong paramLong;
    error = clGetPlatformInfo(platformId, param, sizeof(cl_ulong), &paramLong, 0);
    INVALID_CHECK;
    THROW_CL_ERROR;

    status = napi_create_int64(env, (int64_t) paramLong, result);
    return status;
}

napi_status getDeviceParamString(napi_env env, cl_device_id deviceId,
  cl_device_info param, napi_value* result) {

  cl_int error;
  napi_status status;
  size_t paramSize;
  error = clGetDeviceInfo(deviceId, param, 0, nullptr, &paramSize);
  INVALID_CHECK;
  THROW_CL_ERROR;

  char* paramString = (char *) malloc(sizeof(char) * paramSize);
  error = clGetDeviceInfo(deviceId, param, paramSize, paramString, &paramSize);
  THROW_CL_ERROR;

  status = napi_create_string_utf8(env, paramString, NAPI_AUTO_LENGTH, result);
  free(paramString);
  return status;
}

napi_status getDeviceParamBool(napi_env env, cl_device_id deviceId,
  cl_device_info param, napi_value* result) {

  cl_int error;
  napi_status status;
  cl_bool paramBool;
  error = clGetDeviceInfo(deviceId, param, sizeof(cl_bool), &paramBool, 0);
  INVALID_CHECK;
  THROW_CL_ERROR;

  status = napi_get_boolean(env, (bool) paramBool, result);
  return status;
}

napi_status getDeviceParamUint(napi_env env, cl_device_id deviceId,
  cl_device_info param, napi_value* result) {

    cl_int error;
    napi_status status;
    cl_uint paramInt;
    error = clGetDeviceInfo(deviceId, param, sizeof(cl_uint), &paramInt, 0);
    INVALID_CHECK;
    THROW_CL_ERROR;

    status = napi_create_uint32(env, (uint32_t) paramInt, result);
    return status;
}

napi_status getDeviceParamUlong(napi_env env, cl_device_id deviceId,
  cl_device_info param, napi_value* result) {

    cl_int error;
    napi_status status;
    cl_ulong paramLong;
    error = clGetDeviceInfo(deviceId, param, sizeof(cl_ulong), &paramLong, 0);
    INVALID_CHECK;
    THROW_CL_ERROR;

    status = napi_create_int64(env, (int64_t) paramLong, result);
    return status;
}

napi_status getDeviceParamSizet(napi_env env, cl_device_id deviceId,
  cl_device_info param, napi_value* result) {

    cl_int error;
    napi_status status;
    size_t paramSize;
    error = clGetDeviceInfo(deviceId, param, sizeof(size_t), &paramSize, 0);
    INVALID_CHECK;
    THROW_CL_ERROR;

    status = napi_create_uint32(env, (int32_t) paramSize, result);
    return status;
}

napi_status getDeviceParamEnum(napi_env env, cl_device_id deviceId,
  cl_device_info param, napi_value* result) {

  napi_status status;
  napi_value enumValue;
  status = getDeviceParamUint(env, deviceId, param, &enumValue);
  PASS_STATUS;

  // Field not supported on pre 2.0
  napi_valuetype enumType;
  status = napi_typeof(env, enumValue, &enumType);
  PASS_STATUS;
  if (enumType == napi_undefined) {
    *result = enumValue;
    return napi_ok;
  }

  uint32_t value;
  status = napi_get_value_uint32(env, enumValue, &value);
  PASS_STATUS;

  const char* enumLiteral = getDeviceEnumLiteral(param, value);
  status = napi_create_string_utf8(env, enumLiteral, NAPI_AUTO_LENGTH, result);
  return status;
}

napi_status getDeviceParamBitfield(napi_env env, cl_device_id deviceId,
  cl_device_info param, napi_value* result) {

  napi_status status;
  napi_value fieldValue;
  status = getDeviceParamUlong(env, deviceId, param, &fieldValue);
  PASS_STATUS;

  // Field not supported on pre 2.0
  napi_valuetype fieldType;
  status = napi_typeof(env, fieldValue, &fieldType);
  PASS_STATUS;
  if (fieldType == napi_undefined) {
    *result = fieldValue;
    return napi_ok;
  }

  int64_t value;
  status = napi_get_value_int64(env, fieldValue, &value);
  PASS_STATUS;

  status = napi_create_array(env, result);
  PASS_STATUS;

  uint32_t index = 0;
  for ( int64_t x = 0 ; x < 32 ; x++ ) {
    const char* literal = getDeviceBitfieldLiteral(param, (int64_t) 1 << x);
    if (literal == nullptr) break;
    if (value & ((int64_t) 1 << x)) {
      napi_value jsLiteral;
      status = napi_create_string_utf8(env, literal, NAPI_AUTO_LENGTH, &jsLiteral);
      PASS_STATUS;
      status = napi_set_element(env, *result, index++, jsLiteral);
      PASS_STATUS;
    }
  }

  return status;
}

napi_status getDeviceParamSizetArray(napi_env env, cl_device_id deviceId,
  cl_device_info param, napi_value* result) {

  cl_int error;
  napi_status status;
  size_t paramSize;
  error = clGetDeviceInfo(deviceId, param, 0, nullptr, &paramSize);
  INVALID_CHECK;
  THROW_CL_ERROR;

  size_t* paramArray = (size_t *) malloc(sizeof(size_t) * paramSize);
  error = clGetDeviceInfo(deviceId, param, paramSize, paramArray, &paramSize);
  THROW_CL_ERROR;

  status = napi_create_array(env, result);
  PASS_STATUS;

  for ( uint32_t x = 0 ; x < paramSize / sizeof(size_t) ; x++ ) {
    napi_value sizeValue;
    status = napi_create_int64(env, (int64_t) paramArray[x], &sizeValue);
    PASS_STATUS;
    status = napi_set_element(env, *result, x, sizeValue);
    PASS_STATUS;
  }

  free(paramArray);
  return status;
}

napi_status getDeviceParamEnumArray(napi_env env, cl_device_id deviceId,
  cl_device_info param, napi_value* result) {

  cl_int error;
  napi_status status;
  size_t paramSize;
  error = clGetDeviceInfo(deviceId, param, 0, nullptr, &paramSize);
  INVALID_CHECK;
  THROW_CL_ERROR;

  uint64_t* paramArray = (uint64_t *) malloc(sizeof(uint64_t) * paramSize);
  error = clGetDeviceInfo(deviceId, param, paramSize, paramArray, &paramSize);
  THROW_CL_ERROR;

  status = napi_create_array(env, result);
  PASS_STATUS;

  for ( uint32_t x = 0 ; x < paramSize / sizeof(uint64_t) ; x++ ) {
    napi_value literalValue;
    const char* literal = getDeviceEnumLiteral(param, (uint32_t) paramArray[x]);
    status = napi_create_string_utf8(env, literal, NAPI_AUTO_LENGTH, &literalValue);
    PASS_STATUS;
    status = napi_set_element(env, *result, x, literalValue);
    PASS_STATUS;
  }

  free(paramArray);
  return status;
}

napi_status getDeviceInfo(napi_env env, cl_device_id deviceId, napi_value* result) {
  napi_status status;

  status = napi_create_object(env, result);
  PASS_STATUS;

  napi_value param;
  for ( uint32_t x = 0 ; x < deviceParamCount ; x++) {
    status = deviceParams[x].getParam(env, deviceId, deviceParams[x].deviceInfo, &param);
    PASS_STATUS;
    status = napi_set_named_property(env, *result, deviceParams[x].name, param);
    PASS_STATUS;
  }

  return napi_ok;
}

napi_value getPlatformInfo(napi_env env, napi_callback_info info) {
  cl_int error;
  napi_status status;

  napi_value* args = nullptr;
  napi_valuetype* types = nullptr;
  status = checkArgs(env, info, "getPlatformInfo", args, (size_t) 0, types);
  CHECK_STATUS;

  std::vector<cl_platform_id> platformIds;
  error = getPlatformIds(platformIds);
  CHECK_CL_ERROR;

  napi_value platformArray;
  status = napi_create_array(env, &platformArray);
  CHECK_STATUS;
  for ( uint32_t platformId = 0 ; platformId < platformIds.size() ; platformId++ ) {
    napi_value result;
    status = napi_create_object(env, &result);
    CHECK_STATUS;

    napi_value param;
    status = getPlatformParamString(env, platformIds[platformId], CL_PLATFORM_PROFILE, &param);
    CHECK_STATUS;
    status = napi_set_named_property(env, result, "profile", param);
    CHECK_STATUS;

    status = getPlatformParamString(env, platformIds[platformId], CL_PLATFORM_VERSION, &param);
    CHECK_STATUS;
    status = napi_set_named_property(env, result, "version", param);
    CHECK_STATUS;

    status = getPlatformParamString(env, platformIds[platformId], CL_PLATFORM_NAME, &param);
    CHECK_STATUS;
    status = napi_set_named_property(env, result, "name", param);
    CHECK_STATUS;

    status = getPlatformParamString(env, platformIds[platformId], CL_PLATFORM_VENDOR, &param);
    CHECK_STATUS;
    status = napi_set_named_property(env, result, "vendor", param);
    CHECK_STATUS;

    status = getPlatformParamString(env, platformIds[platformId], CL_PLATFORM_EXTENSIONS, &param);
    CHECK_STATUS;
    status = napi_set_named_property(env, result, "extensions", param);
    CHECK_STATUS;

    status = getPlatformParamUlong(env, platformIds[platformId], CL_PLATFORM_HOST_TIMER_RESOLUTION, &param);
    CHECK_STATUS;
    status = napi_set_named_property(env, result, "hostTimerResolution", param);
    CHECK_STATUS;

    // For extensions. Include "cl_ext,h" - out of scope for now
    /* status = getPlatformParam(env, platformIds[platformId], CL_PLATFORM_ICD_SUFFIX_KHR, &param);
    CHECK_STATUS;
    status = napi_set_named_property(env, result, "icdSuffixKhr", param);
    CHECK_STATUS; */

    std::vector<cl_device_id> deviceIds;
    error = getDeviceIds(platformId, deviceIds);
    CHECK_CL_ERROR;

    napi_value deviceArray;
    status = napi_create_array(env, &deviceArray);
    CHECK_STATUS;
    for ( uint32_t deviceId = 0 ; deviceId < deviceIds.size() ; deviceId++ ) {
      napi_value deviceInfo;
      status = getDeviceInfo(env, deviceIds[deviceId], &deviceInfo);
      CHECK_STATUS;

      napi_value platformIndex;
      status = napi_create_uint32(env, platformId, &platformIndex);
      CHECK_STATUS;
      status = napi_set_named_property(env, deviceInfo, "platformIndex", platformIndex);
      CHECK_STATUS;

      napi_value deviceIndex;
      status = napi_create_uint32(env, deviceId, &deviceIndex);
      CHECK_STATUS;
      status = napi_set_named_property(env, deviceInfo, "deviceIndex", deviceIndex);
      CHECK_STATUS;

      status = napi_set_element(env, deviceArray, deviceId, deviceInfo);
      CHECK_STATUS;
    }
    status = napi_set_named_property(env, result, "devices", deviceArray);
    CHECK_STATUS;

    status = napi_set_element(env, platformArray, platformId, result);
    CHECK_STATUS;
  }

  return platformArray;
}

napi_value findFirstGPU(napi_env env, napi_callback_info info) {
  cl_int error;
  napi_status status;

  if (info != nullptr) { // If called from JS directly, check no args
    napi_value* args = nullptr;
    napi_valuetype* types = nullptr;
    status = checkArgs(env, info, "findFirstGPU", args, (size_t) 0, types);
    CHECK_STATUS;
  }

  std::vector<cl_platform_id> platformIds;
  error = getPlatformIds(platformIds);
  CHECK_CL_ERROR;

  napi_value result;
  status = napi_get_undefined(env, &result);
  CHECK_STATUS;

  for ( uint32_t x = 0 ; x < platformIds.size() ; x++ ) {
    std::vector<cl_device_id> deviceIds;
    error = getDeviceIds(x, deviceIds);
    CHECK_CL_ERROR;
    for ( uint32_t y = 0 ; y < deviceIds.size() ; y++ ) {
      cl_ulong deviceType;
      error = clGetDeviceInfo(deviceIds[y], CL_DEVICE_TYPE, sizeof(cl_ulong),
        &deviceType, nullptr);
      CHECK_CL_ERROR;
      if (deviceType == CL_DEVICE_TYPE_GPU) {
        status = napi_create_object(env, &result);
        CHECK_STATUS;
        napi_value platformIndex, deviceIndex;
        status = napi_create_uint32(env, x, &platformIndex);
        CHECK_STATUS;
        status = napi_create_uint32(env, y, &deviceIndex);
        CHECK_STATUS;
        status = napi_set_named_property(env, result, "platformIndex", platformIndex);
        CHECK_STATUS;
        status = napi_set_named_property(env, result, "deviceIndex", deviceIndex);
        CHECK_STATUS;
        break;
      }
    }
  }

  return result;
}
