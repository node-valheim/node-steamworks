#include <napi.h>
#include "GameNetworkingServer/GameNetworkingServerExport.h"
#include "GameNetworkingClient/GameNetworkingClientExport.h"

Napi::Object InitAll(Napi::Env env, Napi::Object exports) {
  exports = GameNetworkingServerExport::Init(env, exports);
  return GameNetworkingClientExport::Init(env, exports);
}

NODE_API_MODULE(NODE_GYP_MODULE_NAME, InitAll)
