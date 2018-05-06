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

void createBufferExecute(napi_env env, void* data) {
  createBufCarrier* c = (createBufCarrier*) data;
}

void createBufferComplete(napi_env env, napi_status asyncStatus, void* data) {
  createBufCarrier* c = (createBufCarrier*) data;

  delete c;
  napi_delete_async_work(env, c->_request);
}

napi_value createBuffer(napi_env env, napi_callback_info info) {
  napi_status status;
  createBufCarrier* c = new createBufCarrier;

  napi_value thisValue;
  status = napi_get_cb_info(env, info, 0, nullptr, &thisValue, nullptr);

  bool hasProp;
  status = napi_has_named_property(env, thisValue, "name", &hasProp);
  CHECK_STATUS;
  printf("I got called with hasProp %i\n", hasProp);

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
