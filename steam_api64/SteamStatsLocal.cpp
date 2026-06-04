#include "StdInc.h"
#include "SteamStatsLocal.h"
#include "Logger.h"

static std::mutex g_Mutex;
static std::map<std::string, int32_t> g_IntStats;
static std::map<std::string, float> g_FloatStats;
static std::map<std::string, bool> g_Achievements;

namespace SteamStatsLocal
{
    void Init()
    {
        Logger::Info("SteamStatsLocal initialized");
    }

    bool GetInt(const char* name, int32_t* value)
    {
        if (!name || !value)
            return false;

        std::lock_guard<std::mutex> lock(g_Mutex);
        *value = g_IntStats[name];
        return true;
    }

    bool SetInt(const char* name, int32_t value)
    {
        if (!name)
            return false;

        std::lock_guard<std::mutex> lock(g_Mutex);
        g_IntStats[name] = value;

        Logger::Info(std::string("SteamStatsLocal int stat set: ") + name);

        return true;
    }

    bool GetFloat(const char* name, float* value)
    {
        if (!name || !value)
            return false;

        std::lock_guard<std::mutex> lock(g_Mutex);
        *value = g_FloatStats[name];
        return true;
    }

    bool SetFloat(const char* name, float value)
    {
        if (!name)
            return false;

        std::lock_guard<std::mutex> lock(g_Mutex);
        g_FloatStats[name] = value;

        Logger::Info(std::string("SteamStatsLocal float stat set: ") + name);

        return true;
    }

    bool GetAchievement(const char* name, bool* achieved)
    {
        if (!name || !achieved)
            return false;

        std::lock_guard<std::mutex> lock(g_Mutex);
        *achieved = g_Achievements[name];
        return true;
    }

    bool SetAchievement(const char* name)
    {
        if (!name)
            return false;

        std::lock_guard<std::mutex> lock(g_Mutex);
        g_Achievements[name] = true;

        Logger::Info(std::string("SteamStatsLocal achievement set: ") + name);

        return true;
    }

    bool ClearAchievement(const char* name)
    {
        if (!name)
            return false;

        std::lock_guard<std::mutex> lock(g_Mutex);
        g_Achievements[name] = false;

        return true;
    }

    void Save()
    {
        Logger::Info("SteamStatsLocal saved");
    }
}