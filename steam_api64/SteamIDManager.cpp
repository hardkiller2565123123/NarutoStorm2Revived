#include "StdInc.h"
#include "SteamIDManager.h"
#include "Logger.h"

static CSteamID g_LocalSteamID = 76561198000000001ULL;
static CSteamID g_HostSteamID = 76561198000000001ULL;
static CSteamID g_LobbyOwnerSteamID = 76561198000000001ULL;

namespace SteamIDManager
{
    void Init()
    {
        Logger::Info("SteamIDManager initialized");
    }

    CSteamID GetLocalSteamID()
    {
        return g_LocalSteamID;
    }

    CSteamID GetHostSteamID()
    {
        return g_HostSteamID;
    }

    CSteamID GetLobbyOwnerSteamID()
    {
        return g_LobbyOwnerSteamID;
    }

    void SetLocalSteamID(CSteamID id)
    {
        g_LocalSteamID = id;
        Logger::Info("SteamIDManager local SteamID set: " + std::to_string(ToUint64(id)));
    }

    void SetHostSteamID(CSteamID id)
    {
        g_HostSteamID = id;
        Logger::Info("SteamIDManager host SteamID set: " + std::to_string(ToUint64(id)));
    }

    void SetLobbyOwnerSteamID(CSteamID id)
    {
        g_LobbyOwnerSteamID = id;
        Logger::Info("SteamIDManager lobby owner SteamID set: " + std::to_string(ToUint64(id)));
    }

    uint64_t ToUint64(CSteamID id)
    {
        return static_cast<uint64_t>(id);
    }
}