#include "StdInc.h"
#include "AssetConflictScanner.h"
#include "AssetBrowser.h"
#include "AssetNotification.h"
#include "Logger.h"

#include <filesystem>
#include <fstream>
#include <unordered_map>

namespace
{
    std::vector<AssetConflictScanner::Conflict> g_Conflicts;

    std::string Normalize(std::string s)
    {
        std::replace(s.begin(), s.end(), '\\', '/');
        while (!s.empty() && (s[0] == '/' || s[0] == '.')) s.erase(s.begin());
        return s;
    }

    std::string Lower(std::string s)
    {
        std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c){ return static_cast<char>(std::tolower(c)); });
        return s;
    }
}

namespace AssetConflictScanner
{
    bool Init(){ return true; }
    void Shutdown(){ g_Conflicts.clear(); }

    void Rebuild()
    {
        g_Conflicts.clear();
        std::filesystem::path modsRoot = AssetBrowser::GetModsFolder();
        if (!std::filesystem::exists(modsRoot)) return;

        std::unordered_map<std::string, std::vector<std::string>> providers;

        for (const auto& item : std::filesystem::recursive_directory_iterator(modsRoot))
        {
            if (!item.is_regular_file()) continue;
            std::string rel = Normalize(std::filesystem::relative(item.path(), modsRoot).string());
            std::string lower = Lower(rel);
            const char* prefixes[] = { "assets/", "override/", "overrides/", "files/", "raw/", "data/" };
            for (const char* p : prefixes)
            {
                std::string pref = p;
                if (lower.rfind(pref, 0) == 0)
                {
                    rel = rel.substr(pref.size());
                    break;
                }
            }
            providers[Lower(rel)].push_back(item.path().string());
        }

        for (const auto& pair : providers)
        {
            if (pair.second.size() < 2) continue;
            Conflict c{};
            c.VirtualPath = pair.first;
            c.Providers = pair.second;
            g_Conflicts.push_back(c);
        }

        AssetNotification::PushInfo("Conflict Scanner", "Conflicts found: " + std::to_string(g_Conflicts.size()));
        Logger::Info("ConflictScanner conflicts=" + std::to_string(g_Conflicts.size()));
    }

    const std::vector<Conflict>& GetConflicts(){ return g_Conflicts; }

    bool ExportReport(const std::string& outputPath)
    {
        std::filesystem::create_directories(std::filesystem::path(outputPath).parent_path());
        std::ofstream f(outputPath, std::ios::trunc);
        if (!f.is_open()) return false;

        for (const auto& c : g_Conflicts)
        {
            f << "CONFLICT: " << c.VirtualPath << "\\n";
            for (const auto& p : c.Providers)
                f << "  - " << p << "\\n";
            f << "\\n";
        }
        return true;
    }
}
