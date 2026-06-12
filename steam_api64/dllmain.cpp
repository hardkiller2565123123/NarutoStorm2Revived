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
#include "FakeSteamCore.h"
#include "AssetModelViewer.h"
#include "AssetPreloadManager.h"
#include "LuaManager.h"
#include "OverlayConsole.h"
#include "LuaHooks.h"
#include "PatchManager.h"
#include "AssetTaskQueue.h"
#include "AssetRelations.h"
#include "AssetNotification.h"
#include "AssetOverrideManager.h"
#include "AssetHexEditor.h"
#include "AssetPackageCreator.h"
#include "AssetFavorites.h"
#include "AssetDuplicateFinder.h"
#include "AssetConflictScanner.h"
#include "AssetBackupManager.h"
#include "AssetBulkExtractor.h"
#include "RuntimeHookBootstrap.h"
#include "NS2Offsets.h"


#include "NS2Config.h"
#include "AssetBrowser.h"
#include "ModRedirector.h"

static HANDLE g_MainThread = nullptr;

static DWORD WINAPI MainThread(LPVOID)
{
    if (!Logger::Init())
        return 0;

    Logger::Info("NartuoStorm2Revived offline Steam layer loaded");

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
    FakeSteamCore::Init();
    SteamPersonaManager::Init();
    SteamStorageLocal::Init();
    SteamStatsLocal::Init();

    NS2Config::Init();

    ModRedirector::Init();
    LuaManager::Init();
    LuaHooks::Init();
    OverlayConsole::Init();
    PatchManager::Init();

    NS2Offsets::LogAll();
    ModRedirector::Scan();

    AssetBrowser::Init();
    AssetRelations::Init();
    AssetNotification::Init();
    AssetOverrideManager::Init();
    AssetHexEditor::Init();
    AssetPackageCreator::Init();
    AssetBrowser::LoadCache();
    AssetBrowser::StartAsyncScan();
    AssetFavorites::Init();
    AssetDuplicateFinder::Init();
    AssetConflictScanner::Init();
    AssetBackupManager::Init();
    AssetBulkExtractor::Init();
    RuntimeHookBootstrap::Init();
  


    AssetPreloadManager::Init();
    AssetTaskQueue::Init();
    AssetPreloadManager::StartFullPreload(false);
    AssetRelations::Rebuild();

    AssetModelViewer::Init();
  

    SteamFactoryRegistry::Dump();

    if (!NetworkHooks::Init())
        Logger::Error("Network hooks failed to initialize");

    if (!DX11Overlay::Init())
        Logger::Error("DX11 overlay failed to initialize");

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
        g_MainThread = CreateThread(nullptr, 0, MainThread, nullptr, 0, nullptr);
        break;
    }
    case DLL_PROCESS_DETACH:
    {

        RuntimeHookBootstrap::Shutdown();
        AssetBulkExtractor::Shutdown();
        AssetBackupManager::Shutdown();
        AssetConflictScanner::Shutdown();
        AssetDuplicateFinder::Shutdown();
        AssetFavorites::Shutdown();

        AssetPackageCreator::Shutdown();
        AssetHexEditor::Shutdown();
        AssetOverrideManager::Shutdown();
        AssetNotification::Shutdown();

        AssetTaskQueue::Shutdown();
        AssetPreloadManager::Shutdown();
        AssetModelViewer::Shutdown();
        AssetRelations::Shutdown();

        NetworkHooks::Shutdown();
        FakeSteamCore::Shutdown();
        DX11Overlay::Shutdown();
        ModRedirector::Shutdown();
        HookManager::Shutdown();
        Logger::Shutdown();
        PatchManager::Shutdown();
        OverlayConsole::Shutdown();
        LuaHooks::Shutdown();
        LuaManager::Shutdown();
        break;
    }
    }
    return TRUE;
}
