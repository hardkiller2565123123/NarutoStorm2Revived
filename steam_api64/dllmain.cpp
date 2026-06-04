#include "StdInc.h"

#include "Logger.h"
#include "SteamProxy.h"
#include "HookManager.h"
#include "NetworkHooks.h"
#include "DX11Overlay.h"
#include "WindowedFullscreen.h"
#include "ModLoader.h"

#include "SteamConfig.h"
#include "SteamIDManager.h"
#include "SteamSessionManager.h"
#include "SteamLobbyManager.h"
#include "SteamP2PManager.h"
#include "SteamAuth.h"
#include "SteamCallbackManager.h"
#include "SteamCallResultManager.h"
#include "SteamFactoryRegistry.h"
#include "SteamPersonaManager.h"
#include "SteamStorageLocal.h"
#include "SteamStatsLocal.h"

static HANDLE g_MainThread = nullptr;

static DWORD WINAPI MainThread(LPVOID)
{
    if (!Logger::Init())
        return 0;

    Logger::Info("NartuoStorm2Revived Steam proxy loaded");

    if (!SteamProxy::Init())
    {
        Logger::Error("Steam proxy failed to initialize");
        return 0;
    }

    if (!HookManager::Init())
    {
        Logger::Error("Hook manager failed to initialize");
        return 0;
    }

    SteamConfig::Init();
    SteamIDManager::Init();
    SteamSessionManager::Init();
    SteamLobbyManager::Init();
    SteamP2PManager::Init();
    SteamAuth::Init();
    SteamCallbackManager::Init();
    SteamCallResultManager::Init();
    SteamPersonaManager::Init();
    SteamStorageLocal::Init();
    SteamStatsLocal::Init();

    SteamFactoryRegistry::Dump();

    if (!NetworkHooks::Init())
    {
        Logger::Error("Network hooks failed to initialize");
    }

    if (!DX11Overlay::Init())
    {
        Logger::Error("DX11 overlay failed to initialize");
    }

    WindowedFullscreen::Init();

    ModLoader::LoadMods();

    Logger::Info("NartuoStorm2Revived initialized");

    return 0;
}

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID)
{
    switch (reason)
    {
    case DLL_PROCESS_ATTACH:
    {
        DisableThreadLibraryCalls(module);

        g_MainThread = CreateThread(
            nullptr,
            0,
            MainThread,
            nullptr,
            0,
            nullptr);

        break;
    }

    case DLL_PROCESS_DETACH:
    {
        NetworkHooks::Shutdown();
        DX11Overlay::Shutdown();
        HookManager::Shutdown();
        Logger::Shutdown();

        break;
    }
    }

    return TRUE;
}