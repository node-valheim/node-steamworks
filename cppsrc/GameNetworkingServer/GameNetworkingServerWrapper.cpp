#include "GameNetworkingServerWrapper.h"

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

static GameNetworkingServerWrapper *m_Instance;

static void OnConnectionStatusChanged(SteamNetConnectionStatusChangedCallback_t *pInfo) {
    m_Instance->ConnectionStatusChanged(pInfo);
}

GameNetworkingServerWrapper::GameNetworkingServerWrapper(uint16 port) {
    this->m_pInterface = nullptr;
    this->m_Port = port;
    this->m_Quit = true;
}

void GameNetworkingServerWrapper::InitGameNetworkingSockets(uint16 port, const char *productName) {
    if (!SteamGameServer_Init(0, port, port + 1, eServerModeNoAuthentication, "1.0.0.0")) {
        throw std::runtime_error(formatString("SteamGameServer_Init() failed."));
    }

	SteamGameServer()->SetProduct(productName);
	SteamGameServer()->SetModDir(productName);
	SteamGameServer()->SetDedicatedServer(true);
	SteamGameServer()->SetMaxPlayerCount(64);
	SteamGameServer()->LogOnAnonymous();

	int status = SteamGameServerNetworkingSockets()->InitAuthentication();

	while ((status = SteamGameServerNetworkingSockets()->GetAuthenticationStatus(nullptr)) == k_ESteamNetworkingAvailability_NeverTried || (status = SteamGameServerNetworkingSockets()->GetAuthenticationStatus(nullptr)) == k_ESteamNetworkingAvailability_Waiting || (status = SteamGameServerNetworkingSockets()->GetAuthenticationStatus(nullptr)) == k_ESteamNetworkingAvailability_Attempting) {
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
		SteamGameServer_RunCallbacks();
	}

	if (status != k_ESteamNetworkingAvailability_Current)
		throw std::runtime_error(formatString("Authenticate() failed. %d", status));
}

void GameNetworkingServerWrapper::RegisterGameServer(const char *serverName, bool passwordProtected, const char *version) {
	SteamGameServer()->SetServerName(serverName);
	SteamGameServer()->SetMapName(serverName);
	SteamGameServer()->SetPasswordProtected(passwordProtected);
	SteamGameServer()->SetGameTags(version);
	SteamGameServer()->EnableHeartbeats(true);
}

void GameNetworkingServerWrapper::KillGameNetworkingSockets() {
	SteamGameServer_Shutdown();
}

void GameNetworkingServerWrapper::StartServer(const Napi::CallbackInfo& info) {
    if (!this->m_Quit)
        throw std::runtime_error("Server already running");

    this->m_Quit = false;
    
    this->m_pInterface = SteamGameServerNetworkingSockets();
    SteamNetworkingIPAddr serverLocalAddr;
    serverLocalAddr.Clear();
    serverLocalAddr.m_port = this->m_Port;

    SteamNetworkingConfigValue_t opt[2];
    opt[0].SetInt32( k_ESteamNetworkingConfig_IP_AllowWithoutAuth, 1 );
    opt[1].SetPtr( k_ESteamNetworkingConfig_Callback_ConnectionStatusChanged, (void *)OnConnectionStatusChanged);

    this->m_hListenSock = this->m_pInterface->CreateListenSocketIP( serverLocalAddr, 2, opt );

    if ( this->m_hListenSock == k_HSteamListenSocket_Invalid )
        throw std::runtime_error(formatString("Failed to listen on port %u (socket)", this->m_Port));

    this->m_hPollGroup = this->m_pInterface->CreatePollGroup();

    if ( this->m_hPollGroup == k_HSteamNetPollGroup_Invalid )
        throw std::runtime_error(formatString("Failed to listen on port %u (pollgroup)", this->m_Port));
    
    Napi::Env env = info.Env();
    Napi::Function emit = info.This().As<Napi::Object>().Get("emit").As<Napi::Function>();
    emit.Call(info.This(), { Napi::String::New(env, "start") });
}

void GameNetworkingServerWrapper::StopServer() {
    if (this->m_Quit)
        throw std::runtime_error("Server already stopped");

    this->m_Quit = false;
    this->m_pInterface->CloseConnection( this->m_hListenSock, 0, "Goodbye", true );
}

void GameNetworkingServerWrapper::SendMessage(HSteamNetConnection hConn, const void *pData, uint32 cbData) {
    if (this->m_Quit)
        throw std::runtime_error("Server already stopped");

    this->m_pInterface->SendMessageToConnection(hConn, pData, cbData, k_nSteamNetworkingSend_Reliable, nullptr);
}

void GameNetworkingServerWrapper::ConnectionStatusChanged(SteamNetConnectionStatusChangedCallback_t *pInfo) {
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
				auto itClient = m_mapClients.find( pInfo->m_hConn );
				assert( itClient != m_mapClients.end() );

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

				m_mapClients.erase( itClient );
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
		{
			// This must be a new connection
			assert( m_mapClients.find( pInfo->m_hConn ) == m_mapClients.end() );

			// A client is attempting to connect
			// Try to accept the connection.
			if ( m_pInterface->AcceptConnection( pInfo->m_hConn ) != k_EResultOK )
			{
				// This could fail.  If the remote host tried to connect, but then
				// disconnected, the connection may already be half closed.  Just
				// destroy whatever we have on our side.
				m_pInterface->CloseConnection( pInfo->m_hConn, 0, nullptr, false );
				printf( "Can't accept connection.  (It was already closed?)\n" );
				break;
			}

			// Assign the poll group
			if ( !m_pInterface->SetConnectionPollGroup( pInfo->m_hConn, m_hPollGroup ) )
			{
				m_pInterface->CloseConnection( pInfo->m_hConn, 0, nullptr, false );
				printf( "Failed to set poll group?\n" );
				break;
			}

			Napi::Env env = this->m_currentInfo->Env();
			Napi::Function emit = this->m_currentInfo->This().As<Napi::Object>().Get("emit").As<Napi::Function>();

			Napi::Object event = Napi::Object::New(env);
			event.Set("handle", pInfo->m_hConn);
			event.Set("connectionDescription", pInfo->m_info.m_szConnectionDescription);
			event.Set("addrRemoteIPv4", pInfo->m_info.m_addrRemote.GetIPv4());
			event.Set("addrRemotePort", pInfo->m_info.m_addrRemote.m_port);
			event.Set("identityRemoteSteamID64", pInfo->m_info.m_identityRemote.GetSteamID64());
			
			emit.Call(this->m_currentInfo->This(), { Napi::String::New(env, "connected"), event });

			m_mapClients[ pInfo->m_hConn ];
			break;
		}

		case k_ESteamNetworkingConnectionState_Connected:
			break;

		default:
			// Silences -Wswitch
			break;
	}
}

int GameNetworkingServerWrapper::PollIncomingMessages(const Napi::CallbackInfo& info, uint32 nbMessagesMax) {
	ISteamNetworkingMessage *pIncomingMsgs = nullptr;
	
	int numMsgs = m_pInterface->ReceiveMessagesOnPollGroup( this->m_hPollGroup, &pIncomingMsgs, nbMessagesMax );
	if ( numMsgs == 0 )
		return 0;
		
	if ( numMsgs < 0 )
		throw std::runtime_error("Error checking for messages");

	for (int i = 0; i < numMsgs; i++) {
		auto itClient = this->m_mapClients.find( pIncomingMsgs[i].m_conn );
		
		assert( itClient != m_mapClients.end() );

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

void GameNetworkingServerWrapper::RunCallbacks(const Napi::CallbackInfo& info) {
    m_Instance = this;
	this->m_currentInfo = (Napi::CallbackInfo *)&info;
	this->m_pInterface->RunCallbacks();
	SteamGameServer_RunCallbacks();
}