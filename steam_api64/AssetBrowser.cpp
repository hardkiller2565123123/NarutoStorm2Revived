#include "StdInc.h"
#include "AssetBrowser.h"
#include "ModRedirector.h"
#include "CpkArchive.h"
#include "Logger.h"

#include <filesystem>
#include <fstream>
#include <iomanip>
#include <mutex>
#include <atomic>
#include <thread>
#include <unordered_map>
#include <unordered_set>

namespace
{
    std::string g_GameFolder;
    std::string g_DataFolder;
    std::string g_ModsFolder;
    std::string g_DumpFolder;
    std::string g_ToolsFolder;
    std::string g_CacheFolder;

    std::vector<AssetBrowser::AssetEntry> g_Assets;
    std::vector<AssetBrowser::DumpEntry> g_DumpedFiles;

    std::mutex g_Mutex;
    std::thread g_ScanThread;

    std::atomic<bool> g_Scanning(false);
    std::atomic<bool> g_StopScan(false);
    std::atomic<int> g_ScanProgress(0);

    std::string g_ScanStatus = "Idle";
    bool g_CacheLoaded = false;

    std::string ToLower(std::string text)
    {
        std::transform(
            text.begin(),
            text.end(),
            text.begin(),
            [](unsigned char c)
            {
                return static_cast<char>(std::tolower(c));
            });

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

        return text;
    }

    bool StartsWithNoCase(const std::string& value, const std::string& prefix)
    {
        std::string v = ToLower(NormalizeSlashes(value));
        std::string p = ToLower(NormalizeSlashes(prefix));
        return v.rfind(p, 0) == 0;
    }

    void SetScanStatus(const std::string& status)
    {
        std::lock_guard<std::mutex> lock(g_Mutex);
        g_ScanStatus = status;
    }

    std::string GetExeFolder()
    {
        char path[MAX_PATH]{};
        GetModuleFileNameA(nullptr, path, MAX_PATH);
        return std::filesystem::path(path).parent_path().string();
    }

    std::string RelativeTo(const std::filesystem::path& file, const std::filesystem::path& root)
    {
        std::error_code ec;
        auto rel = std::filesystem::relative(file, root, ec);

        if (ec)
            return NormalizeSlashes(file.filename().string());

        return NormalizeSlashes(rel.string());
    }

    bool FileExists(const std::filesystem::path& path)
    {
        std::error_code ec;
        return std::filesystem::exists(path, ec);
    }

    uintmax_t FileSizeSafe(const std::filesystem::path& path)
    {
        std::error_code ec;
        return std::filesystem::file_size(path, ec);
    }

    bool CopyFileSafe(const std::filesystem::path& from, const std::filesystem::path& to)
    {
        try
        {
            std::filesystem::create_directories(to.parent_path());
            std::filesystem::copy_file(
                from,
                to,
                std::filesystem::copy_options::overwrite_existing);

            return true;
        }
        catch (const std::exception& e)
        {
            Logger::Error(std::string("AssetBrowser copy failed: ") + e.what());
            return false;
        }
    }

    std::vector<unsigned char> ReadBytes(const std::string& path, size_t maxBytes)
    {
        std::vector<unsigned char> bytes;

        std::ifstream file(path, std::ios::binary);

        if (!file.is_open())
            return bytes;

        file.seekg(0, std::ios::end);
        size_t size = static_cast<size_t>(std::max<std::streamoff>(0, file.tellg()));
        file.seekg(0, std::ios::beg);

        size = std::min(size, maxBytes);
        bytes.resize(size);

        if (size > 0)
            file.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(size));

        return bytes;
    }

    bool IsTextChar(unsigned char c)
    {
        return c == '\n' ||
            c == '\r' ||
            c == '\t' ||
            (c >= 32 && c < 127);
    }

    std::vector<std::string> ExtractAsciiStrings(const std::vector<unsigned char>& bytes, size_t minLen)
    {
        std::vector<std::string> out;
        std::string current;

        for (unsigned char c : bytes)
        {
            if (IsTextChar(c))
            {
                current.push_back(static_cast<char>(c));
            }
            else
            {
                if (current.size() >= minLen)
                    out.push_back(current);

                current.clear();
            }
        }

        if (current.size() >= minLen)
            out.push_back(current);

        if (out.size() > 512)
            out.resize(512);

        return out;
    }

    bool ContainsAny(const std::string& value, const std::vector<std::string>& terms)
    {
        for (const auto& term : terms)
        {
            if (value.find(term) != std::string::npos)
                return true;
        }

        return false;
    }

    AssetBrowser::AssetType DetectTypeFromPath(const std::filesystem::path& path)
    {
        std::string ext = ToLower(path.extension().string());
        std::string name = ToLower(path.filename().string());
        std::string full = ToLower(NormalizeSlashes(path.string()));

        if (ext == ".cpk")
            return AssetBrowser::AssetType::Package;

        if (ext == ".dds" || ext == ".nut" || ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".tga")
            return AssetBrowser::AssetType::Texture;

        if (ext == ".adx" || ext == ".acb" || ext == ".awb" || ext == ".wav" || ext == ".ogg")
            return AssetBrowser::AssetType::Audio;

        if (ext == ".anm" || ext == ".mot" || full.find("animation") != std::string::npos)
            return AssetBrowser::AssetType::Animation;

        if (ext == ".xfbin" || ext == ".nud")
        {
            if (ContainsAny(full, {
                    "stage",
                    "/stg",
                    "/st/",
                    "battle/st",
                    "map",
                    "field",
                    "arena"
                }))
            {
                return AssetBrowser::AssetType::Stage;
            }

            if (ContainsAny(full, {
                    "char",
                    "character",
                    "player",
                    "/pl/",
                    "/chr",
                    "costume",
                    "body",
                    "face"
                }) ||
                name.find("pl") == 0 ||
                name.find("1p") != std::string::npos ||
                name.find("2p") != std::string::npos)
            {
                return AssetBrowser::AssetType::Character;
            }

            return AssetBrowser::AssetType::Model;
        }

        if (ext == ".lua" ||
            ext == ".dat" ||
            ext == ".bin" ||
            ext == ".txt" ||
            ext == ".xml" ||
            ext == ".json" ||
            ext == ".ini" ||
            ext == ".csv")
        {
            return AssetBrowser::AssetType::Data;
        }

        return AssetBrowser::AssetType::Unknown;
    }

    bool ShouldIndexFile(const std::filesystem::path& path)
    {
        std::error_code ec;

        if (!std::filesystem::is_regular_file(path, ec))
            return false;

        uintmax_t size = std::filesystem::file_size(path, ec);

        if (ec)
            return false;

        // Skip tiny junk files
        if (size == 0)
            return false;

        return true;
    }

    std::string FindBestDataFolder()
    {
        std::filesystem::path game = GetExeFolder();

        std::vector<std::filesystem::path> candidates =
        {
            game / "data",
            game / "DATA",
            game / "data_win32",
            game / "data_win64",
            game / "disc" / "data",
            game / "native" / "data",
            game / "NartuoStorm2Revived" / "data"
        };

        for (const auto& candidate : candidates)
        {
            std::error_code ec;

            if (!std::filesystem::exists(candidate, ec))
                continue;

            if (!std::filesystem::is_directory(candidate, ec))
                continue;

            // Make sure it is not empty
            for (const auto& item : std::filesystem::directory_iterator(candidate, ec))
            {
                if (!ec)
                {
                    Logger::Info("AssetBrowser found data folder: " + candidate.string());
                    return candidate.string();
                }

                break;
            }
        }

        // fallback: scan game folder only for a folder named data*
        for (const auto& item : std::filesystem::directory_iterator(game))
        {
            if (!item.is_directory())
                continue;

            std::string name = item.path().filename().string();
            std::string lower = ToLower(name);

            if (lower.find("data") != std::string::npos)
            {
                Logger::Info("AssetBrowser found fallback data-like folder: " + item.path().string());
                return item.path().string();
            }
        }

        Logger::Error("AssetBrowser could not find data folder. Falling back to game folder: " + game.string());
        return game.string();
    }

    std::string StripKnownModPrefix(const std::string& relativePath)
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
            "data/",
            "cpk/",
            "packages/"
        };

        for (const char* prefix : prefixes)
        {
            std::string p = prefix;

            if (lower.rfind(p, 0) == 0)
                return path.substr(p.size());
        }

        return path;
    }

    void ScanInsideLooseFile(AssetBrowser::AssetEntry& asset)
    {
        if (asset.IsCpkEntry)
            return;

        if (asset.FullPath.empty() || !FileExists(asset.FullPath))
            return;

        size_t readLimit = 1024 * 1024;

        if (asset.Extension == ".xfbin")
            readLimit = 8 * 1024 * 1024;

        if (asset.Extension == ".lua" ||
            asset.Extension == ".txt" ||
            asset.Extension == ".json" ||
            asset.Extension == ".xml")
        {
            readLimit = 2 * 1024 * 1024;
        }

        std::vector<unsigned char> bytes = ReadBytes(asset.FullPath, readLimit);

        if (bytes.empty())
            return;

        asset.Strings = ExtractAsciiStrings(bytes, 4);

        for (const auto& raw : asset.Strings)
        {
            std::string s = ToLower(raw);

            if (ContainsAny(s, {
                    ".xfbin",
                    ".nud",
                    ".nut",
                    ".dds",
                    ".lua",
                    ".adx",
                    ".acb",
                    ".awb",
                    ".cpk"
                }))
            {
                if (std::find(asset.Dependencies.begin(), asset.Dependencies.end(), raw) == asset.Dependencies.end())
                    asset.Dependencies.push_back(raw);
            }

            if (ContainsAny(s, {
                    "nucc",
                    "chunk",
                    "model",
                    "texture",
                    "material",
                    "bone",
                    "camera",
                    "stage",
                    "player",
                    "script",
                    "lua",
                    "event"
                }))
            {
                if (std::find(asset.Markers.begin(), asset.Markers.end(), raw) == asset.Markers.end())
                    asset.Markers.push_back(raw);
            }
        }

        if (asset.Dependencies.size() > 256)
            asset.Dependencies.resize(256);

        if (asset.Markers.size() > 256)
            asset.Markers.resize(256);
    }

    void AddAssetUnlocked(const AssetBrowser::AssetEntry& asset)
    {
        g_Assets.push_back(asset);
    }

    void ScanDataLooseFile(
        const std::filesystem::path& file,
        const std::filesystem::path& dataRoot)
    {
        AssetBrowser::AssetEntry asset{};

        asset.Name = file.filename().string();
        asset.FullPath = file.string();
        asset.RelativePath = RelativeTo(file, dataRoot);
        asset.VirtualPath = NormalizeSlashes(asset.RelativePath);
        asset.Extension = ToLower(file.extension().string());
        asset.Type = DetectTypeFromPath(asset.RelativePath);
        asset.Source = AssetBrowser::AssetSource::Game;
        asset.Size = FileSizeSafe(file);
        asset.IsCpkEntry = false;

        ScanInsideLooseFile(asset);

        Logger::Info(
            "Asset: " +
            asset.RelativePath +
            " | Size=" +
            std::to_string(asset.Size));

        AddAssetUnlocked(asset);
        AddAssetUnlocked(asset);
    }

    void IndexCpkEntriesUnlocked(const AssetBrowser::AssetEntry& packageAsset)
    {
        CpkArchive::ArchiveInfo cpkInfo{};

        if (!CpkArchive::Load(packageAsset.FullPath, cpkInfo))
        {
            Logger::Error("AssetBrowser failed to index CPK: " + packageAsset.FullPath);
            return;
        }

        for (const auto& cpkEntry : cpkInfo.Entries)
        {
            AssetBrowser::AssetEntry embedded{};

            embedded.Name = cpkEntry.Name;
            embedded.FullPath = packageAsset.FullPath;
            embedded.RelativePath = packageAsset.RelativePath + ":" + NormalizeSlashes(cpkEntry.Path);
            embedded.VirtualPath = NormalizeSlashes(cpkEntry.Path);
            embedded.Extension = ToLower(std::filesystem::path(cpkEntry.Path).extension().string());
            embedded.Type = DetectTypeFromPath(cpkEntry.Path);
            embedded.Source = AssetBrowser::AssetSource::Game;
            embedded.Size = cpkEntry.Size;
            embedded.IsCpkEntry = true;
            embedded.ArchivePath = packageAsset.FullPath;
            embedded.ArchiveEntryPath = NormalizeSlashes(cpkEntry.Path);

            if (!embedded.ArchiveEntryPath.empty())
            {
                std::string lower = ToLower(embedded.ArchiveEntryPath);

                if (ContainsAny(lower, {
                        ".xfbin",
                        ".nud",
                        ".nut",
                        ".dds",
                        ".lua",
                        ".adx",
                        ".acb",
                        ".awb"
                    }))
                {
                    embedded.Dependencies.push_back(embedded.ArchiveEntryPath);
                }

                if (ContainsAny(lower, {
                        "nucc",
                        "model",
                        "texture",
                        "material",
                        "stage",
                        "player",
                        "script",
                        "event",
                        "camera"
                    }))
                {
                    embedded.Markers.push_back(embedded.ArchiveEntryPath);
                }
            }

            AddAssetUnlocked(embedded);
        }
    }

    void ScanDataFolderOnly()
    {
        std::filesystem::path dataRoot = g_DataFolder;
        Logger::Info("Data Root = " + dataRoot.string());

        if (!FileExists(dataRoot))
        {
            Logger::Error("AssetBrowser data folder not found: " + dataRoot.string());
            SetScanStatus("Data folder not found: " + dataRoot.string());
            return;
        }

        SetScanStatus("Scanning data folder: " + dataRoot.string());
        Logger::Info("AssetBrowser scanning DATA folder: " + dataRoot.string());

        std::vector<std::filesystem::path> files;
        std::error_code ec;

        for (const auto& entry : std::filesystem::recursive_directory_iterator(dataRoot, ec))
        {
            if (g_StopScan)
                return;

            if (ec)
            {
                Logger::Error("AssetBrowser recursive scan error: " + ec.message());
                break;
            }

            if (!entry.is_regular_file())
                continue;

            // IMPORTANT:
            // add everything, not just known extensions
            files.push_back(entry.path());

            Logger::Info(
                "AssetBrowser Indexed Assets: " +
                std::to_string(g_Assets.size()));
        }

        Logger::Info("AssetBrowser found files in data folder: " + std::to_string(files.size()));

        if (files.empty())
        {
            SetScanStatus("Data folder found but no files indexed: " + dataRoot.string());
            return;
        }

        {
            for (size_t i = 0; i < files.size(); ++i)
            {
                if (g_StopScan)
                    return;

                const auto& file = files[i];

                SetScanStatus("Indexing data: " + file.filename().string());

                AssetBrowser::AssetEntry asset{};

                asset.Name = file.filename().string();
                asset.FullPath = file.string();
                asset.RelativePath = RelativeTo(file, dataRoot);
                asset.VirtualPath = NormalizeSlashes(asset.RelativePath);
                asset.Extension = ToLower(file.extension().string());
                asset.Type = DetectTypeFromPath(asset.RelativePath);
                asset.Source = AssetBrowser::AssetSource::Game;
                asset.Size = FileSizeSafe(file);
                asset.IsCpkEntry = false;

                ScanInsideLooseFile(asset);

                Logger::Info(
                    "Asset: " +
                    asset.RelativePath +
                    " | Size=" +
                    std::to_string(asset.Size));

                {
                    std::lock_guard<std::mutex> lock(g_Mutex);
                    AddAssetUnlocked(asset);
                }

                if (!files.empty())
                    g_ScanProgress = static_cast<int>((static_cast<double>(i + 1) / static_cast<double>(files.size())) * 45.0);
            }
        }

        std::vector<AssetBrowser::AssetEntry> packages;

        {
            std::lock_guard<std::mutex> lock(g_Mutex);

            for (const auto& asset : g_Assets)
            {
                if (asset.Extension == ".cpk" && !asset.IsCpkEntry)
                    packages.push_back(asset);
            }
        }

        Logger::Info("AssetBrowser found CPK files: " + std::to_string(packages.size()));

        for (size_t i = 0; i < packages.size(); ++i)
        {
            if (g_StopScan)
                return;

            SetScanStatus("Indexing CPK: " + packages[i].Name);

            {
                std::lock_guard<std::mutex> lock(g_Mutex);
                IndexCpkEntriesUnlocked(packages[i]);
            }

            if (!packages.empty())
            {
                int cpkProgress = static_cast<int>((static_cast<double>(i + 1) / static_cast<double>(packages.size())) * 45.0);
                g_ScanProgress = 45 + cpkProgress;
            }
        }
    }

    void ScanModOverridesUnlocked()
    {
        std::filesystem::path modsRoot = g_ModsFolder;

        if (!FileExists(modsRoot))
            return;

        SetScanStatus("Indexing mod overrides...");

        std::error_code ec;

        for (const auto& entry : std::filesystem::recursive_directory_iterator(modsRoot, ec))
        {
            if (g_StopScan)
                return;

            if (ec)
                break;

            if (!entry.is_regular_file())
                continue;

            if (!ShouldIndexFile(entry.path()))
                continue;

            AssetBrowser::AssetEntry asset{};

            asset.Name = entry.path().filename().string();
            asset.FullPath = entry.path().string();
            asset.RelativePath = RelativeTo(entry.path(), modsRoot);
            asset.VirtualPath = StripKnownModPrefix(asset.RelativePath);
            asset.Extension = ToLower(entry.path().extension().string());
            asset.Type = DetectTypeFromPath(asset.VirtualPath);
            asset.Source = AssetBrowser::AssetSource::Mod;
            asset.Size = FileSizeSafe(entry.path());
            asset.IsLooseOverride = true;

            ScanInsideLooseFile(asset);

            AddAssetUnlocked(asset);
        }
    }

    void ScanDumpListOnlyUnlocked()
    {
        g_DumpedFiles.clear();

        if (!FileExists(g_DumpFolder))
            return;

        try
        {
            for (const auto& entry : std::filesystem::recursive_directory_iterator(g_DumpFolder))
            {
                if (!entry.is_regular_file())
                    continue;

                const auto& path = entry.path();

                if (!ShouldIndexFile(path) && ToLower(path.extension().string()) != ".txt")
                    continue;

                AssetBrowser::DumpEntry item{};

                item.Name = path.filename().string();
                item.FullPath = path.string();
                item.RelativePath = NormalizeSlashes(std::filesystem::relative(path, g_DumpFolder).string());
                item.Extension = ToLower(path.extension().string());
                item.Type = DetectTypeFromPath(path);
                item.Size = entry.file_size();

                g_DumpedFiles.push_back(item);
            }
        }
        catch (const std::exception& e)
        {
            Logger::Error(std::string("AssetBrowser dump scan exception: ") + e.what());
        }
    }

    void LinkOverridesAndDumpsUnlocked()
    {
        std::unordered_map<std::string, std::string> modByVirtual;
        std::unordered_map<std::string, std::string> dumpByVirtual;

        for (const auto& asset : g_Assets)
        {
            std::string key = ToLower(NormalizeSlashes(asset.VirtualPath));

            if (key.empty())
                continue;

            if (asset.Source == AssetBrowser::AssetSource::Mod)
            {
                modByVirtual[key] = asset.FullPath;
                modByVirtual[ToLower(asset.Name)] = asset.FullPath;
            }
        }

        for (const auto& dumped : g_DumpedFiles)
        {
            std::string key = ToLower(NormalizeSlashes(dumped.RelativePath));

            if (!key.empty())
                dumpByVirtual[key] = dumped.FullPath;

            dumpByVirtual[ToLower(dumped.Name)] = dumped.FullPath;
        }

        for (auto& asset : g_Assets)
        {
            if (asset.Source != AssetBrowser::AssetSource::Game)
                continue;

            std::string key = ToLower(NormalizeSlashes(asset.VirtualPath));
            std::string archiveKey = ToLower(NormalizeSlashes(asset.ArchiveEntryPath));
            std::string nameKey = ToLower(asset.Name);

            auto mod = modByVirtual.find(key);

            if (mod == modByVirtual.end() && !archiveKey.empty())
                mod = modByVirtual.find(archiveKey);

            if (mod == modByVirtual.end())
                mod = modByVirtual.find(nameKey);

            if (mod != modByVirtual.end())
            {
                asset.HasModOverride = true;
                asset.OverrideFullPath = mod->second;
            }

            auto dump = dumpByVirtual.find(key);

            if (dump == dumpByVirtual.end() && !archiveKey.empty())
                dump = dumpByVirtual.find(archiveKey);

            if (dump == dumpByVirtual.end())
                dump = dumpByVirtual.find(nameKey);

            if (dump != dumpByVirtual.end())
            {
                asset.IsDumped = true;
                asset.DumpFullPath = dump->second;
            }
        }
    }

    std::filesystem::path BuildDumpPathForAsset(const AssetBrowser::AssetEntry& asset)
    {
        std::filesystem::path out = g_DumpFolder;

        if (asset.IsCpkEntry && !asset.ArchiveEntryPath.empty())
        {
            out /= "CPK";
            out /= std::filesystem::path(asset.ArchivePath).stem();
            out /= SanitizePath(asset.ArchiveEntryPath);
            return out;
        }

        if (!asset.VirtualPath.empty())
        {
            out /= SanitizePath(asset.VirtualPath);
            return out;
        }

        if (!asset.RelativePath.empty())
        {
            out /= SanitizePath(asset.RelativePath);
            return out;
        }

        out /= asset.Name;
        return out;
    }

    bool LaunchToolAndWait(const std::filesystem::path& exe, const std::string& args)
    {
        if (!std::filesystem::exists(exe))
            return false;

        std::string command = "\"" + exe.string() + "\" " + args;

        STARTUPINFOA si{};
        PROCESS_INFORMATION pi{};
        si.cb = sizeof(si);
        si.dwFlags = STARTF_USESHOWWINDOW;
        si.wShowWindow = SW_HIDE;

        std::vector<char> cmd(command.begin(), command.end());
        cmd.push_back('\0');

        BOOL ok = CreateProcessA(
            nullptr,
            cmd.data(),
            nullptr,
            nullptr,
            FALSE,
            CREATE_NO_WINDOW,
            nullptr,
            nullptr,
            &si,
            &pi);

        if (!ok)
            return false;

        WaitForSingleObject(pi.hProcess, INFINITE);

        DWORD exitCode = 1;
        GetExitCodeProcess(pi.hProcess, &exitCode);

        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);

        return exitCode == 0;
    }

    void BuildStatsUnlocked(AssetBrowser::AssetStats& stats)
    {
        stats.TotalAssets = static_cast<int>(g_Assets.size());
        stats.DumpedAssets = static_cast<int>(g_DumpedFiles.size());

        for (const auto& asset : g_Assets)
        {
            if (asset.Source == AssetBrowser::AssetSource::Game)
                stats.GameAssets++;
            else if (asset.Source == AssetBrowser::AssetSource::Mod)
                stats.ModAssets++;

            if (asset.HasModOverride || asset.IsLooseOverride)
                stats.OverrideAssets++;

            if (asset.Type == AssetBrowser::AssetType::Package)
                stats.PackageAssets++;

            if (asset.Extension == ".xfbin")
                stats.XfbinAssets++;

            if (asset.Type == AssetBrowser::AssetType::Texture)
                stats.TextureAssets++;

            if (asset.Type == AssetBrowser::AssetType::Audio)
                stats.AudioAssets++;

            if (asset.Type == AssetBrowser::AssetType::Stage)
                stats.StageAssets++;

            if (asset.Type == AssetBrowser::AssetType::Character)
                stats.CharacterAssets++;

            if (asset.Type == AssetBrowser::AssetType::Model)
                stats.ModelAssets++;
        }
    }

    void FullScanWorker()
    {
        g_Scanning = true;
        g_StopScan = false;
        g_ScanProgress = 0;
        g_CacheLoaded = false;

        {
            std::lock_guard<std::mutex> lock(g_Mutex);
            g_Assets.clear();
            g_DumpedFiles.clear();
        }

        try
        {
            Logger::Info("Data Root = " + g_DataFolder);

            // IMPORTANT:
            // Do NOT hold g_Mutex around these.
            // These functions add assets and may lock internally.
            ScanDataFolderOnly();

            g_ScanProgress = 92;
            ScanModOverridesUnlocked();

            g_ScanProgress = 96;
            ScanDumpListOnlyUnlocked();

            g_ScanProgress = 98;
            LinkOverridesAndDumpsUnlocked();
        }
        catch (const std::exception& e)
        {
            Logger::Error(std::string("AssetBrowser full scan exception: ") + e.what());
        }

        AssetBrowser::SaveCache();

        g_ScanProgress = 100;
        SetScanStatus("Scan complete");
        g_Scanning = false;

        Logger::Info(
            "AssetBrowser data-only scan complete. Assets=" +
            std::to_string(g_Assets.size()));
    }
}

namespace AssetBrowser
{
    bool Init()
    {
        g_GameFolder = GetExeFolder();
        g_DataFolder = FindBestDataFolder();
        g_ModsFolder = (std::filesystem::path(g_GameFolder) / "NartuoStorm2Revived" / "mods").string();
        g_DumpFolder = (std::filesystem::path(g_GameFolder) / "NartuoStorm2Revived" / "Dump").string();
        g_ToolsFolder = (std::filesystem::path(g_GameFolder) / "NartuoStorm2Revived" / "Tools").string();
        g_CacheFolder = (std::filesystem::path(g_GameFolder) / "NartuoStorm2Revived" / "Cache").string();

        std::filesystem::create_directories(g_ModsFolder);
        std::filesystem::create_directories(g_DumpFolder);
        std::filesystem::create_directories(g_ToolsFolder);
        std::filesystem::create_directories(g_CacheFolder);

        Logger::Info("AssetBrowser initialized");
        Logger::Info("AssetBrowser game folder: " + g_GameFolder);
        Logger::Info("AssetBrowser DATA-ONLY scan folder: " + g_DataFolder);
		Logger::Info("AssetBrowser mods folder: " + g_ModsFolder);
		Logger::Info("AssetBrowser dump folder: " + g_DumpFolder);

        return true;
    }

    void Shutdown()
    {
        g_StopScan = true;

        if (g_ScanThread.joinable())
            g_ScanThread.join();

        std::lock_guard<std::mutex> lock(g_Mutex);
        g_Assets.clear();
        g_DumpedFiles.clear();

        Logger::Info("AssetBrowser shutdown");
    }

    void Scan()
    {
        if (g_Scanning)
            return;

        FullScanWorker();
    }

    void StartAsyncScan(bool force)
    {
        if (g_Scanning)
            return;

        if (g_ScanThread.joinable())
            g_ScanThread.join();

        g_ScanThread = std::thread(FullScanWorker);
    }

    bool IsScanning()
    {
        return g_Scanning;
    }

    int GetScanProgress()
    {
        return g_ScanProgress;
    }

    std::string GetScanStatus()
    {
        std::lock_guard<std::mutex> lock(g_Mutex);
        return g_ScanStatus;
    }

    bool WasCacheLoaded()
    {
        return g_CacheLoaded;
    }

    bool LoadCache()
    {
        std::filesystem::path path = std::filesystem::path(g_CacheFolder) / "asset_index.txt";

        if (!FileExists(path))
            return false;

        std::ifstream file(path, std::ios::binary);

        if (!file.is_open())
            return false;

        std::vector<AssetEntry> loaded;
        std::string line;

        while (std::getline(file, line))
        {
            if (line.empty())
                continue;

            std::stringstream ss(line);
            std::string field;
            std::vector<std::string> fields;

            while (std::getline(ss, field, '|'))
                fields.push_back(field);

            if (fields.size() < 8)
                continue;

            AssetEntry asset{};
            asset.VirtualPath = fields[0];
            asset.RelativePath = fields[1];
            asset.FullPath = fields[2];
            asset.ArchivePath = fields[3];
            asset.ArchiveEntryPath = fields[4];
            asset.Name = fields[5];
            asset.Extension = fields[6];
            asset.Size = static_cast<uintmax_t>(_strtoui64(fields[7].c_str(), nullptr, 10));
            asset.IsCpkEntry = !asset.ArchiveEntryPath.empty();
            asset.Type = DetectTypeFromPath(asset.VirtualPath.empty() ? asset.Name : asset.VirtualPath);
            asset.Source = AssetSource::Game;

            loaded.push_back(asset);
        }

        if (loaded.empty())
            return false;

        {
            std::lock_guard<std::mutex> lock(g_Mutex);
            g_Assets = loaded;
        }

        g_CacheLoaded = true;

        Logger::Info("AssetBrowser loaded cache: " + path.string());
        return true;
    }

    bool SaveCache()
    {
        std::filesystem::create_directories(g_CacheFolder);

        std::filesystem::path path = std::filesystem::path(g_CacheFolder) / "asset_index.txt";
        std::ofstream file(path, std::ios::binary | std::ios::trunc);

        if (!file.is_open())
            return false;

        std::lock_guard<std::mutex> lock(g_Mutex);

        for (const auto& asset : g_Assets)
        {
            if (asset.Source != AssetSource::Game)
                continue;

            file
                << asset.VirtualPath << "|"
                << asset.RelativePath << "|"
                << asset.FullPath << "|"
                << asset.ArchivePath << "|"
                << asset.ArchiveEntryPath << "|"
                << asset.Name << "|"
                << asset.Extension << "|"
                << asset.Size << "\n";
        }

        Logger::Info("AssetBrowser saved cache: " + path.string());
        return true;
    }

    void ScanDumpFolder()
    {
        std::lock_guard<std::mutex> lock(g_Mutex);
        ScanDumpListOnlyUnlocked();
        LinkOverridesAndDumpsUnlocked();
    }

    void DumpToLog()
    {
        std::lock_guard<std::mutex> lock(g_Mutex);

        Logger::Info("AssetBrowser asset dump begin");

        for (const auto& asset : g_Assets)
        {
            Logger::Info(
                "[" +
                std::string(SourceName(asset.Source)) +
                "] [" +
                std::string(TypeName(asset.Type)) +
                "] " +
                asset.VirtualPath);
        }

        Logger::Info("AssetBrowser asset dump end");
    }

    bool ExportCsv(const std::string& relativeOrFullPath)
    {
        std::filesystem::path path = relativeOrFullPath;

        if (path.is_relative())
            path = std::filesystem::path(g_GameFolder) / path;

        std::filesystem::create_directories(path.parent_path());

        std::ofstream file(path, std::ios::binary | std::ios::trunc);

        if (!file.is_open())
            return false;

        file << "Name,Type,Source,Size,Extension,VirtualPath,FullPath,ArchivePath,ArchiveEntry,Override,Dumped,Dependencies,Markers\n";

        std::lock_guard<std::mutex> lock(g_Mutex);

        for (const auto& asset : g_Assets)
        {
            file
                << "\"" << asset.Name << "\","
                << "\"" << TypeName(asset.Type) << "\","
                << "\"" << SourceName(asset.Source) << "\","
                << asset.Size << ","
                << "\"" << asset.Extension << "\","
                << "\"" << asset.VirtualPath << "\","
                << "\"" << asset.FullPath << "\","
                << "\"" << asset.ArchivePath << "\","
                << "\"" << asset.ArchiveEntryPath << "\","
                << "\"" << asset.OverrideFullPath << "\","
                << "\"" << (asset.IsDumped ? "true" : "false") << "\","
                << asset.Dependencies.size() << ","
                << asset.Markers.size()
                << "\n";
        }

        Logger::Info("AssetBrowser exported CSV: " + path.string());
        return true;
    }

    std::string BuildAssetInfoText(const AssetEntry& asset)
    {
        std::ostringstream out;

        out << "NS2 Revived Asset Info\n";
        out << "======================\n";
        out << "Name: " << asset.Name << "\n";
        out << "Type: " << TypeName(asset.Type) << "\n";
        out << "Source: " << SourceName(asset.Source) << "\n";
        out << "Size: " << asset.Size << "\n";
        out << "Extension: " << asset.Extension << "\n";
        out << "VirtualPath: " << asset.VirtualPath << "\n";
        out << "RelativePath: " << asset.RelativePath << "\n";
        out << "FullPath: " << asset.FullPath << "\n";
        out << "IsCpkEntry: " << (asset.IsCpkEntry ? "true" : "false") << "\n";
        out << "ArchivePath: " << asset.ArchivePath << "\n";
        out << "ArchiveEntryPath: " << asset.ArchiveEntryPath << "\n";
        out << "HasModOverride: " << (asset.HasModOverride ? "true" : "false") << "\n";
        out << "OverrideFullPath: " << asset.OverrideFullPath << "\n";
        out << "IsDumped: " << (asset.IsDumped ? "true" : "false") << "\n";
        out << "DumpFullPath: " << asset.DumpFullPath << "\n";

        out << "\nDependencies:\n";
        for (const auto& dep : asset.Dependencies)
            out << "- " << dep << "\n";

        out << "\nMarkers:\n";
        for (const auto& marker : asset.Markers)
            out << "- " << marker << "\n";

        return out.str();
    }

    bool DumpXfbinInfo(const AssetEntry& asset)
    {
        if (asset.Extension != ".xfbin")
            return false;

        try
        {
            std::filesystem::path outPath =
                std::filesystem::path(g_DumpFolder) / "Info" / (asset.Name + ".xfbin_info.txt");

            std::filesystem::create_directories(outPath.parent_path());

            std::ofstream out(outPath, std::ios::out | std::ios::trunc);

            if (!out.is_open())
                return false;

            out << BuildAssetInfoText(asset);

            Logger::Info("AssetBrowser wrote XFBIN quick info: " + outPath.string());
            return true;
        }
        catch (const std::exception& e)
        {
            Logger::Error(std::string("AssetBrowser XFBIN info failed: ") + e.what());
            return false;
        }
    }

    bool ExtractCpkIfToolExists(const AssetEntry& asset)
    {
        if (asset.Extension != ".cpk")
            return false;

        std::filesystem::path tool = std::filesystem::path(g_ToolsFolder) / "YACpkTool.exe";

        if (!std::filesystem::exists(tool))
        {
            Logger::Info("AssetBrowser CPK extract skipped: YACpkTool.exe not found in " + g_ToolsFolder);
            return false;
        }

        std::filesystem::path outDir =
            std::filesystem::path(g_DumpFolder) / "ExtractedCPK" / std::filesystem::path(asset.Name).stem();

        std::filesystem::create_directories(outDir);

        std::string args = "-X -i \"" + asset.FullPath + "\" -o \"" + outDir.string() + "\"";

        bool ok = LaunchToolAndWait(tool, args);

        Logger::Info(std::string("AssetBrowser CPK extract ") + (ok ? "finished: " : "failed: ") + asset.FullPath);

        ScanDumpFolder();
        return ok;
    }

    bool DumpAsset(const AssetEntry& asset, bool tryExtractArchives)
    {
        if (asset.Source == AssetSource::Dump)
            return true;

        std::filesystem::path outPath = BuildDumpPathForAsset(asset);
        bool copied = false;

        if (asset.IsCpkEntry)
        {
            copied = CpkArchive::ExtractEntry(
                asset.ArchivePath.empty() ? asset.FullPath : asset.ArchivePath,
                asset.ArchiveEntryPath,
                outPath.string());
        }
        else
        {
            copied = CopyFileSafe(asset.FullPath, outPath);
        }

        if (!copied)
            return false;

        std::filesystem::path infoPath = outPath;
        infoPath += ".ns2asset.txt";

        try
        {
            std::ofstream info(infoPath, std::ios::out | std::ios::trunc);
            info << BuildAssetInfoText(asset);
        }
        catch (...) {}

        if (asset.Extension == ".xfbin")
            DumpXfbinInfo(asset);

        if (tryExtractArchives && asset.Extension == ".cpk")
            ExtractCpkIfToolExists(asset);

        Logger::Info("AssetBrowser dumped asset: " + asset.VirtualPath + " -> " + outPath.string());

        ScanDumpFolder();
        return true;
    }

    bool DumpAssetByIndex(int index, bool tryExtractArchives)
    {
        AssetEntry copy{};

        {
            std::lock_guard<std::mutex> lock(g_Mutex);

            if (index < 0 || index >= static_cast<int>(g_Assets.size()))
                return false;

            copy = g_Assets[index];
        }

        return DumpAsset(copy, tryExtractArchives);
    }

    int DumpAllAssets(bool tryExtractArchives)
    {
        std::vector<AssetEntry> copy;

        {
            std::lock_guard<std::mutex> lock(g_Mutex);
            copy = g_Assets;
        }

        int count = 0;

        for (const auto& asset : copy)
        {
            if (asset.Source == AssetSource::Dump)
                continue;

            if (DumpAsset(asset, tryExtractArchives))
                count++;
        }

        ScanDumpFolder();
        return count;
    }

    int DumpAssetsByType(AssetType type, bool tryExtractArchives)
    {
        std::vector<AssetEntry> copy;

        {
            std::lock_guard<std::mutex> lock(g_Mutex);
            copy = g_Assets;
        }

        int count = 0;

        for (const auto& asset : copy)
        {
            if (asset.Source == AssetSource::Dump)
                continue;

            if (asset.Type != type)
                continue;

            if (DumpAsset(asset, tryExtractArchives))
                count++;
        }

        ScanDumpFolder();
        return count;
    }

    std::string ReadSmallTextFile(const std::string& path, size_t maxBytes)
    {
        std::ifstream file(path, std::ios::binary);

        if (!file.is_open())
            return "";

        std::string data;
        data.resize(maxBytes);

        file.read(data.data(), static_cast<std::streamsize>(maxBytes));
        data.resize(static_cast<size_t>(file.gcount()));

        return data;
    }

    bool IsTextPreviewable(const AssetEntry& asset)
    {
        return asset.Extension == ".txt" ||
            asset.Extension == ".ini" ||
            asset.Extension == ".json" ||
            asset.Extension == ".xml" ||
            asset.Extension == ".csv" ||
            asset.Extension == ".lua";
    }

    bool IsTexturePreviewable(const AssetEntry& asset)
    {
        return asset.Extension == ".dds" ||
            asset.Extension == ".nut" ||
            asset.Extension == ".png" ||
            asset.Extension == ".jpg" ||
            asset.Extension == ".jpeg" ||
            asset.Extension == ".tga";
    }

    bool IsModelPreviewable(const AssetEntry& asset)
    {
        return asset.Extension == ".xfbin" ||
            asset.Extension == ".nud";
    }

    const std::vector<AssetEntry>& GetAssets()
    {
        return g_Assets;
    }

    const std::vector<DumpEntry>& GetDumpedFiles()
    {
        return g_DumpedFiles;
    }

    AssetStats GetStats()
    {
        AssetStats stats{};

        std::lock_guard<std::mutex> lock(g_Mutex);
        BuildStatsUnlocked(stats);

        return stats;
    }

    const std::string& GetGameFolder()
    {
        return g_GameFolder;
    }

    const std::string& GetDataFolder()
    {
        return g_DataFolder;
    }

    const std::string& GetModsFolder()
    {
        return g_ModsFolder;
    }

    const std::string& GetDumpFolder()
    {
        return g_DumpFolder;
    }

    const std::string& GetToolsFolder()
    {
        return g_ToolsFolder;
    }

    const std::string& GetCacheFolder()
    {
        return g_CacheFolder;
    }

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
        default: return "Unknown";
        }
    }

    const char* SourceName(AssetSource source)
    {
        switch (source)
        {
        case AssetSource::Mod: return "Mod";
        case AssetSource::Dump: return "Dump";
        default: return "Game";
        }
    }
}