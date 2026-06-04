#include "StdInc.h"
#include "SteamP2PManager.h"
#include "SteamIDManager.h"
#include "SteamPacketLogger.h"
#include "Logger.h"

static std::mutex g_Mutex;
static std::vector<NS2P2PPacket> g_Incoming;

namespace SteamP2PManager
{
    void Init()
    {
        Logger::Info("SteamP2PManager initialized");
    }

    bool Send(CSteamID target, const void* data, uint32_t size, int channel)
    {
        if (!data || size == 0)
            return false;

        SteamPacketLogger::LogSend("SteamP2PManager", target, data, size, channel);

        return true;
    }

    bool HasPacket(uint32_t* size, int channel)
    {
        std::lock_guard<std::mutex> lock(g_Mutex);

        for (const NS2P2PPacket& packet : g_Incoming)
        {
            if (packet.Channel == channel)
            {
                if (size)
                    *size = static_cast<uint32_t>(packet.Data.size());

                return true;
            }
        }

        if (size)
            *size = 0;

        return false;
    }

    bool Read(void* dest, uint32_t destSize, uint32_t* msgSize, CSteamID* sender, int channel)
    {
        std::lock_guard<std::mutex> lock(g_Mutex);

        for (auto it = g_Incoming.begin(); it != g_Incoming.end(); ++it)
        {
            if (it->Channel != channel)
                continue;

            uint32_t size = static_cast<uint32_t>(it->Data.size());

            if (msgSize)
                *msgSize = size;

            if (sender)
                *sender = it->Sender;

            if (!dest || destSize < size)
                return false;

            memcpy(dest, it->Data.data(), size);

            SteamPacketLogger::LogReceive("SteamP2PManager", it->Sender, dest, size, channel);

            g_Incoming.erase(it);
            return true;
        }

        if (msgSize)
            *msgSize = 0;

        return false;
    }

    void PushIncoming(CSteamID sender, const void* data, uint32_t size, int channel)
    {
        if (!data || size == 0)
            return;

        std::lock_guard<std::mutex> lock(g_Mutex);

        NS2P2PPacket packet{};
        packet.Sender = sender;
        packet.Target = SteamIDManager::GetLocalSteamID();
        packet.Channel = channel;
        packet.Data.resize(size);

        memcpy(packet.Data.data(), data, size);

        g_Incoming.push_back(packet);
    }
}