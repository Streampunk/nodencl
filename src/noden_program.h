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

#ifndef NODEN_PROGRAM_H
#define NODEC_PROGRAM_H

#ifdef __APPLE__
    #include "OpenCL/opencl.h"
#else
    #include "CL/cl.h"
#endif
#include <chrono>
#include <stdio.h>
#include "node_api.h"

#define NODEN_OUT_OF_RANGE 4097
#define NODEN_ASYNC_FAILURE 4098
#define NODEN_BUILD_ERROR 4099
#define NODEN_SUCCESS 0

typedef struct {
  char* kernelSource;
  size_t sourceLength;
  napi_ref jsProgram;
  uint32_t platformIndex;
  uint32_t deviceIndex;
  cl_device_id deviceId;
  cl_context context;
  cl_command_queue commands;
  cl_program program;
  cl_kernel kernel;
  char* name;
  size_t nameLength;
  int32_t status = NODEN_SUCCESS;
  char* errorMsg;
  long long buildTime;
  napi_deferred _deferred;
  napi_async_work _request;
} buildCarrier;

typedef struct {
  int32_t status = NODEN_SUCCESS;
  char* errorMsg;
  long long createTime;
  napi_deferred _deferred;
  napi_async_work _request;
} createBufCarrier;

typedef struct {
  int32_t status = NODEN_SUCCESS;
  char* errorMsg;
  long long runTime;
  napi_deferred _deferred;
  napi_async_work _request;
} runCarrier;

napi_value createProgram(napi_env env, napi_callback_info info);

#define ASYNC_CL_ERROR if (error != CL_SUCCESS) { \
  c->status = error; \
  c->errorMsg = (char *) malloc(200); \
  sprintf(c->errorMsg, "In file %s line %d, got CL error %i of type %s.", \
    __FILE__, __LINE__ - 4, error, clGetErrorString(error)); \
  return; \
}

#endif
