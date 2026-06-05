#pragma once
#include "StdInc.h"

namespace PatchManager
{
    struct PatchState
    {
        bool BorderlessFullscreen = true;
        bool NativeUltrawide = false;
        bool MultiMonitor = false;
        bool DisableIntro = false;
        bool AutoSkipCutscenes = false;
        bool UnlockAllTemporary = false;
        bool UnlockAllPermanent = false;
        bool HighRefreshFix = false;
        bool BackgroundPauseFix = true;
        bool SkipOnlineChecks = true;
    };

    bool Init();
    void Shutdown();

    PatchState& State();
    const PatchState& GetState();

    void ApplySafePatches();
    void ApplyDisplayPatches();
    void ApplyGameplayPatches();
}
