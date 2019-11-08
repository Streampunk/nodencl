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

#ifndef CL_MEMORY_H
#define CL_MEMORY_H

#ifdef __APPLE__
    #include "OpenCL/opencl.h"
#else
    #include "CL/cl.h"
#endif
#include <stdint.h>
#include <memory>
#include <string>
#include <array>
#include "run_params.h"

class iRunParams;
struct deviceInfo;

enum class eMemFlags : uint8_t { NONE = 0, READWRITE = 1, WRITEONLY = 2, READONLY = 3 };
enum class eSvmType : uint8_t { NONE = 0, COARSE = 1, FINE = 2 };

class iGpuMemory {
public:
  virtual ~iGpuMemory() {}
  virtual cl_int setKernelParam(cl_kernel kernel, uint32_t paramIndex, bool isImageParam, iKernelArg::eAccess access, iRunParams *runParams) const = 0;
};

class iClMemory {
public:
  virtual ~iClMemory() {}

  static iClMemory *create(cl_context context, cl_command_queue commands, eMemFlags memFlags, eSvmType svmType, 
                           uint32_t numBytes, deviceInfo *devInfo, const std::array<uint32_t, 3>& imageDims);

  virtual bool allocate() = 0;
  virtual std::shared_ptr<iGpuMemory> getGPUMemory() = 0;
  virtual void setHostAccess(cl_int &error, eMemFlags haFlags) = 0; // map GPU buffer to host
  virtual void freeAllocation() = 0;

  virtual uint32_t numBytes() const = 0;
  virtual eMemFlags memFlags() const = 0;
  virtual eSvmType svmType() const = 0;
  virtual std::string svmTypeName() const = 0;
  virtual void* hostBuf() const = 0;
  virtual bool hasDimensions() const = 0;
};

#endif
