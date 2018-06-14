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
#include "noden_context.h"
#include "noden_util.h"
#include "noden_info.h"
#include "noden_program.h"
#include "noden_buffer.h"

void finalizeContext(napi_env env, void* data, void* hint) {
  printf("Context finalizer called.\n");
  cl_int error = CL_SUCCESS;
  error = clReleaseContext((cl_context) data);
  if (error != CL_SUCCESS) printf("Failed to release CL context.\n");
}

void finalizeCommands(napi_env env, void* data, void* hint) {
  printf("Command Queue finalizer called.\n");
  cl_int error = CL_SUCCESS;
  error = clReleaseCommandQueue((cl_command_queue) data);
  if (error != CL_SUCCESS) printf("Failed to release CL queue.\n");
}

napi_value createContext(napi_env env, napi_callback_info info) {
  napi_status status;

  napi_value args[2];
  size_t argc = 2;
  status = napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
  CHECK_STATUS;

  if (argc > 1) {
    status = napi_throw_error(env, nullptr, "Wrong number of arguments.");
    return nullptr;
  }

  napi_valuetype t;
  napi_value config;
  if (0 == argc) {
    config = findFirstGPU(env, nullptr);
    if (config == nullptr) {
      status = napi_throw_error(env, nullptr, "Find first GPU failed.");
      return nullptr;
    }
    status = napi_typeof(env, config, &t);
    CHECK_STATUS;
    if (t == napi_undefined) {
      status = napi_throw_error(env, nullptr, "Failed to find a GPU on this system.");
      return nullptr;
    }
  } else {
    config = args[0];
  }

  status = napi_typeof(env, config, &t);
  CHECK_STATUS;
  if (t != napi_object) {
    status = napi_throw_type_error(env, nullptr, "Configuration parameters must be an object.");
    return nullptr;
  }
  bool hasProp;
  status = napi_has_named_property(env, config, "platformIndex", &hasProp);
  CHECK_STATUS;
  if (!hasProp) {
    status = napi_throw_type_error(env, nullptr, "Configuration parameters must have platformIndex.");
    return nullptr;
  }
  status = napi_has_named_property(env, config, "deviceIndex", &hasProp);
  CHECK_STATUS;
  if (!hasProp) {
    status = napi_throw_type_error(env, nullptr, "Configuration parameters must have deviceIndex.");
    return nullptr;
  }

  napi_value platformValue, deviceValue;
  status = napi_get_named_property(env, config, "platformIndex", &platformValue);
  CHECK_STATUS;
  status = napi_typeof(env, platformValue, &t);
  CHECK_STATUS;
  if (t != napi_number) {
    status = napi_throw_type_error(env, nullptr, "Configuration parameter platformIndex must be a number.");
    return nullptr;
  }
  status = napi_get_named_property(env, config, "deviceIndex", &deviceValue);
  CHECK_STATUS;
  status = napi_typeof(env, deviceValue, &t);
  CHECK_STATUS;
  if (t != napi_number) {
    status = napi_throw_type_error(env, nullptr, "Configuration parameter deviceIndex must be a number.");
    return nullptr;
  }

  int32_t checkValue;
  status = napi_get_value_int32(env, platformValue, &checkValue);
  CHECK_STATUS;
  if (checkValue < 0) {
    status = napi_throw_range_error(env, nullptr, "Configuration parameter platformIndex cannot be negative.");
    return nullptr;
  }
  status = napi_get_value_int32(env, deviceValue, &checkValue);
  CHECK_STATUS;
  if (checkValue < 0) {
    status = napi_throw_range_error(env, nullptr, "Configuration parameter deviceIndex cannot be negative.");
    return nullptr;
  }

  uint32_t platformIndex, deviceIndex;
  status = napi_get_value_uint32(env, platformValue, &platformIndex);
  CHECK_STATUS;
  status = napi_get_value_uint32(env, deviceValue, &deviceIndex);
  CHECK_STATUS;

  cl_int error;
  std::vector<cl_platform_id> platformIds;
  error = getPlatformIds(platformIds);
  CHECK_CL_ERROR;

  if (platformIndex >= platformIds.size()) {
    status = napi_throw_range_error(env, nullptr, "Property platformIndex is larger than the available number of platforms.");
    return nullptr;
  }

  std::vector<cl_device_id> deviceIds;
  error = getDeviceIds(platformIndex, deviceIds);
  CHECK_CL_ERROR;

  if (deviceIndex >= deviceIds.size()) {
    char *errorMsg = (char *) malloc(200);
    sprintf(errorMsg, "Property deviceIndex is larger than the available number of devices for platform %i.", platformIndex);
    status = napi_throw_range_error(env, nullptr, errorMsg);
    delete[] errorMsg;
    return nullptr;
  }

  cl_platform_id platformId = platformIds[platformIndex];
  cl_device_id deviceId = deviceIds[deviceIndex];

  cl_ulong svmCaps;
  error = clGetDeviceInfo(deviceId, CL_DEVICE_SVM_CAPABILITIES, sizeof(cl_ulong), &svmCaps, nullptr);
  if (error == CL_INVALID_VALUE) {
    svmCaps = 0;
  } else {
    CHECK_CL_ERROR;
  }

  napi_value context;
  status = napi_create_object(env, &context);
  CHECK_STATUS;

  napi_value svmValue;
  status = napi_create_int64(env, (int64_t)svmCaps, &svmValue);
  CHECK_STATUS;
  status = napi_set_named_property(env, context, "svmCaps", svmValue);
  CHECK_STATUS;

  cl_device_id contextDevices[1];
  contextDevices[0] = deviceId;
  cl_context_properties properties[] =
    { CL_CONTEXT_PLATFORM, (cl_context_properties)platformId, 0 };
  cl_context clContext = clCreateContext(properties, 1, contextDevices, nullptr, nullptr, &error);
  CHECK_CL_ERROR;

  cl_command_queue commands = clCreateCommandQueue(clContext, deviceId, 0, &error);
  CHECK_CL_ERROR;

  napi_value jsContext;
  status = napi_create_external(env, clContext, finalizeContext, nullptr, &jsContext);
  CHECK_STATUS;
  status = napi_set_named_property(env, context, "context", jsContext);
  CHECK_STATUS;

  napi_value jsCommands;
  status = napi_create_external(env, commands, finalizeCommands, nullptr, &jsCommands);
  CHECK_STATUS;
  status = napi_set_named_property(env, context, "commands", jsCommands);
  CHECK_STATUS;

  status = napi_set_named_property(env, context, "platformIndex", platformValue);
  CHECK_STATUS;
  status = napi_set_named_property(env, context, "deviceIndex", deviceValue);
  CHECK_STATUS;

  napi_value deviceIdValue;
  status = napi_create_external(env, deviceId, nullptr, nullptr, &deviceIdValue);
  CHECK_STATUS;
  status = napi_set_named_property(env, context, "deviceId", deviceIdValue);
  CHECK_STATUS;

  napi_value createProgramValue;
  status = napi_create_function(env, "createProgram", NAPI_AUTO_LENGTH,
    createProgram, nullptr, &createProgramValue);
  CHECK_STATUS;
  status = napi_set_named_property(env, context, "createProgram", createProgramValue);
  CHECK_STATUS;

  napi_value createBufValue;
  status = napi_create_function(env, "createBuffer", NAPI_AUTO_LENGTH,
    createBuffer, nullptr, &createBufValue);
  CHECK_STATUS;
  status = napi_set_named_property(env, context, "createBuffer", createBufValue);
  CHECK_STATUS;

  return context;
}
