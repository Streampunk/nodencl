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

#include "noden_util.h"
#include "noden_info.h"
#include "noden_context.h"
#include "noden_program.h"
#include "node_api.h"

napi_value Init(napi_env env, napi_value exports) {
  napi_status status;
  napi_property_descriptor desc[] = {
    DECLARE_NAPI_METHOD("getPlatformInfo", getPlatformInfo),
    DECLARE_NAPI_METHOD("createContext", createContext),
   };
  status = napi_define_properties(env, exports, 2, desc);
  CHECK_STATUS;

  return exports;
}

NAPI_MODULE(nodencl, Init)
