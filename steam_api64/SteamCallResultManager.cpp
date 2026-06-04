#include "StdInc.h"
#include "SteamCallResultManager.h"
#include "Logger.h"

static std::mutex g_Mutex;
static SteamAPICall_t g_NextCall = 1;
static std::map<SteamAPICall_t, NS2CallResult> g_Results;

namespace SteamCallResultManager
{
    void Init()
    {
        Logger::Info("SteamCallResultManager initialized");
    }

    SteamAPICall_t CreateCallResult(int callbackID, const void* data, size_t size, bool failed)
    {
        std::lock_guard<std::mutex> lock(g_Mutex);

        SteamAPICall_t call = g_NextCall++;

        NS2CallResult result{};
        result.Call = call;
        result.CallbackID = callbackID;
        result.Failed = failed;

        if (data && size > 0)
        {
            result.Data.resize(size);
            memcpy(result.Data.data(), data, size);
        }

        g_Results[call] = result;

        Logger::Info(
            "SteamCallResultManager created call=" +
            std::to_string(call) +
            " callbackID=" +
            std::to_string(callbackID));

        return call;
    }

    bool GetCallResult(SteamAPICall_t call, NS2CallResult& outResult)
    {
        std::lock_guard<std::mutex> lock(g_Mutex);

        auto it = g_Results.find(call);

        if (it == g_Results.end())
            return false;

        outResult = it->second;
        return true;
    }

    void RemoveCallResult(SteamAPICall_t call)
    {
        std::lock_guard<std::mutex> lock(g_Mutex);
        g_Results.erase(call);
    }
}