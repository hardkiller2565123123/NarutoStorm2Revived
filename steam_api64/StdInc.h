#pragma once

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define _CRT_SECURE_NO_WARNINGS

#include <Windows.h>
#include <Psapi.h>
#include <TlHelp32.h>
#include <WinSock2.h>
#include <Ws2tcpip.h>

#include <d3d11.h>
#include <dxgi.h>
#include <d2d1.h>
#include <dwrite.h>

#include <cstdint>
#include <string>
#include <sstream>
#include <fstream>
#include <iostream>
#include <cstdio>
#include <vector>
#include <array>
#include <map>
#include <unordered_map>
#include <mutex>
#include <thread>
#include <atomic>
#include <filesystem>
#include <queue>
#include <algorithm>

typedef uint32_t AppId_t;
typedef uint64_t SteamAPICall_t;
typedef int32_t HSteamPipe;
typedef int32_t HSteamUser;
typedef uint64_t CSteamID;

typedef int32_t CallbackMsg_t;
typedef int32_t ESteamAPIInitResult;

#include "MinHook.h"

#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")

#define NS2_VERSION "0.1.0"

#define NSR_UNUSED(x) (void)(x)

#ifndef SAFE_DELETE
#define SAFE_DELETE(x) \
    do                 \
    {                  \
        if (x)         \
        {              \
            delete x;  \
            x = nullptr; \
        }              \
    } while (0)
#endif

#ifndef SAFE_RELEASE
#define SAFE_RELEASE(x) \
    do                  \
    {                   \
        if (x)          \
        {               \
            x = nullptr; \
        }               \
    } while (0)
#endif

#ifndef ARRAYSIZE
#define ARRAYSIZE(x) (sizeof(x) / sizeof(x[0]))
#endif