#include "StdInc.h"
#include "SteamInterfaceRouter.h"
#include "FakeSteamInterfaces.h"
#include "FakeSteamCore.h"
#include "SteamVersionLogger.h"
#include "Logger.h"

#include <cctype>

extern "C" void* __cdecl NS2Revived_SteamNetworking();
extern "C" void* __cdecl NS2Revived_SteamGameServerNetworking();
extern "C" void* __cdecl NS2Revived_SteamNetworkingUtils();
extern "C" void* __cdecl NS2Revived_SteamNetworkingMessages();
extern "C" void* __cdecl NS2Revived_SteamNetworkingSockets();
extern "C" void* __cdecl NS2Revived_SteamGameServerNetworkingSockets();

static std::string UpperInterfaceName(std::string value)
{
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::toupper(c));
    });
    return value;
}

static bool HasInterface(const std::string& value, const char* token)
{
    return value.find(token) != std::string::npos;
}

static void* RouteCommon(const std::string& v)
{
    std::string u = UpperInterfaceName(v);

    if (HasInterface(u, "STEAMNETWORKINGMESSAGES")) return NS2Revived_SteamNetworkingMessages();
    if (HasInterface(u, "STEAMNETWORKINGSOCKETS")) return NS2Revived_SteamNetworkingSockets();
    if (HasInterface(u, "STEAMNETWORKINGUTILS")) return NS2Revived_SteamNetworkingUtils();
    if (HasInterface(u, "STEAMNETWORKING")) return NS2Revived_SteamNetworking();

    if (HasInterface(u, "STEAMFRIENDS")) return FakeSteamInterfaces::Friends();
    if (HasInterface(u, "STEAMMATCHMAKINGSERVERS")) return FakeSteamInterfaces::MatchmakingServers();
    if (HasInterface(u, "STEAMMATCHMAKING")) return FakeSteamInterfaces::Matchmaking();
    if (HasInterface(u, "STEAMREMOTESTORAGE")) return FakeSteamInterfaces::RemoteStorage();
    if (HasInterface(u, "STEAMSCREENSHOTS")) return FakeSteamInterfaces::Screenshots();
    if (HasInterface(u, "STEAMHTTP")) return FakeSteamInterfaces::HTTP();
    if (HasInterface(u, "STEAMCONTROLLER")) return FakeSteamInterfaces::Controller();
    if (HasInterface(u, "STEAMUGC")) return FakeSteamInterfaces::UGC();
    if (HasInterface(u, "STEAMAPPLIST")) return FakeSteamInterfaces::AppList();
    if (HasInterface(u, "STEAMMUSICREMOTE")) return FakeSteamInterfaces::MusicRemote();
    if (HasInterface(u, "STEAMMUSIC")) return FakeSteamInterfaces::Music();
    if (HasInterface(u, "STEAMHTMLSURFACE")) return FakeSteamInterfaces::HTMLSurface();
    if (HasInterface(u, "STEAMINVENTORY")) return FakeSteamInterfaces::Inventory();
    if (HasInterface(u, "STEAMVIDEO")) return FakeSteamInterfaces::Video();
    if (HasInterface(u, "STEAMPARENTALSETTINGS")) return FakeSteamInterfaces::ParentalSettings();
    if (HasInterface(u, "STEAMGAMESEARCH")) return FakeSteamInterfaces::GameSearch();
    if (HasInterface(u, "STEAMINPUT")) return FakeSteamInterfaces::Input();
    if (HasInterface(u, "STEAMPARTIES")) return FakeSteamInterfaces::Parties();
    if (HasInterface(u, "STEAMREMOTEPLAY")) return FakeSteamInterfaces::RemotePlay();

    if (HasInterface(u, "STEAMCLIENT")) return FakeSteamCore::Client();
    if (HasInterface(u, "STEAMGAMESERVERSTATS")) return FakeSteamCore::GameServerStats();
    if (HasInterface(u, "STEAMGAMESERVER")) return FakeSteamCore::GameServer();
    if (HasInterface(u, "STEAMUSERSTATS")) return FakeSteamInterfaces::UserStats();
    if (HasInterface(u, "STEAMUSER")) return FakeSteamCore::User();
    if (HasInterface(u, "STEAMUTILS")) return FakeSteamCore::Utils();
    if (HasInterface(u, "STEAMAPPS")) return FakeSteamCore::Apps();

    return nullptr;
}

namespace SteamInterfaceRouter
{
    void* RouteCreateInterface(const char* version)
    {
        SteamVersionLogger::LogInterfaceRequest("SteamInternal_CreateInterface", version);

        std::string v = version ? version : "";
        return RouteCommon(v);
    }

    void* RouteUserInterface(HSteamUser user, const char* version)
    {
        NSR_UNUSED(user);

        SteamVersionLogger::LogInterfaceRequest("SteamInternal_FindOrCreateUserInterface", version);

        std::string v = version ? version : "";
        return RouteCommon(v);
    }

    void* RouteGameServerInterface(HSteamUser user, const char* version)
    {
        NSR_UNUSED(user);

        SteamVersionLogger::LogInterfaceRequest("SteamInternal_FindOrCreateGameServerInterface", version);

        std::string v = version ? version : "";

        std::string u = UpperInterfaceName(v);

        if (HasInterface(u, "STEAMNETWORKINGSOCKETS"))
            return NS2Revived_SteamGameServerNetworkingSockets();

        if (HasInterface(u, "STEAMNETWORKING"))
            return NS2Revived_SteamGameServerNetworking();

        return RouteCommon(v);
    }
}
