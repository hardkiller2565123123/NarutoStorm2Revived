#pragma once
#include "StdInc.h"

namespace LuaManager
{
    enum class ScriptSource
    {
        GameKnown,
        Autorun,
        Override,
        Dump,
        Unknown
    };

    struct LuaScriptEntry
    {
        std::string Name;
        std::string RelativePath;
        std::string FullPath;
        std::string VirtualGamePath;
        ScriptSource Source = ScriptSource::Unknown;
        uint64_t Size = 0;
        bool ExistsOnDisk = false;
        bool Enabled = true;
    };

    bool Init();
    void Shutdown();

    void Scan();
    void Rescan();

    const std::vector<LuaScriptEntry>& GetScripts();
    std::vector<LuaScriptEntry> GetScriptsBySource(ScriptSource source);

    std::string GetRootFolder();
    std::string GetAutorunFolder();
    std::string GetOverrideFolder();
    std::string GetDumpFolder();
    std::string GetConsoleHistoryPath();

    bool CreateFolders();

    bool AddKnownGamePath(const std::string& gamePath);
    void AddDefaultKnownGamePaths();
    bool DumpKnownGamePaths();

    bool IsLuaFile(const std::string& path);
    const char* SourceName(ScriptSource source);

    bool ReadScriptText(const LuaScriptEntry& script, std::string& outText, size_t maxBytes = 1024 * 256);
    bool WriteOverrideScript(const std::string& virtualGamePath, const std::string& text);

    bool QueueConsoleCommand(const std::string& command);
    const std::vector<std::string>& GetConsoleLog();
    void ClearConsoleLog();

    bool RequestReload();
    bool ConsumeReloadRequest();

    bool AutorunEnabled();
    void SetAutorunEnabled(bool enabled);

    bool OverridesEnabled();
    void SetOverridesEnabled(bool enabled);

    bool ConsoleEnabled();
    void SetConsoleEnabled(bool enabled);
}
