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
  cl_mem input;
  cl_mem output;

  HR_TIME_POINT bufAlloc = NOW;
  // Not recording buffer create time - should probably be done once, before here
  if (c->inputType[0] == NODEN_SVM_NONE_CHAR) {
    input = clCreateBuffer(c->context, CL_MEM_READ_ONLY | CL_MEM_ALLOC_HOST_PTR,
      c->inputSize, nullptr, &error);
    ASYNC_CL_ERROR;
  }
  if (c->outputType[0] == NODEN_SVM_NONE_CHAR) {
    output = clCreateBuffer(c->context, CL_MEM_WRITE_ONLY | CL_MEM_ALLOC_HOST_PTR,
      c->outputSize, nullptr, &error);
    ASYNC_CL_ERROR;
  }

  // printf("Took %lluus to create GPU buffers.\n", microTime(bufAlloc));

  HR_TIME_POINT start = NOW;
  HR_TIME_POINT dataToKernelStart = start;

  if (c->inputType[0] == NODEN_SVM_COARSE_CHAR) {
    error = clEnqueueSVMUnmap(c->commands, c->input, 0, 0, 0);
    ASYNC_CL_ERROR;
  }
  if (c->outputType[0] == NODEN_SVM_COARSE_CHAR) {
    error = clEnqueueSVMUnmap(c->commands, c->output, 0, 0, 0);
    ASYNC_CL_ERROR;
  }

  if (c->inputType[0] != NODEN_SVM_NONE_CHAR) {
    error = clSetKernelArgSVMPointer(c->kernel, 0, c->input);
    ASYNC_CL_ERROR;
  } else {
    error = clEnqueueWriteBuffer(c->commands, input, CL_TRUE, 0, c->inputSize,
      c->input, 0, nullptr, nullptr);
    ASYNC_CL_ERROR;
    error = clSetKernelArg(c->kernel, 0, sizeof(cl_mem), &input);
    ASYNC_CL_ERROR;
  }

  if (c->outputType[0] != NODEN_SVM_NONE_CHAR) {
    error = clSetKernelArgSVMPointer(c->kernel, 1, c->output);
    ASYNC_CL_ERROR;
  } else {
    error = clSetKernelArg(c->kernel, 1, sizeof(cl_mem), &output);
    ASYNC_CL_ERROR;
  }
  // error = clSetKernelArg(c->kernel, 2, sizeof(unsigned int), &c->inputSize);
  // ASYNC_CL_ERROR;

  c->dataToKernel = microTime(dataToKernelStart);
  HR_TIME_POINT kernelExecStart = NOW;

  size_t global = (0 == c->globalWorkItems) ? (size_t) c->inputSize : c->globalWorkItems;
  size_t local = c->workItemsPerGroup;
  error = clEnqueueNDRangeKernel(c->commands, c->kernel, 1, nullptr, &global, &local, 0, nullptr, nullptr);
  ASYNC_CL_ERROR;

  error = clFinish(c->commands);
  ASYNC_CL_ERROR;

  c->kernelExec = microTime(kernelExecStart);
  HR_TIME_POINT dataFromKernelStart = NOW;

  if (c->inputType[0] == NODEN_SVM_COARSE_CHAR) {
    error = clEnqueueSVMMap(c->commands, CL_TRUE, CL_MAP_READ | CL_MAP_WRITE, c->input, c->inputSize, 0, 0, 0);
    ASYNC_CL_ERROR;
  }
  if (c->outputType[0] == NODEN_SVM_COARSE_CHAR) {
    error = clEnqueueSVMMap(c->commands, CL_TRUE, CL_MAP_READ | CL_MAP_WRITE, c->output, c->outputSize, 0, 0, 0);
    ASYNC_CL_ERROR;
  }

  if (c->outputType[0] == NODEN_SVM_NONE_CHAR) {
    error = clEnqueueReadBuffer(c->commands, output, CL_TRUE, 0,
      c->outputSize, c->output, 0, nullptr, nullptr);
    ASYNC_CL_ERROR;
  }
  c->dataFromKernel = microTime(dataFromKernelStart);

  c->totalTime = microTime(start);

  if (c->inputType[0] == NODEN_SVM_NONE_CHAR) {
    error = clReleaseMemObject(input);
    ASYNC_CL_ERROR;
  }
  if (c->outputType[0] == NODEN_SVM_NONE_CHAR) {
    error = clReleaseMemObject(output);
    ASYNC_CL_ERROR;
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

  tidyCarrier(env, c);
}

napi_value run(napi_env env, napi_callback_info info) {
  napi_status status;
  runCarrier* c = new runCarrier;

  napi_value args[2];
  size_t argc = 2;
  napi_value programValue;
  status = napi_get_cb_info(env, info, &argc, args, &programValue, nullptr);
  CHECK_STATUS;

  if (argc != 2) {
    status = napi_throw_error(env, nullptr, "Wrong number of arguments. Two expected.");
    return nullptr;
  }

  bool isBuffer;
  status = napi_is_buffer(env, args[0], &isBuffer);
  CHECK_STATUS;
  if (!isBuffer) {
    status = napi_throw_type_error(env, nullptr, "First argument must be the input buffer.");
    return nullptr;
  }

  status = napi_is_buffer(env, args[1], &isBuffer);
  CHECK_STATUS;
  if (!isBuffer) {
    status = napi_throw_type_error(env, nullptr, "Seconds argument must be the output buffer.");
    return nullptr;
  }

  bool hasProp;
  status = napi_has_named_property(env, args[0], "dataSize", &hasProp);
  CHECK_STATUS;
  if (!hasProp) {
    status = napi_throw_type_error(env, nullptr, "Input buffer must have been created by the program's create buffer.");
    return nullptr;
  }

  status = napi_has_named_property(env, args[1], "dataSize", &hasProp);
  CHECK_STATUS;
  if (!hasProp) {
    status = napi_throw_type_error(env, nullptr, "Output buffer must have been created by the program's create buffer.");
    return nullptr;
  }

  napi_value inputSizeValue;
  status = napi_get_named_property(env, args[0], "dataSize", &inputSizeValue);
  CHECK_STATUS;
  status = napi_get_value_uint32(env, inputSizeValue, &c->inputSize);
  CHECK_STATUS;

  napi_value inputTypeValue;
  status = napi_get_named_property(env, args[0], "sharedMemType", &inputTypeValue);
  CHECK_STATUS;
  status = napi_get_value_string_utf8(env, inputTypeValue, c->inputType, 10, nullptr);
  CHECK_STATUS;

  napi_value outputSizeValue;
  status = napi_get_named_property(env, args[1], "dataSize", &outputSizeValue);
  CHECK_STATUS;
  status = napi_get_value_uint32(env, outputSizeValue, &c->outputSize);
  CHECK_STATUS;

  napi_value outputTypeValue;
  status = napi_get_named_property(env, args[1], "sharedMemType", &outputTypeValue);
  CHECK_STATUS;
  status = napi_get_value_string_utf8(env, outputTypeValue, c->outputType, 10, nullptr);
  CHECK_STATUS;

  size_t length;
  status = napi_get_buffer_info(env, args[0], &c->input, &length);
  CHECK_STATUS;

  status = napi_get_buffer_info(env, args[1], &c->output, &length);
  CHECK_STATUS;

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
