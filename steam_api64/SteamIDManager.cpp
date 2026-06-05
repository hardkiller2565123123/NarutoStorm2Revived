#include "StdInc.h"
#include "SteamIDManager.h"
#include "SteamConfig.h"
#include "Logger.h"

namespace
{
    std::string g_FallbackPersona = "NS2Revived";
}

namespace SteamIDManager
{
    bool Init()
    {
        Logger::Info("SteamIDManager initialized id=" + std::to_string(static_cast<unsigned long long>(GetSteamID64())));
        return true;
    }

    void Shutdown()
    {}

    CSteamID FromUint64(uint64_t value)
    {
        return static_cast<CSteamID>(value);
    }

    CSteamID GetLocalSteamID()
    {
        return FromUint64(GetSteamID64());
    }

    CSteamID GetLocalUser()
    {
        return GetLocalSteamID();
    }

    uint64_t ToUint64(CSteamID value)
    {
        return static_cast<uint64_t>(value);
    }

    uint64_t GetSteamID64()
    {
        return SteamConfig::Get().SteamID;
    }

    uint32_t GetAccountID()
    {
        return static_cast<uint32_t>(GetSteamID64() & 0xFFFFFFFFu);
    }

    const char* GetPersonaName()
    {
        const std::string& name = SteamConfig::Get().PersonaName;
        return name.empty() ? g_FallbackPersona.c_str() : name.c_str();
    }

    const std::string& GetPersonaNameString()
    {
        const std::string& name = SteamConfig::Get().PersonaName;
        return name.empty() ? g_FallbackPersona : name;
    }

    void SetPersonaName(const std::string& name)
    {
        SteamConfig::Mutable().PersonaName = name.empty() ? g_FallbackPersona : name;
        SteamConfig::Save();
    }
}