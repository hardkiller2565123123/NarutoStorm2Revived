#include "StdInc.h"
#include "LuaManager.h"
#include "Logger.h"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <mutex>

namespace
{
    std::mutex g_Mutex;

    std::vector<LuaManager::LuaScriptEntry> g_Scripts;
    std::vector<std::string> g_KnownGamePaths;
    std::vector<std::string> g_ConsoleLog;

    bool g_ReloadRequested = false;
    bool g_AutorunEnabled = true;
    bool g_OverridesEnabled = true;
    bool g_ConsoleEnabled = true;

    std::string NormalizeSlashes(std::string value)
    {
        std::replace(value.begin(), value.end(), '\\', '/');
        return value;
    }

    std::string ToLower(std::string value)
    {
        std::transform(
            value.begin(),
            value.end(),
            value.begin(),
            [](unsigned char c)
            {
                return static_cast<char>(std::tolower(c));
            });

        return value;
    }

    std::string BaseFolder()
    {
        char path[MAX_PATH]{};
        GetModuleFileNameA(nullptr, path, MAX_PATH);

        std::filesystem::path exePath(path);
        return (exePath.parent_path() / "NartuoStorm2Revived").string();
    }

    std::string RelativeTo(const std::filesystem::path& path, const std::filesystem::path& root)
    {
        std::error_code ec;
        auto rel = std::filesystem::relative(path, root, ec);
        if (ec)
            return path.filename().string();

        return NormalizeSlashes(rel.string());
    }

    void AddConsoleLineLocked(const std::string& line)
    {
        g_ConsoleLog.push_back(line);

        if (g_ConsoleLog.size() > 300)
            g_ConsoleLog.erase(g_ConsoleLog.begin(), g_ConsoleLog.begin() + 50);
    }

    void AddDiskScriptsFromFolder(
        const std::filesystem::path& folder,
        LuaManager::ScriptSource source,
        const std::string& virtualPrefix)
    {
        if (!std::filesystem::exists(folder))
            return;

        for (const auto& item : std::filesystem::recursive_directory_iterator(folder))
        {
            if (!item.is_regular_file())
                continue;

            std::filesystem::path filePath = item.path();
            std::string lower = ToLower(filePath.string());

            if (lower.size() < 4 || lower.substr(lower.size() - 4) != ".lua")
                continue;

            LuaManager::LuaScriptEntry entry{};
            entry.Name = filePath.filename().string();
            entry.RelativePath = RelativeTo(filePath, folder);
            entry.FullPath = filePath.string();
            entry.VirtualGamePath = NormalizeSlashes(virtualPrefix + entry.RelativePath);
            entry.Source = source;
            entry.ExistsOnDisk = true;
            entry.Enabled = true;

            std::error_code ec;
            entry.Size = static_cast<uint64_t>(std::filesystem::file_size(filePath, ec));
            if (ec)
                entry.Size = 0;

            g_Scripts.push_back(entry);
        }
    }

    bool WriteTextFile(const std::string& path, const std::string& text)
    {
        std::filesystem::create_directories(std::filesystem::path(path).parent_path());

        std::ofstream file(path, std::ios::binary);
        if (!file.is_open())
            return false;

        file.write(text.data(), static_cast<std::streamsize>(text.size()));
        return true;
    }
}

namespace LuaManager
{
    bool Init()
    {
        CreateFolders();
        AddDefaultKnownGamePaths();
        Scan();

        Logger::Info("LuaManager initialized");
        return true;
    }

    void Shutdown()
    {
        std::lock_guard<std::mutex> lock(g_Mutex);
        g_Scripts.clear();
        g_ConsoleLog.clear();

        Logger::Info("LuaManager shutdown");
    }

    std::string GetRootFolder()
    {
        return (std::filesystem::path(BaseFolder()) / "lua").string();
    }

    std::string GetAutorunFolder()
    {
        return (std::filesystem::path(GetRootFolder()) / "autorun").string();
    }

    std::string GetOverrideFolder()
    {
        return (std::filesystem::path(GetRootFolder()) / "overrides").string();
    }

    std::string GetDumpFolder()
    {
        return (std::filesystem::path(GetRootFolder()) / "dumps").string();
    }

    std::string GetConsoleHistoryPath()
    {
        return (std::filesystem::path(GetRootFolder()) / "console_history.txt").string();
    }

    bool CreateFolders()
    {
        std::error_code ec;
        std::filesystem::create_directories(GetRootFolder(), ec);
        std::filesystem::create_directories(GetAutorunFolder(), ec);
        std::filesystem::create_directories(GetOverrideFolder(), ec);
        std::filesystem::create_directories(GetDumpFolder(), ec);
        return true;
    }

    bool IsLuaFile(const std::string& path)
    {
        std::string lower = ToLower(path);
        return lower.size() >= 4 && lower.substr(lower.size() - 4) == ".lua";
    }

    const char* SourceName(ScriptSource source)
    {
        switch (source)
        {
        case ScriptSource::GameKnown: return "Game";
        case ScriptSource::Autorun: return "Autorun";
        case ScriptSource::Override: return "Override";
        case ScriptSource::Dump: return "Dump";
        default: return "Unknown";
        }
    }

    bool AddKnownGamePath(const std::string& gamePath)
    {
        if (!IsLuaFile(gamePath))
            return false;

        std::string normalized = NormalizeSlashes(gamePath);

        auto it = std::find(g_KnownGamePaths.begin(), g_KnownGamePaths.end(), normalized);
        if (it != g_KnownGamePaths.end())
            return false;

        g_KnownGamePaths.push_back(normalized);
        return true;
    }

    void AddDefaultKnownGamePaths()
    {
        if (!g_KnownGamePaths.empty())
            return;

        AddKnownGamePath("data/boss/b06/b06_drm01.lua");
        AddKnownGamePath("data/boss/b07/b07_drm1.lua");
        AddKnownGamePath("data/boss/b07/b07_drm2.lua");
        AddKnownGamePath("data/boss/b07/b07_drm3.lua");
        AddKnownGamePath("data/boss/b07/b07_drm4.lua");
        AddKnownGamePath("data/boss/b08/b08_drm2.lua");
        AddKnownGamePath("data/boss/b09/b09_drm01.lua");

        for (int d = 1; d <= 9; ++d)
        {
            for (int n = 1; n <= 5; ++n)
            {
                char buffer[128]{};
                sprintf_s(buffer, "data/ia/d%02d/d%02d_%d.lua", d, d, n);
                AddKnownGamePath(buffer);
            }
        }

        const char* chars[] =
        {
            "p2nrt", "p2nrv", "p2nrw", "p2sas", "p2ssk", "p2sai",
            "p2sra", "p2kks", "p2skt", "p2hin", "p2nej", "p2lee",
            "p2gar", "p2ino", "p2cho", "p2shk", "p2kib", "p2sno",
            "p2tnn", "p2jry", "p2itc", "p2kis", "p2hid", "p2kzu1",
            "p2kzu2", "p2pn1", "p2pn2", "p2pn3", "p2pn4", "p2pn5", "p2pn6"
        };

        for (const char* c : chars)
        {
            AddKnownGamePath(std::string("data/no_release/pt/script/") + c + ".lua");
            AddKnownGamePath(std::string("data/no_release/pt/script/") + c + "_l.lua");
        }
    }

    void Scan()
    {
        std::lock_guard<std::mutex> lock(g_Mutex);

        CreateFolders();
        g_Scripts.clear();

        for (const auto& path : g_KnownGamePaths)
        {
            LuaScriptEntry entry{};
            entry.Name = std::filesystem::path(path).filename().string();
            entry.RelativePath = path;
            entry.VirtualGamePath = path;
            entry.FullPath.clear();
            entry.Source = ScriptSource::GameKnown;
            entry.Size = 0;
            entry.ExistsOnDisk = false;
            entry.Enabled = true;
            g_Scripts.push_back(entry);
        }

        AddDiskScriptsFromFolder(GetAutorunFolder(), ScriptSource::Autorun, "autorun/");
        AddDiskScriptsFromFolder(GetOverrideFolder(), ScriptSource::Override, "");
        AddDiskScriptsFromFolder(GetDumpFolder(), ScriptSource::Dump, "dumps/");

        AddConsoleLineLocked("Lua scan complete. Scripts indexed: " + std::to_string(g_Scripts.size()));
        Logger::Info("LuaManager scan complete. Scripts=" + std::to_string(g_Scripts.size()));
    }

    void Rescan()
    {
        Scan();
    }

    const std::vector<LuaScriptEntry>& GetScripts()
    {
        return g_Scripts;
    }

    std::vector<LuaScriptEntry> GetScriptsBySource(ScriptSource source)
    {
        std::vector<LuaScriptEntry> result;

        std::lock_guard<std::mutex> lock(g_Mutex);

        for (const auto& script : g_Scripts)
        {
            if (script.Source == source)
                result.push_back(script);
        }

        return result;
    }

    bool DumpKnownGamePaths()
    {
        CreateFolders();

        std::ostringstream ss;
        ss << "-- NS2 Revived known Lua game paths\n";
        ss << "-- Generated from embedded game string research and LuaManager defaults.\n\n";

        for (const auto& path : g_KnownGamePaths)
            ss << path << "\n";

        std::string outPath = (std::filesystem::path(GetDumpFolder()) / "known_lua_paths.txt").string();

        bool ok = WriteTextFile(outPath, ss.str());

        if (ok)
            Logger::Info("LuaManager dumped known Lua paths: " + outPath);
        else
            Logger::Error("LuaManager failed to dump known Lua paths: " + outPath);

        return ok;
    }

    bool ReadScriptText(const LuaScriptEntry& script, std::string& outText, size_t maxBytes)
    {
        outText.clear();

        if (script.FullPath.empty())
        {
            outText =
                "-- Known game Lua path only. File is inside game archives or CPK data.\n"
                "-- Virtual path:\n" +
                script.VirtualGamePath +
                "\n\n"
                "-- Dump or override this script path to edit it here.\n";
            return true;
        }

        std::ifstream file(script.FullPath, std::ios::binary);
        if (!file.is_open())
            return false;

        file.seekg(0, std::ios::end);
        size_t size = static_cast<size_t>(std::max<std::streamoff>(0, file.tellg()));
        file.seekg(0, std::ios::beg);

        size = std::min(size, maxBytes);
        outText.resize(size);

        if (size > 0)
            file.read(outText.data(), static_cast<std::streamsize>(size));

        return true;
    }

    bool WriteOverrideScript(const std::string& virtualGamePath, const std::string& text)
    {
        if (virtualGamePath.empty())
            return false;

        std::filesystem::path outPath = std::filesystem::path(GetOverrideFolder()) / NormalizeSlashes(virtualGamePath);

        bool ok = WriteTextFile(outPath.string(), text);

        if (ok)
        {
            Logger::Info("LuaManager wrote override script: " + outPath.string());
            Rescan();
        }
        else
        {
            Logger::Error("LuaManager failed to write override script: " + outPath.string());
        }

        return ok;
    }

    bool QueueConsoleCommand(const std::string& command)
    {
        if (command.empty())
            return false;

        std::lock_guard<std::mutex> lock(g_Mutex);

        AddConsoleLineLocked("> " + command);
        AddConsoleLineLocked("Console execution is staged. Hook luaL_loadbuffer/lua_pcall before running inside the game VM.");

        std::ofstream history(GetConsoleHistoryPath(), std::ios::app | std::ios::binary);
        if (history.is_open())
            history << command << "\n";

        Logger::Info("Lua console command queued: " + command);
        return true;
    }

    const std::vector<std::string>& GetConsoleLog()
    {
        return g_ConsoleLog;
    }

    void ClearConsoleLog()
    {
        std::lock_guard<std::mutex> lock(g_Mutex);
        g_ConsoleLog.clear();
    }

    bool RequestReload()
    {
        g_ReloadRequested = true;
        Logger::Info("LuaManager reload requested");
        return true;
    }

    bool ConsumeReloadRequest()
    {
        if (!g_ReloadRequested)
            return false;

        g_ReloadRequested = false;
        return true;
    }

    bool AutorunEnabled()
    {
        return g_AutorunEnabled;
    }

    void SetAutorunEnabled(bool enabled)
    {
        g_AutorunEnabled = enabled;
    }

    bool OverridesEnabled()
    {
        return g_OverridesEnabled;
    }

    void SetOverridesEnabled(bool enabled)
    {
        g_OverridesEnabled = enabled;
    }

    bool ConsoleEnabled()
    {
        return g_ConsoleEnabled;
    }

    void SetConsoleEnabled(bool enabled)
    {
        g_ConsoleEnabled = enabled;
    }
}
