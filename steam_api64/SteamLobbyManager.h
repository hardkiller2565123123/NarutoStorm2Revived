#pragma once
#include "StdInc.h"
#include "SteamIDManager.h"

namespace SteamLobbyManager
{
    struct NS2Lobby
    {
        CSteamID LobbyID = 0;
        CSteamID OwnerID = 0;
        CSteamID GameServerID = 0;
        uint32_t GameServerIP = 0x7F000001;
        uint16_t GameServerPort = 47584;
        int MaxMembers = 4;
        bool Joinable = true;
        bool PrivateLobby = false;
        std::map<std::string, std::string> Data;
        std::map<uint64_t, std::map<std::string, std::string>> MemberData;
        std::vector<CSteamID> Members;
    };

    struct ChatEntry
    {
        CSteamID User = 0;
        std::vector<char> Data;
        int Type = 1;
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
    bool DeleteData(CSteamID lobbyID, const std::string& key);

    const char* GetData(CSteamID lobbyID, const std::string& key);
    const char* GetLobbyData(CSteamID lobbyID, const std::string& key);
    int GetDataCount(CSteamID lobbyID);
    bool GetDataByIndex(CSteamID lobbyID, int index, std::string& key, std::string& value);

    int GetMemberCount(CSteamID lobbyID);
    int GetNumLobbyMembers(CSteamID lobbyID);

    CSteamID GetMemberByIndex(CSteamID lobbyID, int index);
    CSteamID GetLobbyMemberByIndex(CSteamID lobbyID, int index);

    void SetMemberData(CSteamID lobbyID, CSteamID user, const std::string& key, const std::string& value);
    const char* GetMemberData(CSteamID lobbyID, CSteamID user, const std::string& key);

    bool SetMemberLimit(CSteamID lobbyID, int maxMembers);
    int GetMemberLimit(CSteamID lobbyID);
    bool SetJoinable(CSteamID lobbyID, bool joinable);
    bool SetType(CSteamID lobbyID, int lobbyType);
    CSteamID GetOwner(CSteamID lobbyID);
    bool SetOwner(CSteamID lobbyID, CSteamID owner);

    void SetGameServer(CSteamID lobbyID, uint32_t ip, uint16_t port, CSteamID serverID);
    bool GetGameServer(CSteamID lobbyID, uint32_t* ip, uint16_t* port, CSteamID* serverID);

    int AddChatEntry(CSteamID lobbyID, CSteamID user, const void* data, int size, int type);
    int GetChatEntry(CSteamID lobbyID, int index, CSteamID* user, void* data, int dataSize, int* type);

    const std::map<uint64_t, NS2Lobby>& GetLobbies();
}
