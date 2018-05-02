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
#include <stdio.h>
#include <assert.h>
#include <vector>
#include <iostream>
#include <fstream>
#include <math.h>
#include <time.h>
#include "noden_util.h"
#include "node_api.h"

cl_int GetPlatformName(cl_platform_id id, std::string* result) {
  size_t size = 0;
  cl_int error;
  error = clGetPlatformInfo(id, CL_PLATFORM_VERSION, 0, nullptr, &size);
  PASS_CL_ERROR;

  result->resize(size);
  error = clGetPlatformInfo (id, CL_PLATFORM_VERSION, size,
  	const_cast<char*> (result->data()), nullptr);
  PASS_CL_ERROR;

  return CL_SUCCESS;
}

cl_int GetDeviceName(cl_device_id id, std::string* result) {
  size_t size = 0;
  cl_int error;
  error = clGetDeviceInfo (id, CL_DEVICE_NAME, 0, nullptr, &size);
  PASS_CL_ERROR;

  result->resize(size);
  error = clGetDeviceInfo (id, CL_DEVICE_NAME, size,
  	const_cast<char*> (result->data()), nullptr);
  PASS_CL_ERROR;

  return CL_SUCCESS;
}

cl_int GetDeviceType(cl_device_id id, std::string* result) {
  cl_device_type deviceType;
  cl_int error;
  error = clGetDeviceInfo(id, CL_DEVICE_TYPE, sizeof(cl_device_type), &deviceType, nullptr);
  PASS_CL_ERROR;

  char* typeName;
  switch (deviceType) {
  case CL_DEVICE_TYPE_CPU:
    typeName = "CPU";
    break;
  case CL_DEVICE_TYPE_GPU:
    typeName = "GPU";
    break;
  case CL_DEVICE_TYPE_ACCELERATOR:
    typeName = "ACCELERATOR";
    break;
  default:
    typeName = "DEFAULT";
    break;
  }
  result->assign(typeName);
  return CL_SUCCESS;
}

std::vector<cl_platform_id> getPlatformIds() {
  cl_uint platformIdCount = 0;
  clGetPlatformIDs(0, nullptr, &platformIdCount);

  std::vector<cl_platform_id> platformIds(platformIdCount);
  clGetPlatformIDs(platformIdCount, platformIds.data(), nullptr);

  return platformIds;
}

std::vector<cl_device_id> getDeviceIds(cl_uint platformId) {
  std::vector<cl_platform_id> platformIds = getPlatformIds();

  cl_uint deviceIdCount = 0;
  clGetDeviceIDs (platformIds[platformId], CL_DEVICE_TYPE_ALL, 0, nullptr,
    &deviceIdCount);
  std::vector<cl_device_id> deviceIds(deviceIdCount);
  clGetDeviceIDs (platformIds[platformId], CL_DEVICE_TYPE_ALL, deviceIdCount,
    deviceIds.data(), nullptr);

  return deviceIds;
}

napi_value GetPlatformNames(napi_env env, napi_callback_info info) {
  napi_value result;
  napi_status status;
  cl_int error;

  std::vector<cl_platform_id> platformIds = getPlatformIds();

  status = napi_create_array_with_length(env, platformIds.size(), &result);
  CHECK_STATUS;

  for ( uint32_t x = 0 ; x < platformIds.size() ; x++ ) {
    napi_value element;
    std::string platName;
    error = GetPlatformName(platformIds[x], &platName);
    CHECK_CL_ERROR;
    status = napi_create_string_utf8(env, platName.data(), platName.size() - 1, &element);
    CHECK_STATUS;
    status = napi_set_element(env, result, x, element);
    CHECK_STATUS;
  }
  return result;
}

napi_value GetDeviceNames(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  cl_int error;

  size_t argc = 1;
  napi_value args[1];
  status = napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
  CHECK_STATUS;

  if (argc < 1) {
    napi_throw_type_error(env, nullptr, "Wrong number of arguments.");
    return nullptr;
  }

  napi_valuetype t;
  status = napi_typeof(env, args[0], &t);
  CHECK_STATUS;
  if (t != napi_number) {
    napi_throw_type_error(env, nullptr, "First arugment is a number - the platform ID.");
    return nullptr;
  }

  int32_t platformId = 0;
  status = napi_get_value_int32(env, args[0], &platformId);
  CHECK_STATUS;

  std::vector<cl_device_id> deviceIds = getDeviceIds((cl_uint) platformId);

  status = napi_create_array_with_length(env, deviceIds.size(), &result);
  CHECK_STATUS;

  for ( uint32_t x = 0 ; x < deviceIds.size() ; x++ ) {
    napi_value element;
    std::string deviceName;
    error = GetDeviceName(deviceIds[x], &deviceName);
    CHECK_CL_ERROR;
    status = napi_create_string_utf8(env, deviceName.data(), deviceName.size() - 1, &element);
    CHECK_STATUS;
    status = napi_set_element(env, result, x, element);
    CHECK_STATUS;
  }

  return result;
}

napi_value GetDeviceTypes(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  cl_int error;

  size_t argc = 1;
  napi_value args[1];
  status = napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
  CHECK_STATUS;

  if (argc < 1) {
    napi_throw_type_error(env, nullptr, "Wrong number of arguments.");
    return nullptr;
  }

  napi_valuetype t;
  status = napi_typeof(env, args[0], &t);
  CHECK_STATUS;
  if (t != napi_number) {
    napi_throw_type_error(env, nullptr, "First arugment is a number - the platform ID.");
    return nullptr;
  }

  int32_t platformId = 0;
  status = napi_get_value_int32(env, args[0], &platformId);
  CHECK_STATUS;

  std::vector<cl_device_id> deviceIds = getDeviceIds((cl_uint) platformId);

  status = napi_create_array_with_length(env, deviceIds.size(), &result);
  CHECK_STATUS;

  for ( uint32_t x = 0 ; x < deviceIds.size() ; x++ ) {
    napi_value element;
    std::string deviceType;
    error = GetDeviceType(deviceIds[x], &deviceType);
    CHECK_CL_ERROR;
    status = napi_create_string_utf8(env, deviceType.data(), deviceType.size(), &element);
    CHECK_STATUS;
    status = napi_set_element(env, result, x, element);
    CHECK_STATUS;
  }

  return result;
}

void tidyQueue(napi_env env, void* data, void* hint) {
  printf("Queue finalizer called.\n");
  cl_int error = CL_SUCCESS;
  error = clReleaseCommandQueue((cl_command_queue) data);
  assert(error == CL_SUCCESS);
}

void tidyContext(napi_env env, void* data, void* hint) {
  printf("Context finalizer called.\n");
  cl_int error = CL_SUCCESS;
  error = clReleaseContext((cl_context) data);
  assert(error == CL_SUCCESS);
}

void tidyProgram(napi_env env, void* data, void* hint) {
  printf("Program finalizer called.\n");
  cl_int error = CL_SUCCESS;
  error = clReleaseProgram((cl_program) data);
  assert(error == CL_SUCCESS);
}

void tidyKernel(napi_env env, void* data, void* hint) {
  printf("Kernel finalizer called.\n");
  cl_int error = CL_SUCCESS;
  error = clReleaseKernel((cl_kernel) data);
  assert(error == CL_SUCCESS);
}

/* typedef struct {
  char* kernelSource;
  size_t kernelSize;
  int32_t platformIndex;
  int32_t deviceIndex;
  cl_device_id device_id;             // compute device id
  cl_context context;                 // compute context
  cl_command_queue commands;          // compute command queue
  cl_program program;                 // compute program
  cl_kernel kernel;                   // compute kernel
  napi_deferred _deferred;
  napi_async_work _request;
} buildCarrier;

void BuildExecute(napi_env env, void* data) {
  buildCarrier* c = status_cast<buildCarrier*> data;
}

void BuildComplete(napi_env env, napi_status status, void* data) {
  buildCarrier* c = status_cast<buildCarrier*> data;

}

napi_value BuildAProgram(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value promise;
  napi_value resource_name;
  buildCarrier carrier;


  size_t argc = 3;
  napi_value args[3];
  status = napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
  assert(status == napi_ok);

  if (argc < 3) {
    napi_throw_type_error(env, nullptr, "Wrong number of arguments.");
    return nullptr;
  }

  napi_valuetype t;
  status = napi_typeof(env, args[0], &t);
  assert(status == napi_ok);
  if (t != napi_number) {
    napi_throw_type_error(env, nullptr, "First arugment is a number - the platform ID.");
    return nullptr;
  }

  status = napi_typeof(env, args[1], &t);
  assert(status == napi_ok);
  if (t != napi_number) {
    napi_throw_type_error(env, nullptr, "Second argument is a number - the Device ID.");
    return nullptr;
  }

  status = napi_typeof(env, args[2], &t);
  assert(status == napi_ok);
  if (t != napi_string) {
    napi_throw_type_error(env, nullptr, "Third argument is a string - the kernel.");
    return nullptr;
  }

  status = napi_get_value_int32(env, args[0], &carrier.platformIndex);
  assert(status == napi_ok);

  status = napi_get_value_int32(env, args[1], &carrier.deviceIndex);
  assert(status == napi_ok);
  std::vector<cl_device_id> deviceIds = getDeviceIds(carrier.platformIndex);
  carrier.device_id = deviceIds[carrier.deviceIndex];

  status = napi_get_value_string_utf8(env, args[2], nullptr, 0, &carrier.kernelSize);
  assert(status == napi_ok);
  carrier.kernelSource = (char *) malloc(sizeof(char) * (carrier.kernelSize + 1));
  carrier.kernelSource[carrier.kernelSize] = '\0';
  status = napi_get_value_string_utf8(env, args[2], carrier.kernelSource,
    carrier.kernelSize + 1, &carrier.kernelSize);
  assert(status == napi_ok);

  status = napi_create_promise(env, &carrier._deferred, &promise);
  assert(status == napi_ok);

  status = napi_create_string_utf8(env, "BuildProgram", NAPI_AUTO_LENGTH, &resource_name);
  assert(status == napi_ok);
  status = napi_create_async_work(env, NULL, resource_name, BuildExecute,
    BuildComplete, &carrier, &carrier._request);
  assert(status == napi_ok);
  status = napi_queue_async_work(env, carrier._request);
  assert(status == napi_ok);

  return promise;
} */

napi_value BuildAProgram(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  cl_int error = CL_SUCCESS;

  char* kernelSource;
  cl_device_id device_id;             // compute device id
  cl_context context;                 // compute context
  cl_command_queue commands;          // compute command queue
  cl_program program;                 // compute program
  cl_kernel kernel;                   // compute kernel

  cl_mem input;
  cl_mem output;

  clock_t begin;
  clock_t end;

  int32_t count = 8294400;

  unsigned char* data = (unsigned char *) malloc(count);
  unsigned char* results = (unsigned char *) malloc(count);

  for ( int32_t i = 0; i < count ; i++) {
    data[i] = i % 256;
  }

  size_t argc = 3;
  napi_value args[3];
  status = napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
  assert(status == napi_ok);

  if (argc < 3) {
    napi_throw_type_error(env, nullptr, "Wrong number of arguments.");
    return nullptr;
  }

  napi_valuetype t;
  status = napi_typeof(env, args[0], &t);
  assert(status == napi_ok);
  if (t != napi_number) {
    napi_throw_type_error(env, nullptr, "First arugment is a number - the platform ID.");
    return nullptr;
  }

  status = napi_typeof(env, args[1], &t);
  assert(status == napi_ok);
  if (t != napi_number) {
    napi_throw_type_error(env, nullptr, "Second argument is a number - the Device ID.");
    return nullptr;
  }

  status = napi_typeof(env, args[2], &t);
  assert(status == napi_ok);
  if (t != napi_string) {
    napi_throw_type_error(env, nullptr, "Third argument is a string - the kernel.");
    return nullptr;
  }

  int32_t platformIndex = 0;
  status = napi_get_value_int32(env, args[0], &platformIndex);
  assert(status == napi_ok);

  int32_t deviceIndex = 0;
  status = napi_get_value_int32(env, args[1], &deviceIndex);
  assert(status == napi_ok);
  std::vector<cl_device_id> deviceIds = getDeviceIds(platformIndex);
  device_id = deviceIds[deviceIndex];

  cl_device_svm_capabilities caps;
  error = clGetDeviceInfo(device_id, CL_DEVICE_SVM_CAPABILITIES,
    sizeof(cl_device_svm_capabilities), &caps, 0);
  printf("Device caps %i = %i. Bitfield = %i\n", device_id, error, caps);
  assert(error == CL_SUCCESS);

  size_t kernelSize = 0;
  status = napi_get_value_string_utf8(env, args[2], nullptr, 0, &kernelSize);
  assert(status == napi_ok);
  kernelSource = (char *) malloc(sizeof(char) *(kernelSize + 1));
  kernelSource[kernelSize] = '\0';
  status = napi_get_value_string_utf8(env, args[2], kernelSource, kernelSize + 1, &kernelSize);
  assert(status == napi_ok);

  context = clCreateContext(0, 1, &device_id, nullptr, nullptr, &error);
  assert(context != nullptr && error == CL_SUCCESS);

  commands = clCreateCommandQueue(context, device_id, 0, &error);
  assert(commands != nullptr && error == CL_SUCCESS);

  begin = clock();
  program = clCreateProgramWithSource(context, 1, (const char **) &kernelSource,
    nullptr, &error);
  assert(program != nullptr && error == CL_SUCCESS);

  error = clBuildProgram(program, 0, nullptr, nullptr, nullptr, nullptr);
  if (error != CL_SUCCESS) {
    size_t len;
    char buffer[2048];

    printf("Errors: Failed to build executables.\n");
    clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_LOG,
      sizeof(buffer), buffer, &len);
    printf("%s\n", buffer);
    napi_throw_error(env, nullptr, "Failed to build executable.");
    return nullptr;
  }

  kernel = clCreateKernel(program, "square", &error);
  assert(kernel != nullptr && error == CL_SUCCESS);
  end = clock();
  printf("Time spent building kernel is %lf.\n", (double)(end - begin)/CLOCKS_PER_SEC);


  napi_value jscontext;
  napi_value jskernel;
  napi_value jscommands;
  napi_value jsprogram;
  napi_value jsdeviceid;

  status = napi_create_external(env, (void *) context, tidyContext, nullptr, &jscontext);
  assert(status == napi_ok);
  status = napi_create_external(env, (void *) kernel, tidyKernel, nullptr, &jskernel);
  assert(status == napi_ok);
  status = napi_create_external(env, (void *) program, tidyProgram, nullptr, &jsprogram);
  assert(status == napi_ok);
  status = napi_create_external(env, (void *) commands, tidyQueue, nullptr, &jscommands);
  assert(status == napi_ok);
  status = napi_create_external(env, (void *) device_id, nullptr, nullptr, &jsdeviceid);
  assert(status == napi_ok);

  // clReleaseMemObject(input);
  // clReleaseMemObject(output);
  // clReleaseProgram(program);
  // clReleaseKernel(kernel);
  // clReleaseCommandQueue(commands);
  // clReleaseContext(context);

  status = napi_create_object(env, &result);
  assert(status == napi_ok);

  napi_value jsCaps;
  status = napi_create_int32(env, caps, &jsCaps);
  assert(status == napi_ok);

  status = napi_set_named_property(env, result, "platformIndex", args[0]);
  assert(status == napi_ok);
  status = napi_set_named_property(env, result, "deviceIndex", args[1]);
  assert(status == napi_ok);
  status = napi_set_named_property(env, result, "svmCaps", jsCaps);
  assert(status == napi_ok);
  status = napi_set_named_property(env, result, "deviceId", jsdeviceid);
  assert(status == napi_ok);
  status = napi_set_named_property(env, result, "context", jscontext);
  assert(status == napi_ok);
  status = napi_set_named_property(env, result, "commands", jscommands);
  assert(status == napi_ok);
  status = napi_set_named_property(env, result, "program", jsprogram);
  assert(status == napi_ok);
  status = napi_set_named_property(env, result, "kernel", jskernel);
  assert(status == napi_ok);

  return result;
}

#define DECLARE_NAPI_METHOD(name, func) { name, 0, func, 0, 0, 0, napi_default, 0 }

napi_value RunProgram(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  cl_int error;

  cl_mem input;
  cl_mem output;

  clock_t begin;
  clock_t end;

  size_t argc = 2;
  napi_value args[2];
  status = napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
  assert(status == napi_ok);

  if (argc < 2) {
    napi_throw_type_error(env, nullptr, "Wrong number of arguments.");
    return nullptr;
  }

  napi_valuetype t;
  status = napi_typeof(env, args[0], &t);
  assert(status == napi_ok);
  if (t != napi_object) {
    napi_throw_type_error(env, nullptr, "First arugment is an object - kernel details.");
    return nullptr;
  }

  bool isBuffer;
  status = napi_is_buffer(env, args[1], &isBuffer);
  assert(status == napi_ok);
  if (!isBuffer) {
    napi_throw_type_error(env, nullptr, "Second argument is a buffer - the data.");
    return nullptr;
  }

  void* data;
  size_t dataSize;
  status = napi_get_buffer_info(env, args[1], &data, &dataSize);
  assert(status == napi_ok);

  // printf("Data size is %i.\n", dataSize);

  void* extvalue;
  napi_value jsvalue;
  status = napi_get_named_property(env, args[0], "context", &jsvalue);
  assert(jsvalue != nullptr && status == napi_ok);
  status = napi_get_value_external(env, jsvalue, &extvalue);
  cl_context context = (cl_context) extvalue;
  assert(context != nullptr);
  assert(status == napi_ok);

  status = napi_get_named_property(env, args[0], "commands", &jsvalue);
  assert(status == napi_ok);
  status = napi_get_value_external(env, jsvalue, &extvalue);
  cl_command_queue commands = (cl_command_queue) extvalue;
  assert(commands != nullptr && status == napi_ok);

  status = napi_get_named_property(env, args[0], "kernel", &jsvalue);
  assert(status == napi_ok);
  status = napi_get_value_external(env, jsvalue, &extvalue);
  cl_kernel kernel = (cl_kernel) extvalue;
  assert(kernel != nullptr && status == napi_ok);

  status = napi_get_named_property(env, args[0], "deviceId", &jsvalue);
  assert(status == napi_ok);
  status = napi_get_value_external(env, jsvalue, &extvalue);
  cl_device_id device_id = (cl_device_id) extvalue;
  assert(device_id != nullptr && status == napi_ok);

  begin = clock();
  input = clCreateBuffer(context, CL_MEM_READ_ONLY, dataSize,
    nullptr, &error);
  assert(error == CL_SUCCESS);
  output = clCreateBuffer(context, CL_MEM_WRITE_ONLY, dataSize,
    nullptr, &error);
  assert(error == CL_SUCCESS);
  assert(input != nullptr && output != nullptr);
  end = clock();
  printf("Time spent allocating GPU buffers is %lf.\n", (double)(end - begin)/CLOCKS_PER_SEC);

  begin = clock();
  error = clEnqueueWriteBuffer(commands, input, CL_TRUE, 0, dataSize,
    data, 0, nullptr, nullptr);
  assert(error == CL_SUCCESS);
  end = clock();
  //printf("Time spent writing buffer is %lf.\n", (double)(end - begin)/CLOCKS_PER_SEC);

  error = CL_SUCCESS;
  error = clSetKernelArg(kernel, 0, sizeof(cl_mem), &input);
  error |= clSetKernelArg(kernel, 1, sizeof(cl_mem), &output);
  error |= clSetKernelArg(kernel, 2, sizeof(unsigned int), &dataSize);
  assert(error == CL_SUCCESS);

  size_t local;
  error = clGetKernelWorkGroupInfo(kernel, device_id, CL_KERNEL_WORK_GROUP_SIZE,
    sizeof(local), &local, nullptr);
  assert(error == CL_SUCCESS);
  //printf("Kernel workgroup size is %i.\n", local);

  begin = clock();
  size_t global = dataSize;
  error = clEnqueueNDRangeKernel(commands, kernel, 1, nullptr, &global, &local,
    0, nullptr, nullptr);
  assert(error == CL_SUCCESS);

  clFinish(commands);
  end = clock();
  //printf("Time spent calculating result is %lf.\n", (double)(end - begin)/CLOCKS_PER_SEC);
  //printf("Global gets set to %i.\n", global);

  begin = clock();
  error = clEnqueueReadBuffer(commands, output, CL_TRUE, 0,
    dataSize, data, 0, nullptr, nullptr);
  assert(error == CL_SUCCESS);
  end = clock();
  //printf("Time spent reading buffer is %lf.\n", (double)(end - begin)/CLOCKS_PER_SEC);

  return args[1];
}

napi_value RunProgramSVM(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  cl_int error;

  char* svmInput;
  char* svmOutput;

  HR_TIME_POINT start;

  size_t argc = 2;
  napi_value args[2];
  status = napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
  assert(status == napi_ok);

  if (argc < 2) {
    napi_throw_type_error(env, nullptr, "Wrong number of arguments.");
    return nullptr;
  }

  napi_valuetype t;
  status = napi_typeof(env, args[0], &t);
  assert(status == napi_ok);
  if (t != napi_object) {
    napi_throw_type_error(env, nullptr, "First arugment is an object - kernel details.");
    return nullptr;
  }

  bool isBuffer;
  status = napi_is_buffer(env, args[1], &isBuffer);
  assert(status == napi_ok);
  if (!isBuffer) {
    napi_throw_type_error(env, nullptr, "Second argument is a buffer - the data.");
    return nullptr;
  }

  void* data;
  size_t dataSize;
  status = napi_get_buffer_info(env, args[1], &data, &dataSize);
  assert(status == napi_ok);

  // printf("Data size is %i.\n", dataSize);

  void* extvalue;
  napi_value jsvalue;
  status = napi_get_named_property(env, args[0], "context", &jsvalue);
  assert(jsvalue != nullptr && status == napi_ok);
  status = napi_get_value_external(env, jsvalue, &extvalue);
  cl_context context = (cl_context) extvalue;
  assert(context != nullptr);
  assert(status == napi_ok);

  status = napi_get_named_property(env, args[0], "commands", &jsvalue);
  assert(status == napi_ok);
  status = napi_get_value_external(env, jsvalue, &extvalue);
  cl_command_queue commands = (cl_command_queue) extvalue;
  assert(commands != nullptr && status == napi_ok);

  status = napi_get_named_property(env, args[0], "kernel", &jsvalue);
  assert(status == napi_ok);
  status = napi_get_value_external(env, jsvalue, &extvalue);
  cl_kernel kernel = (cl_kernel) extvalue;
  assert(kernel != nullptr && status == napi_ok);

  status = napi_get_named_property(env, args[0], "deviceId", &jsvalue);
  assert(status == napi_ok);
  status = napi_get_value_external(env, jsvalue, &extvalue);
  cl_device_id device_id = (cl_device_id) extvalue;
  assert(device_id != nullptr && status == napi_ok);

  start = std::chrono::high_resolution_clock::now();
  /* input = clCreateBuffer(context, CL_MEM_READ_ONLY, dataSize,
    nullptr, &error);
  assert(error == CL_SUCCESS);
  output = clCreateBuffer(context, CL_MEM_WRITE_ONLY, dataSize,
    nullptr, &error);
  assert(error == CL_SUCCESS);
  assert(input != nullptr && output != nullptr); */
  svmInput = (char *) data; //clSVMAlloc(context, CL_MEM_READ_ONLY, dataSize, 0);
  assert(svmInput != nullptr);
  svmOutput = (char *) clSVMAlloc(context, CL_MEM_WRITE_ONLY, dataSize, 0);
  assert(svmOutput != nullptr);
  long long createBufMS = microTime(start);
  //printf("Time spent allocating GPU buffers is %lld.\n", microseconds);

  start = std::chrono::high_resolution_clock::now();
  /* error = clEnqueueWriteBuffer(commands, input, CL_TRUE, 0, dataSize,
    data, 0, nullptr, nullptr); */
  //error = clEnqueueSVMMap(commands, CL_TRUE, CL_MAP_WRITE, svmInput, dataSize, 0, 0, 0);
  //assert(error == CL_SUCCESS);
  // error = clEnqueueSVMMemcpy(commands, CL_TRUE, svmInput, data, dataSize, 0, 0, 0);
  // assert(error == CL_SUCCESS);

  // memcpy(svmInput, data, dataSize);
  // assert(error == CL_SUCCESS);
  error = clEnqueueSVMUnmap(commands, svmInput, 0, 0, 0);
  assert(error == CL_SUCCESS);
  // end = clock();
  long long writeBufMS = microTime(start);
  //printf("Time spent writing buffer is %lf.\n", (double)(end - begin)/CLOCKS_PER_SEC);

  error = CL_SUCCESS;
  /* error = clSetKernelArg(kernel, 0, sizeof(cl_mem), &input);
  error |= clSetKernelArg(kernel, 1, sizeof(cl_mem), &output); */
  error = clSetKernelArgSVMPointer(kernel, 0, svmInput);
  error = clSetKernelArgSVMPointer(kernel, 1, svmOutput);
  error |= clSetKernelArg(kernel, 2, sizeof(unsigned int), &dataSize);
  assert(error == CL_SUCCESS);

  size_t local;
  error = clGetKernelWorkGroupInfo(kernel, device_id, CL_KERNEL_WORK_GROUP_SIZE,
    sizeof(local), &local, nullptr);
  assert(error == CL_SUCCESS);
  //printf("Kernel workgroup size is %i.\n", local);

  start = std::chrono::high_resolution_clock::now();
  size_t global = dataSize;
  error = clEnqueueNDRangeKernel(commands, kernel, 1, nullptr, &global, &local,
    0, nullptr, nullptr);
  assert(error == CL_SUCCESS);

  clFinish(commands);
  long long kernelMS = microTime(start);
  //printf("Time spent calculating result is %lf.\n", (double)(end - begin)/CLOCKS_PER_SEC);
  //printf("Global gets set to %i.\n", global);

  start = std::chrono::high_resolution_clock::now();
  /* error = clEnqueueReadBuffer(commands, output, CL_TRUE, 0,
    dataSize, data, 0, nullptr, nullptr); */
  error = clEnqueueSVMMap(commands, CL_TRUE, CL_MAP_READ, svmOutput, dataSize, 0, 0, 0);
  assert(error == CL_SUCCESS);
  error = clEnqueueSVMMap(commands, CL_TRUE, CL_MAP_WRITE, svmInput, dataSize, 0, 0, 0);
  assert(error == CL_SUCCESS);
  memcpy(data, svmOutput, dataSize);
  //error = clEnqueueSVMMemcpy(commands, CL_TRUE, data, svmOutput, dataSize, 0, 0, 0);
  //printf("Output error is %i\n", error);
  error = clEnqueueSVMUnmap(commands, svmOutput, 0, 0, 0);
  assert(error == CL_SUCCESS);
  long long readBufMS = microTime(start);
  //printf("Time spent reading buffer is %lf.\n", (double)(end - begin)/CLOCKS_PER_SEC);

  start = std::chrono::high_resolution_clock::now();
  // clSVMFree(context, svmInput);
  clSVMFree(context, svmOutput);
  long long freeMS = microTime(start);
  printf("create = %lld, write = %lld, kernel = %lld, read = %lld, free = %lld, total = %lld\n",
    createBufMS, writeBufMS, kernelMS, readBufMS, freeMS, createBufMS + writeBufMS + kernelMS + readBufMS + freeMS);
  return args[1];
}

#define DECLARE_NAPI_METHOD(name, func) { name, 0, func, 0, 0, 0, napi_default, 0 }

void finalizeSVM(napi_env env, void* data, void* hint) {
  cl_context context = (cl_context) hint;
  printf("Finalizing a SVM buffer.\n");
  clSVMFree(context, data);
}

napi_value CreateSVMBuffer(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  cl_int error;

  size_t argc = 2;
  napi_value args[2];
  status = napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
  assert(status == napi_ok);

  if (argc < 2) {
    napi_throw_type_error(env, nullptr, "Wrong number of arguments.");
    return nullptr;
  }

  napi_valuetype t;
  status = napi_typeof(env, args[0], &t);
  assert(status == napi_ok);
  if (t != napi_object) {
    napi_throw_type_error(env, nullptr, "First arugment is an object - kernel details.");
    return nullptr;
  }

  status = napi_typeof(env, args[1], &t);
  assert(status == napi_ok);
  if (t != napi_number) {
    napi_throw_type_error(env, nullptr, "Second argument is a number - the buffer size.");
    return nullptr;
  }

  void* extvalue;
  napi_value jsvalue;
  status = napi_get_named_property(env, args[0], "context", &jsvalue);
  assert(jsvalue != nullptr && status == napi_ok);
  status = napi_get_value_external(env, jsvalue, &extvalue);
  cl_context context = (cl_context) extvalue;
  assert(context != nullptr);
  assert(status == napi_ok);

  status = napi_get_named_property(env, args[0], "commands", &jsvalue);
  assert(status == napi_ok);
  status = napi_get_value_external(env, jsvalue, &extvalue);
  cl_command_queue commands = (cl_command_queue) extvalue;
  assert(commands != nullptr && status == napi_ok);

  int32_t dataSize;
  status = napi_get_value_int32(env, args[1], &dataSize);
  assert(status == napi_ok);

  void* data = clSVMAlloc(context, CL_MEM_READ_ONLY, dataSize, 0);
  assert(data != nullptr);

  status = napi_create_external_buffer(env, dataSize, data, &finalizeSVM, context, &result);
  assert(status == napi_ok);

  error = clEnqueueSVMMap(commands, CL_TRUE, CL_MAP_WRITE | CL_MAP_READ, data, dataSize, 0, 0, 0);
  assert(error == CL_SUCCESS);
  return result;
}

napi_value Init(napi_env env, napi_value exports) {
  napi_status status;
  napi_property_descriptor desc[] = {
    DECLARE_NAPI_METHOD("getPlatformNames", GetPlatformNames),
    DECLARE_NAPI_METHOD("getDeviceNames", GetDeviceNames),
    DECLARE_NAPI_METHOD("buildAProgram", BuildAProgram),
    DECLARE_NAPI_METHOD("runProgram", RunProgram),
    DECLARE_NAPI_METHOD("runProgramSVM", RunProgramSVM),
    DECLARE_NAPI_METHOD("createSVMBuffer", CreateSVMBuffer),
    DECLARE_NAPI_METHOD("getDeviceTypes", GetDeviceTypes)
   };
  status = napi_define_properties(env, exports, 7, desc);
  assert(status == napi_ok);

  return exports;
}

NAPI_MODULE(nodencl, Init)
