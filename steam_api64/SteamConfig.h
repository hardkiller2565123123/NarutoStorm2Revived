#pragma once
#include "StdInc.h"

namespace SteamConfig
{
    enum class Mode
    {
        Offline,
        CustomServer,
        SteamPassthrough
    };

    struct Config
    {
        Mode CurrentMode = Mode::Offline;
        std::string PersonaName = "NS2Revived";
        uint64_t SteamID = 76561199006065889ull;
        std::string CustomServerHost = "127.0.0.1";
        int CustomServerPort = 27015;
        bool BlockOfficialTraffic = true;
        bool LanOnly = true;
        bool LogPackets = true;
        bool EnableLocalStorage = true;
        bool EnableLocalStats = true;
        bool EnableLocalMatchmaking = true;
    };

    bool Init();
    bool Load();
    bool Save();

    const Config& Get();
    Config& Mutable();

    bool IsOffline();
    bool IsCustomServer();
    bool ShouldBlockOfficialTraffic();
    bool IsLanOnly();

    const std::string& GetPath();
    const char* ModeName(Mode mode);
}
