#include "StdInc.h"
#include "Logger.h"
#include "SteamIDManager.h"

#include <WinSock2.h>
#include <Ws2tcpip.h>

#pragma comment(lib, "Ws2_32.lib")

typedef int32_t SNetSocket_t;
typedef int32_t SNetListenSocket_t;

enum EP2PSend
{
    k_EP2PSendUnreliable = 0,
    k_EP2PSendUnreliableNoDelay = 1,
    k_EP2PSendReliable = 2,
    k_EP2PSendReliableWithBuffering = 3
};

struct P2PSessionState_t
{
    uint8_t m_bConnectionActive;
    uint8_t m_bConnecting;
    uint8_t m_eP2PSessionError;
    uint8_t m_bUsingRelay;
    int32_t m_nBytesQueuedForSend;
    int32_t m_nPacketsQueuedForSend;
    uint32_t m_nRemoteIP;
    uint16_t m_nRemotePort;
};

static const char* NS2_SERVER_IP = "127.0.0.1";
static const uint16_t NS2_SERVER_PORT = 47584;

struct QueuedPacket
{
    CSteamID sender{};
    int channel{};
    std::vector<uint8_t> data;
};

class NS2SteamNetworking
{
private:
    SOCKET socketHandle = INVALID_SOCKET;
    sockaddr_in serverAddress{};
    bool initialized = false;

    std::mutex packetMutex;
    std::vector<QueuedPacket> packets;

private:
    void QueuePacket(CSteamID sender, const void* data, uint32_t size, int channel)
    {
        if (!data || size == 0)
            return;

        QueuedPacket packet{};
        packet.sender = sender;
        packet.channel = channel;
        packet.data.resize(size);
        memcpy(packet.data.data(), data, size);

        std::lock_guard<std::mutex> lock(packetMutex);
        packets.push_back(packet);
    }

private:
    bool EnsureSocket()
    {
        if (initialized)
            return socketHandle != INVALID_SOCKET;

        initialized = true;

        WSADATA wsa{};
        WSAStartup(MAKEWORD(2, 2), &wsa);

        socketHandle = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

        if (socketHandle == INVALID_SOCKET)
        {
            Logger::Error("NS2SteamNetworking: socket creation failed");
            return false;
        }

        u_long nonBlocking = 1;
        ioctlsocket(socketHandle, FIONBIO, &nonBlocking);

        sockaddr_in local{};
        local.sin_family = AF_INET;
        local.sin_addr.s_addr = htonl(INADDR_ANY);
        local.sin_port = htons(0);

        bind(
            socketHandle,
            reinterpret_cast<sockaddr*>(&local),
            sizeof(local));

        serverAddress = {};
        serverAddress.sin_family = AF_INET;
        serverAddress.sin_port = htons(NS2_SERVER_PORT);

        inet_pton(
            AF_INET,
            NS2_SERVER_IP,
            &serverAddress.sin_addr);

        Logger::Info(
            "NS2SteamNetworking: custom server " +
            std::string(NS2_SERVER_IP) +
            ":" +
            std::to_string(NS2_SERVER_PORT));

        return true;
    }

    void Poll()
    {
        if (!EnsureSocket())
            return;

        for (;;)
        {
            uint8_t buffer[8192]{};

            sockaddr_in from{};
            int fromLen = sizeof(from);

            int received = recvfrom(
                socketHandle,
                reinterpret_cast<char*>(buffer),
                sizeof(buffer),
                0,
                reinterpret_cast<sockaddr*>(&from),
                &fromLen);

            if (received <= 0)
                break;

            if (received < 24)
                continue;

            if (memcmp(buffer, "NS2P2P1", 7) != 0)
                continue;

            QueuedPacket packet{};

            uint32_t size = 0;

            memcpy(&packet.sender, buffer + 8, sizeof(CSteamID));
            memcpy(&packet.channel, buffer + 16, sizeof(int));
            memcpy(&size, buffer + 20, sizeof(uint32_t));

            if (size > static_cast<uint32_t>(received - 24))
                continue;

            packet.data.resize(size);

            memcpy(
                packet.data.data(),
                buffer + 24,
                size);
            QueuePacket(packet.sender, packet.data.data(), static_cast<uint32_t>(packet.data.size()), packet.channel);

            Logger::Info(
                "NS2SteamNetworking: packet received size=" +
                std::to_string(size));
        }
    }

public:
    bool SendP2PPacket(
        CSteamID steamIDRemote,
        const void* data,
        uint32_t size,
        EP2PSend sendType,
        int channel)
    {
        NSR_UNUSED(sendType);

        if (!data || size == 0)
            return false;

        if (!EnsureSocket())
            return false;

        std::vector<uint8_t> packet;
        packet.resize(24 + size);

        memcpy(packet.data(), "NS2P2P1", 7);
        memcpy(packet.data() + 8, &steamIDRemote, sizeof(CSteamID));
        memcpy(packet.data() + 16, &channel, sizeof(int));
        memcpy(packet.data() + 20, &size, sizeof(uint32_t));
        memcpy(packet.data() + 24, data, size);

        int sent = sendto(
            socketHandle,
            reinterpret_cast<const char*>(packet.data()),
            static_cast<int>(packet.size()),
            0,
            reinterpret_cast<sockaddr*>(&serverAddress),
            sizeof(serverAddress));

        QueuePacket(steamIDRemote ? steamIDRemote : SteamIDManager::GetLocalSteamID(), data, size, channel);

        Logger::Info(
            "NS2SteamNetworking: SendP2PPacket emulated size=" +
            std::to_string(size) +
            " channel=" +
            std::to_string(channel) +
            " udp=" +
            std::to_string(sent > 0 ? 1 : 0));

        return true;
    }

    bool SendP2PPacket(
        CSteamID steamIDRemote,
        const void* data,
        uint32_t size,
        EP2PSend sendType)
    {
        return SendP2PPacket(
            steamIDRemote,
            data,
            size,
            sendType,
            0);
    }

    bool IsP2PPacketAvailable(uint32_t* size, int channel)
    {
        Poll();

        std::lock_guard<std::mutex> lock(packetMutex);

        for (const QueuedPacket& packet : packets)
        {
            if (packet.channel == channel)
            {
                if (size)
                    *size = static_cast<uint32_t>(packet.data.size());

                return true;
            }
        }

        if (size)
            *size = 0;

        return false;
    }

    bool IsP2PPacketAvailable(uint32_t* size)
    {
        return IsP2PPacketAvailable(size, 0);
    }

    bool ReadP2PPacket(
        void* dest,
        uint32_t destSize,
        uint32_t* msgSize,
        CSteamID* remote,
        int channel)
    {
        Poll();

        std::lock_guard<std::mutex> lock(packetMutex);

        for (auto it = packets.begin(); it != packets.end(); ++it)
        {
            if (it->channel != channel)
                continue;

            uint32_t size = static_cast<uint32_t>(it->data.size());

            if (msgSize)
                *msgSize = size;

            if (remote)
                *remote = it->sender;

            if (!dest || destSize < size)
                return false;

            memcpy(dest, it->data.data(), size);

            packets.erase(it);

            Logger::Info(
                "NS2SteamNetworking: ReadP2PPacket size=" +
                std::to_string(size));

            return true;
        }

        if (msgSize)
            *msgSize = 0;

        return false;
    }

    bool ReadP2PPacket(
        void* dest,
        uint32_t destSize,
        uint32_t* msgSize,
        CSteamID* remote)
    {
        return ReadP2PPacket(dest, destSize, msgSize, remote, 0);
    }

    bool AcceptP2PSessionWithUser(CSteamID steamIDRemote)
    {
        Logger::Info(
            "NS2SteamNetworking: AcceptP2PSessionWithUser " +
            std::to_string(steamIDRemote));

        return true;
    }

    bool CloseP2PSessionWithUser(CSteamID steamIDRemote)
    {
        Logger::Info(
            "NS2SteamNetworking: CloseP2PSessionWithUser " +
            std::to_string(steamIDRemote));

        return true;
    }

    bool CloseP2PChannelWithUser(CSteamID steamIDRemote, int channel)
    {
        Logger::Info(
            "NS2SteamNetworking: CloseP2PChannelWithUser " +
            std::to_string(steamIDRemote) +
            " channel=" +
            std::to_string(channel));

        return true;
    }

    bool GetP2PSessionState(
        CSteamID steamIDRemote,
        P2PSessionState_t* state)
    {
        NSR_UNUSED(steamIDRemote);

        if (!state)
            return false;

        state->m_bConnectionActive = 1;
        state->m_bConnecting = 0;
        state->m_eP2PSessionError = 0;
        state->m_bUsingRelay = 0;
        state->m_nBytesQueuedForSend = 0;
        state->m_nPacketsQueuedForSend = 0;
        state->m_nRemoteIP = 0x0100007F;
        state->m_nRemotePort = NS2_SERVER_PORT;

        return true;
    }

    bool AllowP2PPacketRelay(bool allow)
    {
        Logger::Info(
            std::string("NS2SteamNetworking: AllowP2PPacketRelay ") +
            (allow ? "true" : "false"));

        return true;
    }

    SNetListenSocket_t CreateListenSocket(
        int virtualPort,
        uint32_t ip,
        uint16_t port,
        bool relay)
    {
        NSR_UNUSED(virtualPort);
        NSR_UNUSED(ip);
        NSR_UNUSED(port);
        NSR_UNUSED(relay);

        Logger::Info("NS2SteamNetworking: CreateListenSocket");
        return 1;
    }

    SNetSocket_t CreateP2PConnectionSocket(
        CSteamID target,
        int virtualPort,
        int timeout,
        bool relay)
    {
        NSR_UNUSED(virtualPort);
        NSR_UNUSED(timeout);
        NSR_UNUSED(relay);

        Logger::Info(
            "NS2SteamNetworking: CreateP2PConnectionSocket " +
            std::to_string(target));

        return 1;
    }

    bool DestroySocket(SNetSocket_t socket, bool notifyRemote)
    {
        NSR_UNUSED(socket);
        NSR_UNUSED(notifyRemote);

        Logger::Info("NS2SteamNetworking: DestroySocket");
        return true;
    }

    bool DestroyListenSocket(SNetListenSocket_t socket, bool notifyRemote)
    {
        NSR_UNUSED(socket);
        NSR_UNUSED(notifyRemote);

        Logger::Info("NS2SteamNetworking: DestroyListenSocket");
        return true;
    }

    int GetMaxPacketSize(SNetSocket_t socket)
    {
        NSR_UNUSED(socket);
        return 1200;
    }
};

class NS2SteamNetworkingMessages
{
public:
    int SendMessageToUser(
        const void* identity,
        const void* data,
        uint32_t size,
        int flags,
        int channel)
    {
        NSR_UNUSED(identity);
        NSR_UNUSED(flags);

        Logger::Info(
            "NS2SteamNetworkingMessages: SendMessageToUser size=" +
            std::to_string(size));

        return 1;
    }
};

class NS2SteamNetworkingSockets
{
public:
    bool InitAuthentication()
    {
        Logger::Info("NS2SteamNetworkingSockets: InitAuthentication");
        return true;
    }
};

class NS2SteamNetworkingUtils
{
public:
    bool InitializeRelayAccess()
    {
        Logger::Info("NS2SteamNetworkingUtils: InitializeRelayAccess");
        return true;
    }

    int GetRelayNetworkStatus(void* details)
    {
        NSR_UNUSED(details);

        Logger::Info("NS2SteamNetworkingUtils: GetRelayNetworkStatus");
        return 1;
    }
};

static NS2SteamNetworking g_Networking;
static NS2SteamNetworkingMessages g_Messages;
static NS2SteamNetworkingSockets g_Sockets;
static NS2SteamNetworkingUtils g_Utils;

extern "C" void* __cdecl NS2Revived_SteamNetworking()
{
    Logger::Info("NS2Revived_SteamNetworking returned");
    return &g_Networking;
}

extern "C" void* __cdecl NS2Revived_SteamGameServerNetworking()
{
    Logger::Info("NS2Revived_SteamGameServerNetworking returned");
    return &g_Networking;
}

extern "C" void* __cdecl NS2Revived_SteamNetworkingMessages()
{
    Logger::Info("NS2Revived_SteamNetworkingMessages returned");
    return &g_Messages;
}

extern "C" void* __cdecl NS2Revived_SteamNetworkingSockets()
{
    Logger::Info("NS2Revived_SteamNetworkingSockets returned");
    return &g_Sockets;
}

extern "C" void* __cdecl NS2Revived_SteamGameServerNetworkingSockets()
{
    Logger::Info("NS2Revived_SteamGameServerNetworkingSockets returned");
    return &g_Sockets;
}

extern "C" void* __cdecl NS2Revived_SteamNetworkingUtils()
{
    Logger::Info("NS2Revived_SteamNetworkingUtils returned");
    return &g_Utils;
}
