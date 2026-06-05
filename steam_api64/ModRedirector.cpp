#include "StdInc.h"
#include "ModRedirector.h"
#include "Logger.h"

namespace
{
    std::string g_GameFolder;
    std::string g_ModsFolder;
    std::vector<ModRedirector::OverrideEntry> g_Overrides;

    std::string ToLower(std::string s)
    {
        std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c){ return (char)std::tolower(c); });
        return s;
    }

    std::string Normalize(std::string s)
    {
        std::replace(s.begin(), s.end(), '\\', '/');
        while (!s.empty() && (s[0] == '/' || s[0] == '.')) s.erase(s.begin());
        return s;
    }

    std::string GetExeFolder()
    {
        char path[MAX_PATH]{};
        GetModuleFileNameA(nullptr, path, MAX_PATH);
        return std::filesystem::path(path).parent_path().string();
    }

    std::string StripPrefix(std::string path)
    {
        path = Normalize(path);
        std::string lower = ToLower(path);
        const char* prefixes[] = { "override/", "overrides/", "loose/", "files/", "raw/", "data/" };
        for (const char* p : prefixes)
        {
            std::string pre = p;
            if (lower.rfind(pre, 0) == 0)
                return path.substr(pre.size());
        }
        return path;
    }
}

namespace ModRedirector
{
    bool Init()
    {
        g_GameFolder = GetExeFolder();
        g_ModsFolder = (std::filesystem::path(g_GameFolder) / "NartuoStorm2Revived" / "mods").string();
        try
        {
            std::filesystem::create_directories(g_ModsFolder);
            std::filesystem::create_directories(std::filesystem::path(g_ModsFolder) / "override");
            std::filesystem::create_directories(std::filesystem::path(g_ModsFolder) / "cpk");
        }
        catch (...) {}
        Logger::Info("ModRedirector initialized: " + g_ModsFolder);
        return true;
    }

    void Scan()
    {
        if (g_ModsFolder.empty()) Init();
        g_Overrides.clear();
        if (!std::filesystem::exists(g_ModsFolder)) return;

        try
        {
            for (const auto& item : std::filesystem::recursive_directory_iterator(g_ModsFolder))
            {
                if (!item.is_regular_file()) continue;
                std::string rel = Normalize(std::filesystem::relative(item.path(), g_ModsFolder).string());
                std::string virt = StripPrefix(rel);
                OverrideEntry entry{};
                entry.VirtualPath = virt;
                entry.FullPath = item.path().string();
                entry.SourceName = rel;
                entry.Enabled = true;
                g_Overrides.push_back(entry);
            }
        }
        catch (const std::exception& e)
        {
            Logger::Error(std::string("ModRedirector scan failed: ") + e.what());
        }
        Logger::Info("ModRedirector overrides found: " + std::to_string(g_Overrides.size()));
    }

    void Shutdown()
    {
        g_Overrides.clear();
        Logger::Info("ModRedirector shutdown");
    }

    bool Resolve(const std::string& virtualPath, std::string& outFullPath)
    {
        std::string key = ToLower(Normalize(virtualPath));
        for (const auto& e : g_Overrides)
        {
            if (!e.Enabled) continue;
            if (ToLower(Normalize(e.VirtualPath)) == key)
            {
                outFullPath = e.FullPath;
                return true;
            }
        }
        return false;
    }

    bool HasOverride(const std::string& virtualPath)
    {
        std::string temp;
        return Resolve(virtualPath, temp);
    }

    const std::vector<OverrideEntry>& GetOverrides() { return g_Overrides; }
    const std::string& GetModsFolder() { return g_ModsFolder; }
}
