#include "StdInc.h"
#include "ModRedirector.h"
#include "Logger.h"

namespace
{
    bool g_Enabled = true;
    std::string g_GameFolder;
    std::string g_ModsFolder;
    std::vector<ModRedirector::RedirectEntry> g_Redirects;

    std::string ToLower(std::string text)
    {
        std::transform(text.begin(), text.end(), text.begin(),
            [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        return text;
    }

    std::string NormalizeSlashes(std::string text)
    {
        std::replace(text.begin(), text.end(), '\\', '/');
        return text;
    }

    std::string GetExeFolder()
    {
        char path[MAX_PATH]{};
        GetModuleFileNameA(nullptr, path, MAX_PATH);
        return std::filesystem::path(path).parent_path().string();
    }

    std::string StripModPrefix(const std::string& relativePath)
    {
        std::string path = NormalizeSlashes(relativePath);
        std::string lower = ToLower(path);

        const char* prefixes[] =
        {
            "override/",
            "overrides/",
            "loose/",
            "files/",
            "raw/",
            "data/"
        };

        for (const char* prefix : prefixes)
        {
            std::string p = prefix;
            if (lower.rfind(p, 0) == 0)
                return path.substr(p.size());
        }

        return path;
    }

    bool ShouldRegister(const std::filesystem::path& path)
    {
        std::string ext = ToLower(path.extension().string());

        static const std::vector<std::string> allowed =
        {
            ".cpk", ".xfbin", ".nud", ".nut", ".anm", ".mot", ".seq",
            ".acb", ".awb", ".adx", ".hca", ".bin", ".dat", ".dds",
            ".png", ".jpg", ".jpeg", ".tga", ".wav", ".ogg", ".txt",
            ".xml", ".lua", ".json", ".ini"
        };

        return std::find(allowed.begin(), allowed.end(), ext) != allowed.end();
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

        Logger::Info("ModRedirector mods folder: " + g_ModsFolder);
        return true;
    }

    void Shutdown()
    {
        g_Redirects.clear();
        Logger::Info("ModRedirector shutdown");
    }

    void Scan()
    {
        g_Redirects.clear();

        if (g_ModsFolder.empty())
            Init();

        if (!std::filesystem::exists(g_ModsFolder))
            return;

        try
        {
            for (const auto& entry : std::filesystem::recursive_directory_iterator(g_ModsFolder))
            {
                if (!entry.is_regular_file())
                    continue;

                const auto& path = entry.path();
                if (!ShouldRegister(path))
                    continue;

                RedirectEntry redirect{};
                redirect.ReplacementPath = path.string();
                redirect.SourceName = path.filename().string();
                redirect.IsPackage = ToLower(path.extension().string()) == ".cpk";

                std::string relative = NormalizeSlashes(std::filesystem::relative(path, g_ModsFolder).string());
                redirect.VirtualPath = StripModPrefix(relative);

                if (redirect.IsPackage)
                {
                    // Allow both full virtual path and same-filename CPK replacement lookup.
                    redirect.VirtualPath = path.filename().string();
                }

                g_Redirects.push_back(redirect);
            }
        }
        catch (const std::exception& e)
        {
            Logger::Error(std::string("ModRedirector scan exception: ") + e.what());
        }

        Logger::Info("ModRedirector scan finished. Redirects: " + std::to_string(g_Redirects.size()));
    }

    void DumpToLog()
    {
        Logger::Info("ModRedirector dump begin");
        for (const auto& redirect : g_Redirects)
            Logger::Info("redirect virtual=" + redirect.VirtualPath + " replacement=" + redirect.ReplacementPath + (redirect.IsPackage ? " package=1" : ""));
        Logger::Info("ModRedirector dump end");
    }

    bool IsEnabled()
    {
        return g_Enabled;
    }

    void SetEnabled(bool enabled)
    {
        g_Enabled = enabled;
    }

    bool ResolvePath(const std::string& requestedVirtualPath, std::string& outReplacementPath)
    {
        if (!g_Enabled)
            return false;

        std::string wanted = ToLower(NormalizeSlashes(requestedVirtualPath));
        std::filesystem::path wantedPath(wanted);
        std::string wantedName = ToLower(wantedPath.filename().string());

        for (const auto& redirect : g_Redirects)
        {
            std::string key = ToLower(NormalizeSlashes(redirect.VirtualPath));

            if (key == wanted || (redirect.IsPackage && key == wantedName))
            {
                outReplacementPath = redirect.ReplacementPath;
                return true;
            }
        }

        return false;
    }

    const std::vector<RedirectEntry>& GetRedirects()
    {
        return g_Redirects;
    }

    const std::string& GetModsFolder()
    {
        return g_ModsFolder;
    }
}
