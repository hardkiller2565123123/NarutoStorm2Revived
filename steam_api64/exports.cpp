#include "StdInc.h"
#include "Logger.h"
#include "FakeSteamInterfaces.h"
#include "FakeSteamCore.h"
#include "SteamAuth.h"
#include "SteamCallbackManager.h"
#include "SteamCallResultManager.h"
#include "SteamIDManager.h"
#include "SteamInterfaceRouter.h"

extern "C" void* __cdecl NS2Revived_SteamNetworking();
extern "C" void* __cdecl NS2Revived_SteamGameServerNetworking();
extern "C" void* __cdecl NS2Revived_SteamNetworkingUtils();
extern "C" void* __cdecl NS2Revived_SteamNetworkingMessages();
extern "C" void* __cdecl NS2Revived_SteamNetworkingSockets();
extern "C" void* __cdecl NS2Revived_SteamGameServerNetworkingSockets();

extern "C" __declspec(dllexport)
bool __cdecl SteamAPI_Init()
{
    Logger::Info("SteamAPI_Init: offline emulation success");
    return FakeSteamCore::Init();
}

extern "C" __declspec(dllexport)
bool __cdecl SteamAPI_InitSafe()
{
    Logger::Info("SteamAPI_InitSafe: offline emulation success");
    return FakeSteamCore::Init();
}

extern "C" __declspec(dllexport)
void __cdecl SteamAPI_Shutdown()
{
    Logger::Info("SteamAPI_Shutdown: offline emulation");
}

extern "C" __declspec(dllexport) void __cdecl SteamAPI_RunCallbacks() { SteamCallbackManager::RunCallbacks(); }
extern "C" __declspec(dllexport) void __cdecl SteamAPI_ReleaseCurrentThreadMemory() {}
extern "C" __declspec(dllexport) void __cdecl SteamAPI_ManualDispatch_Init() {}
extern "C" __declspec(dllexport) void __cdecl SteamAPI_ManualDispatch_RunFrame(HSteamPipe pipe)
{
    NSR_UNUSED(pipe);
    SteamCallbackManager::RunCallbacks();
}
extern "C" __declspec(dllexport) bool __cdecl SteamAPI_IsSteamRunning() { return true; }
extern "C" __declspec(dllexport) bool __cdecl SteamAPI_IsSteamRunningOnSteamDeck() { return false; }

extern "C" __declspec(dllexport)
bool __cdecl SteamAPI_RestartAppIfNecessary(AppId_t appId)
{
    NSR_UNUSED(appId);
    return false;
}

extern "C" __declspec(dllexport)
void __cdecl SteamAPI_RegisterCallback(void* callback, int callbackId)
{
    SteamCallbackManager::RegisterCallback(callback, callbackId);
}

extern "C" __declspec(dllexport)
void __cdecl SteamAPI_UnregisterCallback(void* callback)
{
    SteamCallbackManager::UnregisterCallback(callback);
}

extern "C" __declspec(dllexport)
void __cdecl SteamAPI_RegisterCallResult(void* callback, SteamAPICall_t call)
{
    SteamCallResultManager::RegisterCallResult(callback, call);
}

extern "C" __declspec(dllexport)
void __cdecl SteamAPI_UnregisterCallResult(void* callback, SteamAPICall_t call)
{
    SteamCallResultManager::UnregisterCallResult(callback, call);
}

extern "C" __declspec(dllexport)
HSteamUser __cdecl SteamAPI_GetHSteamUser()
{
    return FakeSteamCore::UserHandle();
}

extern "C" __declspec(dllexport)
HSteamPipe __cdecl SteamAPI_GetHSteamPipe()
{
    return FakeSteamCore::PipeHandle();
}

extern "C" __declspec(dllexport)
HSteamPipe __cdecl SteamAPI_GetHSteamPipeDeprecated()
{
    return FakeSteamCore::PipeHandle();
}

extern "C" __declspec(dllexport)
const char* __cdecl SteamAPI_GetSteamInstallPath()
{
    return FakeSteamCore::InstallPath();
}

extern "C" __declspec(dllexport)
uint64_t __cdecl SteamAPI_GetUserSteamID()
{
    return SteamIDManager::GetSteamID64();
}

extern "C" __declspec(dllexport)
void __cdecl SteamAPI_ManualDispatch_FreeLastCallback(HSteamPipe pipe)
{
    NSR_UNUSED(pipe);
}

extern "C" __declspec(dllexport)
bool __cdecl SteamAPI_ManualDispatch_GetNextCallback(HSteamPipe pipe, void* callbackMsg)
{
    NSR_UNUSED(pipe);
    NSR_UNUSED(callbackMsg);
    return false;
}

extern "C" __declspec(dllexport)
bool __cdecl SteamAPI_ManualDispatch_GetAPICallResult(
    HSteamPipe pipe,
    SteamAPICall_t call,
    void* callback,
    int callbackSize,
    int callbackExpected,
    bool* failed)
{
    NSR_UNUSED(pipe);
    if (failed) *failed = false;
    int actualCallback = 0;
    bool ok = SteamCallResultManager::GetData(call, callback, callbackSize, &actualCallback);
    return ok && (callbackExpected == 0 || actualCallback == callbackExpected);
}

// Offline core ownership path
extern "C" __declspec(dllexport) void* __cdecl SteamClient() { return FakeSteamCore::Client(); }
extern "C" __declspec(dllexport) void* __cdecl SteamUser() { return FakeSteamCore::User(); }
extern "C" __declspec(dllexport) void* __cdecl SteamUtils() { return FakeSteamCore::Utils(); }
extern "C" __declspec(dllexport) void* __cdecl SteamApps() { return FakeSteamCore::Apps(); }

// Fake/emulated interfaces
extern "C" __declspec(dllexport) void* __cdecl SteamFriends() { return FakeSteamInterfaces::Friends(); }
extern "C" __declspec(dllexport) void* __cdecl SteamMatchmaking() { return FakeSteamInterfaces::Matchmaking(); }
extern "C" __declspec(dllexport) void* __cdecl SteamMatchmakingServers() { return FakeSteamInterfaces::MatchmakingServers(); }
extern "C" __declspec(dllexport) void* __cdecl SteamUserStats() { return FakeSteamInterfaces::UserStats(); }
extern "C" __declspec(dllexport) void* __cdecl SteamRemoteStorage() { return FakeSteamInterfaces::RemoteStorage(); }
extern "C" __declspec(dllexport) void* __cdecl SteamScreenshots() { return FakeSteamInterfaces::Screenshots(); }
extern "C" __declspec(dllexport) void* __cdecl SteamHTTP() { return FakeSteamInterfaces::HTTP(); }
extern "C" __declspec(dllexport) void* __cdecl SteamController() { return FakeSteamInterfaces::Controller(); }
extern "C" __declspec(dllexport) void* __cdecl SteamUGC() { return FakeSteamInterfaces::UGC(); }
extern "C" __declspec(dllexport) void* __cdecl SteamAppList() { return FakeSteamInterfaces::AppList(); }
extern "C" __declspec(dllexport) void* __cdecl SteamMusic() { return FakeSteamInterfaces::Music(); }
extern "C" __declspec(dllexport) void* __cdecl SteamMusicRemote() { return FakeSteamInterfaces::MusicRemote(); }
extern "C" __declspec(dllexport) void* __cdecl SteamHTMLSurface() { return FakeSteamInterfaces::HTMLSurface(); }
extern "C" __declspec(dllexport) void* __cdecl SteamInventory() { return FakeSteamInterfaces::Inventory(); }
extern "C" __declspec(dllexport) void* __cdecl SteamVideo() { return FakeSteamInterfaces::Video(); }
extern "C" __declspec(dllexport) void* __cdecl SteamParentalSettings() { return FakeSteamInterfaces::ParentalSettings(); }
extern "C" __declspec(dllexport) void* __cdecl SteamGameSearch() { return FakeSteamInterfaces::GameSearch(); }
extern "C" __declspec(dllexport) void* __cdecl SteamInput() { return FakeSteamInterfaces::Input(); }
extern "C" __declspec(dllexport) void* __cdecl SteamParties() { return FakeSteamInterfaces::Parties(); }
extern "C" __declspec(dllexport) void* __cdecl SteamRemotePlay() { return FakeSteamInterfaces::RemotePlay(); }

// Custom networking
extern "C" __declspec(dllexport) void* __cdecl SteamNetworking() { return NS2Revived_SteamNetworking(); }
extern "C" __declspec(dllexport) void* __cdecl SteamNetworkingMessages() { return NS2Revived_SteamNetworkingMessages(); }
extern "C" __declspec(dllexport) void* __cdecl SteamNetworkingSockets() { return NS2Revived_SteamNetworkingSockets(); }
extern "C" __declspec(dllexport) void* __cdecl SteamNetworkingUtils() { return NS2Revived_SteamNetworkingUtils(); }

#define EXPORT_STEAM_ACCESSOR(name, target) \
extern "C" __declspec(dllexport) void* __cdecl name() { return target; }

EXPORT_STEAM_ACCESSOR(SteamAPI_SteamUser_v023, FakeSteamCore::User())
EXPORT_STEAM_ACCESSOR(SteamAPI_SteamUser_v022, FakeSteamCore::User())
EXPORT_STEAM_ACCESSOR(SteamAPI_SteamUser_v021, FakeSteamCore::User())
EXPORT_STEAM_ACCESSOR(SteamAPI_SteamUser_v020, FakeSteamCore::User())
EXPORT_STEAM_ACCESSOR(SteamAPI_SteamFriends_v017, FakeSteamInterfaces::Friends())
EXPORT_STEAM_ACCESSOR(SteamAPI_SteamUtils_v010, FakeSteamCore::Utils())
EXPORT_STEAM_ACCESSOR(SteamAPI_SteamGameServerUtils_v010, FakeSteamCore::Utils())
EXPORT_STEAM_ACCESSOR(SteamAPI_SteamUtils_v009, FakeSteamCore::Utils())
EXPORT_STEAM_ACCESSOR(SteamAPI_SteamGameServerUtils_v009, FakeSteamCore::Utils())
EXPORT_STEAM_ACCESSOR(SteamAPI_SteamMatchmaking_v009, FakeSteamInterfaces::Matchmaking())
EXPORT_STEAM_ACCESSOR(SteamAPI_SteamMatchmakingServers_v002, FakeSteamInterfaces::MatchmakingServers())
EXPORT_STEAM_ACCESSOR(SteamAPI_SteamGameSearch_v001, FakeSteamInterfaces::GameSearch())
EXPORT_STEAM_ACCESSOR(SteamAPI_SteamParties_v002, FakeSteamInterfaces::Parties())
EXPORT_STEAM_ACCESSOR(SteamAPI_SteamRemoteStorage_v014, FakeSteamInterfaces::RemoteStorage())
EXPORT_STEAM_ACCESSOR(SteamAPI_SteamRemoteStorage_v016, FakeSteamInterfaces::RemoteStorage())
EXPORT_STEAM_ACCESSOR(SteamAPI_SteamUserStats_v012, FakeSteamInterfaces::UserStats())
EXPORT_STEAM_ACCESSOR(SteamAPI_SteamUserStats_v011, FakeSteamInterfaces::UserStats())
EXPORT_STEAM_ACCESSOR(SteamAPI_SteamApps_v008, FakeSteamCore::Apps())
EXPORT_STEAM_ACCESSOR(SteamAPI_SteamGameServerApps_v008, FakeSteamCore::Apps())
EXPORT_STEAM_ACCESSOR(SteamAPI_SteamNetworking_v006, NS2Revived_SteamNetworking())
EXPORT_STEAM_ACCESSOR(SteamAPI_SteamGameServerNetworking_v006, NS2Revived_SteamGameServerNetworking())
EXPORT_STEAM_ACCESSOR(SteamAPI_SteamScreenshots_v003, FakeSteamInterfaces::Screenshots())
EXPORT_STEAM_ACCESSOR(SteamAPI_SteamMusic_v001, FakeSteamInterfaces::Music())
EXPORT_STEAM_ACCESSOR(SteamAPI_SteamMusicRemote_v001, FakeSteamInterfaces::MusicRemote())
EXPORT_STEAM_ACCESSOR(SteamAPI_SteamHTTP_v003, FakeSteamInterfaces::HTTP())
EXPORT_STEAM_ACCESSOR(SteamAPI_SteamGameServerHTTP_v003, FakeSteamInterfaces::HTTP())
EXPORT_STEAM_ACCESSOR(SteamAPI_SteamInput_v001, FakeSteamInterfaces::Input())
EXPORT_STEAM_ACCESSOR(SteamAPI_SteamInput_v002, FakeSteamInterfaces::Input())
EXPORT_STEAM_ACCESSOR(SteamAPI_SteamInput_v005, FakeSteamInterfaces::Input())
EXPORT_STEAM_ACCESSOR(SteamAPI_SteamInput_v006, FakeSteamInterfaces::Input())
EXPORT_STEAM_ACCESSOR(SteamAPI_SteamController_v007, FakeSteamInterfaces::Controller())
EXPORT_STEAM_ACCESSOR(SteamAPI_SteamController_v008, FakeSteamInterfaces::Controller())
EXPORT_STEAM_ACCESSOR(SteamAPI_SteamUGC_v014, FakeSteamInterfaces::UGC())
EXPORT_STEAM_ACCESSOR(SteamAPI_SteamUGC_v015, FakeSteamInterfaces::UGC())
EXPORT_STEAM_ACCESSOR(SteamAPI_SteamUGC_v016, FakeSteamInterfaces::UGC())
EXPORT_STEAM_ACCESSOR(SteamAPI_SteamUGC_v017, FakeSteamInterfaces::UGC())
EXPORT_STEAM_ACCESSOR(SteamAPI_SteamGameServerUGC_v014, FakeSteamInterfaces::UGC())
EXPORT_STEAM_ACCESSOR(SteamAPI_SteamGameServerUGC_v015, FakeSteamInterfaces::UGC())
EXPORT_STEAM_ACCESSOR(SteamAPI_SteamGameServerUGC_v016, FakeSteamInterfaces::UGC())
EXPORT_STEAM_ACCESSOR(SteamAPI_SteamGameServerUGC_v017, FakeSteamInterfaces::UGC())
EXPORT_STEAM_ACCESSOR(SteamAPI_SteamAppList_v001, FakeSteamInterfaces::AppList())
EXPORT_STEAM_ACCESSOR(SteamAPI_SteamHTMLSurface_v005, FakeSteamInterfaces::HTMLSurface())
EXPORT_STEAM_ACCESSOR(SteamAPI_SteamInventory_v003, FakeSteamInterfaces::Inventory())
EXPORT_STEAM_ACCESSOR(SteamAPI_SteamGameServerInventory_v003, FakeSteamInterfaces::Inventory())
EXPORT_STEAM_ACCESSOR(SteamAPI_SteamVideo_v002, FakeSteamInterfaces::Video())
EXPORT_STEAM_ACCESSOR(SteamAPI_SteamTV_v001, nullptr)
EXPORT_STEAM_ACCESSOR(SteamAPI_SteamParentalSettings_v001, FakeSteamInterfaces::ParentalSettings())
EXPORT_STEAM_ACCESSOR(SteamAPI_SteamRemotePlay_v001, FakeSteamInterfaces::RemotePlay())
EXPORT_STEAM_ACCESSOR(SteamAPI_SteamNetworkingMessages_v002, NS2Revived_SteamNetworkingMessages())
EXPORT_STEAM_ACCESSOR(SteamAPI_SteamNetworkingMessages_SteamAPI_v002, NS2Revived_SteamNetworkingMessages())
EXPORT_STEAM_ACCESSOR(SteamAPI_SteamGameServerNetworkingMessages_v002, NS2Revived_SteamNetworkingMessages())
EXPORT_STEAM_ACCESSOR(SteamAPI_SteamGameServerNetworkingMessages_SteamAPI_v002, NS2Revived_SteamNetworkingMessages())
EXPORT_STEAM_ACCESSOR(SteamAPI_SteamNetworkingSockets_SteamAPI_v012, NS2Revived_SteamNetworkingSockets())
EXPORT_STEAM_ACCESSOR(SteamAPI_SteamGameServerNetworkingSockets_SteamAPI_v012, NS2Revived_SteamGameServerNetworkingSockets())
EXPORT_STEAM_ACCESSOR(SteamAPI_SteamNetworkingSockets_SteamAPI_v011, NS2Revived_SteamNetworkingSockets())
EXPORT_STEAM_ACCESSOR(SteamAPI_SteamGameServerNetworkingSockets_SteamAPI_v011, NS2Revived_SteamGameServerNetworkingSockets())
EXPORT_STEAM_ACCESSOR(SteamAPI_SteamNetworkingSockets_SteamAPI_v009, NS2Revived_SteamNetworkingSockets())
EXPORT_STEAM_ACCESSOR(SteamAPI_SteamGameServerNetworkingSockets_SteamAPI_v009, NS2Revived_SteamGameServerNetworkingSockets())
EXPORT_STEAM_ACCESSOR(SteamAPI_SteamNetworkingSockets_v009, NS2Revived_SteamNetworkingSockets())
EXPORT_STEAM_ACCESSOR(SteamAPI_SteamGameServerNetworkingSockets_v009, NS2Revived_SteamGameServerNetworkingSockets())
EXPORT_STEAM_ACCESSOR(SteamAPI_SteamNetworkingSockets_v008, NS2Revived_SteamNetworkingSockets())
EXPORT_STEAM_ACCESSOR(SteamAPI_SteamGameServerNetworkingSockets_v008, NS2Revived_SteamGameServerNetworkingSockets())
EXPORT_STEAM_ACCESSOR(SteamAPI_SteamNetworkingUtils_SteamAPI_v003, NS2Revived_SteamNetworkingUtils())
EXPORT_STEAM_ACCESSOR(SteamAPI_SteamNetworkingUtils_SteamAPI_v004, NS2Revived_SteamNetworkingUtils())
EXPORT_STEAM_ACCESSOR(SteamAPI_SteamNetworkingUtils_v003, NS2Revived_SteamNetworkingUtils())
EXPORT_STEAM_ACCESSOR(SteamAPI_SteamGameServer_v013, FakeSteamCore::GameServer())
EXPORT_STEAM_ACCESSOR(SteamAPI_SteamGameServer_v014, FakeSteamCore::GameServer())
EXPORT_STEAM_ACCESSOR(SteamAPI_SteamGameServer_v015, FakeSteamCore::GameServer())
EXPORT_STEAM_ACCESSOR(SteamAPI_SteamGameServerStats_v001, FakeSteamCore::GameServerStats())

#undef EXPORT_STEAM_ACCESSOR

extern "C" __declspec(dllexport)
bool __cdecl SteamGameServer_Init(
    uint32_t ip,
    uint16_t steamPort,
    uint16_t gamePort,
    uint16_t queryPort,
    int serverMode,
    const char* versionString)
{
    NSR_UNUSED(ip);
    NSR_UNUSED(steamPort);
    NSR_UNUSED(gamePort);
    NSR_UNUSED(queryPort);
    NSR_UNUSED(serverMode);
    NSR_UNUSED(versionString);
    Logger::Info("SteamGameServer_Init: offline emulation success");
    return true;
}

extern "C" __declspec(dllexport) void __cdecl SteamGameServer_Shutdown() {}
extern "C" __declspec(dllexport) void __cdecl SteamGameServer_RunCallbacks() { SteamCallbackManager::RunCallbacks(); }

extern "C" __declspec(dllexport)
void* __cdecl SteamGameServer()
{
    return FakeSteamCore::GameServer();
}

extern "C" __declspec(dllexport)
void* __cdecl SteamGameServerStats()
{
    return FakeSteamCore::GameServerStats();
}

extern "C" __declspec(dllexport) void* __cdecl SteamGameServerNetworking() { return NS2Revived_SteamGameServerNetworking(); }
extern "C" __declspec(dllexport) void* __cdecl SteamGameServerNetworkingSockets() { return NS2Revived_SteamGameServerNetworkingSockets(); }

extern "C" __declspec(dllexport)
void* __cdecl SteamInternal_CreateInterface(const char* version)
{
    void* routed = SteamInterfaceRouter::RouteCreateInterface(version);
    if (routed)
        return routed;

    return nullptr;
}

extern "C" __declspec(dllexport)
void* __cdecl SteamInternal_FindOrCreateUserInterface(HSteamUser user, const char* version)
{
    void* routed = SteamInterfaceRouter::RouteUserInterface(user, version);
    if (routed)
        return routed;

    return nullptr;
}

extern "C" __declspec(dllexport)
void* __cdecl SteamInternal_FindOrCreateGameServerInterface(HSteamUser user, const char* version)
{
    void* routed = SteamInterfaceRouter::RouteGameServerInterface(user, version);
    if (routed)
        return routed;

    return nullptr;
}

extern "C" __declspec(dllexport)
void* __cdecl SteamInternal_ContextInit(void* context)
{
    return context;
}

extern "C" __declspec(dllexport)
void __cdecl SteamInternal_GameServer_Init()
{
    Logger::Info("SteamInternal_GameServer_Init: offline emulation");
}

extern "C" __declspec(dllexport)
void __cdecl Breakpad_SteamWriteMiniDumpSetComment(const char* comment)
{
    NSR_UNUSED(comment);
}

extern "C" __declspec(dllexport)
void __cdecl Breakpad_SteamMiniDumpInit(
    uint32_t appId,
    const char* version,
    const char* date,
    const char* time,
    bool fullMemoryDumps,
    void* context,
    void* callback)
{
    NSR_UNUSED(appId);
    NSR_UNUSED(version);
    NSR_UNUSED(date);
    NSR_UNUSED(time);
    NSR_UNUSED(fullMemoryDumps);
    NSR_UNUSED(context);
    NSR_UNUSED(callback);
}
