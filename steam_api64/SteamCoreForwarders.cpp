#include "StdInc.h"
#include "SteamCoreForwarders.h"
#include "SteamProxy.h"
#include "Logger.h"

template <typename T>
static T Resolve(const char* name)
{
    void* exportAddress = SteamProxy::GetExport(name);

    if (!exportAddress)
    {
        Logger::Error(std::string(name) + ": core export not found");
        return nullptr;
    }

    Logger::Info(std::string(name) + ": real core interface forwarded");
    return reinterpret_cast<T>(exportAddress);
}

namespace SteamCoreForwarders
{
    void* Client()
    {
        typedef void* (__cdecl* Fn)();
        static Fn fn = Resolve<Fn>("SteamClient");
        return fn ? fn() : nullptr;
    }

    void* User()
    {
        typedef void* (__cdecl* Fn)();
        static Fn fn = Resolve<Fn>("SteamUser");
        return fn ? fn() : nullptr;
    }

    void* Apps()
    {
        typedef void* (__cdecl* Fn)();
        static Fn fn = Resolve<Fn>("SteamApps");
        return fn ? fn() : nullptr;
    }

    void* Utils()
    {
        typedef void* (__cdecl* Fn)();
        static Fn fn = Resolve<Fn>("SteamUtils");
        return fn ? fn() : nullptr;
    }
}