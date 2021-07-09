#include <napi.h>

#include <map>
#include <string>

#include "steam/isteamgameserver.h" 
#include "steam/isteamnetworkingsockets.h" 
#include "steam/steamclientpublic.h"
#include "steam/steam_gameserver.h"
#include "steam/steam_api.h"

struct Client_t {
    std::string name;
};

class GameNetworkingServerWrapper {
    public: 
        GameNetworkingServerWrapper(uint16 port);
        static void InitGameNetworkingSockets(uint16 port, const char *productName);
        static void RegisterGameServer(const char *serverName, bool passwordProtected, const char *version);
        static void KillGameNetworkingSockets();
        void StartServer(const Napi::CallbackInfo& info);
        void StopServer();
        void SendMessage(HSteamNetConnection hConn, const void *pData, uint32 cbData);
        void ConnectionStatusChanged(SteamNetConnectionStatusChangedCallback_t *pInfo);
        int PollIncomingMessages(const Napi::CallbackInfo& info, uint32 nbMessagesMax);
        void RunCallbacks(const Napi::CallbackInfo& info);

    private:
        HSteamListenSocket m_hListenSock;
        HSteamNetPollGroup m_hPollGroup;
        ISteamNetworkingSockets *m_pInterface;
        uint16 m_Port;
        bool m_Quit;
        std::map< HSteamNetConnection, Client_t > m_mapClients;
        Napi::CallbackInfo *m_currentInfo;
};
