#include "StdInc.h"
#include "SteamCallbackManager.h"
#include "Logger.h"

namespace
{
    std::mutex g_Mutex;
    std::map<int, std::vector<void*>> g_Callbacks;
    std::queue<NS2CallbackMessage> g_Queue;
}

namespace SteamCallbackManager
{
    bool Init()
    {
        std::lock_guard<std::mutex> lock(g_Mutex);
        while (!g_Queue.empty()) g_Queue.pop();
        g_Callbacks.clear();
        Logger::Info("SteamCallbackManager initialized");
        return true;
    }

    void RegisterCallback(void* callback, int callbackID)
    {
        std::lock_guard<std::mutex> lock(g_Mutex);
        if (!callback) return;
        g_Callbacks[callbackID].push_back(callback);
        Logger::Info("SteamAPI_RegisterCallback id=" + std::to_string(callbackID));
    }

    void UnregisterCallback(void* callback)
    {
        std::lock_guard<std::mutex> lock(g_Mutex);
        for (auto& kv : g_Callbacks)
        {
            auto& list = kv.second;
            list.erase(std::remove(list.begin(), list.end(), callback), list.end());
        }
        Logger::Info("SteamAPI_UnregisterCallback");
    }

    void PushCallback(int callbackID, const void* data, size_t size)
    {
        std::lock_guard<std::mutex> lock(g_Mutex);
        NS2CallbackMessage msg{};
        msg.CallbackID = callbackID;
        if (data && size)
        {
            msg.Data.resize(size);
            std::memcpy(msg.Data.data(), data, size);
        }
        g_Queue.push(msg);
    }

    bool PopCallback(NS2CallbackMessage& outMessage)
    {
        std::lock_guard<std::mutex> lock(g_Mutex);
        if (g_Queue.empty()) return false;
        outMessage = g_Queue.front();
        g_Queue.pop();
        return true;
    }

    int Count()
    {
        std::lock_guard<std::mutex> lock(g_Mutex);
        return (int)g_Queue.size();
    }

    void RunCallbacks()
    {
        // This intentionally drains only bookkeeping messages. Real CCallback dispatching needs SDK callback object layout.
        NS2CallbackMessage msg;
        int drained = 0;
        while (PopCallback(msg) && drained++ < 32) {}
    }
}
