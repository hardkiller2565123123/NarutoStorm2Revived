#include "StdInc.h"
#include "NetworkHooks.h"
#include "Logger.h"

#include <ws2tcpip.h>

#pragma comment(lib, "Ws2_32.lib")

typedef int (WSAAPI* connectFn)(
    SOCKET s,
    const sockaddr* name,
    int namelen);

typedef int (WSAAPI* WSAConnectFn)(
    SOCKET s,
    const sockaddr* name,
    int namelen,
    LPWSABUF lpCallerData,
    LPWSABUF lpCalleeData,
    LPQOS lpSQOS,
    LPQOS lpGQOS);

typedef int (WSAAPI* getaddrinfoFn)(
    PCSTR pNodeName,
    PCSTR pServiceName,
    const ADDRINFOA* pHints,
    PADDRINFOA* ppResult);

typedef INT(WSAAPI* GetAddrInfoWFn)(
    PCWSTR pNodeName,
    PCWSTR pServiceName,
    const ADDRINFOW* pHints,
    PADDRINFOW* ppResult);

static connectFn g_OriginalConnect = nullptr;
static WSAConnectFn g_OriginalWSAConnect = nullptr;
static getaddrinfoFn g_OriginalGetAddrInfo = nullptr;
static GetAddrInfoWFn g_OriginalGetAddrInfoW = nullptr;

static constexpr bool FORCE_BLOCK_ALL_TRAFFIC = true;

static std::string WideToUtf8(const wchar_t* text)
{
    if (!text)
        return "null";

    int needed = WideCharToMultiByte(
        CP_UTF8,
        0,
        text,
        -1,
        nullptr,
        0,
        nullptr,
        nullptr);

    if (needed <= 0)
        return "wide-conversion-failed";

    std::string result;
    result.resize(needed - 1);

    WideCharToMultiByte(
        CP_UTF8,
        0,
        text,
        -1,
        &result[0],
        needed,
        nullptr,
        nullptr);

    return result;
}

static bool ExtractIpAndPort(
    const sockaddr* addr,
    std::string& outIp,
    uint16_t& outPort)
{
    outIp.clear();
    outPort = 0;

    if (!addr)
        return false;

    char ip[INET6_ADDRSTRLEN]{};

    if (addr->sa_family == AF_INET)
    {
        const sockaddr_in* ipv4 =
            reinterpret_cast<const sockaddr_in*>(addr);

        if (!inet_ntop(
            AF_INET,
            const_cast<IN_ADDR*>(&ipv4->sin_addr),
            ip,
            sizeof(ip)))
        {
            return false;
        }

        outIp = ip;
        outPort = ntohs(ipv4->sin_port);
        return true;
    }

    if (addr->sa_family == AF_INET6)
    {
        const sockaddr_in6* ipv6 =
            reinterpret_cast<const sockaddr_in6*>(addr);

        if (!inet_ntop(
            AF_INET6,
            const_cast<IN6_ADDR*>(&ipv6->sin6_addr),
            ip,
            sizeof(ip)))
        {
            return false;
        }

        outIp = ip;
        outPort = ntohs(ipv6->sin6_port);
        return true;
    }

    return false;
}

static std::string FormatEndpoint(
    const std::string& ip,
    uint16_t port)
{
    if (ip.empty())
        return "unknown";

    return ip + ":" + std::to_string(port);
}

static int WSAAPI HookedConnect(
    SOCKET s,
    const sockaddr* name,
    int namelen)
{
    std::string ip;
    uint16_t port = 0;

    if (ExtractIpAndPort(name, ip, port))
    {
        Logger::Info(
            "FORCE BLOCK connect -> " +
            FormatEndpoint(ip, port));
    }
    else
    {
        Logger::Info("FORCE BLOCK connect -> unknown endpoint");
    }

    if (FORCE_BLOCK_ALL_TRAFFIC)
    {
        WSASetLastError(WSAECONNREFUSED);
        return SOCKET_ERROR;
    }

    return g_OriginalConnect
        ? g_OriginalConnect(s, name, namelen)
        : SOCKET_ERROR;
}

static int WSAAPI HookedWSAConnect(
    SOCKET s,
    const sockaddr* name,
    int namelen,
    LPWSABUF lpCallerData,
    LPWSABUF lpCalleeData,
    LPQOS lpSQOS,
    LPQOS lpGQOS)
{
    std::string ip;
    uint16_t port = 0;

    if (ExtractIpAndPort(name, ip, port))
    {
        Logger::Info(
            "FORCE BLOCK WSAConnect -> " +
            FormatEndpoint(ip, port));
    }
    else
    {
        Logger::Info("FORCE BLOCK WSAConnect -> unknown endpoint");
    }

    if (FORCE_BLOCK_ALL_TRAFFIC)
    {
        WSASetLastError(WSAECONNREFUSED);
        return SOCKET_ERROR;
    }

    return g_OriginalWSAConnect
        ? g_OriginalWSAConnect(
            s,
            name,
            namelen,
            lpCallerData,
            lpCalleeData,
            lpSQOS,
            lpGQOS)
        : SOCKET_ERROR;
}

static int WSAAPI HookedGetAddrInfo(
    PCSTR pNodeName,
    PCSTR pServiceName,
    const ADDRINFOA* pHints,
    PADDRINFOA* ppResult)
{
    std::string host =
        pNodeName ? pNodeName : "null";

    std::string service =
        pServiceName ? pServiceName : "null";

    Logger::Info(
        "FORCE BLOCK getaddrinfo -> host=" +
        host +
        " service=" +
        service);

    if (FORCE_BLOCK_ALL_TRAFFIC)
    {
        WSASetLastError(WSAHOST_NOT_FOUND);
        return WSAHOST_NOT_FOUND;
    }

    return g_OriginalGetAddrInfo
        ? g_OriginalGetAddrInfo(
            pNodeName,
            pServiceName,
            pHints,
            ppResult)
        : EAI_FAIL;
}

static INT WSAAPI HookedGetAddrInfoW(
    PCWSTR pNodeName,
    PCWSTR pServiceName,
    const ADDRINFOW* pHints,
    PADDRINFOW* ppResult)
{
    std::string host =
        WideToUtf8(pNodeName);

    std::string service =
        WideToUtf8(pServiceName);

    Logger::Info(
        "FORCE BLOCK GetAddrInfoW -> host=" +
        host +
        " service=" +
        service);

    if (FORCE_BLOCK_ALL_TRAFFIC)
    {
        WSASetLastError(WSAHOST_NOT_FOUND);
        return WSAHOST_NOT_FOUND;
    }

    return g_OriginalGetAddrInfoW
        ? g_OriginalGetAddrInfoW(
            pNodeName,
            pServiceName,
            pHints,
            ppResult)
        : EAI_FAIL;
}

static bool HookExport(
    HMODULE module,
    const char* exportName,
    void* detour,
    void** original)
{
    void* target =
        reinterpret_cast<void*>(
            GetProcAddress(module, exportName));

    if (!target)
    {
        Logger::Error(
            std::string("Network hook missing export: ") +
            exportName);

        return false;
    }

    MH_STATUS createStatus = MH_CreateHook(
        target,
        detour,
        original);

    if (createStatus != MH_OK &&
        createStatus != MH_ERROR_ALREADY_CREATED)
    {
        Logger::Error(
            std::string("Network hook create failed: ") +
            exportName);

        return false;
    }

    MH_STATUS enableStatus =
        MH_EnableHook(target);

    if (enableStatus != MH_OK &&
        enableStatus != MH_ERROR_ENABLED)
    {
        Logger::Error(
            std::string("Network hook enable failed: ") +
            exportName);

        return false;
    }

    Logger::Info(
        std::string("Network hook enabled: ") +
        exportName);

    return true;
}

namespace NetworkHooks
{
    bool Init()
    {
        HMODULE ws2 =
            GetModuleHandleA("Ws2_32.dll");

        if (!ws2)
            ws2 = LoadLibraryA("Ws2_32.dll");

        if (!ws2)
        {
            Logger::Error(
                "Failed to load Ws2_32.dll");

            return false;
        }

        Logger::Info(
            "Initializing network hooks with FORCE BLOCK ALL TRAFFIC enabled");

        HookExport(
            ws2,
            "connect",
            reinterpret_cast<void*>(&HookedConnect),
            reinterpret_cast<void**>(&g_OriginalConnect));

        HookExport(
            ws2,
            "WSAConnect",
            reinterpret_cast<void*>(&HookedWSAConnect),
            reinterpret_cast<void**>(&g_OriginalWSAConnect));

        HookExport(
            ws2,
            "getaddrinfo",
            reinterpret_cast<void*>(&HookedGetAddrInfo),
            reinterpret_cast<void**>(&g_OriginalGetAddrInfo));

        HookExport(
            ws2,
            "GetAddrInfoW",
            reinterpret_cast<void*>(&HookedGetAddrInfoW),
            reinterpret_cast<void**>(&g_OriginalGetAddrInfoW));

        Logger::Info("Network hooks initialized");
        Logger::Info("All game-process DNS and socket connections are blocked");

        return true;
    }

    void Shutdown()
    {
        Logger::Info("Network hooks shutdown");
    }
}