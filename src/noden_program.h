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

struct buildCarrier : carrier {
  char* kernelSource;
  size_t sourceLength;
  uint32_t platformIndex;
  uint32_t deviceIndex;
  cl_device_id deviceId;
  cl_context context;
  cl_command_queue commands;
  cl_program program;
  cl_kernel kernel;
  char* name;
  size_t nameLength;
  cl_ulong svmCaps;
  size_t globalWorkItems;
  size_t workItemsPerGroup;
};

napi_value createProgram(napi_env env, napi_callback_info info);

#endif
