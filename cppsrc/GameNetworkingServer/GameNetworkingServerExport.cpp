#include "GameNetworkingServerExport.h"

Napi::FunctionReference GameNetworkingServerExport::constructor;

Napi::Object GameNetworkingServerExport::Init(Napi::Env env, Napi::Object exports) {
  Napi::HandleScope scope(env);

  Napi::Function func = DefineClass(env, "GameNetworkingServer", {
    //InstanceMethod("add", &GameNetworkingSocketsExport::Add),
    InstanceMethod("startServer", &GameNetworkingServerExport::StartServer),
    InstanceMethod("stopServer", &GameNetworkingServerExport::StopServer),
    InstanceMethod("sendMessage", &GameNetworkingServerExport::SendMessage),
    InstanceMethod("pollIncomingMessages", &GameNetworkingServerExport::PollIncomingMessages),
    InstanceMethod("runCallbacks", &GameNetworkingServerExport::RunCallbacks),
    StaticMethod("init", &GameNetworkingServerExport::InitGameNetworkingSockets),
    StaticMethod("register", &GameNetworkingServerExport::RegisterGameServer),
  });

  constructor = Napi::Persistent(func);
  constructor.SuppressDestruct();

  exports.Set("GameNetworkingServer", func);

  return exports;
}

Napi::Value GameNetworkingServerExport::InitGameNetworkingSockets(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    Napi::HandleScope scope(env);

    int length = info.Length();

    if (length != 1) {
        Napi::TypeError::New(env, "Only one argument expected").ThrowAsJavaScriptException();
    } 

    Napi::Number port = info[0].As<Napi::Number>();

    try {
        GameNetworkingServerWrapper::InitGameNetworkingSockets((uint16)port.Uint32Value(), "valheim");
    }
    catch (const std::exception& e) {
        Napi::Error::New(env, e.what()).ThrowAsJavaScriptException();
    }

    return Napi::Boolean::New(env, true);
}

Napi::Value GameNetworkingServerExport::RegisterGameServer(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    Napi::HandleScope scope(env);

    int length = info.Length();

    if (length != 3) {
        Napi::TypeError::New(env, "3 arguments expected").ThrowAsJavaScriptException();
    } 

    Napi::String serverName = info[0].As<Napi::String>();
    Napi::Boolean passwordProtected = info[1].As<Napi::Boolean>();
    Napi::String version = info[2].As<Napi::String>();

    try {
        GameNetworkingServerWrapper::RegisterGameServer(serverName.Utf8Value().c_str(), passwordProtected, version.Utf8Value().c_str());
    }
    catch (const std::exception& e) {
        Napi::Error::New(env, e.what()).ThrowAsJavaScriptException();
    }

    return Napi::Boolean::New(env, true);
}

GameNetworkingServerExport::GameNetworkingServerExport(const Napi::CallbackInfo& info) : Napi::ObjectWrap<GameNetworkingServerExport>(info)  {
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

    uint16 nPort = 0;

    if (options.Has("port")) {
        Napi::Value port = options.Get("port");

        if (!port.IsNumber()) {
            Napi::TypeError::New(env, "Port should be a number").ThrowAsJavaScriptException();
        }

        Napi::Number portNumber = port.As<Napi::Number>();
        nPort = (uint16)(portNumber.Uint32Value());
    }

    this->gameNetworkingServerWrapper_ = new GameNetworkingServerWrapper(nPort);
}

Napi::Value GameNetworkingServerExport::StartServer(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    Napi::HandleScope scope(env);
    
    this->gameNetworkingServerWrapper_->StartServer(info);

    return Napi::Boolean::New(env, true);
}

Napi::Value GameNetworkingServerExport::StopServer(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    Napi::HandleScope scope(env);

    this->gameNetworkingServerWrapper_->StopServer();

    return Napi::Boolean::New(env, true);
}

Napi::Value GameNetworkingServerExport::SendMessage(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    Napi::HandleScope scope(env);

    int length = info.Length();

    if (length != 2) {
        Napi::TypeError::New(env, "2 arguments expected").ThrowAsJavaScriptException();
    } 

    Napi::Number handle = info[0].As<Napi::Number>();
    Napi::Buffer<char> buffer = info[1].As< Napi::Buffer<char> >();
    
    try {
        this->gameNetworkingServerWrapper_->SendMessage((HSteamNetConnection)handle.Uint32Value(), buffer.Data(), buffer.Length());
    }
    catch (const std::exception& e) {
        Napi::Error::New(env, e.what()).ThrowAsJavaScriptException();
    }

    return Napi::Boolean::New(env, true);
}

Napi::Value GameNetworkingServerExport::PollIncomingMessages(const Napi::CallbackInfo& info) {
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

    int numMsgs = this->gameNetworkingServerWrapper_->PollIncomingMessages(info, messages);

    return Napi::Number::New(env, numMsgs);
}

Napi::Value GameNetworkingServerExport::RunCallbacks(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    Napi::HandleScope scope(env);

    this->gameNetworkingServerWrapper_->RunCallbacks(info);

    return Napi::Boolean::New(env, true);
}

GameNetworkingServerWrapper* GameNetworkingServerExport::GetInternalInstance() {
  return this->gameNetworkingServerWrapper_;
}
