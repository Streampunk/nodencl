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

#define DECLARE_NAPI_METHOD(name, func) { name, 0, func, 0, 0, 0, napi_default, 0 }

// Handling NAPI errors - use "napi_status status;" where used
#define CHECK_STATUS if (checkStatus(env, status) != napi_ok) return nullptr
#define PASS_STATUS if (status != napi_ok) return status

napi_status checkStatus(napi_env env, napi_status status);

// Handling CL errors - use "cl_int error;" where used
#define CHECK_CL_ERROR if (clCheckError(env, error) != CL_SUCCESS) return nullptr
#define PASS_CL_ERROR if (error != CL_SUCCESS) return error
#define THROW_CL_ERROR if (error != CL_SUCCESS) { \
  char errorMsg [100]; \
  sprintf(errorMsg, "OpenCL error in subroutine. Error %i: %s", error, \
    clGetErrorString(error)); \
  napi_throw_error(env, nullptr, errorMsg); \
  return napi_pending_exception; \
}

cl_int clCheckError(napi_env env, cl_int error);
const char* clGetErrorString(cl_int error);

// High resolution timing
#define HR_TIME_POINT std::chrono::high_resolution_clock::time_point
long long microTime(std::chrono::high_resolution_clock::time_point start);

// Argument processing
napi_status checkArgs(napi_env env, napi_callback_info info, char* methodName,
  napi_value* args, size_t argc, napi_valuetype* types);
