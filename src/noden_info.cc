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

#ifdef __APPLE__
    #include "OpenCL/opencl.h"
#else
    #include "CL/cl.h"
#endif
#include <vector>
#include "noden_util.h"
#include "noden_info.h"

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

cl_int getDeviceIds(cl_uint platformId, std::vector<cl_device_id> &ids) {
  cl_int error;
  std::vector<cl_platform_id> platformIds;
  error = getPlatformIds(platformIds);
  PASS_CL_ERROR;

  if (platformId < 0 || platformId >= platformIds.size()) {
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

napi_status getPlatformParam(napi_env env, cl_platform_id platformId,
  cl_platform_info param, napi_value* result) {

  cl_int error;
  napi_status status;
  size_t paramSize;
  error = clGetPlatformInfo(platformId, param, 0, nullptr, &paramSize);
  THROW_CL_ERROR;

  char* paramString = (char *) malloc(sizeof(char) * paramSize);
  error = clGetPlatformInfo(platformId, param, paramSize, paramString, &paramSize);
  THROW_CL_ERROR;
  free(paramString);

  status = napi_create_string_utf8(env, paramString, NAPI_AUTO_LENGTH, result);
  return status;
}

napi_status getDeviceParamString(napi_env env, cl_device_id deviceId,
  cl_device_info param, napi_value* result) {

  cl_int error;
  napi_status status;
  size_t paramSize;
  error = clGetDeviceInfo(deviceId, param, 0, nullptr, &paramSize);
  THROW_CL_ERROR;

  char* paramString = (char *) malloc(sizeof(char) * paramSize);
  error = clGetDeviceInfo(deviceId, param, paramSize, paramString, &paramSize);
  THROW_CL_ERROR;
  free(paramString);

  status = napi_create_string_utf8(env, paramString, NAPI_AUTO_LENGTH, result);
  return status;
}

napi_status getDeviceParamBool(napi_env env, cl_device_id deviceId,
  cl_device_info param, napi_value* result) {

  cl_int error;
  napi_status status;
  cl_bool paramBool;
  error = clGetDeviceInfo(deviceId, param, sizeof(cl_bool), &paramBool, 0);
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
    THROW_CL_ERROR;

    status = napi_create_uint32(env, (int32_t) paramSize, result);
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

  /* status = getDeviceParamString(env, deviceId, CL_DEVICE_NAME, &param);
  PASS_STATUS;
  status = napi_set_named_property(env, *result, "name", param);
  PASS_STATUS;

  status = getDeviceParamString(env, deviceId, CL_DEVICE_BUILT_IN_KERNELS, &param);
  PASS_STATUS;
  status = napi_set_named_property(env, *result, "builtInKernels", param);
  PASS_STATUS;

  status = getDeviceParamString(env, deviceId, CL_DEVICE_EXTENSIONS, &param);
  PASS_STATUS;
  status = napi_set_named_property(env, *result, "deviceExtensions", param);
  PASS_STATUS;

  status = getDeviceParamString(env, deviceId, CL_DEVICE_OPENCL_C_VERSION, &param);
  PASS_STATUS;
  status = napi_set_named_property(env, *result, "openclCVersion", param);
  PASS_STATUS;

  status = getDeviceParamString(env, deviceId, CL_DEVICE_PROFILE, &param);
  PASS_STATUS;
  status = napi_set_named_property(env, *result, "deviceProfile", param);
  PASS_STATUS;

  // Property of "cl_ext.h" - out of scope for now
  /* status = getDeviceParamString(env, deviceId, CL_DEVICE_SPIR_VERSIONS, &param);
  PASS_STATUS;
  status = napi_set_named_property(env, *result, "spirVersions", param);
  PASS_STATUS; */

  /* status = getDeviceParamString(env, deviceId, CL_DEVICE_VENDOR, &param);
  PASS_STATUS;
  status = napi_set_named_property(env, *result, "vendor", param);
  PASS_STATUS;

  status = getDeviceParamString(env, deviceId, CL_DEVICE_VERSION, &param);
  PASS_STATUS;
  status = napi_set_named_property(env, *result, "version", param);
  PASS_STATUS;

  status = getDeviceParamString(env, deviceId, CL_DRIVER_VERSION, &param);
  PASS_STATUS;
  status = napi_set_named_property(env, *result, "driverVersion", param);
  PASS_STATUS;

  status = getDeviceParamBool(env, deviceId, CL_DEVICE_AVAILABLE, &param);
  PASS_STATUS;
  status = napi_set_named_property(env, *result, "available", param);
  PASS_STATUS;

  status = getDeviceParamBool(env, deviceId, CL_DEVICE_COMPILER_AVAILABLE, &param);
  PASS_STATUS;
  status = napi_set_named_property(env, *result, "compilerAvailable", param);
  PASS_STATUS;

  status = getDeviceParamBool(env, deviceId, CL_DEVICE_ENDIAN_LITTLE, &param);
  PASS_STATUS;
  status = napi_set_named_property(env, *result, "endianLittle", param);
  PASS_STATUS;

  status = getDeviceParamBool(env, deviceId, CL_DEVICE_ERROR_CORRECTION_SUPPORT, &param);
  PASS_STATUS;
  status = napi_set_named_property(env, *result, "errorCorrectionSupport", param);
  PASS_STATUS;

  status = getDeviceParamBool(env, deviceId, CL_DEVICE_IMAGE_SUPPORT, &param);
  PASS_STATUS;
  status = napi_set_named_property(env, *result, "imageSupport", param);
  PASS_STATUS;

  status = getDeviceParamBool(env, deviceId, CL_DEVICE_LINKER_AVAILABLE, &param);
  PASS_STATUS;
  status = napi_set_named_property(env, *result, "linkedAvailable", param);
  PASS_STATUS;

  status = getDeviceParamBool(env, deviceId, CL_DEVICE_PREFERRED_INTEROP_USER_SYNC, &param);
  PASS_STATUS;
  status = napi_set_named_property(env, *result, "preferredIntropUserSync", param);
  PASS_STATUS; */

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
    status = getPlatformParam(env, platformIds[platformId], CL_PLATFORM_PROFILE, &param);
    CHECK_STATUS;
    status = napi_set_named_property(env, result, "profile", param);
    CHECK_STATUS;

    status = getPlatformParam(env, platformIds[platformId], CL_PLATFORM_VERSION, &param);
    CHECK_STATUS;
    status = napi_set_named_property(env, result, "version", param);
    CHECK_STATUS;

    status = getPlatformParam(env, platformIds[platformId], CL_PLATFORM_NAME, &param);
    CHECK_STATUS;
    status = napi_set_named_property(env, result, "name", param);
    CHECK_STATUS;

    status = getPlatformParam(env, platformIds[platformId], CL_PLATFORM_VENDOR, &param);
    CHECK_STATUS;
    status = napi_set_named_property(env, result, "vendor", param);
    CHECK_STATUS;

    status = getPlatformParam(env, platformIds[platformId], CL_PLATFORM_EXTENSIONS, &param);
    CHECK_STATUS;
    status = napi_set_named_property(env, result, "extensions", param);
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
