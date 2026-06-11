#include "DX11OverlayInternal.h"
#include "AssetTaskQueue.h"
#include "AssetHash.h"
#include "AssetNotification.h"
#include "AssetOverrideManager.h"
#include "AssetHexEditor.h"
#include "AssetReport.h"
#include "AssetPackageCreator.h"
#include "AssetSearchFilter.h"

namespace
{
    char g_ExtensionFilter[32]{};
    char g_ArchiveFilter[128]{};
    char g_ModName[128] = "MyMod";
    char g_ModAuthor[128] = "NS2 Modder";
    char g_ModDescription[256] = "Created with NS2 Revived V2.";
    bool g_FilterExtractableOnly = false;
    bool g_FilterUnknownOnly = false;
    bool g_FilterRegex = false;

    bool IsCtrlDown() { return (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0; }
    bool IsShiftDown() { return (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0; }

    bool AssetMatchesSubPage(const AssetBrowser::AssetEntry& asset, int page)
    {
        using namespace AssetBrowser;

        switch (page)
        {
        case 0: return true;
        case 1: return asset.Type == AssetType::Stage;
        case 2: return asset.Type == AssetType::Character;
        case 3: return asset.Type == AssetType::Model;
        case 4: return asset.Type == AssetType::Texture;
        case 5: return asset.Type == AssetType::Audio;
        case 6: return asset.Type == AssetType::Package || asset.IsCpkEntry;
        case 7: return asset.HasModOverride || asset.IsLooseOverride || AssetOverrideManager::HasOverride(asset);
        case 13: return true;
        default: return true;
        }
    }

    AssetSearchFilter::Filter BuildFilter()
    {
        AssetSearchFilter::Filter f{};
        f.Search = g_AssetSearch;
        f.Extension = g_ExtensionFilter;
        f.ArchiveContains = g_ArchiveFilter;
        f.OverridesOnly = g_AssetOverridesOnly;
        f.DumpedOnly = g_AssetDumpedOnly;
        f.ExtractableOnly = g_FilterExtractableOnly;
        f.UnknownOnly = g_FilterUnknownOnly;
        f.RegexMode = g_FilterRegex;
        return f;
    }

    bool AssetVisible(const AssetBrowser::AssetEntry& asset)
    {
        if (!AssetMatchesSubPage(asset, g_SelectedSubPage))
            return false;

        return AssetSearchFilter::Matches(asset, BuildFilter());
    }

    void SelectAssetIndex(int index)
    {
        if (index < 0)
            return;

        if (IsShiftDown() && g_LastClickedAssetIndex >= 0)
        {
            int begin = std::min(g_LastClickedAssetIndex, index);
            int end = std::max(g_LastClickedAssetIndex, index);

            g_SelectedAssetIndexes.clear();

            for (int i = begin; i <= end; ++i)
                g_SelectedAssetIndexes.insert(i);
        }
        else if (IsCtrlDown())
        {
            if (g_SelectedAssetIndexes.count(index))
                g_SelectedAssetIndexes.erase(index);
            else
                g_SelectedAssetIndexes.insert(index);
        }
        else
        {
            g_SelectedAssetIndexes.clear();
            g_SelectedAssetIndexes.insert(index);
        }

        g_SelectedAssetIndex = index;
        g_LastClickedAssetIndex = index;
    }

    std::vector<AssetBrowser::AssetEntry> GetSelectedAssets(const std::vector<AssetBrowser::AssetEntry>& assets)
    {
        std::vector<AssetBrowser::AssetEntry> selected;

        if (g_SelectedAssetIndexes.empty() && g_SelectedAssetIndex >= 0)
            g_SelectedAssetIndexes.insert(g_SelectedAssetIndex);

        for (int index : g_SelectedAssetIndexes)
        {
            if (index >= 0 && index < static_cast<int>(assets.size()))
                selected.push_back(assets[index]);
        }

        return selected;
    }

    void SelectAllVisibleAssets(const std::vector<AssetBrowser::AssetEntry>& assets)
    {
        g_SelectedAssetIndexes.clear();

        for (int i = 0; i < static_cast<int>(assets.size()); ++i)
        {
            if (AssetVisible(assets[i]))
                g_SelectedAssetIndexes.insert(i);
        }

        if (!g_SelectedAssetIndexes.empty())
            g_SelectedAssetIndex = *g_SelectedAssetIndexes.begin();
    }

    void InvertVisibleSelection(const std::vector<AssetBrowser::AssetEntry>& assets)
    {
        for (int i = 0; i < static_cast<int>(assets.size()); ++i)
        {
            if (!AssetVisible(assets[i]))
                continue;

            if (g_SelectedAssetIndexes.count(i))
                g_SelectedAssetIndexes.erase(i);
            else
                g_SelectedAssetIndexes.insert(i);
        }

        if (!g_SelectedAssetIndexes.empty())
            g_SelectedAssetIndex = *g_SelectedAssetIndexes.begin();
        else
            g_SelectedAssetIndex = -1;
    }

    void JumpToAssetIndex(int index)
    {
        const auto& assets = AssetBrowser::GetAssets();

        if (index < 0 || index >= static_cast<int>(assets.size()))
            return;

        g_SelectedAssetIndexes.clear();
        g_SelectedAssetIndexes.insert(index);
        g_SelectedAssetIndex = index;
        g_LastClickedAssetIndex = index;
    }

    void DrawAssetStatsHeader()
    {
        AssetBrowser::AssetStats stats = AssetBrowser::GetStats();

        ImGui::Text("Total %d | Game %d | Mods %d | Dumped %d | Overrides %d",
            stats.TotalAssets, stats.GameAssets, stats.ModAssets, stats.DumpedAssets, stats.OverrideAssets);

        ImGui::Text("Packages %d | XFBIN %d | Models %d | Textures %d | Audio %d | Stages %d | Characters %d",
            stats.PackageAssets, stats.XfbinAssets, stats.ModelAssets, stats.TextureAssets,
            stats.AudioAssets, stats.StageAssets, stats.CharacterAssets);

        if (AssetBrowser::IsScanning())
        {
            ImGui::TextColored(ImVec4(1.0f, 0.70f, 0.25f, 1.0f),
                "Scanning data folder: %d%%", AssetBrowser::GetScanProgress());
            ImGui::SameLine();
            ImGui::TextDisabled("%s", AssetBrowser::GetScanStatus().c_str());
        }
        else if (AssetTaskQueue::IsBusy())
        {
            ImGui::TextColored(ImVec4(0.45f, 0.75f, 1.0f, 1.0f),
                "Task: %s %d%%", AssetTaskQueue::GetStatus().c_str(), AssetTaskQueue::GetProgress());
            ImGui::SameLine();
            ImGui::TextDisabled("%s", AssetTaskQueue::GetCurrentTaskName().c_str());
        }
        else
        {
            ImGui::TextColored(ImVec4(0.35f, 1.0f, 0.35f, 1.0f), "Ready");
            ImGui::SameLine();
            ImGui::TextDisabled("Data: %s", AssetBrowser::GetDataFolder().c_str());
        }
    }

    void DrawTopToolbar(const std::vector<AssetBrowser::AssetEntry>& assets)
    {
        const float h = 24.0f;

        if (ImGui::Button("Async Rescan", ImVec2(110, h)))
        {
            AssetBrowser::StartAsyncScan(true);
            AssetNotification::PushInfo("Asset Scan", "Async rescan started.");
        }

        ImGui::SameLine();

        if (ImGui::Button("Save Cache", ImVec2(100, h)))
        {
            AssetBrowser::SaveCache();
            AssetNotification::PushSuccess("Cache", "Asset cache saved.");
        }

        ImGui::SameLine();

        if (ImGui::Button("Load Cache", ImVec2(100, h)))
        {
            AssetBrowser::LoadCache();
            AssetNotification::PushInfo("Cache", "Asset cache loaded.");
        }

        ImGui::SameLine();

        if (ImGui::Button("Export CSV", ImVec2(95, h)))
        {
            AssetTaskQueue::EnqueueExportCsv("NartuoStorm2Revived/Dump/asset_index.csv");
            AssetNotification::PushInfo("Report", "CSV export queued.");
        }

        ImGui::SameLine();

        if (ImGui::Button("Full Report", ImVec2(100, h)))
        {
            AssetReport::ExportFullReport("NartuoStorm2Revived/Dump/full_asset_report.txt");
            AssetNotification::PushSuccess("Report", "Full report exported.");
        }

        ImGui::Spacing();

        ImGui::TextDisabled("Selection");
        ImGui::SameLine();

        if (ImGui::Button("Clear Sel", ImVec2(90, h)))
        {
            g_SelectedAssetIndexes.clear();
            g_SelectedAssetIndex = -1;
            g_LastClickedAssetIndex = -1;
        }

        ImGui::SameLine();

        if (ImGui::Button("Select All", ImVec2(90, h)))
            SelectAllVisibleAssets(assets);

        ImGui::SameLine();

        if (ImGui::Button("Invert", ImVec2(70, h)))
            InvertVisibleSelection(assets);

        ImGui::SameLine();

        if (ImGui::Button("Dump Sel", ImVec2(90, h)))
        {
            auto selected = GetSelectedAssets(assets);
            AssetTaskQueue::EnqueueDumpMany(selected, "Dump Selected Assets");
            AssetNotification::PushInfo("Dump", "Selected dump queued.");
        }

        ImGui::SameLine();

        if (ImGui::Button("To Mod", ImVec2(80, h)))
        {
            auto selected = GetSelectedAssets(assets);
            for (const auto& asset : selected)
                AssetOverrideManager::CreateOverrideFromDump(asset);
        }

        ImGui::SameLine();

        if (ImGui::Button("Package", ImVec2(80, h)))
        {
            auto selected = GetSelectedAssets(assets);
            AssetPackageCreator::CreatePackage(g_ModName, g_ModAuthor, g_ModDescription, selected);
        }

        ImGui::SameLine();
        ImGui::TextDisabled("Selected: %d", static_cast<int>(g_SelectedAssetIndexes.size()));
    }

    void DrawFilters()
    {
        ImVec2 contentAvail = ImGui::GetContentRegionAvail();
        float listColumnWidth = contentAvail.x * 0.48f;

        if (listColumnWidth < 440.0f)
            listColumnWidth = 440.0f;

        ImGui::SetNextItemWidth(listColumnWidth - 8.0f);
        ImGui::InputText("##AssetSearch", g_AssetSearch, sizeof(g_AssetSearch));
        ImGui::SameLine();
        ImGui::TextDisabled("Search");

        ImGui::SetNextItemWidth(80);
        ImGui::InputText("Ext", g_ExtensionFilter, sizeof(g_ExtensionFilter));
        ImGui::SameLine();

        ImGui::SetNextItemWidth(120);
        ImGui::InputText("Archive", g_ArchiveFilter, sizeof(g_ArchiveFilter));

        ImGui::Checkbox("Overrides", &g_AssetOverridesOnly);
        ImGui::SameLine();
        ImGui::Checkbox("Dumped", &g_AssetDumpedOnly);
        ImGui::SameLine();
        ImGui::Checkbox("Extractable", &g_FilterExtractableOnly);
        ImGui::SameLine();
        ImGui::Checkbox("Unknown", &g_FilterUnknownOnly);
        ImGui::SameLine();
        ImGui::Checkbox("Regex", &g_FilterRegex);
        ImGui::SameLine();
        ImGui::Checkbox("Full paths", &g_AssetFullPathLabels);
    }

    void DrawAssetList(const std::vector<AssetBrowser::AssetEntry>& assets)
    {
        ImGui::BeginChild("##AssetList", ImVec2(0, 0), true);

        for (int i = 0; i < static_cast<int>(assets.size()); ++i)
        {
            const auto& asset = assets[i];

            if (!AssetVisible(asset))
                continue;

            std::string label;

            if (AssetOverrideManager::HasOverride(asset) || asset.HasModOverride || asset.IsLooseOverride)
                label += "[OVERRIDE] ";

            if (asset.IsDumped || !asset.DumpFullPath.empty())
                label += "[DUMPED] ";

            if (asset.IsCpkEntry)
                label += "[CPK] ";

            label += "[";
            label += AssetBrowser::TypeName(asset.Type);
            label += "] ";
            label += g_AssetFullPathLabels ? asset.VirtualPath : asset.Name;

            bool selected = g_SelectedAssetIndexes.count(i) > 0;

            if (ImGui::Selectable(label.c_str(), selected))
                SelectAssetIndex(i);

            if (ImGui::BeginPopupContextItem())
            {
                if (ImGui::MenuItem("Preview / Select"))
                    SelectAssetIndex(i);

                if (ImGui::MenuItem("Add To Selection"))
                    g_SelectedAssetIndexes.insert(i);

                if (ImGui::MenuItem("Dump This Asset"))
                    AssetTaskQueue::EnqueueDumpAsset(asset);

                if (ImGui::MenuItem("Dump To Mods / Create Override"))
                    AssetOverrideManager::CreateOverrideFromDump(asset);

                if (ImGui::MenuItem("Disable Override"))
                    AssetOverrideManager::DisableOverride(asset);

                if (ImGui::MenuItem("Enable Override"))
                    AssetOverrideManager::EnableOverride(asset);

                if (ImGui::MenuItem("Delete Override"))
                    AssetOverrideManager::DeleteOverride(asset);

                if (ImGui::MenuItem("Load In Hex Editor"))
                    AssetHexEditor::LoadAsset(asset, false);

                if (ImGui::MenuItem("Load Override In Hex Editor"))
                    AssetHexEditor::LoadAsset(asset, true);

                if (ImGui::MenuItem("Copy Name"))
                    ImGui::SetClipboardText(asset.Name.c_str());

                if (ImGui::MenuItem("Copy Virtual Path"))
                    ImGui::SetClipboardText(asset.VirtualPath.c_str());

                if (ImGui::MenuItem("Copy Full Path"))
                    ImGui::SetClipboardText(asset.FullPath.c_str());

                if (ImGui::MenuItem("Copy Metadata"))
                {
                    std::string report = AssetReport::BuildOneAssetReport(asset);
                    ImGui::SetClipboardText(report.c_str());
                }

                if (ImGui::MenuItem(AssetFavorites::IsFavorite(asset) ? "Unfavorite" : "Favorite"))
                    AssetFavorites::ToggleFavorite(asset);

                if (ImGui::MenuItem("Backup Asset"))
                    AssetBackupManager::BackupAsset(asset);

                if (ImGui::MenuItem("Inspect Texture Metadata"))
                {
                    AssetTextureInfo::TextureInfo tex{};
                    AssetTextureInfo::Inspect(asset, tex);
                    std::string text = AssetTextureInfo::BuildText(tex);
                    ImGui::SetClipboardText(text.c_str());
                }

                if (ImGui::MenuItem("Extract Same Archive"))
                    AssetBulkExtractor::ExtractArchive(asset.ArchivePath);


                if (ImGui::MenuItem("Rebuild Relations"))
                    AssetRelations::Rebuild();

                ImGui::EndPopup();
            }
        }

        ImGui::EndChild();
    }

    void DrawInfoTab(const AssetBrowser::AssetEntry& asset)
    {
        ImGui::Text("Name: %s", asset.Name.c_str());
        ImGui::Text("Type: %s", AssetBrowser::TypeName(asset.Type));
        ImGui::Text("Source: %s", AssetBrowser::SourceName(asset.Source));
        ImGui::Text("Size: %.2f MB", static_cast<double>(asset.Size) / 1024.0 / 1024.0);
        ImGui::Text("Extension: %s", asset.Extension.c_str());

        AssetHash::HashInfo hash{};
        if (AssetHash::ComputeAsset(asset, hash))
        {
            ImGui::Separator();
            ImGui::Text("CRC32: %s", AssetHash::ToHex32(hash.CRC32).c_str());
            ImGui::Text("FNV1A64: %s", hash.FNV1A64.c_str());
        }

        ImGui::Separator();
        ImGui::Text("Virtual Path:");
        ImGui::TextWrapped("%s", asset.VirtualPath.c_str());

        ImGui::Text("Full Path:");
        ImGui::TextWrapped("%s", asset.FullPath.c_str());

        if (asset.IsCpkEntry)
        {
            ImGui::Separator();
            ImGui::TextColored(ImVec4(1.0f, 0.65f, 0.2f, 1.0f), "[CPK ENTRY]");
            ImGui::TextWrapped("Archive: %s", asset.ArchivePath.c_str());
            ImGui::TextWrapped("Entry: %s", asset.ArchiveEntryPath.c_str());
        }

        ImGui::Separator();
        ImGui::Text("Override Path:");
        ImGui::TextWrapped("%s", AssetOverrideManager::GetOverridePath(asset).c_str());

        if (AssetOverrideManager::HasOverride(asset))
            ImGui::TextColored(ImVec4(0.35f, 1.0f, 0.35f, 1.0f), "Override active");
    }

    void DrawAssetViewTab(const AssetBrowser::AssetEntry& asset)
    {
        std::string readablePath;
        bool hasReadableFile = AssetPreviewResolver::ResolveReadablePath(asset, readablePath, true);

        if (!hasReadableFile)
        {
            ImGui::TextColored(ImVec4(1.0f, 0.55f, 0.25f, 1.0f), "No readable preview file yet.");
            AssetModelViewer::Draw(asset, ImVec2(0.0f, 0.0f));
            return;
        }

        ImGui::TextDisabled("Preview file:");
        ImGui::TextWrapped("%s", readablePath.c_str());
        ImGui::Separator();

        if (AssetBrowser::IsModelPreviewable(asset))
        {
            AssetModelViewer::Draw(asset, ImVec2(0.0f, 0.0f));
            return;
        }

        if (AssetBrowser::IsTexturePreviewable(asset))
        {
            ImGui::Text("Texture Asset");
            ImGui::Text("Real DDS/NUT D3D11 preview is the next renderer step.");
            return;
        }

        if (AssetBrowser::IsTextPreviewable(asset))
        {
            std::string text = AssetBrowser::ReadSmallTextFile(readablePath, 65536);
            ImGui::TextWrapped("%s", text.c_str());
            return;
        }

        ImGui::TextWrapped("Readable file resolved, but this asset type has no renderer yet.");
    }

    void DrawXfbinInspectorTab(const AssetBrowser::AssetEntry& asset)
    {
        static XfbinInspector::XfbinInfo info{};
        static int lastIndex = -999999;

        if (lastIndex != g_SelectedAssetIndex)
        {
            info = {};
            lastIndex = g_SelectedAssetIndex;
        }

        if (ImGui::Button("Inspect XFBIN", ImVec2(150, 24)) || !info.Valid)
            XfbinInspector::Inspect(asset, info);

        ImGui::SameLine();

        if (ImGui::Button("Copy Report", ImVec2(120, 24)))
        {
            std::string report = XfbinInspector::BuildReport(info);
            ImGui::SetClipboardText(report.c_str());
        }

        ImGui::Separator();
        ImGui::Text("Status: %s", info.Status.c_str());
        ImGui::Text("Magic: %s", info.Magic.c_str());
        ImGui::Text("Strings %d | Chunks %d | Textures %d | Models %d | Materials %d | Animations %d | Lua %d",
            static_cast<int>(info.Strings.size()),
            static_cast<int>(info.Chunks.size()),
            static_cast<int>(info.TextureRefs.size()),
            static_cast<int>(info.ModelRefs.size()),
            static_cast<int>(info.MaterialRefs.size()),
            static_cast<int>(info.AnimationRefs.size()),
            static_cast<int>(info.LuaRefs.size()));
    }

    void DrawHexEditorTab(const AssetBrowser::AssetEntry& asset)
    {
        AssetHexEditor::Session& s = AssetHexEditor::GetSession();

        if (ImGui::Button("Load Read Only", ImVec2(120, 24)))
            AssetHexEditor::LoadAsset(asset, false);

        ImGui::SameLine();

        if (ImGui::Button("Load Override Copy", ImVec2(160, 24)))
            AssetHexEditor::LoadAsset(asset, true);

        ImGui::SameLine();

        if (ImGui::Button("Save", ImVec2(80, 24)))
            AssetHexEditor::Save();

        ImGui::SameLine();

        if (ImGui::Button("Revert", ImVec2(80, 24)))
            AssetHexEditor::Revert();

        ImGui::Separator();

        ImGui::TextWrapped("Status: %s", s.Status.c_str());

        if (!s.Loaded)
        {
            ImGui::TextDisabled("No file loaded in hex editor.");
            return;
        }

        static int offset = 0;
        static int editByte = 0;
        static char searchText[128]{};

        ImGui::InputInt("Offset", &offset);
        ImGui::SameLine();
        ImGui::InputInt("Byte", &editByte);

        if (ImGui::Button("Write Byte", ImVec2(120, 24)))
        {
            if (offset >= 0 && offset < static_cast<int>(s.Bytes.size()))
            {
                s.Bytes[offset] = static_cast<unsigned char>(editByte & 0xFF);
                s.Dirty = true;
                s.Status = "Byte edited. Save required.";
            }
        }

        ImGui::SameLine();
        ImGui::InputText("Search Text", searchText, sizeof(searchText));
        ImGui::SameLine();

        if (ImGui::Button("Find", ImVec2(70, 24)))
        {
            int found = AssetHexEditor::FindString(searchText, static_cast<size_t>(std::max(0, offset)));
            if (found >= 0)
                offset = found;
        }

        ImGui::Separator();

        std::string hex = AssetHexEditor::BuildHexView(static_cast<size_t>(std::max(0, offset)), 8192);
        ImGui::BeginChild("##HexView", ImVec2(0, 0), true);
        ImGui::TextWrapped("%s", hex.c_str());
        ImGui::EndChild();
    }

    void DrawRelatedTab(const AssetBrowser::AssetEntry& asset)
    {
        if (ImGui::Button("Rebuild Relations", ImVec2(160, 24)))
            AssetRelations::Rebuild();

        ImGui::SameLine();
        ImGui::TextDisabled("Family: %s", AssetRelations::GuessFamilyKey(asset).c_str());

        ImGui::Separator();

        auto related = AssetRelations::FindRelated(asset);
        const auto& assets = AssetBrowser::GetAssets();

        auto drawIndexes = [&](const char* title, const std::vector<int>& indexes)
        {
            if (!ImGui::TreeNode(title))
                return;

            for (int index : indexes)
            {
                if (index < 0 || index >= static_cast<int>(assets.size()))
                    continue;

                const auto& other = assets[index];
                std::string label = "[" + std::string(AssetBrowser::TypeName(other.Type)) + "] " + other.VirtualPath;

                if (ImGui::Selectable(label.c_str()))
                    JumpToAssetIndex(index);
            }

            ImGui::TreePop();
        };

        drawIndexes("Dependencies", related.Dependencies);
        drawIndexes("Used By", related.UsedBy);
        drawIndexes("Same Archive", related.SameArchive);
        drawIndexes("Same Folder", related.SameFolder);
        drawIndexes("Same Type", related.SameType);
        drawIndexes("Same Family", related.SameFamily);
    }

    void DrawStringsTab(const AssetBrowser::AssetEntry& asset)
    {
        ImGui::Text("Strings: %d", static_cast<int>(asset.Strings.size()));
        ImGui::Separator();

        ImGui::BeginChild("##AssetStrings", ImVec2(0, 0), true);
        for (const auto& s : asset.Strings)
            ImGui::TextWrapped("%s", s.c_str());
        ImGui::EndChild();
    }

    void DrawModPackageTab(const std::vector<AssetBrowser::AssetEntry>& assets)
    {
        auto selected = GetSelectedAssets(assets);

        ImGui::Text("Create Mod Package From Selected Assets");
        ImGui::Separator();

        ImGui::InputText("Mod Name", g_ModName, sizeof(g_ModName));
        ImGui::InputText("Author", g_ModAuthor, sizeof(g_ModAuthor));
        ImGui::InputTextMultiline("Description", g_ModDescription, sizeof(g_ModDescription), ImVec2(0, 80));

        ImGui::Text("Selected Assets: %d", static_cast<int>(selected.size()));

        if (ImGui::Button("Create Mod Package", ImVec2(180, 24)))
            AssetPackageCreator::CreatePackage(g_ModName, g_ModAuthor, g_ModDescription, selected);
    }

    void DrawPreviewPanel(const std::vector<AssetBrowser::AssetEntry>& assets, const AssetBrowser::AssetEntry& asset)
    {
        const char* tabs[] =
        {
            "Info",
            "Asset View",
            "XFBIN",
            "Hex Editor",
            "Related",
            "Strings",
            "Mod Package"
        };

        for (int i = 0; i < ARRAYSIZE(tabs); ++i)
        {
            if (i > 0)
                ImGui::SameLine();

            if (ImGui::Selectable(tabs[i], g_AssetPreviewTab == i, 0, ImVec2(105, 22)))
                g_AssetPreviewTab = i;
        }

        ImGui::Separator();

        switch (g_AssetPreviewTab)
        {
        case 0: DrawInfoTab(asset); break;
        case 1: DrawAssetViewTab(asset); break;
        case 2: DrawXfbinInspectorTab(asset); break;
        case 3: DrawHexEditorTab(asset); break;
        case 4: DrawRelatedTab(asset); break;
        case 5: DrawStringsTab(asset); break;
        case 6: DrawModPackageTab(assets); break;
        default: DrawInfoTab(asset); break;
        }
    }

    void DrawToolsPage()
    {
        ImGui::Text("Assets > V2 Tools");
        ImGui::Separator();

        ImGui::Text("Data:");
        ImGui::TextWrapped("%s", AssetBrowser::GetDataFolder().c_str());
        ImGui::Text("Mods:");
        ImGui::TextWrapped("%s", AssetBrowser::GetModsFolder().c_str());
        ImGui::Text("Dump:");
        ImGui::TextWrapped("%s", AssetBrowser::GetDumpFolder().c_str());
        ImGui::Text("Cache:");
        ImGui::TextWrapped("%s", AssetBrowser::GetCacheFolder().c_str());

        ImGui::Separator();

        if (ImGui::Button("Export Full Report", ImVec2(170, 24)))
            AssetReport::ExportFullReport("NartuoStorm2Revived/Dump/full_asset_report.txt");

        ImGui::SameLine();

        if (ImGui::Button("Export Dependency Report", ImVec2(210, 24)))
            AssetReport::ExportDependencyReport("NartuoStorm2Revived/Dump/dependency_report.txt");

        ImGui::SameLine();

        if (ImGui::Button("Rebuild Relations", ImVec2(170, 24)))
            AssetRelations::Rebuild();

        AssetToolkitUI::DrawToolkitPage();


        ImGui::Separator();
        ImGui::TextWrapped("V2 adds native CPK dumping, override creation, package creation, hash info, hex safe edits, better filters, notifications, and asset reports.");
    }
}

void DrawAssetsPage()
{
    if (g_SelectedSubPage >= 8 && g_SelectedSubPage <= 12)
    {
        DrawLuaPage();
        return;
    }

    if (g_SelectedSubPage == 13)
    {
        DrawToolsPage();
        return;
    }

    if (!g_AssetStartedAsyncScan)
    {
        AssetBrowser::LoadCache();
        AssetBrowser::StartAsyncScan(false);
        g_AssetStartedAsyncScan = true;
    }

    const auto& assets = AssetBrowser::GetAssets();

    const char* pageNames[] =
    {
        "Browser", "Stages", "Characters", "Models", "Textures", "Audio",
        "Packages", "Mod Overrides", "Lua Scripts", "Lua Autorun", "Lua Overrides",
        "Lua Console", "Lua Functions", "Tools"
    };

    ImGui::Text("Assets > %s",
        pageNames[g_SelectedSubPage >= 0 && g_SelectedSubPage < ARRAYSIZE(pageNames) ? g_SelectedSubPage : 0]);

    ImGui::Separator();

    DrawAssetStatsHeader();
    ImGui::Separator();

    DrawTopToolbar(assets);
    ImGui::Separator();

    DrawFilters();
    ImGui::Separator();

    ImVec2 available = ImGui::GetContentRegionAvail();
    float listWidth = available.x * 0.48f;

    if (listWidth < 440.0f)
        listWidth = 440.0f;

    ImGui::BeginChild("##AssetListColumn", ImVec2(listWidth, 0), false);
    DrawAssetList(assets);
    ImGui::EndChild();

    ImGui::SameLine();

    ImGui::BeginChild("##AssetPreviewColumn", ImVec2(0, 0), true);

    if (g_SelectedAssetIndex >= 0 && g_SelectedAssetIndex < static_cast<int>(assets.size()))
        DrawPreviewPanel(assets, assets[g_SelectedAssetIndex]);
    else
    {
        ImGui::TextWrapped("Select an asset.");
        ImGui::TextWrapped("Ctrl-click multi-select, Shift-click range-select, right-click for V2 asset actions.");
    }

    ImGui::EndChild();
}
