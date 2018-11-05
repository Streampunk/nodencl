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

#include "cl_memory.h"
#include "noden_util.h"

class iGpuReturn {
public:
  virtual ~iGpuReturn() {}
  virtual void onGpuReturn() = 0;
};

class gpuBuffer : public iGpuMemory {
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

class clMemory : public iClMemory, public iGpuReturn {
public:
  clMemory(cl_context context, cl_command_queue commands, eMemFlags memFlags, eSvmType svmType, uint32_t numBytes)
    : mContext(context), mCommands(commands), mMemFlags(memFlags), mSvmType(svmType), mNumBytes(numBytes),
      mPinnedBuf(nullptr), mHostBuf(nullptr), mGpuLocked(false), mHostMapped(false),
      mclMemFlags((eMemFlags::READONLY == mMemFlags) ? CL_MEM_READ_ONLY :
                  (eMemFlags::WRITEONLY == mMemFlags) ? CL_MEM_WRITE_ONLY :
                  CL_MEM_READ_WRITE)
      {}
  ~clMemory() {
    switch (mSvmType) {
    case eSvmType::FINE:
    case eSvmType::COARSE:
      clSVMFree(mContext, mHostBuf);
      break;
    case eSvmType::NONE:
    default:
      if (mPinnedBuf) {
        cl_int error = CL_SUCCESS;
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

  std::shared_ptr<iGpuMemory> getGPUMemory() {
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

  void onGpuReturn() { mGpuLocked = false; }
};

iClMemory *iClMemory::create(cl_context context, cl_command_queue commands, eMemFlags memFlags, eSvmType svmType, uint32_t numBytes) {
  return new clMemory(context, commands, memFlags, svmType, numBytes);
}
