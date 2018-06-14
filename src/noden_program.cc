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
  if (error != CL_SUCCESS) printf("Failed to release CL kernel.\n");
}

void tokenise(const std::string &str, const std::regex &regex, std::vector<std::string> &tokens)
{
  std::sregex_token_iterator it(str.begin(), str.end(), regex, -1);
  std::sregex_token_iterator reg_end;

  for (; it != reg_end; ++it) {
    if (!it->str().empty()) //token could be empty:check
      tokens.emplace_back(it->str());
  }
}

// Promise to create a program with context and queue
void buildExecute(napi_env env, void* data) {
  buildCarrier* c = (buildCarrier*) data;
  cl_int error;

  printf("Execution starting\n");
  printf("globalWorkItems: %zd, workItemsPerGroup: %zd\n", c->globalWorkItems, c->workItemsPerGroup);
  HR_TIME_POINT start = NOW;

  const char* kernelSource[1];
  kernelSource[0] = c->kernelSource.data();
  c->program = clCreateProgramWithSource(c->context, 1, kernelSource,
    nullptr, &error);
  ASYNC_CL_ERROR;

  error = clBuildProgram(c->program, 0, nullptr, nullptr, nullptr, nullptr);
  if (error != CL_SUCCESS) {
    size_t len;
    clGetProgramBuildInfo(c->program, c->deviceId, CL_PROGRAM_BUILD_LOG,
      0, NULL, &len);
    char* buffer = (char*)std::calloc(len, sizeof(char));

    clGetProgramBuildInfo(c->program, c->deviceId, CL_PROGRAM_BUILD_LOG,
      len, buffer, NULL);
    c->status = NODEN_BUILD_ERROR;
    c->errorMsg = std::string(buffer);
    delete[] buffer;
    return;
  }

  c->kernel = clCreateKernel(c->program, c->name.c_str(), &error);
  ASYNC_CL_ERROR;

  size_t deviceWorkGroupSize;
  error = clGetKernelWorkGroupInfo(c->kernel, c->deviceId, CL_KERNEL_WORK_GROUP_SIZE,
    sizeof(size_t), &deviceWorkGroupSize, nullptr);
  ASYNC_CL_ERROR;

  if (0 == c->workItemsPerGroup)
    c->workItemsPerGroup = deviceWorkGroupSize;
  else if (c->workItemsPerGroup > deviceWorkGroupSize) {
    c->status = NODEN_OUT_OF_RANGE;
    char* errorMsg = (char *) malloc(200);
    sprintf(errorMsg, "Property workItemsPerGroup (%zd) is larger than the available workgroup size (%zd) for platform %i.",
            c->workItemsPerGroup, deviceWorkGroupSize, c->platformIndex);
    c->errorMsg = std::string(errorMsg);
    delete[] errorMsg;
    return;
  }

  c->totalTime = microTime(start);
}

void buildComplete(napi_env env, napi_status asyncStatus, void* data) {
  buildCarrier* c = (buildCarrier*) data;
  napi_value result;

  if (asyncStatus != napi_ok) {
    c->status = asyncStatus;
    c->errorMsg = "Async build of program failed to complete.";
  }
  REJECT_STATUS;

  c->status = napi_get_reference_value(env, c->passthru, &result);
  REJECT_STATUS;

  napi_value jsDeviceId;
  c->status = napi_create_external(env, c->deviceId, nullptr, nullptr, &jsDeviceId);
  REJECT_STATUS;
  c->status = napi_set_named_property(env, result, "deviceId", jsDeviceId);
  REJECT_STATUS;

  napi_value jsExtProgram;
  c->status = napi_create_external(env, c->program, tidyProgram, nullptr, &jsExtProgram);
  REJECT_STATUS;
  c->status = napi_set_named_property(env, result, "program", jsExtProgram);
  REJECT_STATUS;

  napi_value jsKernel;
  c->status = napi_create_external(env, c->kernel, tidyKernel, nullptr, &jsKernel);
  REJECT_STATUS;
  c->status = napi_set_named_property(env, result, "kernel", jsKernel);
  REJECT_STATUS;

  napi_value jsBuildTime;
  c->status = napi_create_double(env, c->totalTime / 1000000.0, &jsBuildTime);
  REJECT_STATUS;
  c->status = napi_set_named_property(env, result, "buildTime", jsBuildTime);
  REJECT_STATUS;

  napi_value globalWorkItemsValue;
  c->status = napi_create_int64(env, (int64_t) c->globalWorkItems, &globalWorkItemsValue);
  REJECT_STATUS;
  c->status = napi_set_named_property(env, result, "globalWorkItems", globalWorkItemsValue);
  REJECT_STATUS;

  napi_value workItemsPerGroupValue;
  c->status = napi_create_int64(env, (int64_t) c->workItemsPerGroup, &workItemsPerGroupValue);
  REJECT_STATUS;
  c->status = napi_set_named_property(env, result, "workItemsPerGroup", workItemsPerGroupValue);
  REJECT_STATUS;

  napi_value runValue;
  c->status = napi_create_function(env, "run", NAPI_AUTO_LENGTH, run,
    nullptr, &runValue);
  REJECT_STATUS;
  c->status = napi_set_named_property(env, result, "run", runValue);
  REJECT_STATUS;

  napi_status status;
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
  napi_value contextValue;
  status = napi_get_cb_info(env, info, &argc, args, &contextValue, nullptr);
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
  char* kernelSource = (char*) malloc(carrier->sourceLength + 1);
  status = napi_get_value_string_utf8(env, args[0], kernelSource, carrier->sourceLength + 1, nullptr);
  CHECK_STATUS;
  carrier->kernelSource = std::string(kernelSource);
  delete kernelSource;

  napi_value config = args[1];
  status = napi_typeof(env, config, &t);
  CHECK_STATUS;
  if (t != napi_object) {
    status = napi_throw_type_error(env, nullptr, "Configuration parameters must be an object.");
    return nullptr;
  }

  bool hasProp;
  napi_value nameValue;
  status = napi_has_named_property(env, config, "name", &hasProp);
  CHECK_STATUS;
  if (hasProp) {
    status = napi_get_named_property(env, config, "name", &nameValue);
    CHECK_STATUS;
  } else {
    std::regex re("__kernel\\s+void\\s+([^\\s\\(]+)\\s*\\(");
    std::smatch match;
    std::string parsedName;
    if (std::regex_search(carrier->kernelSource, match, re) && match.size() > 1) {
      parsedName = match.str(1);
    } else {
      parsedName = std::string("noden");
    }
    status = napi_create_string_utf8(env, parsedName.c_str(), parsedName.length(), &nameValue);
    CHECK_STATUS;
  }
  status = napi_set_named_property(env, program, "name", nameValue);
  CHECK_STATUS;
  status = napi_get_value_string_utf8(env, nameValue, nullptr, 0, &carrier->nameLength);
  CHECK_STATUS;

  char* name = (char *) malloc(carrier->nameLength + 1);
  status = napi_get_value_string_utf8(env, nameValue, name, carrier->nameLength + 1, nullptr);
  CHECK_STATUS;
  carrier->name = std::string(name);
  delete name;

  napi_value kernelParamsValue;
  status = napi_create_object(env, &kernelParamsValue);
  CHECK_STATUS;

  // parse the program parameters
  std::string ks(carrier->kernelSource);
  size_t startParams = ks.find('(') + 1;
  std::string paramStr(ks.substr(startParams, ks.find(')') - startParams));
  std::vector<std::string> params;
  tokenise(paramStr, std::regex(",+"), params);
  uint32_t pm=0;
  for (auto& param: params) {
    std::vector<std::string> paramTokens;
    tokenise(param, std::regex("\\s+"), paramTokens);

    std::string paramTypeStr;
    if (0 == paramTokens[paramTokens.size()-3].compare("unsigned"))
      paramTypeStr = std::string("u");
    paramTypeStr.append(paramTokens[paramTokens.size()-2]);

    napi_value paramType;
    status = napi_create_string_utf8(env, paramTypeStr.c_str(), NAPI_AUTO_LENGTH, &paramType);
    CHECK_STATUS;

    status = napi_set_named_property(env, kernelParamsValue, paramTokens.back().c_str(), paramType);
    CHECK_STATUS;
  }
  status = napi_set_named_property(env, program, "kernelParams", kernelParamsValue);
  CHECK_STATUS;

  napi_value globalWorkItemsValue;
  status = napi_has_named_property(env, config, "globalWorkItems", &hasProp);
  CHECK_STATUS;
  if (hasProp) {
    status = napi_get_named_property(env, config, "globalWorkItems", &globalWorkItemsValue);
    CHECK_STATUS;
  } else {
    status = napi_create_int64(env, 0, &globalWorkItemsValue);
    CHECK_STATUS;
  }
  status = napi_get_value_int64(env, globalWorkItemsValue, (int64_t*)&carrier->globalWorkItems);
  CHECK_STATUS;
  status = napi_set_named_property(env, program, "globalWorkItems", globalWorkItemsValue);
  CHECK_STATUS;

  napi_value workItemsPerGroupValue;
  status = napi_has_named_property(env, config, "workItemsPerGroup", &hasProp);
  CHECK_STATUS;
  if (hasProp) {
    status = napi_get_named_property(env, config, "workItemsPerGroup", &workItemsPerGroupValue);
    CHECK_STATUS;
  } else {
    status = napi_create_int64(env, 0, &workItemsPerGroupValue);
    CHECK_STATUS;
  }
  status = napi_get_value_int64(env, workItemsPerGroupValue, (int64_t*)&carrier->workItemsPerGroup);
  CHECK_STATUS;
  status = napi_set_named_property(env, program, "workItemsPerGroup", workItemsPerGroupValue);
  CHECK_STATUS;

  napi_value jsContext;
  status = napi_get_named_property(env, contextValue, "context", &jsContext);
  CHECK_STATUS;
  void* contextData;
  status = napi_get_value_external(env, jsContext, &contextData);
  CHECK_STATUS;
  carrier->context = (cl_context) contextData;
  status = napi_set_named_property(env, program, "context", jsContext);
  CHECK_STATUS;

  napi_value jsCommands;
  status = napi_get_named_property(env, contextValue, "commands", &jsCommands);
  CHECK_STATUS;
  void* commandsData;
  status = napi_get_value_external(env, jsCommands, &commandsData);
  CHECK_STATUS;
  carrier->commands = (cl_command_queue) commandsData;
  status = napi_set_named_property(env, program, "commands", jsCommands);
  CHECK_STATUS;

  napi_value platformValue;
  status = napi_get_named_property(env, contextValue, "platformIndex", &platformValue);
  CHECK_STATUS;
  status = napi_get_value_uint32(env, platformValue, &carrier->platformIndex);
  CHECK_STATUS;

  napi_value deviceValue;
  status = napi_get_named_property(env, contextValue, "deviceIndex", &deviceValue);
  CHECK_STATUS;
  status = napi_get_value_uint32(env, deviceValue, &carrier->deviceIndex);
  CHECK_STATUS;

  napi_value deviceIdValue;
  status = napi_get_named_property(env, contextValue, "deviceId", &deviceIdValue);
  CHECK_STATUS;
  void* deviceIdData;
  status = napi_get_value_external(env, deviceIdValue, &deviceIdData);
  CHECK_STATUS;
  carrier->deviceId = (cl_device_id) deviceIdData;

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
