#include "StdInc.h"
#include "PatchManager.h"
#include "Logger.h"

namespace
{
    PatchManager::PatchState g_State;
}

namespace PatchManager
{
    bool Init()
    {
        Logger::Info("PatchManager initialized");
        return true;
    }

    void Shutdown()
    {
        Logger::Info("PatchManager shutdown");
    }

    PatchState& State()
    {
        return g_State;
    }

    const PatchState& GetState()
    {
        return g_State;
    }

    void ApplySafePatches()
    {
        Logger::Info("PatchManager ApplySafePatches requested");
    }

    void ApplyDisplayPatches()
    {
        Logger::Info("PatchManager ApplyDisplayPatches requested");
    }

    void ApplyGameplayPatches()
    {
        Logger::Info("PatchManager ApplyGameplayPatches requested");
    }
}
