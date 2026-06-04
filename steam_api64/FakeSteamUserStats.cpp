#include "StdInc.h"
#include "FakeSteamInterfaces.h"
#include "Logger.h"
#include "SteamStatsLocal.h"

typedef uint64_t SteamLeaderboard_t;
typedef uint64_t SteamLeaderboardEntries_t;
typedef uint64_t UGCHandle_t;

class FakeSteamUserStats final
{
public:
    virtual bool RequestCurrentStats()
    {
        Logger::Info("SteamUserStats::RequestCurrentStats");
        return true;
    }

    virtual bool GetStat(const char* name, int32_t* data)
    {
        Logger::Info(std::string("SteamUserStats::GetStat int ") + (name ? name : "null"));
        return SteamStatsLocal::GetInt(name, data);
    }

    virtual bool GetStat(const char* name, float* data)
    {
        Logger::Info(std::string("SteamUserStats::GetStat float ") + (name ? name : "null"));
        return SteamStatsLocal::GetFloat(name, data);
    }

    virtual bool SetStat(const char* name, int32_t data)
    {
        Logger::Info(std::string("SteamUserStats::SetStat int ") + (name ? name : "null"));
        return SteamStatsLocal::SetInt(name, data);
    }

    virtual bool SetStat(const char* name, float data)
    {
        Logger::Info(std::string("SteamUserStats::SetStat float ") + (name ? name : "null"));
        return SteamStatsLocal::SetFloat(name, data);
    }

    virtual bool UpdateAvgRateStat(const char* name, float count, double sessionLength)
    {
        NSR_UNUSED(name);
        NSR_UNUSED(count);
        NSR_UNUSED(sessionLength);
        Logger::Info("SteamUserStats::UpdateAvgRateStat");
        return true;
    }

    virtual bool GetAchievement(const char* name, bool* achieved)
    {
        Logger::Info(std::string("SteamUserStats::GetAchievement ") + (name ? name : "null"));
        return SteamStatsLocal::GetAchievement(name, achieved);
    }

    virtual bool SetAchievement(const char* name)
    {
        Logger::Info(std::string("SteamUserStats::SetAchievement ") + (name ? name : "null"));
        return SteamStatsLocal::SetAchievement(name);
    }

    virtual bool ClearAchievement(const char* name)
    {
        Logger::Info(std::string("SteamUserStats::ClearAchievement ") + (name ? name : "null"));
        return SteamStatsLocal::ClearAchievement(name);
    }

    virtual bool GetAchievementAndUnlockTime(const char* name, bool* achieved, uint32_t* unlockTime)
    {
        Logger::Info(std::string("SteamUserStats::GetAchievementAndUnlockTime ") + (name ? name : "null"));

        if (unlockTime)
            *unlockTime = 0;

        return SteamStatsLocal::GetAchievement(name, achieved);
    }

    virtual bool StoreStats()
    {
        Logger::Info("SteamUserStats::StoreStats");
        SteamStatsLocal::Save();
        return true;
    }

    virtual int GetAchievementIcon(const char* name)
    {
        NSR_UNUSED(name);
        Logger::Info("SteamUserStats::GetAchievementIcon");
        return 0;
    }

    virtual const char* GetAchievementDisplayAttribute(const char* name, const char* key)
    {
        NSR_UNUSED(name);
        Logger::Info(std::string("SteamUserStats::GetAchievementDisplayAttribute ") + (key ? key : "null"));

        if (key && strcmp(key, "hidden") == 0)
            return "0";

        return "";
    }

    virtual bool IndicateAchievementProgress(const char* name, uint32_t current, uint32_t max)
    {
        NSR_UNUSED(name);
        NSR_UNUSED(current);
        NSR_UNUSED(max);
        Logger::Info("SteamUserStats::IndicateAchievementProgress");
        return true;
    }

    virtual uint32_t GetNumAchievements()
    {
        Logger::Info("SteamUserStats::GetNumAchievements");
        return 0;
    }

    virtual const char* GetAchievementName(uint32_t achievement)
    {
        NSR_UNUSED(achievement);
        Logger::Info("SteamUserStats::GetAchievementName");
        return "";
    }

    virtual SteamAPICall_t RequestUserStats(CSteamID user)
    {
        NSR_UNUSED(user);
        Logger::Info("SteamUserStats::RequestUserStats");
        return 1;
    }

    virtual bool GetUserStat(CSteamID user, const char* name, int32_t* data)
    {
        NSR_UNUSED(user);
        Logger::Info("SteamUserStats::GetUserStat int");
        if (data) *data = 0;
        return true;
    }

    virtual bool GetUserStat(CSteamID user, const char* name, float* data)
    {
        NSR_UNUSED(user);
        NSR_UNUSED(name);
        Logger::Info("SteamUserStats::GetUserStat float");
        if (data) *data = 0.0f;
        return true;
    }

    virtual bool GetUserAchievement(CSteamID user, const char* name, bool* achieved)
    {
        NSR_UNUSED(user);
        NSR_UNUSED(name);
        Logger::Info("SteamUserStats::GetUserAchievement");
        if (achieved) *achieved = false;
        return true;
    }

    virtual bool GetUserAchievementAndUnlockTime(CSteamID user, const char* name, bool* achieved, uint32_t* unlockTime)
    {
        NSR_UNUSED(user);
        NSR_UNUSED(name);
        Logger::Info("SteamUserStats::GetUserAchievementAndUnlockTime");
        if (achieved) *achieved = false;
        if (unlockTime) *unlockTime = 0;
        return true;
    }

    virtual bool ResetAllStats(bool achievementsToo)
    {
        NSR_UNUSED(achievementsToo);
        Logger::Info("SteamUserStats::ResetAllStats");
        return true;
    }

    virtual SteamAPICall_t FindOrCreateLeaderboard(const char* name, int sortMethod, int displayType)
    {
        NSR_UNUSED(sortMethod);
        NSR_UNUSED(displayType);
        Logger::Info(std::string("SteamUserStats::FindOrCreateLeaderboard ") + (name ? name : "null"));
        return 1;
    }

    virtual SteamAPICall_t FindLeaderboard(const char* name)
    {
        Logger::Info(std::string("SteamUserStats::FindLeaderboard ") + (name ? name : "null"));
        return 1;
    }

    virtual const char* GetLeaderboardName(SteamLeaderboard_t leaderboard)
    {
        NSR_UNUSED(leaderboard);
        Logger::Info("SteamUserStats::GetLeaderboardName");
        return "NS2RevivedLeaderboard";
    }

    virtual int GetLeaderboardEntryCount(SteamLeaderboard_t leaderboard)
    {
        NSR_UNUSED(leaderboard);
        Logger::Info("SteamUserStats::GetLeaderboardEntryCount");
        return 0;
    }

    virtual int GetLeaderboardSortMethod(SteamLeaderboard_t leaderboard)
    {
        NSR_UNUSED(leaderboard);
        Logger::Info("SteamUserStats::GetLeaderboardSortMethod");
        return 0;
    }

    virtual int GetLeaderboardDisplayType(SteamLeaderboard_t leaderboard)
    {
        NSR_UNUSED(leaderboard);
        Logger::Info("SteamUserStats::GetLeaderboardDisplayType");
        return 0;
    }

    virtual SteamAPICall_t DownloadLeaderboardEntries(SteamLeaderboard_t leaderboard, int request, int start, int end)
    {
        NSR_UNUSED(leaderboard);
        NSR_UNUSED(request);
        NSR_UNUSED(start);
        NSR_UNUSED(end);
        Logger::Info("SteamUserStats::DownloadLeaderboardEntries");
        return 1;
    }

    virtual SteamAPICall_t DownloadLeaderboardEntriesForUsers(SteamLeaderboard_t leaderboard, CSteamID* users, int count)
    {
        NSR_UNUSED(leaderboard);
        NSR_UNUSED(users);
        NSR_UNUSED(count);
        Logger::Info("SteamUserStats::DownloadLeaderboardEntriesForUsers");
        return 1;
    }

    virtual bool GetDownloadedLeaderboardEntry(SteamLeaderboardEntries_t entries, int index, void* entry, int32_t* details, int detailsMax)
    {
        NSR_UNUSED(entries);
        NSR_UNUSED(index);
        NSR_UNUSED(details);
        NSR_UNUSED(detailsMax);
        Logger::Info("SteamUserStats::GetDownloadedLeaderboardEntry");

        if (entry)
            memset(entry, 0, 64);

        return false;
    }

    virtual SteamAPICall_t UploadLeaderboardScore(SteamLeaderboard_t leaderboard, int method, int32_t score, const int32_t* details, int detailsCount)
    {
        NSR_UNUSED(leaderboard);
        NSR_UNUSED(method);
        NSR_UNUSED(score);
        NSR_UNUSED(details);
        NSR_UNUSED(detailsCount);
        Logger::Info("SteamUserStats::UploadLeaderboardScore");
        return 1;
    }

    virtual SteamAPICall_t AttachLeaderboardUGC(SteamLeaderboard_t leaderboard, UGCHandle_t ugc)
    {
        NSR_UNUSED(leaderboard);
        NSR_UNUSED(ugc);
        Logger::Info("SteamUserStats::AttachLeaderboardUGC");
        return 1;
    }

    virtual SteamAPICall_t GetNumberOfCurrentPlayers()
    {
        Logger::Info("SteamUserStats::GetNumberOfCurrentPlayers");
        return 1;
    }

    virtual SteamAPICall_t RequestGlobalAchievementPercentages()
    {
        Logger::Info("SteamUserStats::RequestGlobalAchievementPercentages");
        return 1;
    }

    virtual int GetMostAchievedAchievementInfo(char* name, uint32_t nameLen, float* percent, bool* achieved)
    {
        Logger::Info("SteamUserStats::GetMostAchievedAchievementInfo");

        if (name && nameLen > 0)
            name[0] = '\0';

        if (percent) *percent = 0.0f;
        if (achieved) *achieved = false;

        return -1;
    }

    virtual int GetNextMostAchievedAchievementInfo(int previous, char* name, uint32_t nameLen, float* percent, bool* achieved)
    {
        NSR_UNUSED(previous);
        Logger::Info("SteamUserStats::GetNextMostAchievedAchievementInfo");

        if (name && nameLen > 0)
            name[0] = '\0';

        if (percent) *percent = 0.0f;
        if (achieved) *achieved = false;

        return -1;
    }

    virtual bool GetAchievementAchievedPercent(const char* name, float* percent)
    {
        NSR_UNUSED(name);
        Logger::Info("SteamUserStats::GetAchievementAchievedPercent");

        if (percent)
            *percent = 0.0f;

        return false;
    }

    virtual SteamAPICall_t RequestGlobalStats(int historyDays)
    {
        NSR_UNUSED(historyDays);
        Logger::Info("SteamUserStats::RequestGlobalStats");
        return 1;
    }

    virtual bool GetGlobalStat(const char* name, int64_t* data)
    {
        NSR_UNUSED(name);
        Logger::Info("SteamUserStats::GetGlobalStat int64");

        if (data)
            *data = 0;

        return true;
    }

    virtual bool GetGlobalStat(const char* name, double* data)
    {
        NSR_UNUSED(name);
        Logger::Info("SteamUserStats::GetGlobalStat double");

        if (data)
            *data = 0.0;

        return true;
    }

    virtual int32_t GetGlobalStatHistory(const char* name, int64_t* data, uint32_t size)
    {
        NSR_UNUSED(name);
        NSR_UNUSED(size);
        Logger::Info("SteamUserStats::GetGlobalStatHistory int64");

        if (data)
            data[0] = 0;

        return 0;
    }

    virtual int32_t GetGlobalStatHistory(const char* name, double* data, uint32_t size)
    {
        NSR_UNUSED(name);
        NSR_UNUSED(size);
        Logger::Info("SteamUserStats::GetGlobalStatHistory double");

        if (data)
            data[0] = 0.0;

        return 0;
    }
};

static FakeSteamUserStats g_Interface;

void* FakeSteamInterfaces::UserStats()
{
    Logger::Info("SteamUserStats: fixed standalone emulated interface returned");
    return &g_Interface;
}