#include "GameNetworkingClientWrapper.h"

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>
#include <string>
#include <stdexcept>
#include <cctype>
#include <chrono>
#include <thread>

static std::string formatString( const char *fmt, ... )
{
	char text[ 2048 ];
	va_list ap;
	va_start( ap, fmt );
	vsprintf( text, fmt, ap );
	va_end(ap);
	char *nl = strchr( text, '\0' ) - 1;
	if ( nl >= text && *nl == '\n' )
		*nl = '\0';
	
    return std::string(text);
}

static GameNetworkingClientWrapper*m_Instance;

static void OnConnectionStatusChanged(SteamNetConnectionStatusChangedCallback_t *pInfo) {
    m_Instance->ConnectionStatusChanged(pInfo);
}

GameNetworkingClientWrapper::GameNetworkingClientWrapper(const char *server) {
	SteamNetworkingIPAddr address;
	address.ParseString(server);

    this->m_pInterface = nullptr;
    this->m_ServerAddress = address;
    this->m_Quit = true;
}

void GameNetworkingClientWrapper::InitSteamAPI() { 
    if (!SteamAPI_Init()) {
        throw std::runtime_error("SteamAPI_Init() failed.");
    }
}

void GameNetworkingClientWrapper::KillSteamAPI() {
	SteamAPI_Shutdown();
}

void GameNetworkingClientWrapper::StartClient(const Napi::CallbackInfo& info) {
    if (!this->m_Quit)
        throw std::runtime_error("Client already running");

    this->m_Quit = false;
    
    this->m_pInterface = SteamNetworkingSockets();

    SteamNetworkingConfigValue_t opt[2];
    opt[0].SetInt32( k_ESteamNetworkingConfig_IP_AllowWithoutAuth, 1 );
    opt[1].SetPtr( k_ESteamNetworkingConfig_Callback_ConnectionStatusChanged, (void *)OnConnectionStatusChanged);

    this->m_hClientSock = this->m_pInterface->ConnectByIPAddress( this->m_ServerAddress, 2, opt );

    if ( this->m_hClientSock == k_HSteamListenSocket_Invalid )
        throw std::runtime_error(formatString("Failed to connect to server (socket)"));

    Napi::Env env = info.Env();
    Napi::Function emit = info.This().As<Napi::Object>().Get("emit").As<Napi::Function>();
    emit.Call(info.This(), { Napi::String::New(env, "start") });
}

void GameNetworkingClientWrapper::StopClient() {
    if (this->m_Quit)
        throw std::runtime_error("Client already stopped");

    this->m_Quit = false;
    this->m_pInterface->CloseConnection( this->m_hClientSock, 0, "Goodbye", true );
}

void GameNetworkingClientWrapper::SendMessage(const void *pData, uint32 cbData) {
    if (this->m_Quit)
        throw std::runtime_error("Client already stopped");

    this->m_pInterface->SendMessageToConnection(this->m_hClientSock, pData, cbData, k_nSteamNetworkingSend_Reliable, nullptr);
}

bool GameNetworkingClientWrapper::GetAuthSessionTicket(unsigned char *buffer, int bufferMaxLength, uint32 *pcbTicket) {
	HAuthTicket ticket = SteamUser()->GetAuthSessionTicket(buffer, bufferMaxLength, pcbTicket);

	if (ticket == k_HAuthTicketInvalid)
		return false;

	return true;
}

void GameNetworkingClientWrapper::ConnectionStatusChanged(SteamNetConnectionStatusChangedCallback_t *pInfo) {
    switch ( pInfo->m_info.m_eState )
	{
		case k_ESteamNetworkingConnectionState_None:
			// NOTE: We will get callbacks here when we destroy connections.  You can ignore these.
			break;

		case k_ESteamNetworkingConnectionState_ClosedByPeer:
		case k_ESteamNetworkingConnectionState_ProblemDetectedLocally:
		{
			// Ignore if they were not previously connected.  (If they disconnected
			// before we accepted the connection.)
			if ( pInfo->m_eOldState == k_ESteamNetworkingConnectionState_Connected )
			{

				// Locate the client.  Note that it should have been found, because this
				// is the only codepath where we remove clients (except on shutdown),
				// and connection change callbacks are dispatched in queue order.
				auto itClient = pInfo->m_hConn;
				assert( itClient == this->m_hClientSock );

				// Select appropriate log messages
				const char *pszDebugLogAction;

				if ( pInfo->m_info.m_eState == k_ESteamNetworkingConnectionState_ProblemDetectedLocally )
					pszDebugLogAction = "problem detected locally";
				else
					pszDebugLogAction = "closed by peer";

				Napi::Env env = this->m_currentInfo->Env();
				Napi::Function emit = this->m_currentInfo->This().As<Napi::Object>().Get("emit").As<Napi::Function>();

				Napi::Object event = Napi::Object::New(env);
				event.Set("handle", pInfo->m_hConn);
				event.Set("message", pszDebugLogAction);
				event.Set("connectionDescription", pInfo->m_info.m_szConnectionDescription);
				event.Set("endReason", pInfo->m_info.m_eEndReason);
				event.Set("endDebug", pInfo->m_info.m_szEndDebug);

				emit.Call(this->m_currentInfo->This(), { Napi::String::New(env, "disconnected"), event });

				this->m_hClientSock = 0;
			}
			else
			{
				assert( pInfo->m_eOldState == k_ESteamNetworkingConnectionState_Connecting );
			}

			// Clean up the connection.  This is important!
			// The connection is "closed" in the network sense, but
			// it has not been destroyed.  We must close it on our end, too
			// to finish up.  The reason information do not matter in this case,
			// and we cannot linger because it's already closed on the other end,
			// so we just pass 0's.
			m_pInterface->CloseConnection( pInfo->m_hConn, 0, nullptr, false );
			break;
		}

		case k_ESteamNetworkingConnectionState_Connecting:
			break;

		case k_ESteamNetworkingConnectionState_Connected:
		{
			Napi::Env env = this->m_currentInfo->Env();
			Napi::Function emit = this->m_currentInfo->This().As<Napi::Object>().Get("emit").As<Napi::Function>();

			Napi::Object event = Napi::Object::New(env);
			event.Set("handle", pInfo->m_hConn);
			event.Set("connectionDescription", pInfo->m_info.m_szConnectionDescription);
			event.Set("addrRemoteIPv4", pInfo->m_info.m_addrRemote.GetIPv4());
			event.Set("addrRemotePort", pInfo->m_info.m_addrRemote.m_port);
			event.Set("identityRemoteSteamID64", pInfo->m_info.m_identityRemote.GetSteamID64());
			
			emit.Call(this->m_currentInfo->This(), { Napi::String::New(env, "connected"), event });
			break;
		}

		default:
			// Silences -Wswitch
			break;
	}
}

int GameNetworkingClientWrapper::PollIncomingMessages(const Napi::CallbackInfo& info, uint32 nbMessagesMax) {
	ISteamNetworkingMessage *pIncomingMsgs = nullptr;
	
	int numMsgs = m_pInterface->ReceiveMessagesOnConnection( this->m_hClientSock, &pIncomingMsgs, nbMessagesMax );
	if ( numMsgs == 0 )
		return 0;
		
	if ( numMsgs < 0 )
		throw std::runtime_error("Error checking for messages");

	for (int i = 0; i < numMsgs; i++) {
		auto itClient = pIncomingMsgs[i].m_conn;
		
		assert( itClient == this->m_hClientSock );

		// (const char *)pIncomingMsg->m_pData
		// pIncomingMsg->m_cbSize
		// copy to new buffer, emit

		Napi::Env env = info.Env();
		Napi::Function emit = info.This().As<Napi::Object>().Get("emit").As<Napi::Function>();

		Napi::Object event = Napi::Object::New(env);
		event.Set("handle", pIncomingMsgs[i].m_conn);
		event.Set("identityPeerSteamID64", pIncomingMsgs[i].m_identityPeer.GetSteamID64());
		event.Set("data", Napi::Buffer<unsigned char>::Copy(env, (unsigned char *)pIncomingMsgs[i].m_pData, pIncomingMsgs[i].m_cbSize));
		emit.Call(info.This(), { Napi::String::New(env, "message"), event });

		pIncomingMsgs[i].Release();
	}
	
	return numMsgs;
}

void GameNetworkingClientWrapper::RunCallbacks(const Napi::CallbackInfo& info) {
    m_Instance = this;
	this->m_currentInfo = (Napi::CallbackInfo *)&info;
	this->m_pInterface->RunCallbacks();
	SteamAPI_RunCallbacks();
}