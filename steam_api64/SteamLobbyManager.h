#pragma once
#include "StdInc.h"
#include "SteamIDManager.h"

namespace SteamLobbyManager
{
    struct NS2Lobby
    {
        CSteamID LobbyID = 0;
        CSteamID OwnerID = 0;
        int MaxMembers = 4;
        bool Joinable = true;
        bool PrivateLobby = false;
        std::map<std::string, std::string> Data;
        std::vector<CSteamID> Members;
    };

    bool Init();
    void Shutdown();

    CSteamID CreateLobby();
    CSteamID CreateLobby(int maxMembers);
    CSteamID CreateLobby(int type, int maxMembers);

    bool JoinLobby(CSteamID lobbyID);
    void LeaveLobby(CSteamID lobbyID);
    void LeaveCurrentLobby();

    CSteamID GetCurrentLobby();
    uint64_t GetCurrentLobbyID();

    const NS2Lobby* GetLobby(CSteamID lobbyID);
    NS2Lobby* GetMutableLobby(CSteamID lobbyID);

    int GetLobbyCount();
    CSteamID GetLobbyByIndex(int index);

    bool SetData(CSteamID lobbyID, const std::string& key, const std::string& value);
    bool SetLobbyData(CSteamID lobbyID, const std::string& key, const std::string& value);

    const char* GetData(CSteamID lobbyID, const std::string& key);
    const char* GetLobbyData(CSteamID lobbyID, const std::string& key);

    int GetMemberCount(CSteamID lobbyID);
    int GetNumLobbyMembers(CSteamID lobbyID);

    CSteamID GetMemberByIndex(CSteamID lobbyID, int index);
    CSteamID GetLobbyMemberByIndex(CSteamID lobbyID, int index);

    const std::map<uint64_t, NS2Lobby>& GetLobbies();
}