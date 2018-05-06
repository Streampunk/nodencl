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
#include <chrono>
#include <vector>
#include <regex>
#include <stdio.h>
#include "node_api.h"
#include "noden_util.h"
#include "noden_info.h"
#include "noden_program.h"
#include "noden_buffer.h"
#include "noden_run.h"

void tidyContext(napi_env env, void* data, void* hint) {
  printf("Context finalizer called.\n");
  cl_int error = CL_SUCCESS;
  error = clReleaseContext((cl_context) data);
  if (error != CL_SUCCESS) printf("Failed to release CL context.\n");
}

void tidyQueue(napi_env env, void* data, void* hint) {
  printf("Queue finalizer called.\n");
  cl_int error = CL_SUCCESS;
  error = clReleaseCommandQueue((cl_command_queue) data);
  if (error != CL_SUCCESS) printf("Failed to release CL queue.\n");
}

void tidyProgram(napi_env env, void* data, void* hint) {
  printf("Program finalizer called.\n");
  cl_int error = CL_SUCCESS;
  error = clReleaseProgram((cl_program) data);
  if (error != CL_SUCCESS) printf("Failed to release CL program.\n");
}

void tidyKernel(napi_env env, void* data, void* hint) {
  printf("Kernel finalizer called.\n");
  cl_int error = CL_SUCCESS;
  error = clReleaseKernel((cl_kernel) data);
  if (error == CL_SUCCESS) printf("Failed to relase CL kernel.\n");
}

// Promise to create a program with context and queue
void buildExecute(napi_env env, void* data) {
  buildCarrier* c = (buildCarrier*) data;
  cl_int error;

  printf("Execution starting\n");
  HR_TIME_POINT start = NOW;

  std::vector<cl_platform_id> platformIds;
  error = getPlatformIds(platformIds);
  ASYNC_CL_ERROR;

  if (c->platformIndex >= platformIds.size()) {
    c->status = NODEN_OUT_OF_RANGE;
    c->errorMsg = "Property platformIndex is larger than the available number of platforms.";
    return;
  }

  std::vector<cl_device_id> deviceIds;
  error = getDeviceIds(c->platformIndex, deviceIds);
  ASYNC_CL_ERROR;

  if (c->deviceIndex >= deviceIds.size()) {
    c->status = NODEN_OUT_OF_RANGE;
    c->errorMsg = (char *) malloc(200);
    sprintf(c->errorMsg, "Property deviceIndex is larger than the available number of devices for platform %i.", c->platformIndex);
    printf("got here %s\n", c->errorMsg);
    return;
  }

  c->deviceId = deviceIds[c->deviceIndex];
  c->context = clCreateContext(0, 1, &c->deviceId, nullptr, nullptr, &error);
  ASYNC_CL_ERROR;

  c->commands = clCreateCommandQueue(c->context, c->deviceId, 0, &error);
  ASYNC_CL_ERROR;

  c->program = clCreateProgramWithSource(c->context, 1, (const char **) &c->kernelSource,
    nullptr, &error);
  ASYNC_CL_ERROR;

  error = clBuildProgram(c->program, 0, nullptr, nullptr, nullptr, nullptr);
  if (error != CL_SUCCESS) {
    size_t len;
    char buffer[2048];

    clGetProgramBuildInfo(c->program, c->deviceId, CL_PROGRAM_BUILD_LOG,
      sizeof(buffer), buffer, &len);
    c->status = NODEN_BUILD_ERROR;
    c->errorMsg = buffer;
    return;
  }

  c->kernel = clCreateKernel(c->program, c->name, &error);
  ASYNC_CL_ERROR;

  c->totalTime = microTime(start);
}

void buildComplete(napi_env env, napi_status asyncStatus, void* data) {
  buildCarrier* c = (buildCarrier*) data;
  napi_status status;
  napi_value result, source;

  if (asyncStatus != napi_ok) {
    c->status = asyncStatus;
    c->errorMsg = "Async build of program failed to complete.";
  }
  REJECT_STATUS;

  c->status = napi_get_reference_value(env, c->passthru, &result);
  REJECT_STATUS;

  napi_value jsDeviceId;
  status = napi_create_external(env, c->deviceId, nullptr, nullptr, &jsDeviceId);
  REJECT_STATUS;
  status = napi_set_named_property(env, result, "deviceId", jsDeviceId);
  REJECT_STATUS;

  napi_value jsContext;
  status = napi_create_external(env, c->context, tidyContext, nullptr, &jsContext);
  REJECT_STATUS;
  status = napi_set_named_property(env, result, "context", jsContext);
  REJECT_STATUS;

  napi_value jsCommands;
  status = napi_create_external(env, c->commands, tidyQueue, nullptr, &jsCommands);
  REJECT_STATUS;
  status = napi_set_named_property(env, result, "commands", jsCommands);
  REJECT_STATUS;

  napi_value jsExtProgram;
  status = napi_create_external(env, c->program, tidyProgram, nullptr, &jsExtProgram);
  REJECT_STATUS;
  status = napi_set_named_property(env, result, "program", jsExtProgram);
  REJECT_STATUS;

  napi_value jsKernel;
  status = napi_create_external(env, c->program, tidyKernel, nullptr, &jsKernel);
  REJECT_STATUS;
  status = napi_set_named_property(env, result, "kernel", jsKernel);
  REJECT_STATUS;

  napi_value jsBuildTime;
  status = napi_create_double(env, c->totalTime / 1000000.0, &jsBuildTime);
  REJECT_STATUS;
  status = napi_set_named_property(env, result, "buildTime", jsBuildTime);
  REJECT_STATUS;

  napi_value createBufValue;
  status = napi_create_function(env, "createBuffer", NAPI_AUTO_LENGTH,
    createBuffer, nullptr, &createBufValue);
  REJECT_STATUS;
  status = napi_set_named_property(env, result, "createBuffer", createBufValue);
  REJECT_STATUS;

  napi_value runValue;
  status = napi_create_function(env, "run", NAPI_AUTO_LENGTH, run,
    nullptr, &runValue);
  REJECT_STATUS;
  status = napi_set_named_property(env, result, "run", runValue);
  REJECT_STATUS;

  status = napi_resolve_deferred(env, c->_deferred, result);
  FLOATING_STATUS;

  tidyCarrier(env, c);
}

napi_value createProgram(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value promise;
  napi_value resource_name;
  buildCarrier* carrier = new buildCarrier;

  napi_value args[2];
  size_t argc = 2;
  status = napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
  CHECK_STATUS;

  if (argc < 1 || argc > 2) {
    status = napi_throw_error(env, nullptr, "Wrong number of arguments.");
    return nullptr;
  }

  napi_valuetype t;
  status = napi_typeof(env, args[0], &t);
  if (t != napi_string) {
    status = napi_throw_type_error(env, nullptr, "First argument should be a string - the kernel program.");
    return nullptr;
  }

  napi_value program;
  status = napi_create_object(env, &program);
  CHECK_STATUS;

  status = napi_set_named_property(env, program, "kernelSource", args[0]);
  CHECK_STATUS;

  status = napi_get_value_string_utf8(env, args[0], nullptr, 0, &carrier->sourceLength);
  CHECK_STATUS;
  carrier->kernelSource = (char*) malloc(carrier->sourceLength + 1);
  status = napi_get_value_string_utf8(env, args[0], carrier->kernelSource,
    carrier->sourceLength + 1, nullptr);
  CHECK_STATUS;

  napi_value config;
  if (argc == 1) {
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
    config = args[1];
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
    status = napi_throw_type_error(env, nullptr, "Configuration parameters must have platfromIndex.");
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

  status = napi_get_value_uint32(env, platformValue, &carrier->platformIndex);
  CHECK_STATUS;
  status = napi_get_value_uint32(env, deviceValue, &carrier->deviceIndex);
  CHECK_STATUS;

  status = napi_set_named_property(env, program, "platformIndex", platformValue);
  CHECK_STATUS;
  status = napi_set_named_property(env, program, "deviceIndex", deviceValue);
  CHECK_STATUS;

  napi_value nameValue;
  status = napi_has_named_property(env, config, "name", &hasProp);
  CHECK_STATUS;
  if (hasProp) {
    status = napi_get_named_property(env, config, "name", &nameValue);
    CHECK_STATUS;
  } else {
    std::regex re("__kernel\\s+void\\s+([^\\s\\(]+)\\s*\\(");
    std::cmatch match;
    std::string parsedName;
    if (std::regex_search(carrier->kernelSource, match, re) && match.size() > 1) {
      parsedName = match.str(1);
    } else {
      parsedName = std::string("noden");
    }
    status = napi_create_string_utf8(env, parsedName.data(), parsedName.length(), &nameValue);
    CHECK_STATUS;
  }
  status = napi_set_named_property(env, program, "name", nameValue);
  CHECK_STATUS;
  status = napi_get_value_string_utf8(env, nameValue, nullptr, 0, &carrier->nameLength);
  CHECK_STATUS;
  carrier->name = (char *) malloc(carrier->nameLength + 1);
  status = napi_get_value_string_utf8(env, nameValue, carrier->name, carrier->nameLength + 1, nullptr);
  CHECK_STATUS;

  status = napi_create_reference(env, program, 1, &carrier->passthru);
  CHECK_STATUS;

  status = napi_create_promise(env, &carrier->_deferred, &promise);
  CHECK_STATUS;

  status = napi_create_string_utf8(env, "BuildProgram", NAPI_AUTO_LENGTH, &resource_name);
  CHECK_STATUS;
  status = napi_create_async_work(env, NULL, resource_name, buildExecute,
    buildComplete, carrier, &carrier->_request);
  CHECK_STATUS;
  status = napi_queue_async_work(env, carrier->_request);
  CHECK_STATUS;

  return promise;
}
