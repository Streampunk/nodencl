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

#ifndef NODEN_UTIL_H
#define NODEN_UTIL_H

#ifdef __APPLE__
    #include "OpenCL/opencl.h"
#else
    #include "CL/cl.h"
#endif
#include <chrono>
#include <stdio.h>
#include <string>
#include "node_api.h"

#define DECLARE_NAPI_METHOD(name, func) { name, 0, func, 0, 0, 0, napi_default, 0 }

// Handling NAPI errors - use "napi_status status;" where used
#define CHECK_STATUS if (checkStatus(env, status, __FILE__, __LINE__ - 1) != napi_ok) return nullptr
#define PASS_STATUS if (status != napi_ok) return status

napi_status checkStatus(napi_env env, napi_status status,
  const char * file, uint32_t line);

// Handling CL errors - use "cl_int error;" where used
#define CHECK_CL_ERROR if (clCheckError(env, error, __FILE__, __LINE__) != CL_SUCCESS) return nullptr
#define PASS_CL_ERROR if (error != CL_SUCCESS) return error
#define THROW_CL_ERROR if (error != CL_SUCCESS) { \
  char errorMsg [100]; \
  sprintf(errorMsg, "OpenCL error in subroutine. Location %s(%d). Error %i: %s", \
    __FILE__, __LINE__, error, clGetErrorString(error)); \
  napi_throw_error(env, nullptr, errorMsg); \
  return napi_pending_exception; \
}

cl_int clCheckError(napi_env env, cl_int error, const char* file, uint32_t line);
const char* clGetErrorString(cl_int error);

// High resolution timing
#define HR_TIME_POINT std::chrono::high_resolution_clock::time_point
#define NOW std::chrono::high_resolution_clock::now()
long long microTime(std::chrono::high_resolution_clock::time_point start);

// Argument processing
napi_status checkArgs(napi_env env, napi_callback_info info, char* methodName,
  napi_value* args, size_t argc, napi_valuetype* types);

// Async error handling
#define NODEN_OUT_OF_RANGE 4097
#define NODEN_ASYNC_FAILURE 4098
#define NODEN_BUILD_ERROR 4099
#define NODEN_ALLOCATION_FAILURE 4100
#define NODEN_SUCCESS 0

struct carrier {
  napi_ref passthru = nullptr;
  int32_t status = NODEN_SUCCESS;
  std::string errorMsg;
  long long totalTime;
  napi_deferred _deferred;
  napi_async_work _request;
};

void tidyCarrier(napi_env env, carrier* c);
int32_t rejectStatus(napi_env env, carrier* c, char* file, int32_t line);

#define ASYNC_CL_ERROR if (error != CL_SUCCESS) { \
  c->status = error; \
  char errorMsg[200]; \
  sprintf(errorMsg, "In file %s line %d, got CL error %i of type %s.", \
    __FILE__, __LINE__ - 1, error, clGetErrorString(error)); \
  c->errorMsg = std::string(errorMsg); \
  return; \
}

#define REJECT_STATUS if (rejectStatus(env, c, __FILE__, __LINE__) != NODEN_SUCCESS) return;
#define FLOATING_STATUS if (status != napi_ok) { \
  printf("Unexpected N-API status not OK in file %s at line %d value %i.\n", \
    __FILE__, __LINE__ - 1, status); \
}

#endif // NODEN_UTIL_H
