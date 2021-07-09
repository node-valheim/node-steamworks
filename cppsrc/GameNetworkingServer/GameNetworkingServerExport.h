#include <napi.h>
#include "GameNetworkingServerWrapper.h"

class GameNetworkingServerExport : public Napi::ObjectWrap<GameNetworkingServerExport> {
 public:
  static Napi::Object Init(Napi::Env env, Napi::Object exports);
  GameNetworkingServerExport(const Napi::CallbackInfo& info);
  GameNetworkingServerWrapper *GetInternalInstance();

 private:
  static Napi::FunctionReference constructor;
  static Napi::Value InitGameNetworkingSockets(const Napi::CallbackInfo& info);
  static Napi::Value RegisterGameServer(const Napi::CallbackInfo& info);
  Napi::Value StartServer(const Napi::CallbackInfo& info);
  Napi::Value StopServer(const Napi::CallbackInfo& info);
  Napi::Value SendMessage(const Napi::CallbackInfo& info);
  Napi::Value PollIncomingMessages(const Napi::CallbackInfo& info);
  Napi::Value RunCallbacks(const Napi::CallbackInfo& info);
  GameNetworkingServerWrapper *gameNetworkingServerWrapper_;
};
