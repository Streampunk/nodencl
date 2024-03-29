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

#include "noden_program.h"
#include "noden_run.h"
#include "run_params.h"
#include <regex>
#include <sstream>

class kernelArg : public iKernelArg {
  public:
    kernelArg(const std::string& name, const std::string& type, eAccess access)
      : mName(name), mType(type), mAccess(access) {}
    ~kernelArg() {}

    std::string name() const { return mName; }
    std::string type() const { return mType; }
    eAccess access() const { return mAccess; }

    std::string toString() const {
      return mType + " " + mName + (eAccess::READONLY == mAccess ? " readonly" :
                                    eAccess::WRITEONLY == mAccess ? " writeonly" : 
                                    "");
    }
 
  private:
    const std::string mName;
    const std::string mType;
    const eAccess mAccess;
};

class runParams : public iRunParams {
public:
  runParams(const std::vector<size_t>& gwi, const std::vector<size_t>& wig, const tKernelArgMap& kernelArgMap) :
    mGlobalWorkItems(gwi), mWorkItemsPerGroup(wig), mKernelArgMap(kernelArgMap) {}
  ~runParams() {}

  size_t numDims() const { return mGlobalWorkItems.size(); }
  const size_t *globalWorkItems() const { return mGlobalWorkItems.data(); }
  const size_t *workItemsPerGroup() const { return mWorkItemsPerGroup.data(); }
  const tKernelArgMap kernelArgMap() const { return mKernelArgMap; }

  void argDebug(const std::string& kernelName) const {
    printf("%s (\n", kernelName.c_str());
    for (auto& argIter: mKernelArgMap) {
      uint32_t p = argIter.first;
      iKernelArg* arg = argIter.second;
      printf("  %d: %s\n", p, arg->toString().c_str());
    }
    printf(")\n");
  }

private:
  const std::vector<size_t> mGlobalWorkItems;
  const std::vector<size_t> mWorkItemsPerGroup;
  const tKernelArgMap mKernelArgMap;
};

void tidyProgram(napi_env env, void* data, void* hint) {
  printf("Program finalizer called.\n");
  cl_int error = CL_SUCCESS;
  error = clReleaseProgram((cl_program) data);
  if (error != CL_SUCCESS) printf("Failed to release CL program.\n");
}

void tidyKernel(napi_env env, void* data, void* hint) {
  printf("Kernel finalizer called.\n");
  cl_int error = CL_SUCCESS;
  error = clReleaseKernel((cl_kernel) data);
  if (error != CL_SUCCESS) printf("Failed to release CL kernel.\n");
}

void tidyParams(napi_env env, void* data, void* hint) {
  printf("Params finalizer called.\n");
  iRunParams *rp = (iRunParams*)data;
  for (auto& argIter: rp->kernelArgMap())
     delete argIter.second;
  delete rp;
}

void tidyProgramContextRef(napi_env env, void* data, void* hint) {
  napi_ref contextRef = (napi_ref)data;
  printf("Finalizing a program context reference.\n");
  napi_delete_reference(env, contextRef);
}

cl_int getArgInfo(cl_kernel kernel, cl_uint arg, cl_uint param, std::string& info) {
  size_t paramLen = 0;
  cl_int error = clGetKernelArgInfo(kernel, arg, param, 0, nullptr, &paramLen);
  PASS_CL_ERROR;
  char* paramStr = (char *)malloc(sizeof(char) * paramLen);
  error = clGetKernelArgInfo(kernel, arg, param, paramLen, paramStr, NULL);
  PASS_CL_ERROR;
  info = std::string(paramStr);
  free(paramStr);
  return error;
}

// Promise to create a program with context and queue
void buildExecute(napi_env env, void* data) {
  buildCarrier* c = (buildCarrier*) data;
  cl_int error;

  std::stringstream gwiss;
  if (c->globalWorkItems.size() > 1) gwiss << "[ ";
  for (size_t i = 0; i < c->globalWorkItems.size(); ++i) {
    if (i > 0) gwiss << ", ";
    gwiss << c->globalWorkItems[i];
  }
  if (c->globalWorkItems.size() > 1) gwiss << " ]";

  std::stringstream wigss;
  if (0 == c->workItemsPerGroup.size()) wigss << "[]";
  else if (c->workItemsPerGroup.size() > 1) wigss << "[ ";
  for (size_t i = 0; i < c->workItemsPerGroup.size(); ++i) {
    if (i > 0) wigss << ", ";
    wigss << c->workItemsPerGroup[i];
  }
  if (c->workItemsPerGroup.size() > 1) wigss << " ]";

  // printf("globalWorkItems: %s, workItemsPerGroup: %s\n", gwiss.str().c_str(), wigss.str().c_str());
  HR_TIME_POINT start = NOW;

  const char* kernelSource[1];
  kernelSource[0] = c->kernelSource.data();
  c->program = clCreateProgramWithSource(c->context, 1, kernelSource,
    nullptr, &error);
  ASYNC_CL_ERROR;

  const char* buildOptions = "-cl-kernel-arg-info -cl-std=CL3.0";
  error = clBuildProgram(c->program, 0, nullptr, buildOptions, nullptr, nullptr);
  if (error != CL_SUCCESS) {
    size_t len;
    clGetProgramBuildInfo(c->program, c->deviceId, CL_PROGRAM_BUILD_LOG,
      0, NULL, &len);
    char* buffer = (char*)std::calloc(len, sizeof(char));

    clGetProgramBuildInfo(c->program, c->deviceId, CL_PROGRAM_BUILD_LOG,
      len, buffer, NULL);
    c->status = NODEN_BUILD_ERROR;
    c->errorMsg = std::string(buffer);
    delete[] buffer;
    return;
  }

  c->kernel = clCreateKernel(c->program, c->kernelName.c_str(), &error);
  ASYNC_CL_ERROR;

  size_t deviceWorkGroupSize;
  error = clGetKernelWorkGroupInfo(c->kernel, c->deviceId, CL_KERNEL_WORK_GROUP_SIZE,
    sizeof(size_t), &deviceWorkGroupSize, nullptr);
  ASYNC_CL_ERROR;

  size_t requestedWorkItemsSize = 1;
  for (size_t i = 0; i < c->workItemsPerGroup.size(); ++i)
    requestedWorkItemsSize *= c->workItemsPerGroup[i];

  if (requestedWorkItemsSize > deviceWorkGroupSize) {
    c->status = NODEN_OUT_OF_RANGE;
    char* errorMsg = (char *) malloc(200);
    sprintf(errorMsg, "Parameter workItemsPerGroup %s is larger than the available workgroup size (%zd) for platform %i.",
            wigss.str().c_str(), deviceWorkGroupSize, c->platformIndex);
    c->errorMsg = std::string(errorMsg);
    delete[] errorMsg;
    return;
  }

  cl_uint numArgs = 0;
  error = clGetKernelInfo(c->kernel, CL_KERNEL_NUM_ARGS, sizeof(numArgs), &numArgs, NULL);
  ASYNC_CL_ERROR;

  tKernelArgMap kernelArgMap;
  for (cl_uint p=0; p<numArgs; ++p) {
    std::string argName;
    error = getArgInfo(c->kernel, p, CL_KERNEL_ARG_NAME, argName);
    ASYNC_CL_ERROR;

    std::string argType;
    error = getArgInfo(c->kernel, p, CL_KERNEL_ARG_TYPE_NAME, argType);
    ASYNC_CL_ERROR;

    cl_kernel_arg_access_qualifier accessQualifier;
    error = clGetKernelArgInfo(c->kernel, p, CL_KERNEL_ARG_ACCESS_QUALIFIER, sizeof(accessQualifier), &accessQualifier, NULL);
    ASYNC_CL_ERROR;
    kernelArg::eAccess argAccess(CL_KERNEL_ARG_ACCESS_READ_ONLY == accessQualifier ? kernelArg::eAccess::READONLY :
                                 CL_KERNEL_ARG_ACCESS_WRITE_ONLY == accessQualifier ? kernelArg::eAccess::WRITEONLY :
                                 kernelArg::eAccess::NONE);
    kernelArg *ka = new kernelArg(argName, argType, argAccess);
    kernelArgMap.emplace(p, ka);
  }
  c->runParams = new runParams(c->globalWorkItems, c->workItemsPerGroup, kernelArgMap);

  c->totalTime = microTime(start);
}

void buildComplete(napi_env env, napi_status asyncStatus, void* data) {
  buildCarrier* c = (buildCarrier*) data;
  napi_value result;

  if (asyncStatus != napi_ok) {
    c->status = asyncStatus;
    c->errorMsg = "Async build of program failed to complete.";
  }
  REJECT_STATUS;

  c->status = napi_get_reference_value(env, c->passthru, &result);
  REJECT_STATUS;

  napi_value jsDeviceId;
  c->status = napi_create_external(env, c->deviceId, nullptr, nullptr, &jsDeviceId);
  REJECT_STATUS;
  c->status = napi_set_named_property(env, result, "deviceId", jsDeviceId);
  REJECT_STATUS;

  napi_value jsExtProgram;
  c->status = napi_create_external(env, c->program, tidyProgram, nullptr, &jsExtProgram);
  REJECT_STATUS;
  c->status = napi_set_named_property(env, result, "program", jsExtProgram);
  REJECT_STATUS;

  napi_value jsKernel;
  c->status = napi_create_external(env, c->kernel, tidyKernel, nullptr, &jsKernel);
  REJECT_STATUS;
  c->status = napi_set_named_property(env, result, "kernel", jsKernel);
  REJECT_STATUS;

  napi_value jsBuildTime;
  c->status = napi_create_double(env, c->totalTime / 1000000.0, &jsBuildTime);
  REJECT_STATUS;
  c->status = napi_set_named_property(env, result, "buildTime", jsBuildTime);
  REJECT_STATUS;

  napi_value runParamsValue;
  c->status = napi_create_external(env, c->runParams, tidyParams, nullptr, &runParamsValue);
  REJECT_STATUS;
  c->status = napi_set_named_property(env, result, "runParams", runParamsValue);
  REJECT_STATUS;

  napi_value runValue;
  c->status = napi_create_function(env, "run", NAPI_AUTO_LENGTH, run,
    nullptr, &runValue);
  REJECT_STATUS;
  c->status = napi_set_named_property(env, result, "run", runValue);
  REJECT_STATUS;

  napi_status status;
  status = napi_resolve_deferred(env, c->_deferred, result);
  FLOATING_STATUS;

  tidyCarrier(env, c);
}

napi_value createProgram(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value promise;
  napi_value resource_name;
  buildCarrier* carrier = new buildCarrier;

  napi_value args[2];
  size_t argc = 2;
  napi_value contextValue;
  status = napi_get_cb_info(env, info, &argc, args, &contextValue, nullptr);
  CHECK_STATUS;

  if (argc < 1 || argc > 2) {
    status = napi_throw_error(env, nullptr, "Wrong number of arguments.");
    return nullptr;
  }

  napi_valuetype t;
  status = napi_typeof(env, args[0], &t);
  if (t != napi_string) {
    status = napi_throw_type_error(env, nullptr, "First argument should be a string - the kernel program.");
    return nullptr;
  }

  napi_value program;
  status = napi_create_object(env, &program);
  CHECK_STATUS;

  status = napi_set_named_property(env, program, "kernelSource", args[0]);
  CHECK_STATUS;

  status = napi_get_value_string_utf8(env, args[0], nullptr, 0, &carrier->sourceLength);
  CHECK_STATUS;
  char* kernelSource = (char*) malloc(carrier->sourceLength + 1);
  status = napi_get_value_string_utf8(env, args[0], kernelSource, carrier->sourceLength + 1, nullptr);
  CHECK_STATUS;
  carrier->kernelSource = std::string(kernelSource);
  delete kernelSource;

  napi_value config = args[1];
  status = napi_typeof(env, config, &t);
  CHECK_STATUS;
  if (t != napi_object) {
    status = napi_throw_type_error(env, nullptr, "Configuration parameters must be an object.");
    return nullptr;
  }

  bool hasProp;
  napi_value nameValue;
  status = napi_has_named_property(env, config, "name", &hasProp);
  CHECK_STATUS;
  if (hasProp) {
    status = napi_get_named_property(env, config, "name", &nameValue);
    CHECK_STATUS;
  } else {
    std::regex re("__kernel\\s+void\\s+([^\\s\\(]+)\\s*\\(");
    std::smatch match;
    std::string parsedName;
    if (std::regex_search(carrier->kernelSource, match, re) && match.size() > 1) {
      parsedName = match.str(1);
    } else {
      parsedName = std::string("noden");
    }
    status = napi_create_string_utf8(env, parsedName.c_str(), parsedName.length(), &nameValue);
    CHECK_STATUS;
  }

  size_t nameLength;
  status = napi_get_value_string_utf8(env, nameValue, nullptr, 0, &nameLength);
  CHECK_STATUS;

  char* kernelName = (char *)malloc(nameLength + 1);
  status = napi_get_value_string_utf8(env, nameValue, kernelName, nameLength + 1, nullptr);
  CHECK_STATUS;
  carrier->kernelName = std::string(kernelName);
  free(kernelName);

  napi_value globalWorkItemsValue;
  status = napi_has_named_property(env, config, "globalWorkItems", &hasProp);
  CHECK_STATUS;
  if (!hasProp) {
    status = napi_throw_type_error(env, nullptr, "globalWorkItems parameter must be provided.");
    return nullptr;
  }
  status = napi_get_named_property(env, config, "globalWorkItems", &globalWorkItemsValue);
  CHECK_STATUS;

  bool hasWIG = false;
  napi_value workItemsPerGroupValue;
  status = napi_has_named_property(env, config, "workItemsPerGroup", &hasWIG);
  CHECK_STATUS;
  if (hasWIG) {
    status = napi_get_named_property(env, config, "workItemsPerGroup", &workItemsPerGroupValue);
    CHECK_STATUS;
  }

  status = napi_typeof(env, globalWorkItemsValue, &t);
  CHECK_STATUS;
  if (napi_number == t) {
    // OpenCL 1 dimension buffer mode
    uint32_t gwi, wig;
    status = napi_get_value_uint32(env, globalWorkItemsValue, &gwi);
    CHECK_STATUS;
    carrier->globalWorkItems.push_back(gwi);
    if (hasWIG) {
      status = napi_get_value_uint32(env, workItemsPerGroupValue, &wig);
      CHECK_STATUS;
      carrier->workItemsPerGroup.push_back(wig);
    }
  } else {
    // OpenCL 2+ dimension image mode
    napi_typedarray_type taType;
    size_t gwiNumDims;
    uint32_t* gwiData;
    napi_value arrbuf;
    size_t byteOffset;
    status = napi_get_typedarray_info(env, globalWorkItemsValue, &taType, &gwiNumDims, (void**)&gwiData, &arrbuf, &byteOffset);
    CHECK_STATUS;
    if (napi_uint32_array != taType) {
      status = napi_throw_type_error(env, nullptr, "globalWorkItems parameter must be a Uint32Array.");
      return nullptr;
    }
    for (size_t i = 0; i < gwiNumDims; ++i)
      carrier->globalWorkItems.push_back(gwiData[i]);

    if (hasWIG) {
      size_t wigNumDims;
      uint32_t* wigData;
      status = napi_get_typedarray_info(env, workItemsPerGroupValue, &taType, &wigNumDims, (void**)&wigData, &arrbuf, &byteOffset);
      CHECK_STATUS;
      if (napi_uint32_array != taType) {
        status = napi_throw_type_error(env, nullptr, "workItemsPerGroup parameter must be a Uint32Array.");
        return nullptr;
      }
      if (gwiNumDims != wigNumDims) {
        status = napi_throw_type_error(env, nullptr, "globalWorkItems and workItemsPerGroup must have the same array dimensions.");
        return nullptr;
      }
      for (size_t i = 0; i < wigNumDims; ++i) {
        if (0 == wigData[i]) { // if any paramater is zero deliver a null vector
          carrier->workItemsPerGroup.clear();
          break;
        }
        carrier->workItemsPerGroup.push_back(wigData[i]);
      }
    }
  }

  napi_value jsContext;
  status = napi_get_named_property(env, contextValue, "context", &jsContext);
  CHECK_STATUS;
  void* contextData;
  status = napi_get_value_external(env, jsContext, &contextData);
  CHECK_STATUS;
  carrier->context = (cl_context) contextData;
  status = napi_set_named_property(env, program, "context", jsContext);
  CHECK_STATUS;

  uint32_t numQueues;
  napi_value numQueuesVal;
  status = napi_get_named_property(env, contextValue, "numQueues", &numQueuesVal);
  CHECK_STATUS;
  status = napi_get_value_uint32(env, numQueuesVal, &numQueues);
  CHECK_STATUS;

  status = napi_set_named_property(env, program, "numQueues", numQueuesVal);
  CHECK_STATUS;
  for (uint32_t i = 0; i < numQueues; ++i) {
    std::stringstream ss;
    ss << "commands_" << i;
    napi_value commandQueue;
    status = napi_get_named_property(env, contextValue, ss.str().c_str(), &commandQueue);
    CHECK_STATUS;
    status = napi_set_named_property(env, program, ss.str().c_str(), commandQueue);
    CHECK_STATUS;
  }

  napi_value platformValue;
  status = napi_get_named_property(env, contextValue, "platformIndex", &platformValue);
  CHECK_STATUS;
  status = napi_get_value_uint32(env, platformValue, &carrier->platformIndex);
  CHECK_STATUS;

  napi_value deviceValue;
  status = napi_get_named_property(env, contextValue, "deviceIndex", &deviceValue);
  CHECK_STATUS;
  status = napi_get_value_uint32(env, deviceValue, &carrier->deviceIndex);
  CHECK_STATUS;

  napi_value deviceIdValue;
  status = napi_get_named_property(env, contextValue, "deviceId", &deviceIdValue);
  CHECK_STATUS;
  void* deviceIdData;
  status = napi_get_value_external(env, deviceIdValue, &deviceIdData);
  CHECK_STATUS;
  carrier->deviceId = (cl_device_id) deviceIdData;

  status = napi_create_reference(env, program, 1, &carrier->passthru);
  CHECK_STATUS;

  napi_ref contextRef;
  status = napi_create_reference(env, contextValue, 1, &contextRef);
  CHECK_STATUS;
  napi_value contextRefValue;
  status = napi_create_external(env, (void*)contextRef, tidyProgramContextRef, nullptr, &contextRefValue);
  CHECK_STATUS;
  status = napi_set_named_property(env, program, "contextRef", contextRefValue);
  CHECK_STATUS;

  status = napi_create_promise(env, &carrier->_deferred, &promise);
  CHECK_STATUS;

  status = napi_create_string_utf8(env, "BuildProgram", NAPI_AUTO_LENGTH, &resource_name);
  CHECK_STATUS;
  status = napi_create_async_work(env, NULL, resource_name, buildExecute,
    buildComplete, carrier, &carrier->_request);
  CHECK_STATUS;
  status = napi_queue_async_work(env, carrier->_request);
  CHECK_STATUS;

  return promise;
}
