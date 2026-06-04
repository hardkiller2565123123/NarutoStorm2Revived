#include "StdInc.h"
#include "SteamInterfaceRouter.h"
#include "FakeSteamInterfaces.h"
#include "SteamVersionLogger.h"
#include "Logger.h"

extern "C" void* __cdecl NS2Revived_SteamNetworking();
extern "C" void* __cdecl NS2Revived_SteamGameServerNetworking();
extern "C" void* __cdecl NS2Revived_SteamNetworkingUtils();
extern "C" void* __cdecl NS2Revived_SteamNetworkingMessages();
extern "C" void* __cdecl NS2Revived_SteamNetworkingSockets();
extern "C" void* __cdecl NS2Revived_SteamGameServerNetworkingSockets();

static void* RouteCommon(const std::string& v)
{
    if (v.find("SteamFriends") != std::string::npos) return FakeSteamInterfaces::Friends();
    if (v.find("SteamMatchMakingServers") != std::string::npos) return FakeSteamInterfaces::MatchmakingServers();
    if (v.find("SteamMatchMaking") != std::string::npos) return FakeSteamInterfaces::Matchmaking();
    if (v.find("SteamUserStats") != std::string::npos) return FakeSteamInterfaces::UserStats();
    if (v.find("SteamRemoteStorage") != std::string::npos) return FakeSteamInterfaces::RemoteStorage();
    if (v.find("SteamScreenshots") != std::string::npos) return FakeSteamInterfaces::Screenshots();
    if (v.find("STEAMHTTP") != std::string::npos || v.find("SteamHTTP") != std::string::npos) return FakeSteamInterfaces::HTTP();
    if (v.find("SteamController") != std::string::npos) return FakeSteamInterfaces::Controller();
    if (v.find("STEAMUGC") != std::string::npos || v.find("SteamUGC") != std::string::npos) return FakeSteamInterfaces::UGC();
    if (v.find("STEAMAPPLIST") != std::string::npos || v.find("SteamAppList") != std::string::npos) return FakeSteamInterfaces::AppList();
    if (v.find("STEAMMUSICREMOTE") != std::string::npos || v.find("SteamMusicRemote") != std::string::npos) return FakeSteamInterfaces::MusicRemote();
    if (v.find("STEAMMUSIC") != std::string::npos || v.find("SteamMusic") != std::string::npos) return FakeSteamInterfaces::Music();
    if (v.find("STEAMHTMLSURFACE") != std::string::npos || v.find("SteamHTMLSurface") != std::string::npos) return FakeSteamInterfaces::HTMLSurface();
    if (v.find("STEAMINVENTORY") != std::string::npos || v.find("SteamInventory") != std::string::npos) return FakeSteamInterfaces::Inventory();
    if (v.find("STEAMVIDEO") != std::string::npos || v.find("SteamVideo") != std::string::npos) return FakeSteamInterfaces::Video();
    if (v.find("STEAMPARENTALSETTINGS") != std::string::npos || v.find("SteamParentalSettings") != std::string::npos) return FakeSteamInterfaces::ParentalSettings();
    if (v.find("SteamGameSearch") != std::string::npos) return FakeSteamInterfaces::GameSearch();
    if (v.find("SteamInput") != std::string::npos) return FakeSteamInterfaces::Input();
    if (v.find("SteamParties") != std::string::npos) return FakeSteamInterfaces::Parties();
    if (v.find("SteamRemotePlay") != std::string::npos) return FakeSteamInterfaces::RemotePlay();

    if (v.find("SteamNetworkingMessages") != std::string::npos) return NS2Revived_SteamNetworkingMessages();
    if (v.find("SteamNetworkingSockets") != std::string::npos) return NS2Revived_SteamNetworkingSockets();
    if (v.find("SteamNetworkingUtils") != std::string::npos) return NS2Revived_SteamNetworkingUtils();
    if (v.find("SteamNetworking") != std::string::npos) return NS2Revived_SteamNetworking();

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

        if (v.find("SteamNetworkingSockets") != std::string::npos)
            return NS2Revived_SteamGameServerNetworkingSockets();

        if (v.find("SteamNetworking") != std::string::npos)
            return NS2Revived_SteamGameServerNetworking();

        return RouteCommon(v);
    }
}