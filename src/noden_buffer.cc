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

#include "noden_buffer.h"
#include "noden_util.h"
#include "cl_memory.h"
#include <cstring>

struct deviceInfo;

struct createBufCarrier : carrier {
  napi_ref contextRef = nullptr;
  iClMemory *clMem = nullptr;
};

struct hostAccessCarrier : carrier {
  iClMemory *clMem = nullptr;
  eMemFlags haFlags = eMemFlags::READWRITE;
  void* srcBuf = nullptr;
  size_t srcBufSize = 0;
};

void hostAccessExecute(napi_env env, void* data) {
  hostAccessCarrier* c = (hostAccessCarrier*) data;
  cl_int error;

  c->clMem->setHostAccess(error, c->haFlags);
  ASYNC_CL_ERROR;

  if (c->srcBuf)
    memcpy(c->clMem->hostBuf(), c->srcBuf, c->srcBufSize);
}

void hostAccessComplete(napi_env env, napi_status asyncStatus, void* data) {
  hostAccessCarrier* c = (hostAccessCarrier*) data;
  if (asyncStatus != napi_ok) {
    c->status = asyncStatus;
    c->errorMsg = "Async buffer creation failed to complete.";
  }
  REJECT_STATUS;

  napi_value result;
  c->status = napi_get_undefined(env, &result);
  REJECT_STATUS;

  napi_status status;
  status = napi_resolve_deferred(env, c->_deferred, result);
  FLOATING_STATUS;

  tidyCarrier(env, c);
}

napi_value hostAccess(napi_env env, napi_callback_info info) {
  napi_status status;
  hostAccessCarrier* c = new hostAccessCarrier;

  napi_value args[2];
  size_t argc = 2;
  napi_value bufferValue;
  status = napi_get_cb_info(env, info, &argc, args, &bufferValue, nullptr);
  CHECK_STATUS;

  if (argc > 2) {
    status = napi_throw_error(env, nullptr, "Wrong number of arguments to hostAccess.");
    delete c;
    return nullptr;
  }

  napi_valuetype t;
  napi_value hostDirValue;
  if (argc > 0) {
    status = napi_typeof(env, args[0], &t);
    CHECK_STATUS;
    if (t != napi_string) {
      status = napi_throw_type_error(env, nullptr, "First argument must be a string.");
      delete c;
      return nullptr;
    }
    hostDirValue = args[0];
  } else {
    status = napi_create_string_utf8(env, "readwrite", 10, &hostDirValue);
    CHECK_STATUS;
  }
  char haflag[10];
  status = napi_get_value_string_utf8(env, hostDirValue, haflag, 10, nullptr);
  CHECK_STATUS;
  if ((strcmp(haflag, "readwrite") != 0) && (strcmp(haflag, "writeonly") != 0) && (strcmp(haflag, "readonly") != 0)) {
    status = napi_throw_error(env, nullptr, "Host access direction must be one of 'readwrite', 'writeonly' or 'readonly'.");
    delete c;
    return nullptr;
  }
  c->haFlags = (0==strcmp("readwrite", haflag)) ? eMemFlags::READWRITE :
               (0==strcmp("writeonly", haflag)) ? eMemFlags::WRITEONLY :
               eMemFlags::READONLY;

  if ((argc == 2) && (c->haFlags == eMemFlags::READONLY)) {
    napi_throw_type_error(env, nullptr, "Optional second argument source buffer provided when access is readonly.");
    delete c;
    return nullptr;
  }

  void* data = nullptr;
  size_t dataSize = 0;
  if (argc == 2) {
    bool isBuffer;
    status = napi_is_buffer(env, args[1], &isBuffer);
    CHECK_STATUS;
    if (!isBuffer) {
      napi_throw_type_error(env, nullptr, "Optional second argument must be a buffer - the source data.");
      delete c;
      return nullptr;
    }

    status = napi_get_buffer_info(env, args[1], &data, &dataSize);
    CHECK_STATUS;
  }
  
  napi_value clMemValue;
  status = napi_get_named_property(env, bufferValue, "clMemory", &clMemValue);
  CHECK_STATUS;
  status = napi_get_value_external(env, clMemValue, (void**)&c->clMem);
  CHECK_STATUS;

  if (data) {
    if (dataSize > c->clMem->numBytes()) {
      printf("Source buffer is larger than requested OpenCL allocation - trimming.\n");
      dataSize = c->clMem->numBytes();
    }
    c->srcBuf = data;
    c->srcBufSize = dataSize;
  }

  napi_value promise, resource_name;
  status = napi_create_promise(env, &c->_deferred, &promise);
  CHECK_STATUS;

  status = napi_create_string_utf8(env, "HostAccess", NAPI_AUTO_LENGTH, &resource_name);
  CHECK_STATUS;
  status = napi_create_async_work(env, NULL, resource_name, hostAccessExecute,
    hostAccessComplete, c, &c->_request);
  CHECK_STATUS;
  status = napi_queue_async_work(env, c->_request);
  CHECK_STATUS;

  return promise;
}

void finalizeClMemory(napi_env env, void* data, void* hint) {
  iClMemory *clMem = (iClMemory*)data;
  printf("Finalizing OpenCL memory of type %s, size %d.\n", clMem->svmTypeName().c_str(), clMem->numBytes());
  delete clMem;
}

void finalizeContextRef(napi_env env, void* data, void* hint) {
  printf("Finalizing OpenCL context reference.\n");
  napi_ref contextRef = (napi_ref)data;
  napi_status status = napi_delete_reference(env, contextRef);
  checkStatus(env, status, __FILE__, __LINE__ - 1);
}

napi_value freeAllocation(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value args[1];
  size_t argc = 1;
  napi_value bufferValue;
  iClMemory *clMem = nullptr;
  status = napi_get_cb_info(env, info, &argc, args, &bufferValue, (void**)&clMem);
  CHECK_STATUS;

  printf("Freeing OpenCL memory of type %s, size %d.\n", clMem->svmTypeName().c_str(), clMem->numBytes());
  clMem->freeAllocation();

  napi_value result;
  status = napi_get_undefined(env, &result);
  CHECK_STATUS;
  return result;
}

void createBufferExecute(napi_env env, void* data) {
  createBufCarrier* c = (createBufCarrier*) data;
  // printf("Create a buffer of type %s, size %d.\n", c->clMem->svmTypeName().c_str(), c->clMem->numBytes());

  HR_TIME_POINT start = NOW;

  if (!c->clMem->allocate()) {
    c->status = NODEN_ALLOCATION_FAILURE;
    c->errorMsg = "Failed to allocate memory for buffer.";
  }

  c->totalTime = microTime(start);
}

void createBufferComplete(napi_env env, napi_status asyncStatus, void* data) {
  createBufCarrier* c = (createBufCarrier*) data;
  if (asyncStatus != napi_ok) {
    c->status = asyncStatus;
    c->errorMsg = "Async buffer creation failed to complete.";
  }
  REJECT_STATUS;

  napi_value result;
  c->status = napi_create_external_buffer(env, c->clMem->numBytes(), c->clMem->hostBuf(), nullptr, nullptr, &result);
  REJECT_STATUS;

  napi_value clMemValue;
  c->status = napi_create_external(env, c->clMem, finalizeClMemory, nullptr, &clMemValue);
  REJECT_STATUS;
  c->status = napi_set_named_property(env, result, "clMemory", clMemValue);
  REJECT_STATUS;

  napi_value contextRefValue;
  c->status = napi_create_external(env, c->contextRef, finalizeContextRef, nullptr, &contextRefValue);
  REJECT_STATUS;
  c->status = napi_set_named_property(env, result, "contextRef", contextRefValue);
  REJECT_STATUS;

  napi_value numBytesValue;
  c->status = napi_create_uint32(env, (int32_t) c->clMem->numBytes(), &numBytesValue);
  REJECT_STATUS;
  c->status = napi_set_named_property(env, result, "numBytes", numBytesValue);
  REJECT_STATUS;

  napi_value creationValue;
  c->status = napi_create_int64(env, (int64_t) c->totalTime, &creationValue);
  REJECT_STATUS;
  c->status = napi_set_named_property(env, result, "creationTime", creationValue);
  REJECT_STATUS;

  napi_value hostAccessValue;
  c->status = napi_create_function(env, "hostAccess", NAPI_AUTO_LENGTH,
    hostAccess, nullptr, &hostAccessValue);
  REJECT_STATUS;
  c->status = napi_set_named_property(env, result, "hostAccess", hostAccessValue);
  REJECT_STATUS;

  napi_value freeAllocValue;
  c->status = napi_create_function(env, "freeAllocation", NAPI_AUTO_LENGTH,
    freeAllocation, c->clMem, &freeAllocValue);
  REJECT_STATUS;
  c->status = napi_set_named_property(env, result, "freeAllocation", freeAllocValue);
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
  uint32_t numBytes = (uint32_t)paramSize;

  status = napi_typeof(env, args[1], &t);
  CHECK_STATUS;
  if (t != napi_string) {
    status = napi_throw_type_error(env, nullptr, "Second argument must be a string - the buffer direction.");
    delete c;
    return nullptr;
  }

  char memflag[10];
  status = napi_get_value_string_utf8(env, args[1], memflag, 10, nullptr);
  CHECK_STATUS;
  if ((strcmp(memflag, "readwrite") != 0) && (strcmp(memflag, "writeonly") != 0) && (strcmp(memflag, "readonly") != 0)) {
    status = napi_throw_error(env, nullptr, "Buffer direction must be one of 'readwrite', 'writeonly' or 'readonly'.");
    delete c;
    return nullptr;
  }
  eMemFlags memFlags = (0==strcmp("readwrite", memflag)) ? eMemFlags::READWRITE :
                       (0==strcmp("writeonly", memflag)) ? eMemFlags::WRITEONLY :
                       eMemFlags::READONLY;

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
  char svmFlag[10];
  status = napi_get_value_string_utf8(env, bufTypeValue, svmFlag, 10, nullptr);
  CHECK_STATUS;

  napi_value svmCapsValue;
  status = napi_get_named_property(env, contextValue, "svmCaps", &svmCapsValue);
  CHECK_STATUS;
  cl_ulong svmCaps;
  status = napi_get_value_int64(env, svmCapsValue, (int64_t*)&svmCaps);
  CHECK_STATUS;

  if ((strcmp(svmFlag, "fine") != 0) &&
    (strcmp(svmFlag, "coarse") != 0) &&
    (strcmp(svmFlag, "none") != 0)) {
    status = napi_throw_error(env, nullptr, "Buffer type must be one of 'fine', 'coarse' or 'none'.");
    delete c;
    return nullptr;
  }
  eSvmType svmType = (0 == strcmp(svmFlag, "fine")) ? eSvmType::FINE :
                     (0 == strcmp(svmFlag, "coarse")) ? eSvmType::COARSE :
                     eSvmType::NONE;

  if (((eSvmType::FINE == svmType) && ((svmCaps & CL_DEVICE_SVM_FINE_GRAIN_BUFFER) == 0)) ||
      ((eSvmType::COARSE == svmType) && ((svmCaps & CL_DEVICE_SVM_COARSE_GRAIN_BUFFER) == 0))) {
    status = napi_throw_error(env, nullptr, "Buffer type requested is not supported by device.");
    delete c;
    return nullptr;
  }

  // Extract externals into variables
  napi_value jsContext;
  void* contextData;
  status = napi_get_named_property(env, contextValue, "context", &jsContext);
  CHECK_STATUS;
  status = napi_get_value_external(env, jsContext, &contextData);
  CHECK_STATUS;
  cl_context context = (cl_context) contextData;

  napi_value jsCommands;
  void* commandsData;
  status = napi_get_named_property(env, contextValue, "commands", &jsCommands);
  CHECK_STATUS;
  status = napi_get_value_external(env, jsCommands, &commandsData);
  CHECK_STATUS;
  cl_command_queue commands = (cl_command_queue) commandsData;

  napi_value jsDevInfo;
  deviceInfo *devInfo;
  status = napi_get_named_property(env, contextValue, "deviceInfo", &jsDevInfo);
  CHECK_STATUS;
  status = napi_get_value_external(env, jsDevInfo, (void**)&devInfo);
  CHECK_STATUS;

  status = napi_create_reference(env, contextValue, 1, &c->contextRef);
  CHECK_STATUS;

  // Create holder for host and gpu buffers
  c->clMem = iClMemory::create(context, commands, memFlags, svmType, numBytes, devInfo);

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
