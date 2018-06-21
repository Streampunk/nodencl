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

#ifndef NODEN_BUFFER_H
#define NODEN_BUFFER_H

#ifdef __APPLE__
    #include "OpenCL/opencl.h"
#else
    #include "CL/cl.h"
#endif
#include <chrono>
#include <stdio.h>
#include <memory>
#include "node_api.h"
#include "noden_util.h"

napi_value createBuffer(napi_env env, napi_callback_info info);

enum class eMemFlags : uint8_t { READWRITE = 0, WRITEONLY = 1, READONLY = 2 };
enum class eSvmType : uint8_t { NONE = 0, COARSE = 1, FINE = 2 };

class iGpuBuffer {
public:
  virtual ~iGpuBuffer() {}
  virtual cl_int setKernelParam(cl_kernel kernel, uint32_t paramIndex) const = 0;
};

class iHostBuffer {
public:
  virtual ~iHostBuffer() {}

  virtual void *buf() const = 0;
};

class iNodenBuffer {
public:
  virtual ~iNodenBuffer() {}

  virtual std::shared_ptr<iGpuBuffer> getGPUBuffer(cl_int &error) = 0; // allocates GPU buffer for 'NONE' type buffers
  virtual std::shared_ptr<iHostBuffer> getHostBuffer(cl_int &error, eMemFlags hsFlags) = 0; // map/copy GPU buffer to host - host buffer is already created

  virtual uint32_t numBytes() const = 0;
  virtual eMemFlags memFlags() const = 0;
  virtual eSvmType svmType() const = 0;
  virtual std::string svmTypeName() const = 0;
};

#endif
