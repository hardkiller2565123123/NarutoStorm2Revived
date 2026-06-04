#pragma once
#include "StdInc.h"

struct NS2Lobby
{
    CSteamID ID = 0;
    CSteamID Owner = 0;
    int MaxMembers = 8;
    bool Joinable = true;
    std::vector<CSteamID> Members;
    std::map<std::string, std::string> Data;
};

namespace SteamLobbyManager
{
    void Init();

    CSteamID CreateLobby(int maxMembers);
    bool JoinLobby(CSteamID lobbyID);
    void LeaveLobby(CSteamID lobbyID);

    NS2Lobby* GetLobby(CSteamID lobbyID);
    CSteamID GetCurrentLobby();

    bool SetData(CSteamID lobbyID, const char* key, const char* value);
    const char* GetData(CSteamID lobbyID, const char* key);

    int GetMemberCount(CSteamID lobbyID);
    CSteamID GetMemberByIndex(CSteamID lobbyID, int index);
}