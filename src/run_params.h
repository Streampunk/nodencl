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

#ifndef RUN_PARAMS_H
#define RUN_PARAMS_H

#include <string>
#include <map>

class iKernelArg {
public:
  enum class eAccess : uint8_t { NONE = 0, READONLY = 1, WRITEONLY = 2 };
  virtual ~iKernelArg() {}

  virtual std::string name() const = 0;
  virtual std::string type() const = 0;
  virtual eAccess access() const = 0;

  virtual std::string toString() const = 0;
};
typedef std::map<uint32_t, iKernelArg *> tKernelArgMap;

class iRunParams {
public:
  virtual ~iRunParams() {}

  virtual size_t numDims() const = 0;
  virtual const size_t *globalWorkItems() const = 0;
  virtual const size_t *workItemsPerGroup() const = 0;
  virtual const tKernelArgMap kernelArgMap() const = 0;
};

#endif
