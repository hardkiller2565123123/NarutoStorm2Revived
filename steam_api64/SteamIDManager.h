#pragma once
#include "StdInc.h"

namespace SteamIDManager
{
    void Init();

    CSteamID GetLocalSteamID();
    CSteamID GetHostSteamID();
    CSteamID GetLobbyOwnerSteamID();

    void SetLocalSteamID(CSteamID id);
    void SetHostSteamID(CSteamID id);
    void SetLobbyOwnerSteamID(CSteamID id);

    uint64_t ToUint64(CSteamID id);
}