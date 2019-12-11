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

#include "noden_run.h"
#include "cl_memory.h"
#include "sstream"

void runExecute(napi_env env, void* data) {
  runCarrier* c = (runCarrier*) data;
  cl_int error = CL_SUCCESS;
  // HR_TIME_POINT bufAlloc = NOW;
  // Not recording buffer create time - should probably be done once, before here

  for (auto& paramIter: c->kernelParams) {
    kernelParam* param = paramIter.second;
    if (eParamFlags::VALUE != param->valueType)
      param->gpuAccess = param->value.clMem->getGPUMemory();
  }
  
  // printf("Took %lluus to create GPU buffers.\n", microTime(bufAlloc));
  HR_TIME_POINT start = NOW;
  HR_TIME_POINT dataToKernelStart = start;

  for (auto& paramIter: c->kernelParams) {
    uint32_t p = paramIter.first;
    kernelParam* param = paramIter.second;
    if (eParamFlags::VALUE != param->valueType) {
      error = param->gpuAccess->setKernelParam(c->kernel, p, eParamFlags::IMAGE == param->valueType, 
                                               param->access, c->runParams, c->queueNum);
      ASYNC_CL_ERROR;
      param->gpuAccess.reset();
    }
  }

  for (auto& paramIter: c->kernelParams) {
    uint32_t p = paramIter.first;
    kernelParam* param = paramIter.second;
    if (0 == param->paramType.compare("uint"))
      error = clSetKernelArg(c->kernel, p, sizeof(uint32_t), &param->value.uint32);
    else if (0 == param->paramType.compare("int"))
      error = clSetKernelArg(c->kernel, p, sizeof(int32_t), &param->value.int32);
    else if (0 == param->paramType.compare("long"))
      error = clSetKernelArg(c->kernel, p, sizeof(int64_t), &param->value.int64);
    else if (0 == param->paramType.compare("float"))
      error = clSetKernelArg(c->kernel, p, sizeof(float), &param->value.flt);
    else if (0 == param->paramType.compare("double"))
      error = clSetKernelArg(c->kernel, p, sizeof(double), &param->value.dbl);
    ASYNC_CL_ERROR;
  }

  uint32_t q = c->queueNum;
  if (q >= (uint32_t)c->commandQueues.size()) {
    printf("Invalid queue \'%d\', defaulting to 0\n", q);
    q = 0;
  }
  cl_command_queue commandQueue = c->commandQueues.at(q);

  c->dataToKernel = microTime(dataToKernelStart);
  HR_TIME_POINT kernelExecStart = NOW;

  size_t numDims = c->runParams->numDims();
  const size_t *global = c->runParams->globalWorkItems();
  const size_t *local = c->runParams->workItemsPerGroup();
  error = clEnqueueNDRangeKernel(commandQueue, c->kernel, numDims, nullptr, global, local, 0, nullptr, nullptr);
  ASYNC_CL_ERROR;

  if (1 == c->commandQueues.size()) {
    error = clFinish(commandQueue);
    ASYNC_CL_ERROR;
  }

  c->kernelExec = microTime(kernelExecStart);
  HR_TIME_POINT dataFromKernelStart = NOW;

  // set host readonly access for any buffers that are declared writeonly for the kernel
  // for (auto& paramIter: c->kernelParams) {
  //   uint32_t p = paramIter.first;
  //   kernelParam* param = paramIter.second;
  //   if ((eParamFlags::VALUE != param->valueType) && (eMemFlags::WRITEONLY == param->value.clMem->memFlags())) {
  //     param->value.clMem->setHostAccess(error, eMemFlags::READONLY, c->queueNum);
  //     ASYNC_CL_ERROR;
  //   }
  // }

  c->dataFromKernel = microTime(dataFromKernelStart);
  c->totalTime = microTime(start);
}

void runComplete(napi_env env, napi_status asyncStatus, void* data) {
  runCarrier* c = (runCarrier*) data;

  if (asyncStatus != napi_ok) {
    c->status = asyncStatus;
    c->errorMsg = "Async run of program failed to complete.";
  }
  REJECT_STATUS;

  napi_value result;
  c->status = napi_create_object(env, &result);
  REJECT_STATUS;

  napi_value totalValue;
  c->status = napi_create_int64(env, (int64_t) c->totalTime, &totalValue);
  REJECT_STATUS;
  c->status = napi_set_named_property(env, result, "totalTime", totalValue);
  REJECT_STATUS;

  napi_value dataToValue;
  c->status = napi_create_int64(env, (int64_t) c->dataToKernel, &dataToValue);
  REJECT_STATUS;
  c->status = napi_set_named_property(env, result, "dataToKernel", dataToValue);
  REJECT_STATUS;

  napi_value kernelExecValue;
  c->status = napi_create_int64(env, (int64_t) c->kernelExec, &kernelExecValue);
  REJECT_STATUS;
  c->status = napi_set_named_property(env, result, "kernelExec", kernelExecValue);
  REJECT_STATUS;

  napi_value dataFromValue;
  c->status = napi_create_int64(env, (int64_t) c->dataFromKernel, &dataFromValue);
  REJECT_STATUS;
  c->status = napi_set_named_property(env, result, "dataFromKernel", dataFromValue);
  REJECT_STATUS;

  napi_status status;
  status = napi_resolve_deferred(env, c->_deferred, result);
  FLOATING_STATUS;

  for (auto& paramIter: c->kernelParams)
    delete paramIter.second;
  tidyCarrier(env, c);
}

napi_value run(napi_env env, napi_callback_info info) {
  napi_status status;
  runCarrier* c = new runCarrier;

  napi_value args[2];
  size_t argc = 2;
  napi_value programValue;
  status = napi_get_cb_info(env, info, &argc, args, &programValue, nullptr);
  CHECK_STATUS;

  if (!((argc > 0) && (argc <= 2))) {
    status = napi_throw_error(env, nullptr, "Wrong number of arguments. One or two expected.");
    return nullptr;
  }

  napi_valuetype t;
  status = napi_typeof(env, args[0], &t);
  CHECK_STATUS;
  if (t != napi_object) {
    status = napi_throw_type_error(env, nullptr, "Parameter must be an object.");
    return nullptr;
  }

  napi_value runParamsValue;
  status = napi_get_named_property(env, programValue, "runParams", &runParamsValue);
  CHECK_STATUS;
  status = napi_get_value_external(env, runParamsValue, (void**)&c->runParams);
  CHECK_STATUS;

  napi_value runNamesValue;
  status = napi_get_property_names(env, args[0], &runNamesValue);
  CHECK_STATUS;

  uint32_t runNamesCount;
  status = napi_get_array_length(env, runNamesValue, &runNamesCount);
  CHECK_STATUS;

  uint32_t argNamesCount = (uint32_t)c->runParams->kernelArgMap().size();
  if (argNamesCount != runNamesCount) {
    status = napi_throw_error(env, nullptr, "Incorrect number of parameters");
    return nullptr;
  }

  for (uint32_t p=0; p<argNamesCount; ++p) {
    iKernelArg *ka = c->runParams->kernelArgMap().at(p);
    std::string argName(ka->name());
    std::string argType(ka->type());
    iKernelArg::eAccess argAccess(ka->access());

    napi_value argNameValue;
    status = napi_create_string_utf8(env, argName.c_str(), argName.length(), &argNameValue);

    napi_value paramValue;
    status = napi_get_property(env, args[0], argNameValue, &paramValue);
    CHECK_STATUS;
    
    napi_valuetype valueType;
    status = napi_typeof(env, paramValue, &valueType);
    CHECK_STATUS;

    kernelParam* kp = new kernelParam(argName.c_str(), argType.c_str(), argAccess);
    switch (valueType) {
    case napi_undefined:
      printf("Parameter name \'%s\' not found during run\n", argName.c_str());
      status = napi_throw_error(env, nullptr, "Parameter name not found during run");
      delete kp;
      return nullptr;
      break;
    case napi_number:
      if (0 == argType.compare("uint"))
        status = napi_get_value_uint32(env, paramValue, &kp->value.uint32);
      else if (0 == argType.compare("int"))
        status = napi_get_value_int32(env, paramValue, &kp->value.int32);
      else if (0 == argType.compare("long"))
        status = napi_get_value_int64(env, paramValue, &kp->value.int64);
      else if (0 == argType.compare("float")) {
        double tmp = 0.0;
        status = napi_get_value_double(env, paramValue, &tmp);
        kp->value.flt = (float)tmp;
      }
      else if (0 == argType.compare("double"))
        status = napi_get_value_double(env, paramValue, &kp->value.dbl);
      else {
        printf("Unsupported numeric parameter type: \'%s\'\n", argType.c_str());
        status = napi_throw_type_error(env, nullptr, "Unsupported numeric parameter type");
        delete kp;
        return nullptr;
      }
      break;
    case napi_object:
      if (0 == argType.compare("image2d_t")) {
        kp->valueType = eParamFlags::IMAGE;
        kp->paramType = std::string("image");
      } else if (std::string::npos != argType.find('*')) {
        kp->valueType = eParamFlags::BUFFER;
        kp->paramType = std::string("ptr");
      } else {
        printf("Parameter type \'%s\' not recognised as a buffer type\n", argType.c_str());
        status = napi_throw_error(env, nullptr, "Parameter type not recognised during run");
        delete kp;
        return nullptr;
      }
      napi_value clMemValue;
      status = napi_get_named_property(env, paramValue, "clMemory", &clMemValue);
      CHECK_STATUS;
      status = napi_get_value_external(env, clMemValue, (void**)&kp->value.clMem);
      CHECK_STATUS;
      if ((eParamFlags::IMAGE == kp->valueType) && !kp->value.clMem->hasDimensions()) {
        status = napi_throw_error(env, nullptr, "Buffer used as image type must provide image dimensions");
        delete kp;
        return nullptr;
      }
      break;
    default:
      printf("Unsupported parameter value type: \'%d\'\n", valueType);
      status = napi_throw_type_error(env, nullptr, "Unsupported parameter value type");
      delete kp;
      return nullptr;
    }
    
    c->kernelParams.emplace(p, kp);
  }

  uint32_t numQueues;
  napi_value numQueuesVal;
  status = napi_get_named_property(env, programValue, "numQueues", &numQueuesVal);
  CHECK_STATUS;
  status = napi_get_value_uint32(env, numQueuesVal, &numQueues);
  CHECK_STATUS;

  if (argc > 1) {
    status = napi_typeof(env, args[1], &t);
    CHECK_STATUS;
    if (t != napi_number) {
      status = napi_throw_type_error(env, nullptr, "Optional parameter queueNum must be a number.");
      return nullptr;
    }

    int32_t checkValue;
    status = napi_get_value_int32(env, args[1], &checkValue);
    CHECK_STATUS;
    if (!((checkValue >= 0) && (checkValue < (int32_t)numQueues))) {
      status = napi_throw_range_error(env, nullptr, "Optional parameter queueNum out of range.");
      delete c;
      return nullptr;
    }
    status = napi_get_value_uint32(env, args[1], &c->queueNum);
    CHECK_STATUS;
  } else {
    printf("run queueNum parameter not provided - defaulting to 0\n");
    c->queueNum = 0;
  }

  // Extract externals into variables
  napi_value jsContext;
  void* contextData;
  status = napi_get_named_property(env, programValue, "context", &jsContext);
  CHECK_STATUS;
  status = napi_get_value_external(env, jsContext, &contextData);
  c->context = (cl_context) contextData;
  CHECK_STATUS;

  c->commandQueues.resize(numQueues);
  for (uint32_t i = 0; i < numQueues; ++i) {
    std::stringstream ss;
    ss << "commands_" << i;
    napi_value commandQueue;
    status = napi_get_named_property(env, programValue, ss.str().c_str(), &commandQueue);
    CHECK_STATUS;
    c->status = napi_get_value_external(env, commandQueue, (void**)&c->commandQueues.at(i));
    CHECK_STATUS;
  }

  napi_value jsKernel;
  void* kernelData;
  status = napi_get_named_property(env, programValue, "kernel", &jsKernel);
  CHECK_STATUS;
  status = napi_get_value_external(env, jsKernel, &kernelData);
  c->kernel = (cl_kernel) kernelData;
  CHECK_STATUS;

  status = napi_create_reference(env, programValue, 1, &c->passthru);
  CHECK_STATUS;

  napi_value promise, resource_name;
  status = napi_create_promise(env, &c->_deferred, &promise);
  CHECK_STATUS;

  status = napi_create_string_utf8(env, "Run", NAPI_AUTO_LENGTH, &resource_name);
  CHECK_STATUS;
  status = napi_create_async_work(env, NULL, resource_name, runExecute,
    runComplete, c, &c->_request);
  CHECK_STATUS;
  status = napi_queue_async_work(env, c->_request);
  CHECK_STATUS;

  return promise;
}
