#include "StdInc.h"
#include "SteamLobbyManager.h"
#include "SteamIDManager.h"
#include "Logger.h"

static std::mutex g_Mutex;
static uint64_t g_NextLobbyID = 50000;
static CSteamID g_CurrentLobby = 0;
static std::map<uint64_t, NS2Lobby> g_Lobbies;

namespace SteamLobbyManager
{
    void Init()
    {
        Logger::Info("SteamLobbyManager initialized");
    }

    CSteamID CreateLobby(int maxMembers)
    {
        std::lock_guard<std::mutex> lock(g_Mutex);

        CSteamID id = g_NextLobbyID++;

        NS2Lobby lobby{};
        lobby.ID = id;
        lobby.Owner = SteamIDManager::GetLocalSteamID();
        lobby.MaxMembers = maxMembers > 0 ? maxMembers : 8;
        lobby.Joinable = true;
        lobby.Members.push_back(SteamIDManager::GetLocalSteamID());

        g_Lobbies[static_cast<uint64_t>(id)] = lobby;
        g_CurrentLobby = id;

        Logger::Info("SteamLobbyManager created lobby " + std::to_string(static_cast<uint64_t>(id)));

        return id;
    }

    bool JoinLobby(CSteamID lobbyID)
    {
        std::lock_guard<std::mutex> lock(g_Mutex);

        uint64_t id = static_cast<uint64_t>(lobbyID);

        if (!g_Lobbies.count(id))
        {
            NS2Lobby lobby{};
            lobby.ID = lobbyID;
            lobby.Owner = lobbyID;
            lobby.MaxMembers = 8;
            lobby.Joinable = true;
            g_Lobbies[id] = lobby;
        }

        NS2Lobby& lobby = g_Lobbies[id];

        CSteamID local = SteamIDManager::GetLocalSteamID();

        if (std::find(lobby.Members.begin(), lobby.Members.end(), local) == lobby.Members.end())
            lobby.Members.push_back(local);

        g_CurrentLobby = lobbyID;

        Logger::Info("SteamLobbyManager joined lobby " + std::to_string(id));

        return true;
    }

    void LeaveLobby(CSteamID lobbyID)
    {
        std::lock_guard<std::mutex> lock(g_Mutex);

        if (g_CurrentLobby == lobbyID)
            g_CurrentLobby = 0;

        Logger::Info("SteamLobbyManager left lobby " + std::to_string(static_cast<uint64_t>(lobbyID)));
    }

    NS2Lobby* GetLobby(CSteamID lobbyID)
    {
        auto it = g_Lobbies.find(static_cast<uint64_t>(lobbyID));

        if (it == g_Lobbies.end())
            return nullptr;

        return &it->second;
    }

    CSteamID GetCurrentLobby()
    {
        return g_CurrentLobby;
    }

    bool SetData(CSteamID lobbyID, const char* key, const char* value)
    {
        NS2Lobby* lobby = GetLobby(lobbyID);

        if (!lobby || !key)
            return false;

        lobby->Data[key] = value ? value : "";

        Logger::Info(std::string("SteamLobbyManager set data ") + key);

        return true;
    }

    const char* GetData(CSteamID lobbyID, const char* key)
    {
        static std::string empty;

        NS2Lobby* lobby = GetLobby(lobbyID);

        if (!lobby || !key)
            return empty.c_str();

        auto it = lobby->Data.find(key);

        if (it == lobby->Data.end())
            return empty.c_str();

        return it->second.c_str();
    }

    int GetMemberCount(CSteamID lobbyID)
    {
        NS2Lobby* lobby = GetLobby(lobbyID);
        return lobby ? static_cast<int>(lobby->Members.size()) : 0;
    }

    CSteamID GetMemberByIndex(CSteamID lobbyID, int index)
    {
        NS2Lobby* lobby = GetLobby(lobbyID);

        if (!lobby)
            return 0;

        if (index < 0 || index >= static_cast<int>(lobby->Members.size()))
            return 0;

        return lobby->Members[index];
    }
}