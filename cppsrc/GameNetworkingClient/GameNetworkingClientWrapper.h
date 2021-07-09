#include <napi.h>

#include <map>
#include <string>

#include "steam/isteamgameserver.h" 
#include "steam/isteamnetworkingsockets.h" 
#include "steam/steamclientpublic.h"
#include "steam/steam_gameserver.h"
#include "steam/steam_api.h"

class GameNetworkingClientWrapper {
    public: 
        GameNetworkingClientWrapper(const char *server);
        static void InitSteamAPI();
        static void KillSteamAPI();
        static bool GetAuthSessionTicket(unsigned char *buffer, int bufferMaxLength, uint32 *pcbTicket);
        void StartClient(const Napi::CallbackInfo& info);
        void StopClient();
        void SendMessage(const void *pData, uint32 cbData);
        void ConnectionStatusChanged(SteamNetConnectionStatusChangedCallback_t *pInfo);
        int PollIncomingMessages(const Napi::CallbackInfo& info, uint32 nbMessagesMax);
        void RunCallbacks(const Napi::CallbackInfo& info);

    private:
        HSteamListenSocket m_hClientSock;
        ISteamNetworkingSockets *m_pInterface;
        SteamNetworkingIPAddr m_ServerAddress;
        bool m_Quit;
        Napi::CallbackInfo *m_currentInfo;
};
