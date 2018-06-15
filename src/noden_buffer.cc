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

struct tidyHint {
  cl_context context;
  char svmType[10] = "none";
  napi_ref program; // Make sure program does not get GCd
};

void tidyBuffer(napi_env env, void* data, void* hint) {
  tidyHint* h = (tidyHint*) hint;
  printf("Finalizing a SVM buffer of type %s.\n", h->svmType);

  switch (h->svmType[0]) {
    case NODEN_SVM_FINE_CHAR:
    case NODEN_SVM_COARSE_CHAR:
      clSVMFree(h->context, data);
      break;
    case NODEN_SVM_NONE_CHAR:
    default:
      free(data);
      break;
  }

  delete h;
}

void createBufferExecute(napi_env env, void* data) {
  createBufCarrier* c = (createBufCarrier*) data;
  cl_int error;

  printf("Trying to create a buffer of type %s.\n", c->svmType);

  HR_TIME_POINT start = NOW;
  // Allocate memory
  switch (c->svmType[0]) {
    case NODEN_SVM_FINE_CHAR:
      c->data = clSVMAlloc(c->context,
        CL_MEM_READ_WRITE | CL_MEM_SVM_FINE_GRAIN_BUFFER, c->numBytes, 0);
      if (c->data == nullptr) {
        c->status = NODEN_ALLOCATION_FAILURE;
        c->errorMsg = "Failed to allocate fine grained memory for buffer.";
        return;
      }
      break;
    case NODEN_SVM_COARSE_CHAR:
      c->data = clSVMAlloc(c->context, CL_MEM_READ_WRITE, c->numBytes, 0);
      if (c->data == nullptr) {
        c->status = NODEN_ALLOCATION_FAILURE;
        c->errorMsg = "Failed to allocate fine grained memory for buffer.";
        return;
      }
      break;
    default:
    case NODEN_SVM_NONE_CHAR:
      c->data = (void *) malloc(c->numBytes);
      break;
  }

  if (c->svmType[0] == NODEN_SVM_COARSE_CHAR) {
    error = clEnqueueSVMMap(c->commands, CL_TRUE, CL_MAP_WRITE | CL_MAP_READ, c->data, c->numBytes, 0, 0, 0);
    ASYNC_CL_ERROR;
  }

  c->totalTime = microTime(start);
}

void createBufferComplete(napi_env env, napi_status asyncStatus, void* data) {
  createBufCarrier* c = (createBufCarrier*) data;

  printf("Buffer created with status %i.\n", asyncStatus);

  if (asyncStatus != napi_ok) {
    c->status = asyncStatus;
    c->errorMsg = "Async buffer creation failed to complete.";
  }
  REJECT_STATUS;

  napi_value result;
  tidyHint* hint = new tidyHint;
  hint->context = c->context;
  strcpy(hint->svmType, c->svmType);
  c->status = napi_create_external_buffer(env, c->numBytes, c->data, &tidyBuffer, hint, &result);
  REJECT_STATUS;

  napi_value mappedValue;
  c->status = napi_get_boolean(env, c->svmType[0] == NODEN_SVM_COARSE_CHAR, &mappedValue);
  REJECT_STATUS;
  c->status = napi_set_named_property(env, result, "mapped", mappedValue);
  REJECT_STATUS;

  napi_value numBytesValue;
  c->status = napi_create_uint32(env, (int32_t) c->numBytes, &numBytesValue);
  REJECT_STATUS;
  c->status = napi_set_named_property(env, result, "numBytes", numBytesValue);
  REJECT_STATUS;

  napi_value creationValue;
  c->status = napi_create_int64(env, (int64_t) c->totalTime, &creationValue);
  REJECT_STATUS;
  c->status = napi_set_named_property(env, result, "creationTime", creationValue);
  REJECT_STATUS;

  napi_value svmTypeValue;
  c->status = napi_create_string_utf8(env, c->svmType, NAPI_AUTO_LENGTH, &svmTypeValue);
  REJECT_STATUS;
  c->status = napi_set_named_property(env, result, "sharedMemType", svmTypeValue);
  REJECT_STATUS;

  napi_value bufDirValue;
  c->status = napi_get_boolean(env, c->isInput, &bufDirValue);
  REJECT_STATUS;
  c->status = napi_set_named_property(env, result, "isInput", bufDirValue);
  REJECT_STATUS;

  napi_status status;
  status = napi_resolve_deferred(env, c->_deferred, result);
  FLOATING_STATUS;

  tidyCarrier(env, c);
}

napi_value createBuffer(napi_env env, napi_callback_info info) {
  napi_status status;
  createBufCarrier* c = new createBufCarrier;

  napi_value args[3];
  size_t argc = 3;
  napi_value contextValue;
  status = napi_get_cb_info(env, info, &argc, args, &contextValue, nullptr);
  CHECK_STATUS;

  if (argc < 2 || argc > 3) {
    status = napi_throw_error(env, nullptr, "Wrong number of arguments to create buffer.");
    delete c;
    return nullptr;
  }

  napi_valuetype t;
  status = napi_typeof(env, args[0], &t);
  CHECK_STATUS;
  if (t != napi_number) {
    status = napi_throw_type_error(env, nullptr, "First argument must be a number - buffer size.");
    delete c;
    return nullptr;
  }
  int32_t paramSize;
  status = napi_get_value_int32(env, args[0], &paramSize);
  CHECK_STATUS;
  if (paramSize < 0) {
    status = napi_throw_error(env, nullptr, "Size of the buffer cannot be negative.");
    delete c;
    return nullptr;
  }
  c->numBytes = (size_t) paramSize;

  status = napi_typeof(env, args[1], &t);
  CHECK_STATUS;
  if (t != napi_string) {
    status = napi_throw_type_error(env, nullptr, "Second argument must be a string - the buffer direction.");
    delete c;
    return nullptr;
  }

  char bufDir[10];
  status = napi_get_value_string_utf8(env, args[1], bufDir, 10, nullptr);
  CHECK_STATUS;

  if ((strcmp(bufDir, "in") != 0) && (strcmp(bufDir, "out") != 0)) {
    status = napi_throw_error(env, nullptr, "Buffer direction must be one of 'in' or 'out'.");
    delete c;
    return nullptr;
  }
  c->isInput = 0==strcmp("in", bufDir);

  napi_value bufTypeValue;
  if (argc == 3) {
    status = napi_typeof(env, args[2], &t);
    CHECK_STATUS;
    if (t != napi_string) {
      status = napi_throw_type_error(env, nullptr, "Third argument must be a string - the buffer type.");
      delete c;
      return nullptr;
    }
    bufTypeValue = args[2];
  } else {
    status = napi_create_string_utf8(env, "none", 10, &bufTypeValue);
    CHECK_STATUS;
  }
  status = napi_get_value_string_utf8(env, bufTypeValue, c->svmType, 10, nullptr);
  CHECK_STATUS;

  napi_value svmCapsValue;
  status = napi_get_named_property(env, contextValue, "svmCaps", &svmCapsValue);
  CHECK_STATUS;
  cl_ulong svmCaps;
  status = napi_get_value_int64(env, svmCapsValue, (int64_t*)&svmCaps);
  CHECK_STATUS;

  if ((strcmp(c->svmType, "fine") != 0) &&
    (strcmp(c->svmType, "coarse") != 0) &&
    (strcmp(c->svmType, "none") != 0)) {
    status = napi_throw_error(env, nullptr, "Buffer type must be one of 'fine', 'coarse' or 'none'.");
    delete c;
    return nullptr;
  }

  if (((strcmp(c->svmType, "fine") == 0) && ((svmCaps & CL_MEM_SVM_FINE_GRAIN_BUFFER) == 0)) ||
      ((strcmp(c->svmType, "coarse") == 0) && ((svmCaps & CL_DEVICE_SVM_COARSE_GRAIN_BUFFER) == 0))) {
    status = napi_throw_error(env, nullptr, "Buffer type requested is not supported by device.");
    delete c;
    return nullptr;
  }

  printf("Buffer size %zi.\n", c->numBytes);

  // Extract externals into variables
  napi_value jsContext;
  void* contextData;
  status = napi_get_named_property(env, contextValue, "context", &jsContext);
  CHECK_STATUS;
  status = napi_get_value_external(env, jsContext, &contextData);
  CHECK_STATUS;
  c->context = (cl_context) contextData;

  napi_value jsCommands;
  void* commandsData;
  status = napi_get_named_property(env, contextValue, "commands", &jsCommands);
  CHECK_STATUS;
  status = napi_get_value_external(env, jsCommands, &commandsData);
  CHECK_STATUS;
  c->commands = (cl_command_queue) commandsData;

  status = napi_create_reference(env, contextValue, 1, &c->passthru);
  CHECK_STATUS;

  napi_value promise, resource_name;
  status = napi_create_promise(env, &c->_deferred, &promise);
  CHECK_STATUS;

  status = napi_create_string_utf8(env, "CreateBuffer", NAPI_AUTO_LENGTH, &resource_name);
  CHECK_STATUS;
  status = napi_create_async_work(env, NULL, resource_name, createBufferExecute,
    createBufferComplete, c, &c->_request);
  CHECK_STATUS;
  status = napi_queue_async_work(env, c->_request);
  CHECK_STATUS;

  return promise;
}
