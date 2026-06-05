#include "StdInc.h"
#include "SteamConfig.h"
#include "Logger.h"

namespace
{
    SteamConfig::Config g_Config;
    std::string g_Path;

    std::string GetExeFolder()
    {
        char path[MAX_PATH]{};
        GetModuleFileNameA(nullptr, path, MAX_PATH);
        return std::filesystem::path(path).parent_path().string();
    }

    std::string ReadString(const char* key, const char* fallback)
    {
        char buffer[512]{};
        GetPrivateProfileStringA("steam", key, fallback, buffer, sizeof(buffer), g_Path.c_str());
        return buffer;
    }

    int ReadInt(const char* key, int fallback)
    {
        return GetPrivateProfileIntA("steam", key, fallback, g_Path.c_str());
    }

    void WriteString(const char* key, const std::string& value)
    {
        WritePrivateProfileStringA("steam", key, value.c_str(), g_Path.c_str());
    }

    void WriteInt(const char* key, int value)
    {
        WritePrivateProfileStringA("steam", key, std::to_string(value).c_str(), g_Path.c_str());
    }

    SteamConfig::Mode ParseMode(std::string value)
    {
        std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c){ return (char)std::tolower(c); });
        if (value == "custom" || value == "customserver" || value == "server")
            return SteamConfig::Mode::CustomServer;
        if (value == "steam" || value == "passthrough")
            return SteamConfig::Mode::SteamPassthrough;
        return SteamConfig::Mode::Offline;
    }
}

namespace SteamConfig
{
    bool Init()
    {
        g_Path = (std::filesystem::path(GetExeFolder()) / "NartuoStorm2Revived" / "steam_config.ini").string();
        try { std::filesystem::create_directories(std::filesystem::path(g_Path).parent_path()); } catch (...) {}
        Load();
        Logger::Info(std::string("SteamConfig initialized mode=") + ModeName(g_Config.CurrentMode) + " path=" + g_Path);
        return true;
    }

    bool Load()
    {
        if (g_Path.empty())
            g_Path = (std::filesystem::path(GetExeFolder()) / "NartuoStorm2Revived" / "steam_config.ini").string();

        if (!std::filesystem::exists(g_Path))
            Save();

        g_Config.CurrentMode = ParseMode(ReadString("mode", "offline"));
        g_Config.PersonaName = ReadString("persona", "NS2Revived");
        g_Config.SteamID = std::strtoull(ReadString("steamid", "76561199006065889").c_str(), nullptr, 10);
        g_Config.CustomServerHost = ReadString("custom_server", "127.0.0.1");
        g_Config.CustomServerPort = ReadInt("custom_port", 27015);
        g_Config.BlockOfficialTraffic = ReadInt("block_official", 1) != 0;
        g_Config.LanOnly = ReadInt("lan_only", 1) != 0;
        g_Config.LogPackets = ReadInt("log_packets", 1) != 0;
        g_Config.EnableLocalStorage = ReadInt("local_storage", 1) != 0;
        g_Config.EnableLocalStats = ReadInt("local_stats", 1) != 0;
        g_Config.EnableLocalMatchmaking = ReadInt("local_matchmaking", 1) != 0;
        return true;
    }

    bool Save()
    {
        if (g_Path.empty())
            g_Path = (std::filesystem::path(GetExeFolder()) / "NartuoStorm2Revived" / "steam_config.ini").string();
        try { std::filesystem::create_directories(std::filesystem::path(g_Path).parent_path()); } catch (...) {}
        WriteString("mode", ModeName(g_Config.CurrentMode));
        WriteString("persona", g_Config.PersonaName);
        WriteString("steamid", std::to_string(g_Config.SteamID));
        WriteString("custom_server", g_Config.CustomServerHost);
        WriteInt("custom_port", g_Config.CustomServerPort);
        WriteInt("block_official", g_Config.BlockOfficialTraffic ? 1 : 0);
        WriteInt("lan_only", g_Config.LanOnly ? 1 : 0);
        WriteInt("log_packets", g_Config.LogPackets ? 1 : 0);
        WriteInt("local_storage", g_Config.EnableLocalStorage ? 1 : 0);
        WriteInt("local_stats", g_Config.EnableLocalStats ? 1 : 0);
        WriteInt("local_matchmaking", g_Config.EnableLocalMatchmaking ? 1 : 0);
        return true;
    }

    const Config& Get() { return g_Config; }
    Config& Mutable() { return g_Config; }
    bool IsOffline() { return g_Config.CurrentMode == Mode::Offline; }
    bool IsCustomServer() { return g_Config.CurrentMode == Mode::CustomServer; }
    bool ShouldBlockOfficialTraffic() { return g_Config.BlockOfficialTraffic; }
    bool IsLanOnly() { return g_Config.LanOnly; }
    const std::string& GetPath() { return g_Path; }

    const char* ModeName(Mode mode)
    {
        switch (mode)
        {
        case Mode::CustomServer: return "custom";
        case Mode::SteamPassthrough: return "steam";
        default: return "offline";
        }
    }
}
