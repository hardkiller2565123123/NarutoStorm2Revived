#include "StdInc.h"
#include "SteamPersonaManager.h"
#include "Logger.h"

static std::mutex g_Mutex;
static std::string g_Name = "NS2Revived";
static std::map<std::string, std::string> g_RichPresence;

namespace SteamPersonaManager
{
    void Init()
    {
        Logger::Info("SteamPersonaManager initialized");
    }

    const char* GetPersonaName()
    {
        return g_Name.c_str();
    }

    void SetPersonaName(const std::string& name)
    {
        std::lock_guard<std::mutex> lock(g_Mutex);
        g_Name = name.empty() ? "NS2Revived" : name;
        Logger::Info("SteamPersonaManager name set: " + g_Name);
    }

    bool SetRichPresence(const std::string& key, const std::string& value)
    {
        std::lock_guard<std::mutex> lock(g_Mutex);
        g_RichPresence[key] = value;
        Logger::Info("SteamPersonaManager rich presence set: " + key);
        return true;
    }

    const char* GetRichPresence(const std::string& key)
    {
        static std::string empty;

        auto it = g_RichPresence.find(key);

        if (it == g_RichPresence.end())
            return empty.c_str();

        return it->second.c_str();
    }

    void ClearRichPresence()
    {
        std::lock_guard<std::mutex> lock(g_Mutex);
        g_RichPresence.clear();
        Logger::Info("SteamPersonaManager rich presence cleared");
    }
}