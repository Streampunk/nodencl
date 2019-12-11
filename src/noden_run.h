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
#include <memory>
#include <string>
#include <vector>
#include <map>
#include "node_api.h"
#include "noden_util.h"
#include "run_params.h"

class iClMemory;
class iGpuMemory;

enum class eParamFlags : uint8_t { VALUE = 0, BUFFER = 1, IMAGE = 2 };

struct kernelParam {
  kernelParam(const std::string& paramName, const std::string& paramType, iKernelArg::eAccess access) : 
    name(paramName), paramType(paramType), access(access), valueType(eParamFlags::VALUE), value(0) {}
  const std::string name;
  std::string paramType;
  iKernelArg::eAccess access;
  eParamFlags valueType;
  union paramVal {
    paramVal(int64_t i): int64(i) {}
    uint32_t uint32;
    int32_t int32;
    int64_t int64;
    float flt;
    double dbl;
    iClMemory* clMem;
  } value;
  std::shared_ptr<iGpuMemory> gpuAccess;
};

struct runCarrier : carrier {
  std::map<uint32_t, kernelParam*> kernelParams;
  iRunParams *runParams;
  uint32_t queueNum = 0;
  long long dataToKernel;
  long long kernelExec;
  long long dataFromKernel;
  cl_context context;
  std::vector<cl_command_queue> commandQueues;
  cl_kernel kernel;
};

napi_value run(napi_env env, napi_callback_info info);

#endif
