#include "StdInc.h"
#include "SteamCallResultManager.h"
#include "SteamCallbackManager.h"
#include "Logger.h"

namespace
{
    std::mutex g_Mutex;
    SteamAPICall_t g_NextCall = 1;
    std::vector<SteamCallResultManager::CallResultRecord> g_Results;
    std::map<SteamAPICall_t, std::vector<void*>> g_Registered;
}

namespace SteamCallResultManager
{
    bool Init()
    {
        std::lock_guard<std::mutex> lock(g_Mutex);
        g_NextCall = 1;
        g_Results.clear();
        g_Registered.clear();
        Logger::Info("SteamCallResultManager initialized");
        return true;
    }

    void Shutdown()
    {
        std::lock_guard<std::mutex> lock(g_Mutex);
        g_Results.clear();
        g_Registered.clear();
    }

    SteamAPICall_t CreateRaw(int callbackID, const void* data, size_t size)
    {
        std::lock_guard<std::mutex> lock(g_Mutex);

        CallResultRecord record{};
        record.Call = g_NextCall++;
        record.CallbackID = callbackID;
        record.Completed = true;

        if (data && size > 0)
        {
            record.Data.resize(size);
            std::memcpy(record.Data.data(), data, size);
        }

        g_Results.push_back(record);

        if (callbackID != 0)
        {
            SteamCallbackManager::PushCallback(
                callbackID,
                record.Data.empty() ? nullptr : record.Data.data(),
                record.Data.size());
        }

        Logger::Info(
            "SteamCallResultManager created call=" +
            std::to_string(static_cast<unsigned long long>(record.Call)) +
            " callbackID=" +
            std::to_string(callbackID));

        return record.Call;
    }

    SteamAPICall_t CreateCall(int callbackID, const void* data, size_t size)
    {
        return CreateRaw(callbackID, data, size);
    }

    SteamAPICall_t CreateCallResult(int callbackID)
    {
        return CreateRaw(callbackID, nullptr, 0);
    }

    SteamAPICall_t CreateCallResult(int callbackID, const void* data, size_t size)
    {
        return CreateRaw(callbackID, data, size);
    }

    SteamAPICall_t CreateCallResult(int callbackID, const void* data, size_t size, bool)
    {
        return CreateRaw(callbackID, data, size);
    }

    bool GetData(SteamAPICall_t call, void* outData, int outSize, int* callbackID)
    {
        std::lock_guard<std::mutex> lock(g_Mutex);

        for (const auto& result : g_Results)
        {
            if (result.Call != call)
                continue;

            if (callbackID)
                *callbackID = result.CallbackID;

            if (outData && outSize > 0 && !result.Data.empty())
            {
                size_t copySize = std::min<size_t>(static_cast<size_t>(outSize), result.Data.size());
                std::memcpy(outData, result.Data.data(), copySize);
            }

            return true;
        }

        return false;
    }

    bool IsCompleted(SteamAPICall_t call)
    {
        std::lock_guard<std::mutex> lock(g_Mutex);

        for (const auto& result : g_Results)
        {
            if (result.Call == call)
                return result.Completed;
        }

        return false;
    }

    bool Complete(SteamAPICall_t call)
    {
        std::lock_guard<std::mutex> lock(g_Mutex);

        for (auto& result : g_Results)
        {
            if (result.Call != call)
                continue;

            result.Completed = true;

            if (result.CallbackID != 0)
            {
                SteamCallbackManager::PushCallback(
                    result.CallbackID,
                    result.Data.empty() ? nullptr : result.Data.data(),
                    result.Data.size());
            }

            return true;
        }

        return false;
    }

    void Register(void* callResult, SteamAPICall_t call)
    {
        if (!callResult)
            return;

        std::lock_guard<std::mutex> lock(g_Mutex);
        g_Registered[call].push_back(callResult);
    }

    void Unregister(void* callResult, SteamAPICall_t call)
    {
        std::lock_guard<std::mutex> lock(g_Mutex);

        if (call != 0)
        {
            auto& list = g_Registered[call];
            list.erase(std::remove(list.begin(), list.end(), callResult), list.end());
            return;
        }

        for (auto& item : g_Registered)
        {
            auto& list = item.second;
            list.erase(std::remove(list.begin(), list.end(), callResult), list.end());
        }
    }

    void RegisterCallResult(void* callResult, SteamAPICall_t call)
    {
        Register(callResult, call);
    }

    void UnregisterCallResult(void* callResult, SteamAPICall_t call)
    {
        Unregister(callResult, call);
    }

    void UnregisterCallResult(void* callResult)
    {
        Unregister(callResult, 0);
    }

    int Count()
    {
        std::lock_guard<std::mutex> lock(g_Mutex);
        return static_cast<int>(g_Results.size());
    }

    int PendingCount()
    {
        return Count();
    }

    const std::vector<CallResultRecord>& GetResults()
    {
        return g_Results;
    }
}