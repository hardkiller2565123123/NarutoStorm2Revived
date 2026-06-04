#pragma once
#include "StdInc.h"

struct NS2CallResult
{
    SteamAPICall_t Call = 0;
    int CallbackID = 0;
    bool Failed = false;
    std::vector<uint8_t> Data;
};

namespace SteamCallResultManager
{
    void Init();

    SteamAPICall_t CreateCallResult(int callbackID, const void* data, size_t size, bool failed = false);
    bool GetCallResult(SteamAPICall_t call, NS2CallResult& outResult);
    void RemoveCallResult(SteamAPICall_t call);
}