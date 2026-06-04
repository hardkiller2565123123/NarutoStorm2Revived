#include "StdInc.h"
#include "SteamCallbackManager.h"
#include "Logger.h"

#include <queue>
#include <algorithm>

static std::mutex g_Mutex;
static std::map<int, std::vector<void*>> g_Callbacks;
static std::queue<NS2CallbackMessage> g_Queue;

namespace SteamCallbackManager
{
    void Init()
    {
        Logger::Info("SteamCallbackManager initialized");
    }

    void RegisterCallback(void* callback, int callbackID)
    {
        std::lock_guard<std::mutex> lock(g_Mutex);

        g_Callbacks[callbackID].push_back(callback);

        Logger::Info(
            "SteamCallbackManager registered callback id=" +
            std::to_string(callbackID));
    }

    void UnregisterCallback(void* callback)
    {
        std::lock_guard<std::mutex> lock(g_Mutex);

        for (auto& item : g_Callbacks)
        {
            auto& list = item.second;

            list.erase(
                std::remove(list.begin(), list.end(), callback),
                list.end());
        }

        Logger::Info("SteamCallbackManager unregistered callback");
    }

    void PushCallback(int callbackID, const void* data, size_t size)
    {
        std::lock_guard<std::mutex> lock(g_Mutex);

        NS2CallbackMessage message{};
        message.CallbackID = callbackID;

        if (data && size > 0)
        {
            message.Data.resize(size);
            memcpy(message.Data.data(), data, size);
        }

        g_Queue.push(message);

        Logger::Info(
            "SteamCallbackManager queued callback id=" +
            std::to_string(callbackID));
    }

    bool PopCallback(NS2CallbackMessage& outMessage)
    {
        std::lock_guard<std::mutex> lock(g_Mutex);

        if (g_Queue.empty())
            return false;

        outMessage = g_Queue.front();
        g_Queue.pop();

        return true;
    }

    int Count()
    {
        std::lock_guard<std::mutex> lock(g_Mutex);
        return static_cast<int>(g_Queue.size());
    }
}