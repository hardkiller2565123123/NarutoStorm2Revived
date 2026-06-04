#include "StdInc.h"
#include "SteamSessionManager.h"
#include "SteamIDManager.h"
#include "Logger.h"

static std::mutex g_Mutex;
static std::map<uint64_t, NS2LobbyInfo> g_Lobbies;
static CSteamID g_CurrentLobby = 0;
static uint64_t g_NextLobbyID = 100000;

namespace SteamSessionManager
{
    void Init()
    {
        Logger::Info("SteamSessionManager initialized");
    }

    CSteamID CreateLobby(int memberLimit)
    {
        std::lock_guard<std::mutex> lock(g_Mutex);

        CSteamID lobbyID = g_NextLobbyID++;

        NS2LobbyInfo lobby{};
        lobby.LobbyID = lobbyID;
        lobby.OwnerID = SteamIDManager::GetLocalSteamID();
        lobby.MemberLimit = memberLimit > 0 ? memberLimit : 8;
        lobby.Joinable = true;
        lobby.Members.push_back(SteamIDManager::GetLocalSteamID());

        g_Lobbies[SteamIDManager::ToUint64(lobbyID)] = lobby;
        g_CurrentLobby = lobbyID;

        Logger::Info("SteamSessionManager lobby created: " + std::to_string(SteamIDManager::ToUint64(lobbyID)));

        return lobbyID;
    }

    bool JoinLobby(CSteamID lobby)
    {
        std::lock_guard<std::mutex> lock(g_Mutex);

        uint64_t id = SteamIDManager::ToUint64(lobby);

        if (!g_Lobbies.count(id))
        {
            NS2LobbyInfo info{};
            info.LobbyID = lobby;
            info.OwnerID = lobby;
            info.MemberLimit = 8;
            info.Joinable = true;
            info.Members.push_back(SteamIDManager::GetLocalSteamID());
            g_Lobbies[id] = info;
        }

        g_CurrentLobby = lobby;

        Logger::Info("SteamSessionManager joined lobby: " + std::to_string(id));

        return true;
    }

    void LeaveLobby(CSteamID lobby)
    {
        std::lock_guard<std::mutex> lock(g_Mutex);

        if (SteamIDManager::ToUint64(g_CurrentLobby) == SteamIDManager::ToUint64(lobby))
            g_CurrentLobby = 0;

        Logger::Info("SteamSessionManager left lobby: " + std::to_string(SteamIDManager::ToUint64(lobby)));
    }

    NS2LobbyInfo* GetCurrentLobby()
    {
        return GetLobby(g_CurrentLobby);
    }

    NS2LobbyInfo* GetLobby(CSteamID lobby)
    {
        uint64_t id = SteamIDManager::ToUint64(lobby);

        auto it = g_Lobbies.find(id);

        if (it == g_Lobbies.end())
            return nullptr;

        return &it->second;
    }

    bool SetLobbyData(CSteamID lobby, const char* key, const char* value)
    {
        NS2LobbyInfo* info = GetLobby(lobby);

        if (!info || !key)
            return false;

        info->Data[key] = value ? value : "";

        Logger::Info(std::string("SteamSessionManager lobby data set: ") + key);

        return true;
    }

    const char* GetLobbyData(CSteamID lobby, const char* key)
    {
        static std::string empty;

        NS2LobbyInfo* info = GetLobby(lobby);

        if (!info || !key)
            return empty.c_str();

        auto it = info->Data.find(key);

        if (it == info->Data.end())
            return empty.c_str();

        return it->second.c_str();
    }

    int GetLobbyMemberCount(CSteamID lobby)
    {
        NS2LobbyInfo* info = GetLobby(lobby);

        return info ? static_cast<int>(info->Members.size()) : 0;
    }

    CSteamID GetLobbyMember(CSteamID lobby, int index)
    {
        NS2LobbyInfo* info = GetLobby(lobby);

        if (!info || index < 0 || index >= static_cast<int>(info->Members.size()))
            return 0;

        return info->Members[index];
    }
}