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
#include "node_api.h"
#include "noden_util.h"

#define NODEN_SVM_FINE_CHAR 'f'
#define NODEN_SVM_COARSE_CHAR 'c'
#define NODEN_SVM_NONE_CHAR 'n'

struct createBufCarrier : carrier {
  bool isInput = true;
  char svmType[10] = "none";
  void* data = nullptr;
  size_t dataSize = 0;
  size_t actualSize = 0;
  cl_context context;
  cl_command_queue commands;
};

// void createBufferExecute(napi_env env, void* data);
// void createBufferComplete(napi_env env, napi_status status, void* data);
napi_value createBuffer(napi_env env, napi_callback_info info);

#endif
