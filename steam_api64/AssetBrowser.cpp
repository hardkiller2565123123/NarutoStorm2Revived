#include "StdInc.h"
#include "AssetBrowser.h"
#include "ModRedirector.h"
#include "CpkArchive.h"
#include "Logger.h"
#include <fstream>
#include <sstream>
#include <atomic>

namespace
{
    std::string g_GameFolder;
    std::string g_ModsFolder;
    std::string g_DumpFolder;
    std::string g_ToolsFolder;
    std::vector<AssetBrowser::AssetEntry> g_Assets;
    std::vector<AssetBrowser::DumpEntry> g_DumpedFiles;
    std::mutex g_DataMutex;
    std::atomic<bool> g_Scanning{ false };
    std::atomic<int> g_ScanProgress{ 0 };
    std::atomic<bool> g_CacheLoaded{ false };
    std::string g_ScanStatus;
    std::mutex g_StatusMutex;


    void SetStatus(const std::string& s, int progress)
    {
        g_ScanProgress = progress;
        std::lock_guard<std::mutex> lock(g_StatusMutex);
        g_ScanStatus = s;
    }

    std::string ToLower(std::string text)
    {
        std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c){ return (char)std::tolower(c); });
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
        while (!text.empty() && (text[0] == '/' || text[0] == '.')) text.erase(text.begin());
        return text;
    }

    bool StartsWithNoCase(const std::string& value, const std::string& prefix)
    {
        std::string v = ToLower(NormalizeSlashes(value));
        std::string p = ToLower(NormalizeSlashes(prefix));
        return v.rfind(p, 0) == 0;
    }

    std::string StripKnownModPrefix(const std::string& relativePath)
    {
        std::string path = NormalizeSlashes(relativePath);
        std::string lower = ToLower(path);
        const char* prefixes[] = { "override/", "overrides/", "loose/", "files/", "raw/", "data/", "cpk/", "packages/" };
        for (const char* prefix : prefixes)
        {
            std::string p = prefix;
            if (lower.rfind(p, 0) == 0) return path.substr(p.size());
        }
        return path;
    }

    std::string GetExeFolder()
    {
        char path[MAX_PATH]{};
        GetModuleFileNameA(nullptr, path, MAX_PATH);
        return std::filesystem::path(path).parent_path().string();
    }

    AssetBrowser::AssetType DetectType(const std::filesystem::path& path)
    {
        std::string ext = ToLower(path.extension().string());
        std::string name = ToLower(path.filename().string());
        std::string full = ToLower(NormalizeSlashes(path.string()));
        if (ext == ".cpk") return AssetBrowser::AssetType::Package;
        if (ext == ".dat" || ext == ".bin" || ext == ".txt" || ext == ".xml" || ext == ".lua" || ext == ".json" || ext == ".ini" || ext == ".csv") return AssetBrowser::AssetType::Data;
        if (ext == ".xfbin" || ext == ".nud")
        {
            if (full.find("stage") != std::string::npos || full.find("stg") != std::string::npos || full.find("map") != std::string::npos || full.find("battle/st") != std::string::npos) return AssetBrowser::AssetType::Stage;
            if (full.find("char") != std::string::npos || full.find("player") != std::string::npos || full.find("character") != std::string::npos || name.find("pl") == 0 || name.find("1p") != std::string::npos) return AssetBrowser::AssetType::Character;
            return AssetBrowser::AssetType::Model;
        }
        if (ext == ".nut" || ext == ".dds" || ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".tga") return AssetBrowser::AssetType::Texture;
        if (ext == ".anm" || ext == ".mot" || ext == ".seq") return AssetBrowser::AssetType::Animation;
        if (ext == ".acb" || ext == ".awb" || ext == ".adx" || ext == ".wav" || ext == ".ogg" || ext == ".hca") return AssetBrowser::AssetType::Audio;
        return AssetBrowser::AssetType::Unknown;
    }

    bool ShouldInclude(const std::filesystem::path& path)
    {
        std::string ext = ToLower(path.extension().string());
        static const std::vector<std::string> allowed = { ".cpk", ".xfbin", ".nud", ".nut", ".anm", ".mot", ".seq", ".acb", ".awb", ".adx", ".hca", ".bin", ".dat", ".dds", ".png", ".jpg", ".jpeg", ".tga", ".wav", ".ogg", ".txt", ".xml", ".lua", ".json", ".ini", ".csv" };
        return std::find(allowed.begin(), allowed.end(), ext) != allowed.end();
    }

    std::filesystem::path CachePath()
    {
        return std::filesystem::path(g_GameFolder) / "NartuoStorm2Revived" / "cache" / "assets_cache.tsv";
    }

    std::filesystem::path BuildDumpPathForAsset(const AssetBrowser::AssetEntry& asset)
    {
        std::filesystem::path out = g_DumpFolder;
        out /= AssetBrowser::SourceName(asset.Source);
        out /= AssetBrowser::TypeName(asset.Type);
        std::string relative = !asset.VirtualPath.empty() ? asset.VirtualPath : asset.RelativePath;
        relative = SanitizePath(relative);
        if (relative.empty()) relative = asset.Name;
        out /= std::filesystem::path(relative);
        return out;
    }

    void AddAssetUnlocked(const AssetBrowser::AssetEntry& asset)
    {
        g_Assets.push_back(asset);
    }

    void ScanFolderUnlocked(const std::filesystem::path& root, AssetBrowser::AssetSource source)
    {
        if (root.empty() || !std::filesystem::exists(root)) return;
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
            AssetBrowser::AssetEntry asset{};
            asset.Name = path.filename().string();
            asset.FullPath = path.string();
            asset.RelativePath = NormalizeSlashes(std::filesystem::relative(path, root).string());
            asset.Extension = ToLower(path.extension().string());
            asset.Type = DetectType(path);
            asset.Source = source;
            asset.Size = entry.file_size();
            if (source == AssetBrowser::AssetSource::Game) asset.VirtualPath = NormalizeSlashes(asset.RelativePath);
            else if (source == AssetBrowser::AssetSource::Mod)
            {
                asset.VirtualPath = StripKnownModPrefix(asset.RelativePath);
                std::string lowerRel = ToLower(NormalizeSlashes(asset.RelativePath));
                asset.IsLooseOverride = StartsWithNoCase(lowerRel, "override/") || StartsWithNoCase(lowerRel, "overrides/") || StartsWithNoCase(lowerRel, "loose/") || StartsWithNoCase(lowerRel, "files/");
            }
            else { asset.VirtualPath = NormalizeSlashes(asset.RelativePath); asset.IsDumped = true; }
            AddAssetUnlocked(asset);
            if (asset.Extension == ".cpk")
            {
                CpkArchive::ArchiveInfo cpkInfo;
                if (CpkArchive::Load(asset.FullPath, cpkInfo))
                {
                    for (const auto& cpkEntry : cpkInfo.Entries)
                    {
                        AssetBrowser::AssetEntry embedded{};
                        embedded.Name = cpkEntry.Name;
                        embedded.FullPath = asset.FullPath;
                        embedded.RelativePath = asset.RelativePath + ":" + cpkEntry.Path;
                        embedded.VirtualPath = cpkEntry.Path;
                        embedded.Extension = ToLower(std::filesystem::path(cpkEntry.Path).extension().string());
                        embedded.Type = DetectType(std::filesystem::path(cpkEntry.Path));
                        embedded.Source = source;
                        embedded.Size = cpkEntry.Size;
                        embedded.IsCpkEntry = true;
                        embedded.ArchivePath = asset.FullPath;
                        embedded.ArchiveEntryPath = cpkEntry.Path;
                        AddAssetUnlocked(embedded);
                    }
                }
            }
        }
    }

    void ScanDumpListOnlyUnlocked()
    {
        g_DumpedFiles.clear();
        if (g_DumpFolder.empty() || !std::filesystem::exists(g_DumpFolder)) return;
        for (const auto& entry : std::filesystem::recursive_directory_iterator(g_DumpFolder))
        {
            if (!entry.is_regular_file()) continue;
            const auto& path = entry.path();
            if (!ShouldInclude(path) && ToLower(path.extension().string()) != ".txt") continue;
            AssetBrowser::DumpEntry item{};
            item.Name = path.filename().string();
            item.FullPath = path.string();
            item.RelativePath = NormalizeSlashes(std::filesystem::relative(path, g_DumpFolder).string());
            item.Extension = ToLower(path.extension().string());
            item.Type = DetectType(path);
            item.Size = entry.file_size();
            g_DumpedFiles.push_back(item);
        }
    }

    void LinkOverridesAndDumpsUnlocked()
    {
        std::unordered_map<std::string, std::string> modByVirtual;
        std::unordered_map<std::string, std::string> dumpByVirtual;
        for (const auto& asset : g_Assets)
        {
            std::string key = ToLower(NormalizeSlashes(asset.VirtualPath));
            if (key.empty()) continue;
            if (asset.Source == AssetBrowser::AssetSource::Mod) modByVirtual[key] = asset.FullPath;
            else if (asset.Source == AssetBrowser::AssetSource::Dump) dumpByVirtual[key] = asset.FullPath;
        }
        for (auto& asset : g_Assets)
        {
            if (asset.Source != AssetBrowser::AssetSource::Game) continue;
            std::string key = ToLower(NormalizeSlashes(asset.VirtualPath));
            auto mod = modByVirtual.find(key);
            if (mod != modByVirtual.end()) { asset.HasModOverride = true; asset.OverrideFullPath = mod->second; }
            auto dump = dumpByVirtual.find(key);
            if (dump != dumpByVirtual.end()) { asset.IsDumped = true; asset.DumpFullPath = dump->second; }
        }
    }

    bool CopyFileSafe(const std::filesystem::path& from, const std::filesystem::path& to)
    {
        try { std::filesystem::create_directories(to.parent_path()); std::filesystem::copy_file(from, to, std::filesystem::copy_options::overwrite_existing); return true; }
        catch (const std::exception& e) { Logger::Error(std::string("AssetBrowser copy failed: ") + e.what()); return false; }
    }

    std::string ReadAsciiWindow(const std::string& path, size_t maxBytes)
    {
        std::ifstream file(path, std::ios::binary); if (!file.is_open()) return "";
        std::string data; data.resize(maxBytes); file.read(data.data(), (std::streamsize)data.size()); data.resize((size_t)file.gcount()); return data;
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
        try { std::filesystem::create_directories(g_ModsFolder); std::filesystem::create_directories(std::filesystem::path(g_ModsFolder)/"override"); std::filesystem::create_directories(std::filesystem::path(g_ModsFolder)/"cpk"); std::filesystem::create_directories(g_DumpFolder); std::filesystem::create_directories(g_ToolsFolder); std::filesystem::create_directories(CachePath().parent_path()); } catch (...) {}
        Logger::Info("AssetBrowser initialized");
        return true;
    }

    void Scan()
    {
        if (g_GameFolder.empty()) Init();
        SetStatus("Scanning game files", 5);
        std::lock_guard<std::mutex> lock(g_DataMutex);
        g_Assets.clear();
        g_DumpedFiles.clear();
        try
        {
            SetStatus("Scanning game folder", 15); ScanFolderUnlocked(g_GameFolder, AssetSource::Game);
            SetStatus("Scanning mods folder", 55); ScanFolderUnlocked(g_ModsFolder, AssetSource::Mod);
            SetStatus("Scanning dump folder", 72); ScanFolderUnlocked(g_DumpFolder, AssetSource::Dump);
            SetStatus("Linking overrides and dumps", 84); ScanDumpListOnlyUnlocked(); LinkOverridesAndDumpsUnlocked();
            SetStatus("Saving cache", 94); SaveCache();
            ModRedirector::Scan();
            SetStatus("Assets ready", 100);
        }
        catch (const std::exception& e) { Logger::Error(std::string("AssetBrowser scan failed: ") + e.what()); SetStatus("Asset scan failed", 100); }
        Logger::Info("AssetBrowser scan finished. Assets found: " + std::to_string(g_Assets.size()));
    }

    void ScanDumpFolder()
    {
        std::lock_guard<std::mutex> lock(g_DataMutex);
        ScanDumpListOnlyUnlocked(); LinkOverridesAndDumpsUnlocked();
    }

    bool LoadCache()
    {
        if (g_GameFolder.empty()) Init();
        std::ifstream in(CachePath(), std::ios::in);
        if (!in.is_open()) return false;
        std::vector<AssetEntry> loaded;
        std::string line;
        while (std::getline(in, line))
        {
            if (line.empty() || line[0] == '#') continue;
            std::vector<std::string> cols; std::stringstream ss(line); std::string cell;
            while (std::getline(ss, cell, '\t')) cols.push_back(cell);
            if (cols.size() < 9) continue;
            AssetEntry a{};
            a.Name=cols[0]; a.FullPath=cols[1]; a.RelativePath=cols[2]; a.VirtualPath=cols[3]; a.Extension=cols[4];
            a.Type=(AssetType)std::atoi(cols[5].c_str()); a.Source=(AssetSource)std::atoi(cols[6].c_str()); a.Size=(uintmax_t)_strtoui64(cols[7].c_str(), nullptr, 10); a.IsCpkEntry=std::atoi(cols[8].c_str())!=0;
            if (cols.size()>9) a.ArchivePath=cols[9]; if (cols.size()>10) a.ArchiveEntryPath=cols[10];
            loaded.push_back(a);
        }
        if (loaded.empty()) return false;
        { std::lock_guard<std::mutex> lock(g_DataMutex); g_Assets.swap(loaded); LinkOverridesAndDumpsUnlocked(); }
        g_CacheLoaded = true; SetStatus("Loaded cached asset index", 100);
        Logger::Info("AssetBrowser loaded cache: " + std::to_string(g_Assets.size()));
        return true;
    }

    bool SaveCache()
    {
        if (g_GameFolder.empty()) Init();
        try
        {
            std::filesystem::create_directories(CachePath().parent_path());
            std::ofstream out(CachePath(), std::ios::out|std::ios::trunc);
            if (!out.is_open()) return false;
            out << "# NS2 Revived asset cache v1\n";
            for (const auto& a : g_Assets)
            {
                out << a.Name << '\t' << a.FullPath << '\t' << a.RelativePath << '\t' << a.VirtualPath << '\t' << a.Extension << '\t' << (int)a.Type << '\t' << (int)a.Source << '\t' << (unsigned long long)a.Size << '\t' << (a.IsCpkEntry?1:0) << '\t' << a.ArchivePath << '\t' << a.ArchiveEntryPath << "\n";
            }
            return true;
        }
        catch (...) { return false; }
    }

    void StartAsyncScan(bool forceFullRefresh)
    {
        if (g_Scanning.exchange(true)) return;
        std::thread([forceFullRefresh]()
        {
            SetStatus("Preparing asset index", 1);
            if (!forceFullRefresh && !WasCacheLoaded()) LoadCache();
            Scan();
            g_Scanning = false;
        }).detach();
    }

    bool IsScanning() { return g_Scanning.load(); }
    int GetScanProgress() { return g_ScanProgress.load(); }
    std::string GetScanStatus() { std::lock_guard<std::mutex> lock(g_StatusMutex); return g_ScanStatus; }
    bool WasCacheLoaded() { return g_CacheLoaded.load(); }

    void DumpToLog(){ Logger::Info("AssetBrowser dump begin"); for (const auto& a : g_Assets) Logger::Info("["+std::string(SourceName(a.Source))+"] ["+TypeName(a.Type)+"] "+a.RelativePath); Logger::Info("AssetBrowser dump end"); }

    bool ExportCsv(const std::string& relativeOrFullPath)
    {
        std::filesystem::path outPath(relativeOrFullPath); if (outPath.is_relative()) outPath = std::filesystem::path(g_GameFolder)/outPath;
        try { std::filesystem::create_directories(outPath.parent_path()); std::ofstream out(outPath); if(!out.is_open()) return false; out << "Source,Type,Extension,Size,Name,RelativePath,VirtualPath,HasOverride,OverridePath,IsDumped,DumpPath\n"; for (const auto& a:g_Assets) out << '"'<<SourceName(a.Source)<<"\",\""<<TypeName(a.Type)<<"\",\""<<a.Extension<<"\","<<a.Size<<",\""<<a.Name<<"\",\""<<a.RelativePath<<"\",\""<<a.VirtualPath<<"\","<<(a.HasModOverride?1:0)<<",\""<<a.OverrideFullPath<<"\","<<(a.IsDumped?1:0)<<",\""<<a.DumpFullPath<<"\"\n"; return true; } catch(...) { return false; }
    }

    std::string BuildAssetInfoText(const AssetEntry& asset)
    {
        std::ostringstream ss; ss << "NS2 Revived Asset\nName: "<<asset.Name<<"\nType: "<<TypeName(asset.Type)<<"\nSource: "<<SourceName(asset.Source)<<"\nExtension: "<<asset.Extension<<"\nSize: "<<asset.Size<<" bytes\nRelativePath: "<<asset.RelativePath<<"\nVirtualPath: "<<asset.VirtualPath<<"\nFullPath: "<<asset.FullPath<<"\nCPK Entry: "<<(asset.IsCpkEntry?"true":"false")<<"\nArchive: "<<asset.ArchivePath<<"\nEntry: "<<asset.ArchiveEntryPath<<"\n"; return ss.str();
    }

    bool DumpXfbinInfo(const AssetEntry& asset)
    {
        std::filesystem::path dumpPath = BuildDumpPathForAsset(asset); std::filesystem::path infoPath = dumpPath; infoPath += ".xfbin_info.txt";
        try { std::filesystem::create_directories(infoPath.parent_path()); std::ofstream out(infoPath); if(!out.is_open()) return false; out << BuildAssetInfoText(asset) << "\nDetected markers:\n"; std::string data = ReadAsciiWindow(asset.IsCpkEntry ? asset.ArchivePath : asset.FullPath, 1024*1024); const char* terms[]={"nuccChunk","NUCC","stage","char","model","texture","material","anm","camera"}; for(auto t:terms){ size_t p=0; int c=0; while((p=data.find(t,p))!=std::string::npos && c<64){ size_t b=p,e=p; while(b>0&&data[b-1]>=32&&data[b-1]<127)b--; while(e<data.size()&&data[e]>=32&&data[e]<127)e++; out << "- " << data.substr(b,e-b) << "\n"; p+=strlen(t); c++; }} return true; } catch(...) { return false; }
    }

    bool ExtractCpkIfToolExists(const AssetEntry&) { return false; }

    bool DumpAsset(const AssetEntry& asset, bool tryExtractArchives)
    {
        NSR_UNUSED(tryExtractArchives);
        if (asset.Source == AssetSource::Dump) return true;
        std::filesystem::path outPath = BuildDumpPathForAsset(asset); bool copied=false;
        if (asset.IsCpkEntry) { CpkArchive::Entry e{}; e.Name=asset.Name; e.Path=asset.ArchiveEntryPath; e.Size=(uint64_t)asset.Size; e.Heuristic=true; copied = CpkArchive::DumpFile(asset.ArchivePath.empty()?asset.FullPath:asset.ArchivePath, e, outPath.string()+".cpkentry.txt"); }
        else copied = CopyFileSafe(asset.FullPath, outPath);
        if (!copied) return false;
        try { std::ofstream info(outPath.string()+".ns2asset.txt"); info << BuildAssetInfoText(asset); } catch(...) {}
        if (asset.Extension == ".xfbin" || asset.IsCpkEntry) DumpXfbinInfo(asset);
        ScanDumpFolder(); return true;
    }

    bool DumpAssetByIndex(int index, bool tryExtractArchives){ const auto& a=GetAssets(); if(index<0||index>=(int)a.size())return false; return DumpAsset(a[index],tryExtractArchives); }
    int DumpAllAssets(bool tryExtractArchives){ int c=0; auto copy=GetAssets(); for(auto&a:copy) if(a.Source!=AssetSource::Dump && DumpAsset(a,tryExtractArchives)) c++; return c; }
    int DumpAssetsByType(AssetType type, bool tryExtractArchives){ int c=0; auto copy=GetAssets(); for(auto&a:copy) if(a.Source!=AssetSource::Dump && a.Type==type && DumpAsset(a,tryExtractArchives)) c++; return c; }
    std::string ReadSmallTextFile(const std::string& path, size_t maxBytes){ std::ifstream f(path,std::ios::binary); if(!f.is_open()) return ""; std::string d; d.resize(maxBytes); f.read(d.data(),(std::streamsize)maxBytes); d.resize((size_t)f.gcount()); return d; }
    bool IsTextPreviewable(const AssetEntry& a){ return a.Extension==".txt"||a.Extension==".ini"||a.Extension==".json"||a.Extension==".xml"||a.Extension==".csv"||a.Extension==".lua"; }
    bool IsTexturePreviewable(const AssetEntry& a){ return a.Extension==".dds"||a.Extension==".nut"||a.Extension==".png"||a.Extension==".jpg"||a.Extension==".jpeg"||a.Extension==".tga"; }
    bool IsModelPreviewable(const AssetEntry& a){ return a.Extension==".xfbin"||a.Extension==".nud"||a.Type==AssetType::Model||a.Type==AssetType::Character||a.Type==AssetType::Stage; }
    const std::vector<AssetEntry>& GetAssets(){ return g_Assets; }
    const std::vector<DumpEntry>& GetDumpedFiles(){ return g_DumpedFiles; }
    AssetStats GetStats(){ AssetStats s{}; s.TotalAssets=(int)g_Assets.size(); s.DumpedAssets=(int)g_DumpedFiles.size(); for(auto&a:g_Assets){ if(a.Source==AssetSource::Game)s.GameAssets++; else if(a.Source==AssetSource::Mod)s.ModAssets++; if(a.HasModOverride||a.IsLooseOverride)s.OverrideAssets++; if(a.Type==AssetType::Package)s.PackageAssets++; if(a.Extension==".xfbin")s.XfbinAssets++; if(a.Type==AssetType::Texture)s.TextureAssets++; if(a.Type==AssetType::Audio)s.AudioAssets++; if(a.Type==AssetType::Stage)s.StageAssets++; if(a.Type==AssetType::Character)s.CharacterAssets++; if(a.Type==AssetType::Model)s.ModelAssets++; } return s; }
    const std::string& GetGameFolder(){ return g_GameFolder; } const std::string& GetModsFolder(){ return g_ModsFolder; } const std::string& GetDumpFolder(){ return g_DumpFolder; } const std::string& GetToolsFolder(){ return g_ToolsFolder; }
    const char* TypeName(AssetType type){ switch(type){case AssetType::Package:return"Package";case AssetType::Model:return"Model";case AssetType::Texture:return"Texture";case AssetType::Animation:return"Animation";case AssetType::Audio:return"Audio";case AssetType::Stage:return"Stage";case AssetType::Character:return"Character";case AssetType::Data:return"Data";default:return"Unknown";} }
    const char* SourceName(AssetSource source){ switch(source){case AssetSource::Mod:return"Mod";case AssetSource::Dump:return"Dump";default:return"Game";} }
}
