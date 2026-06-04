#include "StdInc.h"
#include "AssetBrowser.h"
#include "ModRedirector.h"
#include "Logger.h"

#include <fstream>
#include <sstream>
#include <iomanip>

namespace
{
    std::string g_GameFolder;
    std::string g_ModsFolder;
    std::string g_DumpFolder;
    std::string g_ToolsFolder;
    std::vector<AssetBrowser::AssetEntry> g_Assets;
    std::vector<AssetBrowser::DumpEntry> g_DumpedFiles;
    std::vector<CpkArchive::CpkInfo> g_LoadedPackages;

    std::string ToLower(std::string text)
    {
        std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        return text;
    }

    std::string NormalizeSlashes(std::string text)
    {
        std::replace(text.begin(), text.end(), '\\', '/');
        return text;
    }

    std::string SanitizePath(std::string text)
    {
        text = NormalizeSlashes(text);
        while (!text.empty() && (text[0] == '/' || text[0] == '.'))
            text.erase(text.begin());
        for (char& c : text)
        {
            if (c == ':' || c == '*' || c == '?' || c == '"' || c == '<' || c == '>' || c == '|')
                c = '_';
        }
        return text;
    }

    std::string StripKnownModPrefix(const std::string& relativePath)
    {
        std::string path = NormalizeSlashes(relativePath);
        std::string lower = ToLower(path);
        const char* prefixes[] = { "override/", "overrides/", "loose/", "files/", "raw/", "data/" };
        for (const char* prefix : prefixes)
        {
            std::string p = prefix;
            if (lower.rfind(p, 0) == 0)
                return path.substr(p.size());
        }
        return path;
    }

    std::string GetExeFolder()
    {
        char path[MAX_PATH]{};
        GetModuleFileNameA(nullptr, path, MAX_PATH);
        return std::filesystem::path(path).parent_path().string();
    }

    AssetBrowser::AssetType DetectTypeFromStrings(const std::string& extRaw, const std::string& nameRaw, const std::string& fullRaw)
    {
        std::string ext = ToLower(extRaw);
        std::string name = ToLower(nameRaw);
        std::string full = ToLower(NormalizeSlashes(fullRaw));

        if (ext == ".cpk") return AssetBrowser::AssetType::Package;
        if (ext == ".dat" || ext == ".bin" || ext == ".txt" || ext == ".xml" || ext == ".lua" || ext == ".json" || ext == ".ini" || ext == ".csv") return AssetBrowser::AssetType::Data;
        if (ext == ".xfbin" || ext == ".nud")
        {
            if (full.find("stage") != std::string::npos || full.find("stg") != std::string::npos || full.find("map") != std::string::npos || full.find("battle/st") != std::string::npos)
                return AssetBrowser::AssetType::Stage;
            if (full.find("char") != std::string::npos || full.find("player") != std::string::npos || full.find("character") != std::string::npos || name.rfind("pl", 0) == 0 || name.find("1p") != std::string::npos)
                return AssetBrowser::AssetType::Character;
            return AssetBrowser::AssetType::Model;
        }
        if (ext == ".nut" || ext == ".dds" || ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".tga") return AssetBrowser::AssetType::Texture;
        if (ext == ".anm" || ext == ".mot" || ext == ".seq") return AssetBrowser::AssetType::Animation;
        if (ext == ".acb" || ext == ".awb" || ext == ".adx" || ext == ".wav" || ext == ".ogg" || ext == ".hca") return AssetBrowser::AssetType::Audio;
        return AssetBrowser::AssetType::Unknown;
    }

    AssetBrowser::AssetType DetectType(const std::filesystem::path& path)
    {
        return DetectTypeFromStrings(path.extension().string(), path.filename().string(), path.string());
    }

    bool ShouldInclude(const std::filesystem::path& path)
    {
        std::string ext = ToLower(path.extension().string());
        static const std::vector<std::string> allowed =
        {
            ".cpk", ".xfbin", ".nud", ".nut", ".anm", ".mot", ".seq",
            ".acb", ".awb", ".adx", ".hca", ".bin", ".dat", ".dds",
            ".png", ".jpg", ".jpeg", ".tga", ".wav", ".ogg", ".txt",
            ".xml", ".lua", ".json", ".ini", ".csv"
        };
        return std::find(allowed.begin(), allowed.end(), ext) != allowed.end();
    }

    std::filesystem::path BuildDumpPathForAsset(const AssetBrowser::AssetEntry& asset)
    {
        std::filesystem::path out = g_DumpFolder;
        out /= AssetBrowser::SourceName(asset.Source);
        out /= AssetBrowser::TypeName(asset.Type);
        std::string relative = !asset.VirtualPath.empty() ? asset.VirtualPath : asset.RelativePath;
        relative = SanitizePath(relative.empty() ? asset.Name : relative);
        out /= std::filesystem::path(relative);
        return out;
    }

    void AddAssetForFile(const std::filesystem::path& root, const std::filesystem::path& path, AssetBrowser::AssetSource source)
    {
        AssetBrowser::AssetEntry asset{};
        asset.Name = path.filename().string();
        asset.FullPath = path.string();
        asset.RelativePath = NormalizeSlashes(std::filesystem::relative(path, root).string());
        asset.Extension = ToLower(path.extension().string());
        asset.Type = DetectType(path);
        asset.Source = source;
        try { asset.Size = std::filesystem::file_size(path); } catch (...) {}
        if (source == AssetBrowser::AssetSource::Game)
            asset.VirtualPath = NormalizeSlashes(asset.RelativePath);
        else if (source == AssetBrowser::AssetSource::Mod)
        {
            asset.VirtualPath = StripKnownModPrefix(asset.RelativePath);
            std::string lowerRel = ToLower(asset.RelativePath);
            asset.IsLooseOverride = lowerRel.rfind("override/", 0) == 0 || lowerRel.rfind("overrides/", 0) == 0 || lowerRel.rfind("loose/", 0) == 0 || lowerRel.rfind("files/", 0) == 0;
        }
        else
        {
            asset.VirtualPath = NormalizeSlashes(asset.RelativePath);
            asset.IsDumped = true;
            asset.DumpFullPath = asset.FullPath;
        }
        g_Assets.push_back(asset);

        if (asset.Type == AssetBrowser::AssetType::Package && source != AssetBrowser::AssetSource::Dump)
        {
            CpkArchive::CpkInfo info;
            if (CpkArchive::Load(asset.FullPath, info))
            {
                g_LoadedPackages.push_back(info);
                for (const auto& e : info.Entries)
                {
                    AssetBrowser::AssetEntry inner{};
                    inner.Name = e.Name;
                    inner.FullPath = asset.FullPath;
                    inner.RelativePath = std::filesystem::path(asset.Name).string() + ":/" + e.VirtualPath;
                    inner.VirtualPath = e.VirtualPath;
                    inner.Extension = ToLower(std::filesystem::path(e.Name).extension().string());
                    inner.Type = DetectTypeFromStrings(inner.Extension, e.Name, e.VirtualPath);
                    inner.Source = AssetBrowser::AssetSource::PackageInner;
                    inner.Size = e.Size;
                    inner.IsVirtualPackageEntry = true;
                    inner.CanDump = e.HasPhysicalData;
                    inner.CpkEntryData = e;
                    g_Assets.push_back(inner);
                }
            }
        }
    }

    void ScanFolder(const std::filesystem::path& root, AssetBrowser::AssetSource source)
    {
        if (root.empty() || !std::filesystem::exists(root)) return;
        try
        {
            for (const auto& entry : std::filesystem::recursive_directory_iterator(root))
            {
                if (!entry.is_regular_file()) continue;
                const auto& path = entry.path();
                if (source != AssetBrowser::AssetSource::Dump && !g_DumpFolder.empty())
                {
                    std::string p = ToLower(NormalizeSlashes(path.string()));
                    std::string d = ToLower(NormalizeSlashes(g_DumpFolder));
                    if (p.rfind(d, 0) == 0) continue;
                }
                if (!ShouldInclude(path)) continue;
                AddAssetForFile(root, path, source);
            }
        }
        catch (const std::exception& e)
        {
            Logger::Error(std::string("AssetBrowser scan exception: ") + e.what());
        }
    }

    void LinkOverridesAndDumps()
    {
        std::unordered_map<std::string, std::string> modByVirtual;
        std::unordered_map<std::string, std::string> dumpByVirtual;
        for (const auto& asset : g_Assets)
        {
            std::string key = ToLower(NormalizeSlashes(asset.VirtualPath));
            if (key.empty()) continue;
            if (asset.Source == AssetBrowser::AssetSource::Mod)
            {
                modByVirtual[key] = asset.FullPath;
                modByVirtual[ToLower(asset.Name)] = asset.FullPath;
            }
            else if (asset.Source == AssetBrowser::AssetSource::Dump)
            {
                dumpByVirtual[key] = asset.FullPath;
                dumpByVirtual[ToLower(asset.Name)] = asset.FullPath;
            }
        }
        for (auto& asset : g_Assets)
        {
            if (asset.Source != AssetBrowser::AssetSource::Game && asset.Source != AssetBrowser::AssetSource::PackageInner) continue;
            std::string key = ToLower(NormalizeSlashes(asset.VirtualPath));
            auto mod = modByVirtual.find(key);
            if (mod == modByVirtual.end()) mod = modByVirtual.find(ToLower(asset.Name));
            if (mod != modByVirtual.end()) { asset.HasModOverride = true; asset.OverrideFullPath = mod->second; }
            auto dump = dumpByVirtual.find(key);
            if (dump == dumpByVirtual.end()) dump = dumpByVirtual.find(ToLower(asset.Name));
            if (dump != dumpByVirtual.end()) { asset.IsDumped = true; asset.DumpFullPath = dump->second; }
        }
    }

    void ScanDumpListOnly()
    {
        g_DumpedFiles.clear();
        if (g_DumpFolder.empty() || !std::filesystem::exists(g_DumpFolder)) return;
        try
        {
            for (const auto& entry : std::filesystem::recursive_directory_iterator(g_DumpFolder))
            {
                if (!entry.is_regular_file()) continue;
                const auto& path = entry.path();
                AssetBrowser::DumpEntry dump{};
                dump.Name = path.filename().string();
                dump.FullPath = path.string();
                dump.RelativePath = NormalizeSlashes(std::filesystem::relative(path, g_DumpFolder).string());
                dump.Extension = ToLower(path.extension().string());
                dump.Type = DetectType(path);
                try { dump.Size = entry.file_size(); } catch (...) {}
                g_DumpedFiles.push_back(dump);
            }
        }
        catch (...) {}
    }

    bool CopyFileTo(const std::string& from, const std::filesystem::path& to)
    {
        try
        {
            std::filesystem::create_directories(to.parent_path());
            std::filesystem::copy_file(from, to, std::filesystem::copy_options::overwrite_existing);
            return true;
        }
        catch (const std::exception& e)
        {
            Logger::Error(std::string("AssetBrowser copy failed: ") + e.what());
            return false;
        }
    }
}

namespace AssetBrowser
{
    bool Init()
    {
        g_GameFolder = GetExeFolder();
        g_ModsFolder = (std::filesystem::path(g_GameFolder) / "NartuoStorm2Revived" / "mods").string();
        g_DumpFolder = (std::filesystem::path(g_GameFolder) / "NartuoStorm2Revived" / "Dump").string();
        g_ToolsFolder = (std::filesystem::path(g_GameFolder) / "NartuoStorm2Revived" / "tools").string();
        try
        {
            std::filesystem::create_directories(g_ModsFolder);
            std::filesystem::create_directories(g_DumpFolder);
            std::filesystem::create_directories(g_ToolsFolder);
        }
        catch (...) {}
        Logger::Info("AssetBrowser dump folder: " + g_DumpFolder);
        return true;
    }

    void Scan()
    {
        if (g_GameFolder.empty()) Init();
        g_Assets.clear();
        g_LoadedPackages.clear();
        ScanFolder(g_GameFolder, AssetSource::Game);
        ScanFolder(g_ModsFolder, AssetSource::Mod);
        ScanFolder(g_DumpFolder, AssetSource::Dump);
        ScanDumpListOnly();
        LinkOverridesAndDumps();
        ModRedirector::Scan();
        Logger::Info("AssetBrowser scan finished. Assets found: " + std::to_string(g_Assets.size()));
    }

    void ScanDumpFolder()
    {
        ScanDumpListOnly();
    }

    void DumpToLog()
    {
        Logger::Info("AssetBrowser dump begin");
        for (const auto& asset : g_Assets)
        {
            Logger::Info("[" + std::string(SourceName(asset.Source)) + "][" + TypeName(asset.Type) + "] " + asset.RelativePath + " size=" + std::to_string(asset.Size));
        }
        Logger::Info("AssetBrowser dump end");
    }

    bool ExportCsv(const std::string& relativeOrFullPath)
    {
        std::filesystem::path out = relativeOrFullPath.empty() ? (std::filesystem::path(g_DumpFolder) / "asset_index.csv") : std::filesystem::path(relativeOrFullPath);
        if (!out.is_absolute()) out = std::filesystem::path(g_DumpFolder) / out;
        try
        {
            std::filesystem::create_directories(out.parent_path());
            std::ofstream file(out);
            file << "Source,Type,Size,CanDump,VirtualPath,FullPath,Override,Dumped\n";
            for (const auto& a : g_Assets)
                file << SourceName(a.Source) << ',' << TypeName(a.Type) << ',' << a.Size << ',' << (a.CanDump ? 1 : 0) << ",\"" << a.VirtualPath << "\",\"" << a.FullPath << "\",\"" << a.OverrideFullPath << "\",\"" << a.DumpFullPath << "\"\n";
            Logger::Info("AssetBrowser CSV exported: " + out.string());
            return true;
        }
        catch (...) { return false; }
    }

    bool DumpAsset(const AssetEntry& asset, bool tryExtractArchives)
    {
        std::string outPath;
        if (asset.IsVirtualPackageEntry)
        {
            bool ok = CpkArchive::DumpEntry(asset.CpkEntryData, g_DumpFolder, outPath);
            if (ok) Logger::Info("Dumped CPK entry: " + outPath);
            else Logger::Error(std::string("CPK entry dump failed: ") + CpkArchive::GetLastErrorText());
            ScanDumpListOnly();
            return ok;
        }
        if (asset.Type == AssetType::Package)
        {
            CpkArchive::CpkInfo info;
            if (CpkArchive::Load(asset.FullPath, info))
            {
                bool ok = CpkArchive::DumpArchive(info, g_DumpFolder, outPath);
                if (ok) Logger::Info("Dumped package copy: " + outPath);
                if (tryExtractArchives)
                {
                    int count = 0;
                    for (const auto& e : info.Entries)
                    {
                        std::string entryOut;
                        if (CpkArchive::DumpEntry(e, g_DumpFolder, entryOut)) ++count;
                    }
                    Logger::Info("Built-in CPK extract attempted. Extracted entries: " + std::to_string(count));
                }
                ScanDumpListOnly();
                return ok;
            }
        }
        auto out = BuildDumpPathForAsset(asset);
        bool ok = CopyFileTo(asset.FullPath, out);
        if (ok) Logger::Info("Dumped asset: " + out.string());
        ScanDumpListOnly();
        return ok;
    }

    bool DumpAssetByIndex(int index, bool tryExtractArchives)
    {
        if (index < 0 || index >= static_cast<int>(g_Assets.size())) return false;
        return DumpAsset(g_Assets[index], tryExtractArchives);
    }

    int DumpAllAssets(bool tryExtractArchives)
    {
        int count = 0;
        for (const auto& a : g_Assets)
            if (DumpAsset(a, tryExtractArchives)) ++count;
        return count;
    }

    int DumpAssetsByType(AssetType type, bool tryExtractArchives)
    {
        int count = 0;
        for (const auto& a : g_Assets)
            if (a.Type == type && DumpAsset(a, tryExtractArchives)) ++count;
        return count;
    }

    bool DumpXfbinInfo(const AssetEntry& asset)
    {
        try
        {
            std::filesystem::path out = std::filesystem::path(g_DumpFolder) / "XFBIN_Info" / (std::filesystem::path(asset.Name).stem().string() + ".txt");
            std::filesystem::create_directories(out.parent_path());
            std::ofstream file(out);
            file << BuildAssetInfoText(asset);
            file << "\n\nFull XFBIN decoding is planned. Current build records metadata and allows raw dump/hex preview.\n";
            return true;
        }
        catch (...) { return false; }
    }

    bool ExtractCpkIfToolExists(const AssetEntry& asset)
    {
        return DumpAsset(asset, true);
    }

    bool LoadCpkEntries(const AssetEntry& packageAsset)
    {
        CpkArchive::CpkInfo info;
        if (!CpkArchive::Load(packageAsset.FullPath, info)) return false;
        g_LoadedPackages.push_back(info);
        return true;
    }

    std::string BuildAssetInfoText(const AssetEntry& asset)
    {
        std::ostringstream oss;
        oss << "Name: " << asset.Name << "\n";
        oss << "Type: " << TypeName(asset.Type) << "\n";
        oss << "Source: " << SourceName(asset.Source) << "\n";
        oss << "Size: " << asset.Size << "\n";
        oss << "Extension: " << asset.Extension << "\n";
        oss << "Virtual: " << asset.VirtualPath << "\n";
        oss << "Relative: " << asset.RelativePath << "\n";
        oss << "Full: " << asset.FullPath << "\n";
        oss << "Can Dump: " << (asset.CanDump ? "yes" : "no/list-only") << "\n";
        if (asset.HasModOverride) oss << "Override: " << asset.OverrideFullPath << "\n";
        if (asset.IsDumped) oss << "Dumped: " << asset.DumpFullPath << "\n";
        if (asset.IsVirtualPackageEntry)
        {
            oss << "Archive Entry Offset: " << asset.CpkEntryData.Offset << "\n";
            oss << "Archive Entry Size: " << asset.CpkEntryData.Size << "\n";
            oss << "Archive Entry Extract Size: " << asset.CpkEntryData.ExtractSize << "\n";
        }
        return oss.str();
    }

    std::string ReadSmallTextFile(const std::string& path, size_t maxBytes)
    {
        std::ifstream file(path, std::ios::binary);
        if (!file) return {};
        std::string text(maxBytes, '\0');
        file.read(text.data(), static_cast<std::streamsize>(maxBytes));
        text.resize(static_cast<size_t>(file.gcount()));
        return text;
    }

    bool IsTextPreviewable(const AssetEntry& asset)
    {
        return asset.Extension == ".txt" || asset.Extension == ".ini" || asset.Extension == ".json" || asset.Extension == ".xml" || asset.Extension == ".csv" || asset.Extension == ".lua";
    }
    bool IsTexturePreviewable(const AssetEntry& asset)
    {
        return asset.Extension == ".dds" || asset.Extension == ".png" || asset.Extension == ".jpg" || asset.Extension == ".jpeg" || asset.Extension == ".nut";
    }
    bool IsModelPreviewable(const AssetEntry& asset)
    {
        return asset.Extension == ".xfbin" || asset.Extension == ".nud";
    }

    const std::vector<AssetEntry>& GetAssets() { return g_Assets; }
    const std::vector<DumpEntry>& GetDumpedFiles() { return g_DumpedFiles; }
    const std::vector<CpkArchive::CpkInfo>& GetLoadedPackages() { return g_LoadedPackages; }

    AssetStats GetStats()
    {
        AssetStats s{};
        s.TotalAssets = static_cast<int>(g_Assets.size());
        for (const auto& a : g_Assets)
        {
            if (a.Source == AssetSource::Game) ++s.GameAssets;
            if (a.Source == AssetSource::Mod) ++s.ModAssets;
            if (a.Source == AssetSource::Dump) ++s.DumpedAssets;
            if (a.Source == AssetSource::PackageInner) ++s.PackageInnerAssets;
            if (a.HasModOverride) ++s.OverrideAssets;
            switch (a.Type)
            {
            case AssetType::Package: ++s.PackageAssets; break;
            case AssetType::Texture: ++s.TextureAssets; break;
            case AssetType::Audio: ++s.AudioAssets; break;
            case AssetType::Stage: ++s.StageAssets; break;
            case AssetType::Character: ++s.CharacterAssets; break;
            case AssetType::Model: ++s.ModelAssets; break;
            default: break;
            }
            if (a.Extension == ".xfbin") ++s.XfbinAssets;
        }
        return s;
    }

    const std::string& GetGameFolder() { return g_GameFolder; }
    const std::string& GetModsFolder() { return g_ModsFolder; }
    const std::string& GetDumpFolder() { return g_DumpFolder; }
    const std::string& GetToolsFolder() { return g_ToolsFolder; }

    const char* TypeName(AssetType type)
    {
        switch (type)
        {
        case AssetType::Package: return "Package";
        case AssetType::Model: return "Model";
        case AssetType::Texture: return "Texture";
        case AssetType::Animation: return "Animation";
        case AssetType::Audio: return "Audio";
        case AssetType::Stage: return "Stage";
        case AssetType::Character: return "Character";
        case AssetType::Data: return "Data";
        case AssetType::CpkEntry: return "CPK Entry";
        default: return "Unknown";
        }
    }
    const char* SourceName(AssetSource source)
    {
        switch (source)
        {
        case AssetSource::Game: return "Game";
        case AssetSource::Mod: return "Mod";
        case AssetSource::Dump: return "Dump";
        case AssetSource::PackageInner: return "CPK";
        default: return "Unknown";
        }
    }
}
