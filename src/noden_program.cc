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
#include <stdio.h>
#include <windows.h>
#include "node_api.h"
#include "noden_util.h"
#include "noden_info.h"
#include "noden_program.h"

// Promise to create a program with context and queue
void buildExecute(napi_env env, void* data) {
  buildCarrier* c = (buildCarrier*) data;
  cl_int error;

  printf("Execution starting\n");

  std::vector<cl_platform_id> platformIds;
  error = getPlatformIds(platformIds);

  if (c->platformIndex >= platformIds.size()) {
    c->status = NODEN_OUT_OF_RANGE;
    c->errorMsg = "Property platformIndex is larger than the available number of platforms.";
    return;
  }

  std::vector<cl_device_id> deviceIds;
  error = getDeviceIds(c->platformIndex, deviceIds);

  if (c->deviceIndex >= deviceIds.size()) {
    c->status = NODEN_OUT_OF_RANGE;
    c->errorMsg = (char *) malloc(200);
    sprintf(c->errorMsg, "Property deviceIndex is larger than the available number of devices for platform %i.", c->platformIndex);
    printf("got here %s\n", c->errorMsg);
    return;
  }

  Sleep(1000);
}

void buildComplete(napi_env env, napi_status asyncStatus, void* data) {
  buildCarrier* c = (buildCarrier*) data;
  napi_status status;
  napi_value result, source;

  if (asyncStatus != napi_ok) {
    c->status = asyncStatus;
    c->errorMsg = "Async build of program failed to complete.";
  }

  if (c->status != NODEN_SUCCESS) {
    napi_value errorValue, errorCode, errorMsg;
    char errorChars[20];
    status = napi_create_string_utf8(env, itoa(c->status, errorChars, 10),
      NAPI_AUTO_LENGTH, &errorCode);
    status = napi_create_string_utf8(env, c->errorMsg, NAPI_AUTO_LENGTH, &errorMsg);
    status = napi_create_error(env, errorCode, errorMsg, &errorValue);
    status = napi_reject_deferred(env, c->_deferred, errorValue);
    napi_delete_reference(env, c->program);
    delete(c);
    napi_delete_async_work(env, c->_request);
    return;
  }

  status = napi_get_reference_value(env, c->program, &result);

  status = napi_resolve_deferred(env, c->_deferred, result);

  napi_delete_reference(env, c->program);
  delete(c);
  napi_delete_async_work(env, c->_request);
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

  status = napi_create_reference(env, program, 1, &carrier->program);
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
