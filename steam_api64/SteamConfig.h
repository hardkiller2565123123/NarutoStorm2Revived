#pragma once
#include "StdInc.h"

namespace SteamConfig
{
    void Init();

    bool KeepOwnershipReal();
    bool UseFakeInterfaces();
    bool UseCustomNetworking();
    bool UseLocalStats();
    bool UseLocalStorage();
}