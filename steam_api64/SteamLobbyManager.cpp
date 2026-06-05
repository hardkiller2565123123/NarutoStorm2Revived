#include "StdInc.h"
#include "SteamLobbyManager.h"
#include "Logger.h"

namespace
{
    std::mutex g_Mutex;
    std::map<uint64_t, SteamLobbyManager::NS2Lobby> g_Lobbies;
    uint64_t g_NextLobbyID = 50000;
    uint64_t g_CurrentLobbyID = 0;
    std::string g_ReturnString;

    uint64_t Key(CSteamID id)
    {
        return SteamIDManager::ToUint64(id);
    }

    CSteamID MakeID(uint64_t id)
    {
        return SteamIDManager::FromUint64(id);
    }

    void EnsureLocalMember(SteamLobbyManager::NS2Lobby& lobby)
    {
        CSteamID local = SteamIDManager::GetLocalSteamID();
        uint64_t localKey = Key(local);

        for (const auto& member : lobby.Members)
        {
            if (Key(member) == localKey)
                return;
        }

        lobby.Members.push_back(local);
    }
}

namespace SteamLobbyManager
{
    bool Init()
    {
        std::lock_guard<std::mutex> lock(g_Mutex);
        g_Lobbies.clear();
        g_CurrentLobbyID = 0;
        g_NextLobbyID = 50000;
        Logger::Info("SteamLobbyManager initialized");
        return true;
    }

    void Shutdown()
    {
        std::lock_guard<std::mutex> lock(g_Mutex);
        g_Lobbies.clear();
        g_CurrentLobbyID = 0;
    }

    CSteamID CreateLobby()
    {
        return CreateLobby(4);
    }

    CSteamID CreateLobby(int maxMembers)
    {
        return CreateLobby(0, maxMembers);
    }

    CSteamID CreateLobby(int type, int maxMembers)
    {
        std::lock_guard<std::mutex> lock(g_Mutex);

        uint64_t lobbyID = g_NextLobbyID++;

        NS2Lobby lobby{};
        lobby.LobbyID = MakeID(lobbyID);
        lobby.OwnerID = SteamIDManager::GetLocalSteamID();
        lobby.MaxMembers = maxMembers > 0 ? maxMembers : 4;
        lobby.Joinable = true;
        lobby.PrivateLobby = type != 0;

        lobby.Data["name"] = "NS2 Revived Lobby";
        lobby.Data["mode"] = "offline";
        lobby.Data["server"] = "local";

        EnsureLocalMember(lobby);

        g_Lobbies[lobbyID] = lobby;
        g_CurrentLobbyID = lobbyID;

        Logger::Info("SteamLobbyManager created lobby " + std::to_string(static_cast<unsigned long long>(lobbyID)));

        return lobby.LobbyID;
    }

    bool JoinLobby(CSteamID lobbyID)
    {
        std::lock_guard<std::mutex> lock(g_Mutex);

        uint64_t id = Key(lobbyID);

        if (id == 0)
            id = g_NextLobbyID++;

        auto it = g_Lobbies.find(id);

        if (it == g_Lobbies.end())
        {
            NS2Lobby lobby{};
            lobby.LobbyID = MakeID(id);
            lobby.OwnerID = SteamIDManager::GetLocalSteamID();
            lobby.MaxMembers = 4;
            lobby.Joinable = true;
            lobby.PrivateLobby = false;
            lobby.Data["name"] = "NS2 Revived Lobby";
            lobby.Data["mode"] = "offline";
            lobby.Data["server"] = "local";

            g_Lobbies[id] = lobby;
            it = g_Lobbies.find(id);
        }

        EnsureLocalMember(it->second);
        g_CurrentLobbyID = id;

        Logger::Info("SteamLobbyManager joined lobby " + std::to_string(static_cast<unsigned long long>(id)));
        return true;
    }

    void LeaveLobby(CSteamID lobbyID)
    {
        std::lock_guard<std::mutex> lock(g_Mutex);

        uint64_t id = Key(lobbyID);

        if (id == 0)
            id = g_CurrentLobbyID;

        auto it = g_Lobbies.find(id);

        if (it != g_Lobbies.end())
        {
            uint64_t localKey = SteamIDManager::GetSteamID64();

            auto& members = it->second.Members;

            members.erase(
                std::remove_if(
                    members.begin(),
                    members.end(),
                    [&](CSteamID member)
                    {
                        return Key(member) == localKey;
                    }),
                members.end());
        }

        if (g_CurrentLobbyID == id)
            g_CurrentLobbyID = 0;

        Logger::Info("SteamLobbyManager left lobby " + std::to_string(static_cast<unsigned long long>(id)));
    }

    void LeaveCurrentLobby()
    {
        LeaveLobby(MakeID(g_CurrentLobbyID));
    }

    CSteamID GetCurrentLobby()
    {
        std::lock_guard<std::mutex> lock(g_Mutex);
        return MakeID(g_CurrentLobbyID);
    }

    uint64_t GetCurrentLobbyID()
    {
        std::lock_guard<std::mutex> lock(g_Mutex);
        return g_CurrentLobbyID;
    }

    const NS2Lobby* GetLobby(CSteamID lobbyID)
    {
        auto it = g_Lobbies.find(Key(lobbyID));
        return it == g_Lobbies.end() ? nullptr : &it->second;
    }

    NS2Lobby* GetMutableLobby(CSteamID lobbyID)
    {
        auto it = g_Lobbies.find(Key(lobbyID));
        return it == g_Lobbies.end() ? nullptr : &it->second;
    }

    int GetLobbyCount()
    {
        std::lock_guard<std::mutex> lock(g_Mutex);
        return static_cast<int>(g_Lobbies.size());
    }

    CSteamID GetLobbyByIndex(int index)
    {
        std::lock_guard<std::mutex> lock(g_Mutex);

        if (index < 0 || index >= static_cast<int>(g_Lobbies.size()))
            return 0;

        auto it = g_Lobbies.begin();
        std::advance(it, index);
        return it->second.LobbyID;
    }

    bool SetData(CSteamID lobbyID, const std::string& key, const std::string& value)
    {
        NS2Lobby* lobby = GetMutableLobby(lobbyID);

        if (!lobby)
            return false;

        lobby->Data[key] = value;
        return true;
    }

    bool SetLobbyData(CSteamID lobbyID, const std::string& key, const std::string& value)
    {
        return SetData(lobbyID, key, value);
    }

    const char* GetData(CSteamID lobbyID, const std::string& key)
    {
        const NS2Lobby* lobby = GetLobby(lobbyID);

        if (!lobby)
        {
            g_ReturnString.clear();
            return g_ReturnString.c_str();
        }

        auto it = lobby->Data.find(key);

        if (it == lobby->Data.end())
        {
            g_ReturnString.clear();
            return g_ReturnString.c_str();
        }

        g_ReturnString = it->second;
        return g_ReturnString.c_str();
    }

    const char* GetLobbyData(CSteamID lobbyID, const std::string& key)
    {
        return GetData(lobbyID, key);
    }

    int GetMemberCount(CSteamID lobbyID)
    {
        const NS2Lobby* lobby = GetLobby(lobbyID);
        return lobby ? static_cast<int>(lobby->Members.size()) : 0;
    }

    int GetNumLobbyMembers(CSteamID lobbyID)
    {
        return GetMemberCount(lobbyID);
    }

    CSteamID GetMemberByIndex(CSteamID lobbyID, int index)
    {
        const NS2Lobby* lobby = GetLobby(lobbyID);

        if (!lobby)
            return 0;

        if (index < 0 || index >= static_cast<int>(lobby->Members.size()))
            return 0;

        return lobby->Members[index];
    }

    CSteamID GetLobbyMemberByIndex(CSteamID lobbyID, int index)
    {
        return GetMemberByIndex(lobbyID, index);
    }

    const std::map<uint64_t, NS2Lobby>& GetLobbies()
    {
        return g_Lobbies;
    }
}