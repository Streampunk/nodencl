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

#ifndef NODEN_RUN_H
#define NODEN_RUN_H

#ifdef __APPLE__
    #include "OpenCL/opencl.h"
#else
    #include "CL/cl.h"
#endif
#include <chrono>
#include <stdio.h>
#include <map>
#include "node_api.h"
#include "noden_util.h"
#include "noden_buffer.h"

struct kernelParam {
  std::string name;
  std::string type;
  bool isBuf;
  union {
    uint32_t uint32;
    int32_t int32;
    int64_t int64;
    double dbl;
    iNodenBuffer* nodenBuf;
  } value;
  std::shared_ptr<iGpuBuffer> gpuAccess;
};

struct runCarrier : carrier {
  std::map<uint32_t, kernelParam*> kernelParams;
  size_t globalWorkItems;
  size_t workItemsPerGroup;
  long long dataToKernel;
  long long kernelExec;
  long long dataFromKernel;
  cl_context context;
  cl_command_queue commands;
  cl_kernel kernel;
};

napi_value run(napi_env env, napi_callback_info info);

#endif
