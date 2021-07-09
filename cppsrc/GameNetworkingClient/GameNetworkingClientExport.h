#include <napi.h>
#include "GameNetworkingClientWrapper.h"

class GameNetworkingClientExport : public Napi::ObjectWrap<GameNetworkingClientExport> {
 public:
  static Napi::Object Init(Napi::Env env, Napi::Object exports);
  GameNetworkingClientExport(const Napi::CallbackInfo& info);
  GameNetworkingClientWrapper *GetInternalInstance();

 private:
  static Napi::FunctionReference constructor;
  static Napi::Value InitSteamAPI(const Napi::CallbackInfo& info);
  static Napi::Value KillSteamAPI(const Napi::CallbackInfo& info);
  static Napi::Value GetAuthSessionTicket(const Napi::CallbackInfo& info);
  Napi::Value StartClient(const Napi::CallbackInfo& info);
  Napi::Value StopClient(const Napi::CallbackInfo& info);
  Napi::Value SendMessage(const Napi::CallbackInfo& info);
  Napi::Value PollIncomingMessages(const Napi::CallbackInfo& info);
  Napi::Value RunCallbacks(const Napi::CallbackInfo& info);
  GameNetworkingClientWrapper *gameNetworkingClientWrapper_;
};
