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
#include <chrono>
#include <stdio.h>
#include "node_api.h"
#include "noden_util.h"
#include "noden_info.h"
#include "noden_buffer.h"

enum class eBufLatest : uint8_t { HOST = 0, GPU = 1 };

class iGpuReturn {
public:
  virtual ~iGpuReturn() {}
  virtual void onGpuReturn() = 0;
};

class iHostReturn {
public:
  virtual ~iHostReturn() {}
  virtual void onHostReturn() = 0;
};

class gpuBuffer : public iGpuBuffer {
public:
  gpuBuffer(cl_mem pinnedBuf, void *hostBuf, uint32_t numBytes, cl_command_queue commands, 
            eSvmType svmType, bool hostMapped, iGpuReturn *gpuReturn)
    : mPinnedBuf(pinnedBuf), mHostBuf(hostBuf), mNumBytes(numBytes), mCommands(commands), 
      mSvmType(svmType), mHostMapped(hostMapped), mGpuReturn(gpuReturn) {}
  ~gpuBuffer() {
    mGpuReturn->onGpuReturn();
  }

  cl_int setKernelParam(cl_kernel kernel, uint32_t paramIndex) const {
    // printf("setKernelParam type %d, hostMapped %s, gpuCopy %s, numBytes %d\n", mSvmType, mHostMapped?"true":"false", mNumBytes);
    cl_int error = CL_SUCCESS;
    if (eSvmType::NONE == mSvmType) {
      if (mHostMapped) {
        error = clEnqueueUnmapMemObject(mCommands, mPinnedBuf, mHostBuf, 0, nullptr, nullptr);
        PASS_CL_ERROR;
      }
      error = clSetKernelArg(kernel, paramIndex, sizeof(cl_mem), &mPinnedBuf);
    } else {
      if (mHostMapped) {
        error = clEnqueueSVMUnmap(mCommands, mHostBuf, 0, 0, 0);
        PASS_CL_ERROR;
      }
      error = clSetKernelArgSVMPointer(kernel, paramIndex, mHostBuf);
    }

    return error;
  }

private:
  cl_mem mPinnedBuf;
  void *const mHostBuf;
  const uint32_t mNumBytes;
  cl_command_queue mCommands;
  const eSvmType mSvmType;
  const bool mHostMapped;
  iGpuReturn *mGpuReturn;
};

class nodenBuffer : public iNodenBuffer, public iGpuReturn {
public:
  nodenBuffer(cl_context context, cl_command_queue commands, eMemFlags memFlags, eSvmType svmType, uint32_t numBytes, napi_ref contextRef)
    : mContext(context), mCommands(commands), mMemFlags(memFlags), mSvmType(svmType), mNumBytes(numBytes),
      mPinnedBuf(nullptr), mHostBuf(nullptr), mGpuLocked(false), mHostMapped(false),
      mclMemFlags((eMemFlags::READONLY == mMemFlags) ? CL_MEM_READ_ONLY :
                  (eMemFlags::WRITEONLY == mMemFlags) ? CL_MEM_WRITE_ONLY :
                  CL_MEM_READ_WRITE),
      mContextRef(contextRef)
      {}
  ~nodenBuffer() {
    switch (mSvmType) {
    case eSvmType::FINE:
    case eSvmType::COARSE:
      clSVMFree(mContext, mHostBuf);
      break;
    case eSvmType::NONE:
    default:
      if (mPinnedBuf) {
        cl_int error = CL_SUCCESS;
        // if (eBufLatest::HOST == mBufLatest) {
        if (mHostMapped) {
          error = clEnqueueUnmapMemObject(mCommands, mPinnedBuf, mHostBuf, 0, nullptr, nullptr);
          if (CL_SUCCESS != error)
            printf("OpenCL error in subroutine. Location %s(%d). Error %i: %s\n",
              __FILE__, __LINE__, error, clGetErrorString(error));
        }
        error = clReleaseMemObject(mPinnedBuf);
        if (CL_SUCCESS != error)
          printf("OpenCL error in subroutine. Location %s(%d). Error %i: %s\n",
            __FILE__, __LINE__, error, clGetErrorString(error));
      }
      break;
    }
  }

  bool hostAllocate() {
    cl_int error = CL_SUCCESS;
    cl_svm_mem_flags clMemFlags = mclMemFlags;
    switch (mSvmType) {
    case eSvmType::FINE:
      clMemFlags |= CL_MEM_SVM_FINE_GRAIN_BUFFER;
      // continues...
    case eSvmType::COARSE:
      mHostBuf = clSVMAlloc(mContext, mclMemFlags, mNumBytes, 0);
      break;
    case eSvmType::NONE:
    default:
      mPinnedBuf = clCreateBuffer(mContext, mclMemFlags | CL_MEM_ALLOC_HOST_PTR, mNumBytes, nullptr, &error);
      if (CL_SUCCESS == error)
        mHostBuf = clEnqueueMapBuffer(mCommands, mPinnedBuf, CL_TRUE, CL_MAP_WRITE | CL_MAP_READ, 0, mNumBytes, 0, nullptr, nullptr, nullptr);
      else
        printf("OpenCL error in subroutine. Location %s(%d). Error %i: %s\n",
          __FILE__, __LINE__, error, clGetErrorString(error));
      mHostMapped = true;
      break;
    }

    return nullptr != mHostBuf;
  }

  std::shared_ptr<iGpuBuffer> getGPUBuffer() {
    // printf("getGpuBuffer type %d, host mapped %s, numBytes %d\n", mSvmType, mHostMapped?"true":"false", mNumBytes);
    bool isHostMapped = mHostMapped;
    mHostMapped = false;
    mGpuLocked = true;
    return std::make_shared<gpuBuffer>(mPinnedBuf, mHostBuf, mNumBytes, mCommands, mSvmType, isHostMapped, this);
  }

  void setHostAccess(cl_int &error, eMemFlags haFlags) {
    error = CL_SUCCESS;
    if (mGpuLocked) {
      printf("GPU buffer access must be released before host access - %d\n", mNumBytes);
      error = CL_MAP_FAILURE;
      return;
    }

    if (!mHostMapped) {
      cl_map_flags mapFlags = (eMemFlags::READWRITE == haFlags) ? CL_MAP_WRITE | CL_MAP_READ :
                              (eMemFlags::WRITEONLY == haFlags) ? CL_MAP_WRITE :
                              CL_MAP_READ;
      if (eSvmType::NONE == mSvmType) {
        void *hostBuf = clEnqueueMapBuffer(mCommands, mPinnedBuf, CL_TRUE, mapFlags, 0, mNumBytes, 0, nullptr, nullptr, nullptr);
        if (mHostBuf != hostBuf) {
          printf("Unexpected behaviour - mapped buffer address is not the same: %p != %p\n", mHostBuf, hostBuf);
          error = CL_MAP_FAILURE;
          return;
        }
        mHostMapped = true;
      } else if (eSvmType::COARSE == mSvmType) {
        error = clEnqueueSVMMap(mCommands, CL_TRUE, mapFlags, mHostBuf, mNumBytes, 0, 0, 0);
        mHostMapped = true;
      }
    }
  }

  uint32_t numBytes() const { return mNumBytes; }
  eMemFlags memFlags() const { return mMemFlags; }
  eSvmType svmType() const { return mSvmType; }
  std::string svmTypeName() const {
    switch (mSvmType) {
    case eSvmType::FINE: return "fine";
    case eSvmType::COARSE: return "coarse";
    case eSvmType::NONE: return "none";
    default: return "unknown";
    }
  }
  void* hostBuf() const { return mHostBuf; }
  napi_ref contextRef() const { return mContextRef; }

private:
  cl_context mContext;
  cl_command_queue mCommands;
  const eMemFlags mMemFlags;
  const eSvmType mSvmType;
  const uint32_t mNumBytes;
  cl_mem mPinnedBuf;
  void *mHostBuf;
  bool mGpuLocked;
  bool mHostMapped;
  const cl_svm_mem_flags mclMemFlags;
  napi_ref mContextRef;

  void onGpuReturn() { mGpuLocked = false; }
};

struct createBufCarrier : carrier {
  nodenBuffer *nodenBuf = nullptr;
};

struct hostAccessCarrier : carrier {
  nodenBuffer *nodenBuf = nullptr;
  eMemFlags haFlags = eMemFlags::READWRITE;
  void* srcBuf = nullptr;
  size_t srcBufSize = 0;
};

void hostAccessExecute(napi_env env, void* data) {
  hostAccessCarrier* c = (hostAccessCarrier*) data;
  cl_int error;

  c->nodenBuf->setHostAccess(error, c->haFlags);
  ASYNC_CL_ERROR;

  if (c->srcBuf)
    memcpy(c->nodenBuf->hostBuf(), c->srcBuf, c->srcBufSize);
}

void hostAccessComplete(napi_env env, napi_status asyncStatus, void* data) {
  hostAccessCarrier* c = (hostAccessCarrier*) data;
  if (asyncStatus != napi_ok) {
    c->status = asyncStatus;
    c->errorMsg = "Async buffer creation failed to complete.";
  }
  REJECT_STATUS;

  napi_value result;
  c->status = napi_get_undefined(env, &result);
  REJECT_STATUS;

  napi_status status;
  status = napi_resolve_deferred(env, c->_deferred, result);
  FLOATING_STATUS;

  tidyCarrier(env, c);
}

napi_value hostAccess(napi_env env, napi_callback_info info) {
  napi_status status;
  hostAccessCarrier* c = new hostAccessCarrier;

  napi_value args[2];
  size_t argc = 2;
  napi_value bufferValue;
  status = napi_get_cb_info(env, info, &argc, args, &bufferValue, nullptr);
  CHECK_STATUS;

  if (argc > 2) {
    status = napi_throw_error(env, nullptr, "Wrong number of arguments to hostAccess.");
    delete c;
    return nullptr;
  }

  napi_valuetype t;
  napi_value hostDirValue;
  if (argc > 0) {
    status = napi_typeof(env, args[0], &t);
    CHECK_STATUS;
    if (t != napi_string) {
      status = napi_throw_type_error(env, nullptr, "First argument must be a string.");
      delete c;
      return nullptr;
    }
    hostDirValue = args[0];
  } else {
    status = napi_create_string_utf8(env, "readwrite", 10, &hostDirValue);
    CHECK_STATUS;
  }
  char haflag[10];
  status = napi_get_value_string_utf8(env, hostDirValue, haflag, 10, nullptr);
  CHECK_STATUS;
  if ((strcmp(haflag, "readwrite") != 0) && (strcmp(haflag, "writeonly") != 0) && (strcmp(haflag, "readonly") != 0)) {
    status = napi_throw_error(env, nullptr, "Host access direction must be one of 'readwrite', 'writeonly' or 'readonly'.");
    delete c;
    return nullptr;
  }
  c->haFlags = (0==strcmp("readwrite", haflag)) ? eMemFlags::READWRITE :
               (0==strcmp("writeonly", haflag)) ? eMemFlags::WRITEONLY :
               eMemFlags::READONLY;

  if ((argc == 2) && (c->haFlags == eMemFlags::READONLY)) {
    napi_throw_type_error(env, nullptr, "Optional second argument source buffer provided when access is readonly.");
    delete c;
    return nullptr;
  }

  void* data = nullptr;
  size_t dataSize = 0;
  if (argc == 2) {
    bool isBuffer;
    status = napi_is_buffer(env, args[1], &isBuffer);
    CHECK_STATUS;
    if (!isBuffer) {
      napi_throw_type_error(env, nullptr, "Optional second argument must be a buffer - the source data.");
      delete c;
      return nullptr;
    }

    status = napi_get_buffer_info(env, args[1], &data, &dataSize);
    CHECK_STATUS;
  }
  
  napi_value nodenBufValue;
  status = napi_get_named_property(env, bufferValue, "nodenBuf", &nodenBufValue);
  CHECK_STATUS;
  status = napi_get_value_external(env, nodenBufValue, (void**)&c->nodenBuf);
  CHECK_STATUS;

  if (data) {
    if (dataSize > c->nodenBuf->numBytes()) {
      napi_throw_type_error(env, nullptr, "Source buffer is larger than requested OpenCL allocation.");
      delete c;
      return nullptr;
    } else {
      c->srcBuf = data;
      c->srcBufSize = dataSize;
    }
  }

  napi_value promise, resource_name;
  status = napi_create_promise(env, &c->_deferred, &promise);
  CHECK_STATUS;

  status = napi_create_string_utf8(env, "HostAccess", NAPI_AUTO_LENGTH, &resource_name);
  CHECK_STATUS;
  status = napi_create_async_work(env, NULL, resource_name, hostAccessExecute,
    hostAccessComplete, c, &c->_request);
  CHECK_STATUS;
  status = napi_queue_async_work(env, c->_request);
  CHECK_STATUS;

  return promise;
}

void tidyNodenBuf(napi_env env, void* data, void* hint) {
  nodenBuffer *nodenBuf = (nodenBuffer*)data;
  printf("Finalizing a noden buffer of type %s, size %d.\n", nodenBuf->svmTypeName().c_str(), nodenBuf->numBytes());
  napi_ref contextRef = nodenBuf->contextRef();
  delete nodenBuf;

  napi_status status = napi_delete_reference(env, contextRef);
  checkStatus(env, status, __FILE__, __LINE__ - 1);
}

void createBufferExecute(napi_env env, void* data) {
  createBufCarrier* c = (createBufCarrier*) data;
  printf("Create a buffer of type %s, size %d.\n", c->nodenBuf->svmTypeName().c_str(), c->nodenBuf->numBytes());

  HR_TIME_POINT start = NOW;

  if (!c->nodenBuf->hostAllocate()) {
    c->status = NODEN_ALLOCATION_FAILURE;
    c->errorMsg = "Failed to allocate memory for buffer.";
  }

  c->totalTime = microTime(start);
}

void createBufferComplete(napi_env env, napi_status asyncStatus, void* data) {
  createBufCarrier* c = (createBufCarrier*) data;
  if (asyncStatus != napi_ok) {
    c->status = asyncStatus;
    c->errorMsg = "Async buffer creation failed to complete.";
  }
  REJECT_STATUS;

  napi_value result;
  c->status = napi_create_external_buffer(env, c->nodenBuf->numBytes(), c->nodenBuf->hostBuf(), nullptr, nullptr, &result);
  REJECT_STATUS;

  napi_value nodenBufValue;
  c->status = napi_create_external(env, c->nodenBuf, tidyNodenBuf, nullptr, &nodenBufValue);
  REJECT_STATUS;
  c->status = napi_set_named_property(env, result, "nodenBuf", nodenBufValue);
  REJECT_STATUS;

  napi_value numBytesValue;
  c->status = napi_create_uint32(env, (int32_t) c->nodenBuf->numBytes(), &numBytesValue);
  REJECT_STATUS;
  c->status = napi_set_named_property(env, result, "numBytes", numBytesValue);
  REJECT_STATUS;

  napi_value creationValue;
  c->status = napi_create_int64(env, (int64_t) c->totalTime, &creationValue);
  REJECT_STATUS;
  c->status = napi_set_named_property(env, result, "creationTime", creationValue);
  REJECT_STATUS;

  napi_value hostAccessValue;
  c->status = napi_create_function(env, "hostAccess", NAPI_AUTO_LENGTH,
    hostAccess, nullptr, &hostAccessValue);
  REJECT_STATUS;
  c->status = napi_set_named_property(env, result, "hostAccess", hostAccessValue);
  REJECT_STATUS;

  napi_status status;
  status = napi_resolve_deferred(env, c->_deferred, result);
  FLOATING_STATUS;

  tidyCarrier(env, c);
}

napi_value createBuffer(napi_env env, napi_callback_info info) {
  napi_status status;
  createBufCarrier* c = new createBufCarrier;

  napi_value args[3];
  size_t argc = 3;
  napi_value contextValue;
  status = napi_get_cb_info(env, info, &argc, args, &contextValue, nullptr);
  CHECK_STATUS;

  if (argc < 2 || argc > 3) {
    status = napi_throw_error(env, nullptr, "Wrong number of arguments to create buffer.");
    delete c;
    return nullptr;
  }

  napi_valuetype t;
  status = napi_typeof(env, args[0], &t);
  CHECK_STATUS;
  if (t != napi_number) {
    status = napi_throw_type_error(env, nullptr, "First argument must be a number - buffer size.");
    delete c;
    return nullptr;
  }
  int32_t paramSize;
  status = napi_get_value_int32(env, args[0], &paramSize);
  CHECK_STATUS;
  if (paramSize < 0) {
    status = napi_throw_error(env, nullptr, "Size of the buffer cannot be negative.");
    delete c;
    return nullptr;
  }
  uint32_t numBytes = (uint32_t)paramSize;

  status = napi_typeof(env, args[1], &t);
  CHECK_STATUS;
  if (t != napi_string) {
    status = napi_throw_type_error(env, nullptr, "Second argument must be a string - the buffer direction.");
    delete c;
    return nullptr;
  }

  char memflag[10];
  status = napi_get_value_string_utf8(env, args[1], memflag, 10, nullptr);
  CHECK_STATUS;
  if ((strcmp(memflag, "readwrite") != 0) && (strcmp(memflag, "writeonly") != 0) && (strcmp(memflag, "readonly") != 0)) {
    status = napi_throw_error(env, nullptr, "Buffer direction must be one of 'readwrite', 'writeonly' or 'readonly'.");
    delete c;
    return nullptr;
  }
  eMemFlags memFlags = (0==strcmp("readwrite", memflag)) ? eMemFlags::READWRITE :
                       (0==strcmp("writeonly", memflag)) ? eMemFlags::WRITEONLY :
                       eMemFlags::READONLY;

  napi_value bufTypeValue;
  if (argc == 3) {
    status = napi_typeof(env, args[2], &t);
    CHECK_STATUS;
    if (t != napi_string) {
      status = napi_throw_type_error(env, nullptr, "Third argument must be a string - the buffer type.");
      delete c;
      return nullptr;
    }
    bufTypeValue = args[2];
  } else {
    status = napi_create_string_utf8(env, "none", 10, &bufTypeValue);
    CHECK_STATUS;
  }
  char svmFlag[10];
  status = napi_get_value_string_utf8(env, bufTypeValue, svmFlag, 10, nullptr);
  CHECK_STATUS;

  napi_value svmCapsValue;
  status = napi_get_named_property(env, contextValue, "svmCaps", &svmCapsValue);
  CHECK_STATUS;
  cl_ulong svmCaps;
  status = napi_get_value_int64(env, svmCapsValue, (int64_t*)&svmCaps);
  CHECK_STATUS;

  if ((strcmp(svmFlag, "fine") != 0) &&
    (strcmp(svmFlag, "coarse") != 0) &&
    (strcmp(svmFlag, "none") != 0)) {
    status = napi_throw_error(env, nullptr, "Buffer type must be one of 'fine', 'coarse' or 'none'.");
    delete c;
    return nullptr;
  }
  eSvmType svmType = (0 == strcmp(svmFlag, "fine")) ? eSvmType::FINE :
                     (0 == strcmp(svmFlag, "coarse")) ? eSvmType::COARSE :
                     eSvmType::NONE;

  if (((eSvmType::FINE == svmType) && ((svmCaps & CL_DEVICE_SVM_FINE_GRAIN_BUFFER) == 0)) ||
      ((eSvmType::COARSE == svmType) && ((svmCaps & CL_DEVICE_SVM_COARSE_GRAIN_BUFFER) == 0))) {
    status = napi_throw_error(env, nullptr, "Buffer type requested is not supported by device.");
    delete c;
    return nullptr;
  }

  // Extract externals into variables
  napi_value jsContext;
  void* contextData;
  status = napi_get_named_property(env, contextValue, "context", &jsContext);
  CHECK_STATUS;
  status = napi_get_value_external(env, jsContext, &contextData);
  CHECK_STATUS;
  cl_context context = (cl_context) contextData;

  napi_value jsCommands;
  void* commandsData;
  status = napi_get_named_property(env, contextValue, "commands", &jsCommands);
  CHECK_STATUS;
  status = napi_get_value_external(env, jsCommands, &commandsData);
  CHECK_STATUS;
  cl_command_queue commands = (cl_command_queue) commandsData;

  napi_ref contextRef;
  status = napi_create_reference(env, contextValue, 1, &contextRef);
  CHECK_STATUS;

  // Create holder for host and gpu buffers
  c->nodenBuf = new nodenBuffer(context, commands, memFlags, svmType, numBytes, contextRef);

  status = napi_create_reference(env, contextValue, 1, &c->passthru);
  CHECK_STATUS;

  napi_value promise, resource_name;
  status = napi_create_promise(env, &c->_deferred, &promise);
  CHECK_STATUS;

  status = napi_create_string_utf8(env, "CreateBuffer", NAPI_AUTO_LENGTH, &resource_name);
  CHECK_STATUS;
  status = napi_create_async_work(env, NULL, resource_name, createBufferExecute,
    createBufferComplete, c, &c->_request);
  CHECK_STATUS;
  status = napi_queue_async_work(env, c->_request);
  CHECK_STATUS;

  return promise;
}
