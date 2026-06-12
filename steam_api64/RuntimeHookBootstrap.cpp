#include "StdInc.h"
#include "RuntimeHookBootstrap.h"
#include "RuntimeHookAddresses.h"
#include "RuntimeAssetTelemetry.h"
#include "RuntimeFileHooks.h"
#include "RuntimeGameHooks.h"
#include "Logger.h"

namespace RuntimeHookBootstrap
{
    bool Init()
    {
        Logger::Info("RuntimeHookBootstrap starting");
        RuntimeHookAddresses::Init();
        RuntimeAssetTelemetry::Init();
        bool fileOk = RuntimeFileHooks::Init();
        bool gameOk = RuntimeGameHooks::Init();
        Logger::Info(std::string("RuntimeHookBootstrap initialized file=") + (fileOk ? "1" : "0") + " game=" + (gameOk ? "1" : "0"));
        return fileOk && gameOk;
    }

    void Shutdown()
    {
        RuntimeGameHooks::Shutdown();
        RuntimeFileHooks::Shutdown();
        RuntimeAssetTelemetry::Shutdown();
        Logger::Info("RuntimeHookBootstrap shutdown");
    }
}
