#include "StdInc.h"
#include "RuntimeGameHooks.h"
#include "RuntimeHookAddresses.h"
#include "RuntimeAssetTelemetry.h"
#include "Logger.h"
#include "MinHook.h"

namespace
{
    using GenericGameFn = void(__fastcall*)(void*, void*, void*, void*);

    GenericGameFn oXfbinLoadRequest = nullptr;
    GenericGameFn oXfbinRead = nullptr;
    GenericGameFn oReadCpk = nullptr;
    GenericGameFn oCpkAnalyzer = nullptr;
    GenericGameFn oLoadModel = nullptr;
    GenericGameFn oBindTexture = nullptr;
    GenericGameFn oLoadSound = nullptr;
    GenericGameFn oLuaExecute = nullptr;

    void __fastcall hkXfbinLoadRequest(void* a1, void* a2, void* a3, void* a4){ RuntimeAssetTelemetry::Push(RuntimeAssetTelemetry::EventType::Xfbin, "Game_XfbinLoadRequest", "called"); oXfbinLoadRequest(a1,a2,a3,a4); }
    void __fastcall hkXfbinRead(void* a1, void* a2, void* a3, void* a4){ RuntimeAssetTelemetry::Push(RuntimeAssetTelemetry::EventType::Xfbin, "Game_XfbinRead", "called"); oXfbinRead(a1,a2,a3,a4); }
    void __fastcall hkReadCpk(void* a1, void* a2, void* a3, void* a4){ RuntimeAssetTelemetry::Push(RuntimeAssetTelemetry::EventType::Cpk, "Game_ReadCpk", "called"); oReadCpk(a1,a2,a3,a4); }
    void __fastcall hkCpkAnalyzer(void* a1, void* a2, void* a3, void* a4){ RuntimeAssetTelemetry::Push(RuntimeAssetTelemetry::EventType::Cpk, "Game_CriCpkAnalyzer", "called"); oCpkAnalyzer(a1,a2,a3,a4); }
    void __fastcall hkLoadModel(void* a1, void* a2, void* a3, void* a4){ RuntimeAssetTelemetry::Push(RuntimeAssetTelemetry::EventType::Model, "Game_LoadModel", "called"); oLoadModel(a1,a2,a3,a4); }
    void __fastcall hkBindTexture(void* a1, void* a2, void* a3, void* a4){ RuntimeAssetTelemetry::Push(RuntimeAssetTelemetry::EventType::Texture, "Game_BindTexture", "called"); oBindTexture(a1,a2,a3,a4); }
    void __fastcall hkLoadSound(void* a1, void* a2, void* a3, void* a4){ RuntimeAssetTelemetry::Push(RuntimeAssetTelemetry::EventType::Sound, "Game_LoadSound", "called"); oLoadSound(a1,a2,a3,a4); }
    void __fastcall hkLuaExecute(void* a1, void* a2, void* a3, void* a4){ RuntimeAssetTelemetry::Push(RuntimeAssetTelemetry::EventType::Lua, "Game_LuaExecute", "called"); oLuaExecute(a1,a2,a3,a4); }

    bool HookAddress(const char* name, uintptr_t address, void* detour, void** original)
    {
        if (!address)
        {
            Logger::Info(std::string("RuntimeGameHooks skipped ") + name + " address not set");
            return true;
        }

        if (MH_CreateHook(reinterpret_cast<void*>(address), detour, original) != MH_OK)
        {
            Logger::Error(std::string("RuntimeGameHooks CreateHook failed ") + name);
            return false;
        }

        if (MH_EnableHook(reinterpret_cast<void*>(address)) != MH_OK)
        {
            Logger::Error(std::string("RuntimeGameHooks EnableHook failed ") + name);
            return false;
        }

        Logger::Info(std::string("RuntimeGameHooks hooked ") + name);
        return true;
    }
}

namespace RuntimeGameHooks
{
    bool Init()
    {
        const auto& a = RuntimeHookAddresses::Get();
        bool ok = true;
        ok &= HookAddress("Game_XfbinLoadRequest", a.Game_XfbinLoadRequest, reinterpret_cast<void*>(&hkXfbinLoadRequest), reinterpret_cast<void**>(&oXfbinLoadRequest));
        ok &= HookAddress("Game_XfbinRead", a.Game_XfbinRead, reinterpret_cast<void*>(&hkXfbinRead), reinterpret_cast<void**>(&oXfbinRead));
        ok &= HookAddress("Game_ReadCpk", a.Game_ReadCpk, reinterpret_cast<void*>(&hkReadCpk), reinterpret_cast<void**>(&oReadCpk));
        ok &= HookAddress("Game_CriCpkAnalyzer", a.Game_CriCpkAnalyzer, reinterpret_cast<void*>(&hkCpkAnalyzer), reinterpret_cast<void**>(&oCpkAnalyzer));
        ok &= HookAddress("Game_LoadModel", a.Game_LoadModel, reinterpret_cast<void*>(&hkLoadModel), reinterpret_cast<void**>(&oLoadModel));
        ok &= HookAddress("Game_BindTexture", a.Game_BindTexture, reinterpret_cast<void*>(&hkBindTexture), reinterpret_cast<void**>(&oBindTexture));
        ok &= HookAddress("Game_LoadSound", a.Game_LoadSound, reinterpret_cast<void*>(&hkLoadSound), reinterpret_cast<void**>(&oLoadSound));
        ok &= HookAddress("Game_LuaExecute", a.Game_LuaExecute, reinterpret_cast<void*>(&hkLuaExecute), reinterpret_cast<void**>(&oLuaExecute));
        return ok;
    }

    void Shutdown()
    {
        Logger::Info("RuntimeGameHooks shutdown");
    }
}
