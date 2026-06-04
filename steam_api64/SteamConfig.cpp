#include "StdInc.h"
#include "SteamConfig.h"
#include "Logger.h"

namespace SteamConfig
{
    void Init()
    {
        Logger::Info("SteamConfig initialized");
    }

    bool KeepOwnershipReal() { return true; }
    bool UseFakeInterfaces() { return true; }
    bool UseCustomNetworking() { return true; }
    bool UseLocalStats() { return true; }
    bool UseLocalStorage() { return true; }
}