#include "StdInc.h"
#include "FakeSteamInterfaces.h"
#include "FakeSteamBase.h"

#define DEFINE_FAKE_INTERFACE(functionName, className, logName) \
class className final : public FakeSteamBase \
{ \
protected: \
    const char* InterfaceName() const override { return logName; } \
public: \
    virtual bool Init() { Logger::Info(std::string(logName) + "::Init"); return true; } \
    virtual bool Shutdown() { Logger::Info(std::string(logName) + "::Shutdown"); return true; } \
    virtual bool IsEnabled() { Logger::Info(std::string(logName) + "::IsEnabled"); return false; } \
    virtual int GetCount() { Logger::Info(std::string(logName) + "::GetCount"); return 0; } \
    virtual bool Request() { Logger::Info(std::string(logName) + "::Request"); return true; } \
}; \
static className g_##className; \
void* FakeSteamInterfaces::functionName() \
{ \
    Logger::Info(std::string(logName) + ": emulated generic interface returned"); \
    return &g_##className; \
}

DEFINE_FAKE_INTERFACE(MatchmakingServers, FakeSteamMatchmakingServers, "SteamMatchmakingServers")
DEFINE_FAKE_INTERFACE(Screenshots, FakeSteamScreenshots, "SteamScreenshots")
DEFINE_FAKE_INTERFACE(HTTP, FakeSteamHTTP, "SteamHTTP")
DEFINE_FAKE_INTERFACE(Controller, FakeSteamController, "SteamController")
DEFINE_FAKE_INTERFACE(UGC, FakeSteamUGC, "SteamUGC")
DEFINE_FAKE_INTERFACE(AppList, FakeSteamAppList, "SteamAppList")
DEFINE_FAKE_INTERFACE(Music, FakeSteamMusic, "SteamMusic")
DEFINE_FAKE_INTERFACE(MusicRemote, FakeSteamMusicRemote, "SteamMusicRemote")
DEFINE_FAKE_INTERFACE(HTMLSurface, FakeSteamHTMLSurface, "SteamHTMLSurface")
DEFINE_FAKE_INTERFACE(Inventory, FakeSteamInventory, "SteamInventory")
DEFINE_FAKE_INTERFACE(Video, FakeSteamVideo, "SteamVideo")
DEFINE_FAKE_INTERFACE(ParentalSettings, FakeSteamParentalSettings, "SteamParentalSettings")
DEFINE_FAKE_INTERFACE(GameSearch, FakeSteamGameSearch, "SteamGameSearch")
DEFINE_FAKE_INTERFACE(Input, FakeSteamInput, "SteamInput")
DEFINE_FAKE_INTERFACE(Parties, FakeSteamParties, "SteamParties")
DEFINE_FAKE_INTERFACE(RemotePlay, FakeSteamRemotePlay, "SteamRemotePlay")