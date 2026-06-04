#pragma once
#include "StdInc.h"

struct NS2LobbyInfo
{
    CSteamID LobbyID = 1;
    CSteamID OwnerID = 1;
    int MemberLimit = 8;
    bool Joinable = true;
    std::map<std::string, std::string> Data;
    std::vector<CSteamID> Members;
};

namespace SteamSessionManager
{
    void Init();

    CSteamID CreateLobby(int memberLimit);
    bool JoinLobby(CSteamID lobby);
    void LeaveLobby(CSteamID lobby);

    NS2LobbyInfo* GetCurrentLobby();
    NS2LobbyInfo* GetLobby(CSteamID lobby);

    bool SetLobbyData(CSteamID lobby, const char* key, const char* value);
    const char* GetLobbyData(CSteamID lobby, const char* key);

    int GetLobbyMemberCount(CSteamID lobby);
    CSteamID GetLobbyMember(CSteamID lobby, int index);
}