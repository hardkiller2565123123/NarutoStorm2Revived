#include "StdInc.h"
#include "SteamProxy.h"
#include "Logger.h"
#include "FakeSteamInterfaces.h"
#include "SteamCoreForwarders.h"
#include "SteamInterfaceRouter.h"

extern "C" void* __cdecl NS2Revived_SteamNetworking();
extern "C" void* __cdecl NS2Revived_SteamGameServerNetworking();
extern "C" void* __cdecl NS2Revived_SteamNetworkingUtils();
extern "C" void* __cdecl NS2Revived_SteamNetworkingMessages();
extern "C" void* __cdecl NS2Revived_SteamNetworkingSockets();
extern "C" void* __cdecl NS2Revived_SteamGameServerNetworkingSockets();

#define RESOLVE_STEAM_EXPORT(name, type) \
    static type fn = nullptr; \
    if (!fn) \
    { \
        fn = reinterpret_cast<type>(SteamProxy::GetExport(#name)); \
        if (fn) Logger::ExportInitialized(#name); \
        else Logger::Error(std::string(#name) + ": export not found"); \
    }

#define FORWARD_VOID(name) \
extern "C" __declspec(dllexport) void __cdecl name() \
{ \
    typedef void(__cdecl* Fn)(); \
    RESOLVE_STEAM_EXPORT(name, Fn); \
    if (fn) fn(); \
}

#define FORWARD_BOOL(name) \
extern "C" __declspec(dllexport) bool __cdecl name() \
{ \
    typedef bool(__cdecl* Fn)(); \
    RESOLVE_STEAM_EXPORT(name, Fn); \
    return fn ? fn() : false; \
}

extern "C" __declspec(dllexport)
bool __cdecl SteamAPI_Init()
{
    typedef bool(__cdecl* Fn)();
    RESOLVE_STEAM_EXPORT(SteamAPI_Init, Fn);
    return fn ? fn() : false;
}

extern "C" __declspec(dllexport)
bool __cdecl SteamAPI_InitSafe()
{
    typedef bool(__cdecl* Fn)();
    RESOLVE_STEAM_EXPORT(SteamAPI_InitSafe, Fn);
    return fn ? fn() : false;
}

extern "C" __declspec(dllexport)
void __cdecl SteamAPI_Shutdown()
{
    typedef void(__cdecl* Fn)();
    RESOLVE_STEAM_EXPORT(SteamAPI_Shutdown, Fn);
    if (fn) fn();
}

FORWARD_VOID(SteamAPI_RunCallbacks)
FORWARD_VOID(SteamAPI_ReleaseCurrentThreadMemory)
FORWARD_VOID(SteamAPI_ManualDispatch_Init)
FORWARD_VOID(SteamAPI_ManualDispatch_RunFrame)
FORWARD_BOOL(SteamAPI_IsSteamRunning)
FORWARD_BOOL(SteamAPI_IsSteamRunningOnSteamDeck)

extern "C" __declspec(dllexport)
bool __cdecl SteamAPI_RestartAppIfNecessary(AppId_t appId)
{
    typedef bool(__cdecl* Fn)(AppId_t);
    RESOLVE_STEAM_EXPORT(SteamAPI_RestartAppIfNecessary, Fn);
    return fn ? fn(appId) : false;
}

extern "C" __declspec(dllexport)
void __cdecl SteamAPI_RegisterCallback(void* callback, int callbackId)
{
    typedef void(__cdecl* Fn)(void*, int);
    RESOLVE_STEAM_EXPORT(SteamAPI_RegisterCallback, Fn);
    if (fn) fn(callback, callbackId);
}

extern "C" __declspec(dllexport)
void __cdecl SteamAPI_UnregisterCallback(void* callback)
{
    typedef void(__cdecl* Fn)(void*);
    RESOLVE_STEAM_EXPORT(SteamAPI_UnregisterCallback, Fn);
    if (fn) fn(callback);
}

extern "C" __declspec(dllexport)
void __cdecl SteamAPI_RegisterCallResult(void* callback, SteamAPICall_t call)
{
    typedef void(__cdecl* Fn)(void*, SteamAPICall_t);
    RESOLVE_STEAM_EXPORT(SteamAPI_RegisterCallResult, Fn);
    if (fn) fn(callback, call);
}

extern "C" __declspec(dllexport)
void __cdecl SteamAPI_UnregisterCallResult(void* callback, SteamAPICall_t call)
{
    typedef void(__cdecl* Fn)(void*, SteamAPICall_t);
    RESOLVE_STEAM_EXPORT(SteamAPI_UnregisterCallResult, Fn);
    if (fn) fn(callback, call);
}

extern "C" __declspec(dllexport)
HSteamUser __cdecl SteamAPI_GetHSteamUser()
{
    typedef HSteamUser(__cdecl* Fn)();
    RESOLVE_STEAM_EXPORT(SteamAPI_GetHSteamUser, Fn);
    return fn ? fn() : 0;
}

extern "C" __declspec(dllexport)
HSteamPipe __cdecl SteamAPI_GetHSteamPipe()
{
    typedef HSteamPipe(__cdecl* Fn)();
    RESOLVE_STEAM_EXPORT(SteamAPI_GetHSteamPipe, Fn);
    return fn ? fn() : 0;
}

extern "C" __declspec(dllexport)
HSteamPipe __cdecl SteamAPI_GetHSteamPipeDeprecated()
{
    typedef HSteamPipe(__cdecl* Fn)();
    RESOLVE_STEAM_EXPORT(SteamAPI_GetHSteamPipeDeprecated, Fn);
    return fn ? fn() : 0;
}

extern "C" __declspec(dllexport)
const char* __cdecl SteamAPI_GetSteamInstallPath()
{
    typedef const char* (__cdecl* Fn)();
    RESOLVE_STEAM_EXPORT(SteamAPI_GetSteamInstallPath, Fn);
    return fn ? fn() : "";
}

extern "C" __declspec(dllexport)
uint64_t __cdecl SteamAPI_GetUserSteamID()
{
    typedef uint64_t(__cdecl* Fn)();
    RESOLVE_STEAM_EXPORT(SteamAPI_GetUserSteamID, Fn);
    return fn ? fn() : 0;
}

extern "C" __declspec(dllexport)
void __cdecl SteamAPI_ManualDispatch_FreeLastCallback(HSteamPipe pipe)
{
    typedef void(__cdecl* Fn)(HSteamPipe);
    RESOLVE_STEAM_EXPORT(SteamAPI_ManualDispatch_FreeLastCallback, Fn);
    if (fn) fn(pipe);
}

extern "C" __declspec(dllexport)
bool __cdecl SteamAPI_ManualDispatch_GetNextCallback(HSteamPipe pipe, void* callbackMsg)
{
    typedef bool(__cdecl* Fn)(HSteamPipe, void*);
    RESOLVE_STEAM_EXPORT(SteamAPI_ManualDispatch_GetNextCallback, Fn);
    return fn ? fn(pipe, callbackMsg) : false;
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
    typedef bool(__cdecl* Fn)(HSteamPipe, SteamAPICall_t, void*, int, int, bool*);
    RESOLVE_STEAM_EXPORT(SteamAPI_ManualDispatch_GetAPICallResult, Fn);
    return fn ? fn(pipe, call, callback, callbackSize, callbackExpected, failed) : false;
}

// Real/core ownership path
extern "C" __declspec(dllexport) void* __cdecl SteamClient() { return SteamCoreForwarders::Client(); }
extern "C" __declspec(dllexport) void* __cdecl SteamUser() { return SteamCoreForwarders::User(); }
extern "C" __declspec(dllexport) void* __cdecl SteamUtils() { return SteamCoreForwarders::Utils(); }
extern "C" __declspec(dllexport) void* __cdecl SteamApps() { return SteamCoreForwarders::Apps(); }

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

extern "C" __declspec(dllexport)
bool __cdecl SteamGameServer_Init(
    uint32_t ip,
    uint16_t steamPort,
    uint16_t gamePort,
    uint16_t queryPort,
    int serverMode,
    const char* versionString)
{
    typedef bool(__cdecl* Fn)(uint32_t, uint16_t, uint16_t, uint16_t, int, const char*);
    RESOLVE_STEAM_EXPORT(SteamGameServer_Init, Fn);
    return fn ? fn(ip, steamPort, gamePort, queryPort, serverMode, versionString) : false;
}

FORWARD_VOID(SteamGameServer_Shutdown)
FORWARD_VOID(SteamGameServer_RunCallbacks)

extern "C" __declspec(dllexport)
void* __cdecl SteamGameServer()
{
    typedef void* (__cdecl* Fn)();
    RESOLVE_STEAM_EXPORT(SteamGameServer, Fn);
    return fn ? fn() : nullptr;
}

extern "C" __declspec(dllexport)
void* __cdecl SteamGameServerStats()
{
    typedef void* (__cdecl* Fn)();
    RESOLVE_STEAM_EXPORT(SteamGameServerStats, Fn);
    return fn ? fn() : nullptr;
}

extern "C" __declspec(dllexport) void* __cdecl SteamGameServerNetworking() { return NS2Revived_SteamGameServerNetworking(); }
extern "C" __declspec(dllexport) void* __cdecl SteamGameServerNetworkingSockets() { return NS2Revived_SteamGameServerNetworkingSockets(); }

extern "C" __declspec(dllexport)
void* __cdecl SteamInternal_CreateInterface(const char* version)
{
    typedef void* (__cdecl* Fn)(const char*);
    RESOLVE_STEAM_EXPORT(SteamInternal_CreateInterface, Fn);

    void* routed = SteamInterfaceRouter::RouteCreateInterface(version);
    if (routed)
        return routed;

    return fn ? fn(version) : nullptr;
}

extern "C" __declspec(dllexport)
void* __cdecl SteamInternal_FindOrCreateUserInterface(HSteamUser user, const char* version)
{
    typedef void* (__cdecl* Fn)(HSteamUser, const char*);
    RESOLVE_STEAM_EXPORT(SteamInternal_FindOrCreateUserInterface, Fn);

    void* routed = SteamInterfaceRouter::RouteUserInterface(user, version);
    if (routed)
        return routed;

    return fn ? fn(user, version) : nullptr;
}

extern "C" __declspec(dllexport)
void* __cdecl SteamInternal_FindOrCreateGameServerInterface(HSteamUser user, const char* version)
{
    typedef void* (__cdecl* Fn)(HSteamUser, const char*);
    RESOLVE_STEAM_EXPORT(SteamInternal_FindOrCreateGameServerInterface, Fn);

    void* routed = SteamInterfaceRouter::RouteGameServerInterface(user, version);
    if (routed)
        return routed;

    return fn ? fn(user, version) : nullptr;
}

extern "C" __declspec(dllexport)
void* __cdecl SteamInternal_ContextInit(void* context)
{
    typedef void* (__cdecl* Fn)(void*);
    RESOLVE_STEAM_EXPORT(SteamInternal_ContextInit, Fn);
    return fn ? fn(context) : nullptr;
}

extern "C" __declspec(dllexport)
void __cdecl SteamInternal_GameServer_Init()
{
    typedef void(__cdecl* Fn)();
    RESOLVE_STEAM_EXPORT(SteamInternal_GameServer_Init, Fn);
    if (fn) fn();
}

extern "C" __declspec(dllexport)
void __cdecl Breakpad_SteamWriteMiniDumpSetComment(const char* comment)
{
    typedef void(__cdecl* Fn)(const char*);
    RESOLVE_STEAM_EXPORT(Breakpad_SteamWriteMiniDumpSetComment, Fn);
    if (fn) fn(comment);
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
    typedef void(__cdecl* Fn)(uint32_t, const char*, const char*, const char*, bool, void*, void*);
    RESOLVE_STEAM_EXPORT(Breakpad_SteamMiniDumpInit, Fn);
    if (fn) fn(appId, version, date, time, fullMemoryDumps, context, callback);
}