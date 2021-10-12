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

#include "noden_util.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <chrono>
#include "node_api.h"

napi_status checkStatus(napi_env env, napi_status status,
  const char* file, uint32_t line) {

  napi_status infoStatus, throwStatus;
  const napi_extended_error_info *errorInfo;

  if (status == napi_ok) {
    // printf("Received status OK.\n");
    return status;
  }

  infoStatus = napi_get_last_error_info(env, &errorInfo);
  assert(infoStatus == napi_ok);
  printf("NAPI error in file %s on line %i. Error %i: %s\n", file, line,
    errorInfo->error_code, errorInfo->error_message);

  if (status == napi_pending_exception) {
    printf("NAPI pending exception. Engine error code: %i\n", errorInfo->engine_error_code);
    return status;
  }

  char errorCode[20];
  snprintf(errorCode, 20, "%d", errorInfo->error_code);
  throwStatus = napi_throw_error(env, errorCode, errorInfo->error_message);
  assert(throwStatus == napi_ok);

  return napi_pending_exception; // Expect to be cast to void
}

const char* clGetErrorString(cl_int errorCode) {
  switch (errorCode) {
  case 0: return "CL_SUCCESS";
  case -1: return "CL_DEVICE_NOT_FOUND";
  case -2: return "CL_DEVICE_NOT_AVAILABLE";
  case -3: return "CL_COMPILER_NOT_AVAILABLE";
  case -4: return "CL_MEM_OBJECT_ALLOCATION_FAILURE";
  case -5: return "CL_OUT_OF_RESOURCES";
  case -6: return "CL_OUT_OF_HOST_MEMORY";
  case -7: return "CL_PROFILING_INFO_NOT_AVAILABLE";
  case -8: return "CL_MEM_COPY_OVERLAP";
  case -9: return "CL_IMAGE_FORMAT_MISMATCH";
  case -10: return "CL_IMAGE_FORMAT_NOT_SUPPORTED";
  case -11: return "CL_BUILD_PROGRAM_FAILURE";
  case -12: return "CL_MAP_FAILURE";
  case -13: return "CL_MISALIGNED_SUB_BUFFER_OFFSET";
  case -14: return "CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST";
  case -15: return "CL_COMPILE_PROGRAM_FAILURE";
  case -16: return "CL_LINKER_NOT_AVAILABLE";
  case -17: return "CL_LINK_PROGRAM_FAILURE";
  case -18: return "CL_DEVICE_PARTITION_FAILED";
  case -19: return "CL_KERNEL_ARG_INFO_NOT_AVAILABLE";

  case -30: return "CL_INVALID_VALUE";
  case -31: return "CL_INVALID_DEVICE_TYPE";
  case -32: return "CL_INVALID_PLATFORM";
  case -33: return "CL_INVALID_DEVICE";
  case -34: return "CL_INVALID_CONTEXT";
  case -35: return "CL_INVALID_QUEUE_PROPERTIES";
  case -36: return "CL_INVALID_COMMAND_QUEUE";
  case -37: return "CL_INVALID_HOST_PTR";
  case -38: return "CL_INVALID_MEM_OBJECT";
  case -39: return "CL_INVALID_IMAGE_FORMAT_DESCRIPTOR";
  case -40: return "CL_INVALID_IMAGE_SIZE";
  case -41: return "CL_INVALID_SAMPLER";
  case -42: return "CL_INVALID_BINARY";
  case -43: return "CL_INVALID_BUILD_OPTIONS";
  case -44: return "CL_INVALID_PROGRAM";
  case -45: return "CL_INVALID_PROGRAM_EXECUTABLE";
  case -46: return "CL_INVALID_KERNEL_NAME";
  case -47: return "CL_INVALID_KERNEL_DEFINITION";
  case -48: return "CL_INVALID_KERNEL";
  case -49: return "CL_INVALID_ARG_INDEX";
  case -50: return "CL_INVALID_ARG_VALUE";
  case -51: return "CL_INVALID_ARG_SIZE";
  case -52: return "CL_INVALID_KERNEL_ARGS";
  case -53: return "CL_INVALID_WORK_DIMENSION";
  case -54: return "CL_INVALID_WORK_GROUP_SIZE";
  case -55: return "CL_INVALID_WORK_ITEM_SIZE";
  case -56: return "CL_INVALID_GLOBAL_OFFSET";
  case -57: return "CL_INVALID_EVENT_WAIT_LIST";
  case -58: return "CL_INVALID_EVENT";
  case -59: return "CL_INVALID_OPERATION";
  case -60: return "CL_INVALID_GL_OBJECT";
  case -61: return "CL_INVALID_BUFFER_SIZE";
  case -62: return "CL_INVALID_MIP_LEVEL";
  case -63: return "CL_INVALID_GLOBAL_WORK_SIZE";
  case -64: return "CL_INVALID_PROPERTY";
  case -65: return "CL_INVALID_IMAGE_DESCRIPTOR";
  case -66: return "CL_INVALID_COMPILER_OPTIONS";
  case -67: return "CL_INVALID_LINKER_OPTIONS";
  case -68: return "CL_INVALID_DEVICE_PARTITION_COUNT";
  case -69: return "CL_INVALID_PIPE_SIZE";
  case -70: return "CL_INVALID_DEVICE_QUEUE";
  default: return "CL_UNKNOWN_ERROR";
  };
};

cl_int clCheckError(napi_env env, cl_int error,
  const char* file, uint32_t line) {

  napi_status throwStatus;
  if (error == CL_SUCCESS) return error;

  printf("OpenCL error in file %s line %i. Error %i: %s\n",
    file, line, error, clGetErrorString(error));

  char errorCode[20];
  snprintf(errorCode, 20, "%d", error);
  throwStatus = napi_throw_error(env, errorCode, clGetErrorString(error));
  assert(throwStatus == napi_ok);

  return error;
}

long long microTime(std::chrono::high_resolution_clock::time_point start) {
  auto elapsed = std::chrono::high_resolution_clock::now() - start;
  return std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count();
}

const char* getNapiTypeName(napi_valuetype t) {
  switch (t) {
    case napi_undefined: return "undefined";
    case napi_null: return "null";
    case napi_boolean: return "boolean";
    case napi_number: return "number";
    case napi_string: return "string";
    case napi_symbol: return "symbol";
    case napi_object: return "object";
    case napi_function: return "function";
    case napi_external: return "external";
    default: return "unknown";
  }
}

napi_status checkArgs(napi_env env, napi_callback_info info, const char* methodName,
  napi_value* args, size_t argc, napi_valuetype* types) {

  napi_status status;

  size_t realArgc = argc;
  status = napi_get_cb_info(env, info, &realArgc, args, nullptr, nullptr);
  if (status != napi_ok) return status;

  if (realArgc != argc) {
    char errorMsg[100];
    sprintf(errorMsg, "For method %s, expected %zi arguments and got %zi.",
      methodName, argc, realArgc);
    napi_throw_error(env, nullptr, errorMsg);
    return napi_pending_exception;
  }

  napi_valuetype t;
  for ( int x = 0 ; x < (int)argc ; x++ ) {
    status = napi_typeof(env, args[x], &t);
    if (status != napi_ok) return status;
    if (t != types[x]) {
      char errorMsg[100];
      sprintf(errorMsg, "For method %s argument %i, expected type %s and got %s.",
        methodName, x + 1, getNapiTypeName(types[x]), getNapiTypeName(t));
      napi_throw_error(env, nullptr, errorMsg);
      return napi_pending_exception;
    }
  }

  return napi_ok;
};


void tidyCarrier(napi_env env, carrier* c) {
  napi_status status;
  if (c->passthru != nullptr) {
    status = napi_delete_reference(env, c->passthru);
    FLOATING_STATUS;
  }
  status = napi_delete_async_work(env, c->_request);
  FLOATING_STATUS;
  delete c;
}

int32_t rejectStatus(napi_env env, carrier* c, const char* file, int32_t line) {
  if (c->status != NODEN_SUCCESS) {
    napi_value errorValue, errorCode, errorMsg;
    napi_status status;
    char statusChars[20];
    snprintf(statusChars, 20, "%d", c->status);
    char* extMsg = (char *) malloc(sizeof(char) * c->errorMsg.length() + 200);
    sprintf(extMsg, "In file %s on line %i, found error: %s", file, line, c->errorMsg.c_str());
    status = napi_create_string_utf8(env, statusChars, NAPI_AUTO_LENGTH, &errorCode);
    FLOATING_STATUS;
    status = napi_create_string_utf8(env, extMsg, NAPI_AUTO_LENGTH, &errorMsg);
    FLOATING_STATUS;
    status = napi_create_error(env, errorCode, errorMsg, &errorValue);
    FLOATING_STATUS;
    status = napi_reject_deferred(env, c->_deferred, errorValue);
    FLOATING_STATUS;

    delete[] extMsg;
    tidyCarrier(env, c);
  }
  return c->status;
}
