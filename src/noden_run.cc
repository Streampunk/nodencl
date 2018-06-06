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
#include <stdio.h>
#include "node_api.h"
#include "noden_util.h"
#include "noden_info.h"
#include "noden_buffer.h"
#include "noden_run.h"

void runExecute(napi_env env, void* data) {
  runCarrier* c = (runCarrier*) data;
  cl_int error;
  size_t inputSize = 0;

  HR_TIME_POINT bufAlloc = NOW;
  // Not recording buffer create time - should probably be done once, before here

  for (auto& paramIter: c->kernelParams) {
    uint32_t p = paramIter.first;
    kernelParam* param = paramIter.second;
    if (param->isBuf) {
      if (NODEN_SVM_NONE_CHAR == param->bufType) {
        param->buffer = clCreateBuffer(c->context, (param->bufIsInput?CL_MEM_READ_ONLY:CL_MEM_WRITE_ONLY) | CL_MEM_ALLOC_HOST_PTR,
          param->bufSize, nullptr, &error);
        ASYNC_CL_ERROR;
      }
      if (!inputSize && param->bufIsInput)
        inputSize = (size_t)param->bufSize;
    }
  }

  // printf("Took %lluus to create GPU buffers.\n", microTime(bufAlloc));
  HR_TIME_POINT start = NOW;
  HR_TIME_POINT dataToKernelStart = start;

  for (auto& paramIter: c->kernelParams) {
    uint32_t p = paramIter.first;
    kernelParam* param = paramIter.second;
    if (param->isBuf) {
      if (NODEN_SVM_COARSE_CHAR == param->bufType) {
        error = clEnqueueSVMUnmap(c->commands, param->value.ptr, 0, 0, 0);
        ASYNC_CL_ERROR;
      }

      if (NODEN_SVM_NONE_CHAR != param->bufType) {
        error = clSetKernelArgSVMPointer(c->kernel, p, param->value.ptr);
        ASYNC_CL_ERROR;
      } else {
        if (param->bufIsInput) {
          error = clEnqueueWriteBuffer(c->commands, param->buffer, CL_TRUE, 0, param->bufSize,
            param->value.ptr, 0, nullptr, nullptr);
          ASYNC_CL_ERROR;
        }
        error = clSetKernelArg(c->kernel, p, sizeof(cl_mem), &param->buffer);
        ASYNC_CL_ERROR;
      }
    }
  }

  for (auto& paramIter: c->kernelParams) {
    uint32_t p = paramIter.first;
    kernelParam* param = paramIter.second;
    if (0 == param->type.compare("uint")) {
      error = clSetKernelArg(c->kernel, p, sizeof(uint32_t), &param->value.uint32);
      ASYNC_CL_ERROR;
    } else if (0 == param->type.compare("int")) {
      error = clSetKernelArg(c->kernel, p, sizeof(int32_t), &param->value.int32);
      ASYNC_CL_ERROR;
    } else if (0 == param->type.compare("long")) {
      error = clSetKernelArg(c->kernel, p, sizeof(int64_t), &param->value.int64);
      ASYNC_CL_ERROR;
    } else if (0 == param->type.compare("float")) {
      error = clSetKernelArg(c->kernel, p, sizeof(float), &param->value.dbl);
      ASYNC_CL_ERROR;
    } else if (0 == param->type.compare("double")) {
      error = clSetKernelArg(c->kernel, p, sizeof(double), &param->value.dbl);
      ASYNC_CL_ERROR;
    }
    ++p;
  }

  c->dataToKernel = microTime(dataToKernelStart);
  HR_TIME_POINT kernelExecStart = NOW;

  size_t global = (0 == c->globalWorkItems) ? inputSize : c->globalWorkItems;
  size_t local = c->workItemsPerGroup;
  error = clEnqueueNDRangeKernel(c->commands, c->kernel, 1, nullptr, &global, &local, 0, nullptr, nullptr);
  ASYNC_CL_ERROR;

  error = clFinish(c->commands);
  ASYNC_CL_ERROR;

  c->kernelExec = microTime(kernelExecStart);
  HR_TIME_POINT dataFromKernelStart = NOW;

  for (auto& paramIter: c->kernelParams) {
    kernelParam* param = paramIter.second;
    if (param->isBuf) {
      if (NODEN_SVM_COARSE_CHAR == param->bufType) {
        error = clEnqueueSVMMap(c->commands, CL_TRUE, CL_MAP_READ | CL_MAP_WRITE, param->value.ptr, param->bufSize, 0, 0, 0);
        ASYNC_CL_ERROR;
      }

      if (!param->bufIsInput && (NODEN_SVM_NONE_CHAR == param->bufType)) {
        error = clEnqueueReadBuffer(c->commands, param->buffer, CL_TRUE, 0,
          param->bufSize, param->value.ptr, 0, nullptr, nullptr);
        ASYNC_CL_ERROR;
      }
    }
  }

  c->dataFromKernel = microTime(dataFromKernelStart);
  c->totalTime = microTime(start);

  for (auto& paramIter: c->kernelParams) {
    kernelParam* param = paramIter.second;
    if (param->isBuf && (NODEN_SVM_NONE_CHAR == param->bufType)) {
      error = clReleaseMemObject(param->buffer);
      ASYNC_CL_ERROR;
    }
  }
}

void runComplete(napi_env env, napi_status asyncStatus, void* data) {
  runCarrier* c = (runCarrier*) data;

  if (asyncStatus != napi_ok) {
    c->status = asyncStatus;
    c->errorMsg = "Async run of program failed to complete.";
  }
  REJECT_STATUS;

  napi_value result;
  c->status = napi_create_object(env, &result);
  REJECT_STATUS;

  napi_value totalValue;
  c->status = napi_create_int64(env, (int64_t) c->totalTime, &totalValue);
  REJECT_STATUS;
  c->status = napi_set_named_property(env, result, "totalTime", totalValue);
  REJECT_STATUS;

  napi_value dataToValue;
  c->status = napi_create_int64(env, (int64_t) c->dataToKernel, &dataToValue);
  REJECT_STATUS;
  c->status = napi_set_named_property(env, result, "dataToKernel", dataToValue);
  REJECT_STATUS;

  napi_value kernelExecValue;
  c->status = napi_create_int64(env, (int64_t) c->kernelExec, &kernelExecValue);
  REJECT_STATUS;
  c->status = napi_set_named_property(env, result, "kernelExec", kernelExecValue);
  REJECT_STATUS;

  napi_value dataFromValue;
  c->status = napi_create_int64(env, (int64_t) c->dataFromKernel, &dataFromValue);
  REJECT_STATUS;
  c->status = napi_set_named_property(env, result, "dataFromKernel", dataFromValue);
  REJECT_STATUS;

  napi_status status;
  status = napi_resolve_deferred(env, c->_deferred, result);

  for (auto& paramIter: c->kernelParams)
    delete(paramIter.second);
  tidyCarrier(env, c);
}

napi_value run(napi_env env, napi_callback_info info) {
  napi_status status;
  runCarrier* c = new runCarrier;

  napi_value args[1];
  size_t argc = 1;
  napi_value programValue;
  status = napi_get_cb_info(env, info, &argc, args, &programValue, nullptr);
  CHECK_STATUS;

  if (argc != 1) {
    status = napi_throw_error(env, nullptr, "Wrong number of arguments. One expected.");
    return nullptr;
  }

  napi_valuetype t;
  status = napi_typeof(env, args[0], &t);
  CHECK_STATUS;
  if (t != napi_object) {
    status = napi_throw_type_error(env, nullptr, "Parameter must be an object.");
    return nullptr;
  }

  napi_value kernelParamsValue;
  status = napi_get_named_property(env, programValue, "kernelParams", &kernelParamsValue);
  CHECK_STATUS;

  napi_value paramNamesValue;
  status = napi_get_property_names(env, kernelParamsValue, &paramNamesValue);
  CHECK_STATUS;

  uint32_t paramNamesCount;
  status = napi_get_array_length(env, paramNamesValue, &paramNamesCount);
  CHECK_STATUS;

  napi_value runNamesValue;
  status = napi_get_property_names(env, args[0], &runNamesValue);
  CHECK_STATUS;

  uint32_t runNamesCount;
  status = napi_get_array_length(env, runNamesValue, &runNamesCount);
  CHECK_STATUS;

  if (paramNamesCount != runNamesCount) {
    status = napi_throw_error(env, nullptr, "Incorrect number of parameters");
    return nullptr;
  }

  for (uint32_t p=0; p<paramNamesCount; ++p) {
    napi_value paramNameValue;
    status = napi_get_element(env, paramNamesValue, p, &paramNameValue);
    CHECK_STATUS;

    size_t paramNameLength;
    status = napi_get_value_string_utf8(env, paramNameValue, nullptr, 0, &paramNameLength);
    CHECK_STATUS;
    char* paramName = (char*)malloc(paramNameLength + 1);
    status = napi_get_value_string_utf8(env, paramNameValue, paramName, paramNameLength + 1, nullptr);
    CHECK_STATUS;
  
    napi_value paramTypeValue;
    status = napi_get_property(env, kernelParamsValue, paramNameValue, &paramTypeValue);
    CHECK_STATUS;

    size_t paramTypeLength;
    status = napi_get_value_string_utf8(env, paramTypeValue, nullptr, 0, &paramTypeLength);
    char* paramType = (char*)malloc(paramTypeLength + 1);
    status = napi_get_value_string_utf8(env, paramTypeValue, paramType, paramTypeLength + 1, nullptr);
    CHECK_STATUS;

    napi_value paramValue;
    status = napi_get_property(env, args[0], paramNameValue, &paramValue);
    CHECK_STATUS;
    
    napi_valuetype valueType;
    status = napi_typeof(env, paramValue, &valueType);
    CHECK_STATUS;

    kernelParam* kp = new kernelParam;
    kp->name = std::string(paramName);
    kp->type = std::string(paramType);
    kp->isBuf = false;
    switch (valueType) {
    case napi_undefined:
      printf("Parameter name \'%s\' not found during run\n", paramName);
      status = napi_throw_error(env, nullptr, "Parameter name not found during run");
      return nullptr;
      break;
    case napi_number:
      if (0 == strcmp("uint", paramType))
        status = napi_get_value_uint32(env, paramValue, &kp->value.uint32);
      else if (0 == strcmp("int", paramType))
        status = napi_get_value_int32(env, paramValue, &kp->value.int32);
      else if (0 == strcmp("long", paramType))
        status = napi_get_value_int64(env, paramValue, &kp->value.int64);
      else if (0 == strcmp("float", paramType) || 0 == strcmp("double", paramType))
        status = napi_get_value_double(env, paramValue, &kp->value.dbl);
      else {
        printf("Unsupported numeric parameter type: \'%s\'\n", paramType);
        status = napi_throw_type_error(env, nullptr, "Unsupported numeric parameter type");
        return nullptr;
      }
      break;
    case napi_object:
      bool isBuffer;
      status = napi_is_buffer(env, paramValue, &isBuffer);
      CHECK_STATUS;
      if (isBuffer) {
        kp->isBuf = true;
        kp->type = std::string("ptr");
        size_t length;
        status = napi_get_buffer_info(env, paramValue, &kp->value.ptr, &length);
        CHECK_STATUS;

        napi_value bufSizeValue;
        status = napi_get_named_property(env, paramValue, "dataSize", &bufSizeValue);
        CHECK_STATUS;
        status = napi_get_value_uint32(env, bufSizeValue, &kp->bufSize);
        CHECK_STATUS;

        napi_value bufTypeValue;
        status = napi_get_named_property(env, paramValue, "sharedMemType", &bufTypeValue);
        CHECK_STATUS;
        
        char bufType[10];
        status = napi_get_value_string_utf8(env, bufTypeValue, bufType, 10, nullptr);
        CHECK_STATUS;
        kp->bufType = bufType[0];

        napi_value bufDirValue;
        status = napi_get_named_property(env, paramValue, "isInput", &bufDirValue);
        CHECK_STATUS;
        status = napi_get_value_bool(env, bufDirValue, &kp->bufIsInput);
        CHECK_STATUS;
      } else {
        status = napi_throw_type_error(env, nullptr, "Unsupported object parameter type");
        return nullptr;
      }
      break;
    default:
      printf("Unsupported parameter value type: \'%d\'\n", valueType);
      status = napi_throw_type_error(env, nullptr, "Unsupported parameter value type");
      return nullptr;
    }
    c->kernelParams.emplace(p, kp);
    delete paramName;
    delete paramType;
  }

  // Extract externals into variables
  napi_value jsContext;
  void* contextData;
  status = napi_get_named_property(env, programValue, "context", &jsContext);
  CHECK_STATUS;
  status = napi_get_value_external(env, jsContext, &contextData);
  c->context = (cl_context) contextData;
  CHECK_STATUS;

  napi_value jsCommands;
  void* commandsData;
  status = napi_get_named_property(env, programValue, "commands", &jsCommands);
  CHECK_STATUS;
  status = napi_get_value_external(env, jsCommands, &commandsData);
  c->commands = (cl_command_queue) commandsData;
  CHECK_STATUS;

  napi_value jsKernel;
  void* kernelData;
  status = napi_get_named_property(env, programValue, "kernel", &jsKernel);
  CHECK_STATUS;
  status = napi_get_value_external(env, jsKernel, &kernelData);
  c->kernel = (cl_kernel) kernelData;
  CHECK_STATUS;

  napi_value globalWorkItemsValue;
  status = napi_get_named_property(env, programValue, "globalWorkItems", &globalWorkItemsValue);
  CHECK_STATUS;
  status = napi_get_value_int64(env, globalWorkItemsValue, (int64_t*)&c->globalWorkItems);
  CHECK_STATUS;

  napi_value workItemsPerGroupValue;
  status = napi_get_named_property(env, programValue, "workItemsPerGroup", &workItemsPerGroupValue);
  CHECK_STATUS;
  status = napi_get_value_int64(env, workItemsPerGroupValue, (int64_t*)&c->workItemsPerGroup);
  CHECK_STATUS;

  status = napi_create_reference(env, programValue, 1, &c->passthru);
  CHECK_STATUS;

  napi_value promise, resource_name;
  status = napi_create_promise(env, &c->_deferred, &promise);
  CHECK_STATUS;

  status = napi_create_string_utf8(env, "Run", NAPI_AUTO_LENGTH, &resource_name);
  CHECK_STATUS;
  status = napi_create_async_work(env, NULL, resource_name, runExecute,
    runComplete, c, &c->_request);
  CHECK_STATUS;
  status = napi_queue_async_work(env, c->_request);
  CHECK_STATUS;

  return promise;
}
