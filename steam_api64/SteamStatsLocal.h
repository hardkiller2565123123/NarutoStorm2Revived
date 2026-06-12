#pragma once
#include "StdInc.h"

namespace SteamStatsLocal
{
    void Init();

    bool GetInt(const char* name, int32_t* value);
    bool SetInt(const char* name, int32_t value);

    bool GetFloat(const char* name, float* value);
    bool SetFloat(const char* name, float value);

    bool GetAchievement(const char* name, bool* achieved);
    bool SetAchievement(const char* name);
    bool ClearAchievement(const char* name);
    uint32_t GetAchievementCount();
    const char* GetAchievementName(uint32_t index);

    bool ResetAll(bool achievementsToo);

    void Save();
}
