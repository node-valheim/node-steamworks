#include "GameNetworkingClientExport.h"

Napi::FunctionReference GameNetworkingClientExport::constructor;

Napi::Object GameNetworkingClientExport::Init(Napi::Env env, Napi::Object exports) {
  Napi::HandleScope scope(env);

  Napi::Function func = DefineClass(env, "GameNetworkingClient", {
    //InstanceMethod("add", &GameNetworkingSocketsExport::Add),
    InstanceMethod("startClient", &GameNetworkingClientExport::StartClient),
    InstanceMethod("stopClient", &GameNetworkingClientExport::StopClient),
    InstanceMethod("sendMessage", &GameNetworkingClientExport::SendMessage),
    InstanceMethod("pollIncomingMessages", &GameNetworkingClientExport::PollIncomingMessages),
    InstanceMethod("runCallbacks", &GameNetworkingClientExport::RunCallbacks),
    StaticMethod("init", &GameNetworkingClientExport::InitSteamAPI),
    StaticMethod("kill", &GameNetworkingClientExport::KillSteamAPI),
    StaticMethod("getAuthSessionTicket", &GameNetworkingClientExport::GetAuthSessionTicket),
  });

  constructor = Napi::Persistent(func);
  constructor.SuppressDestruct();

  exports.Set("GameNetworkingClient", func);

  return exports;
}

Napi::Value GameNetworkingClientExport::InitSteamAPI(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    Napi::HandleScope scope(env);

    int length = info.Length();

    if (length != 0) {
        Napi::TypeError::New(env, "Zero arguments expected").ThrowAsJavaScriptException();
    } 

    try {
        GameNetworkingClientWrapper::InitSteamAPI();
    }
    catch (const std::exception& e) {
        Napi::Error::New(env, e.what()).ThrowAsJavaScriptException();
    }

    return Napi::Boolean::New(env, true);
}

Napi::Value GameNetworkingClientExport::KillSteamAPI(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    Napi::HandleScope scope(env);

    int length = info.Length();

    if (length != 0) {
        Napi::TypeError::New(env, "Zero arguments expected").ThrowAsJavaScriptException();
    } 

    try {
        GameNetworkingClientWrapper::KillSteamAPI();
    }
    catch (const std::exception& e) {
        Napi::Error::New(env, e.what()).ThrowAsJavaScriptException();
    }

    return Napi::Boolean::New(env, true);
}

GameNetworkingClientExport::GameNetworkingClientExport(const Napi::CallbackInfo& info) : Napi::ObjectWrap<GameNetworkingClientExport>(info)  {
    Napi::Env env = info.Env();
    Napi::HandleScope scope(env);

    int length = info.Length();

    if (length != 1) {
        Napi::TypeError::New(env, "Only one argument expected").ThrowAsJavaScriptException();
    }

    if (!info[0].IsObject()) {
        Napi::TypeError::New(env, "Expected object as argument").ThrowAsJavaScriptException();
    }

    Napi::Object options = info[0].As<Napi::Object>();

    const char *server = nullptr;

    if (options.Has("server")) {
        Napi::Value serverValue = options.Get("server");

        if (!serverValue.IsString()) {
            Napi::TypeError::New(env, "server should be a string").ThrowAsJavaScriptException();
        }

        Napi::String serverString = serverValue.As<Napi::String>();
        server = serverString.Utf8Value().c_str();
    }

    this->gameNetworkingClientWrapper_ = new GameNetworkingClientWrapper(server);
}

Napi::Value GameNetworkingClientExport::StartClient(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    Napi::HandleScope scope(env);
    
    this->gameNetworkingClientWrapper_->StartClient(info);

    return Napi::Boolean::New(env, true);
}

Napi::Value GameNetworkingClientExport::StopClient(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    Napi::HandleScope scope(env);

    this->gameNetworkingClientWrapper_->StopClient();

    return Napi::Boolean::New(env, true);
}

Napi::Value GameNetworkingClientExport::GetAuthSessionTicket(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    Napi::HandleScope scope(env);

    unsigned char buffer[1024];
    uint32 ticketLength;

    if (!GameNetworkingClientWrapper::GetAuthSessionTicket(buffer, 1024, &ticketLength))
        return Napi::Boolean::New(env, false);

    return Napi::Buffer<unsigned char>::Copy(env, buffer, ticketLength);
}


Napi::Value GameNetworkingClientExport::SendMessage(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    Napi::HandleScope scope(env);

    int length = info.Length();

    if (length != 1) {
        Napi::TypeError::New(env, "Only one argument expected").ThrowAsJavaScriptException();
    } 

    Napi::Buffer<char> buffer = info[0].As< Napi::Buffer<char> >();
    
    try {
        this->gameNetworkingClientWrapper_->SendMessage(buffer.Data(), buffer.Length());
    }
    catch (const std::exception& e) {
        Napi::Error::New(env, e.what()).ThrowAsJavaScriptException();
    }

    return Napi::Boolean::New(env, true);
}

Napi::Value GameNetworkingClientExport::PollIncomingMessages(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    Napi::HandleScope scope(env);

    int messages = 1;

    int length = info.Length();

    if (length > 1) {
        Napi::TypeError::New(env, "Only zero or 1 arguments expected").ThrowAsJavaScriptException();
    }
    
    if (length == 1) {
      Napi::Number messagesCount = info[0].As<Napi::Number>();
      messages = messagesCount.Uint32Value();
    }

    try {
        int numMsgs = this->gameNetworkingClientWrapper_->PollIncomingMessages(info, messages);

        return Napi::Number::New(env, numMsgs);
    }
    catch (std::runtime_error error) {
        return Napi::Number::New(env, 0);
    }

    return Napi::Boolean::New(env, false);
}

Napi::Value GameNetworkingClientExport::RunCallbacks(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    Napi::HandleScope scope(env);

    this->gameNetworkingClientWrapper_->RunCallbacks(info);

    return Napi::Boolean::New(env, true);
}

GameNetworkingClientWrapper *GameNetworkingClientExport::GetInternalInstance() {
  return this->gameNetworkingClientWrapper_;
}
