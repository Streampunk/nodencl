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

#ifndef NODEN_CONTEXT_H
#define NODEN_CONTEXT_H

#ifdef __APPLE__
    #include "OpenCL/opencl.h"
#else
    #include "CL/cl.h"
#endif
#include <stdio.h>
#include <string>
#include <vector>
#include <tuple>
#include "node_api.h"
#include "noden_util.h"

class clVersion {
  public:
    clVersion(uint32_t major, uint32_t minor) : mMajor(major), mMinor(minor) {}
    clVersion(const std::string& clVersionStr) : mMajor(1), mMinor(0) { fromString(clVersionStr); }
    ~clVersion() {}

    friend bool operator<(const clVersion& l, const clVersion& r) {
      return std::tie(l.mMajor, l.mMinor) < std::tie(r.mMajor, r.mMinor);
    }
    friend bool operator>(const clVersion& l, const clVersion& r) { return r < l; }
    friend bool operator<=(const clVersion& l, const clVersion& r) { return !(l > r); }
    friend bool operator>=(const clVersion& l, const clVersion& r) { return !(l < r); }

    friend bool operator==(const clVersion& l, const clVersion& r) { 
      return std::tie(l.mMajor, l.mMinor) == std::tie(r.mMajor, r.mMinor);
    }
    friend bool operator!=(const clVersion& l, const clVersion& r) { return !(l == r); }

    std::string toString() const { return std::string("OpenCL ") + std::to_string(mMajor) + "." + std::to_string(mMinor); }

  private:
    uint32_t mMajor;
    uint32_t mMinor;

    void fromString(const std::string& clVersionStr) {
      if (2 != sscanf(clVersionStr.c_str(), "%*s%d.%d", &mMajor, &mMinor))
        printf("Unexpected version string %s - should start with \'OpenCL\'\n", clVersionStr.c_str());
    }
};

struct deviceInfo {
  clVersion oclVer;

  deviceInfo(const clVersion& v) : oclVer(v) {}
};

struct createContextCarrier : carrier {
  cl_platform_id platformId;
  cl_device_id deviceId;
  cl_context context;
  uint32_t numQueues;
  std::vector<cl_command_queue> commandQueues;
  std::string deviceVersion;
};

napi_value createContext(napi_env env, napi_callback_info info);

#endif
