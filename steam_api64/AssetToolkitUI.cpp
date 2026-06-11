#include "StdInc.h"
#include "AssetToolkitUI.h"
#include "AssetBrowser.h"
#include "AssetDuplicateFinder.h"
#include "AssetConflictScanner.h"
#include "AssetBackupManager.h"
#include "AssetBulkExtractor.h"
#include "imgui.h"

namespace AssetToolkitUI
{
    void DrawToolkitPage()
    {
        ImGui::Separator();
        ImGui::Text("V3 Asset Toolkit");
        ImGui::Separator();

        if (ImGui::CollapsingHeader("Duplicate Finder", ImGuiTreeNodeFlags_DefaultOpen))
        {
            if (ImGui::Button("Find By Name", ImVec2(130, 24))) AssetDuplicateFinder::RebuildByName();
            ImGui::SameLine();
            if (ImGui::Button("Find By Size", ImVec2(130, 24))) AssetDuplicateFinder::RebuildBySize();
            ImGui::SameLine();
            if (ImGui::Button("Find By Hash", ImVec2(130, 24))) AssetDuplicateFinder::RebuildByHash();
            ImGui::SameLine();
            if (ImGui::Button("Export Duplicates", ImVec2(160, 24)))
                AssetDuplicateFinder::ExportReport("NartuoStorm2Revived/Dump/duplicate_report.txt");

            const auto& groups = AssetDuplicateFinder::GetGroups();
            const auto& assets = AssetBrowser::GetAssets();

            ImGui::Text("Duplicate groups: %d", static_cast<int>(groups.size()));

            ImGui::BeginChild("##DuplicateGroups", ImVec2(0, 160), true);
            for (const auto& g : groups)
            {
                if (!ImGui::TreeNode(g.Key.c_str())) continue;
                for (int index : g.AssetIndexes)
                    if (index >= 0 && index < static_cast<int>(assets.size()))
                        ImGui::BulletText("%s", assets[index].VirtualPath.c_str());
                ImGui::TreePop();
            }
            ImGui::EndChild();
        }

        if (ImGui::CollapsingHeader("Mod Conflict Scanner", ImGuiTreeNodeFlags_DefaultOpen))
        {
            if (ImGui::Button("Scan Conflicts", ImVec2(150, 24))) AssetConflictScanner::Rebuild();
            ImGui::SameLine();
            if (ImGui::Button("Export Conflicts", ImVec2(160, 24)))
                AssetConflictScanner::ExportReport("NartuoStorm2Revived/Dump/conflict_report.txt");

            const auto& conflicts = AssetConflictScanner::GetConflicts();
            ImGui::Text("Conflicts: %d", static_cast<int>(conflicts.size()));

            ImGui::BeginChild("##Conflicts", ImVec2(0, 150), true);
            for (const auto& c : conflicts)
            {
                if (!ImGui::TreeNode(c.VirtualPath.c_str())) continue;
                for (const auto& p : c.Providers)
                    ImGui::BulletText("%s", p.c_str());
                ImGui::TreePop();
            }
            ImGui::EndChild();
        }

        if (ImGui::CollapsingHeader("Bulk Extraction", ImGuiTreeNodeFlags_DefaultOpen))
        {
            if (ImGui::Button("Extract All CPK Entries", ImVec2(190, 24)))
                AssetBulkExtractor::ExtractAllCpkEntries();
            ImGui::SameLine();
            if (ImGui::Button("Extract All Models", ImVec2(160, 24)))
                AssetBulkExtractor::ExtractCurrentCategory(AssetBrowser::AssetType::Model);
            ImGui::SameLine();
            if (ImGui::Button("Extract All Textures", ImVec2(170, 24)))
                AssetBulkExtractor::ExtractCurrentCategory(AssetBrowser::AssetType::Texture);
            ImGui::TextWrapped("Bulk extraction uses your native CpkArchive extractor. Large archive extraction can take time.");
        }

        if (ImGui::CollapsingHeader("Backups", ImGuiTreeNodeFlags_DefaultOpen))
        {
            if (ImGui::Button("Export Backup Manifest", ImVec2(190, 24)))
                AssetBackupManager::ExportManifest("NartuoStorm2Revived/Dump/backup_manifest_export.txt");

            ImGui::SameLine();
            ImGui::TextDisabled("Root: %s", AssetBackupManager::GetBackupRoot().string().c_str());

            auto backups = AssetBackupManager::ListBackups();
            ImGui::Text("Backups: %d", static_cast<int>(backups.size()));

            ImGui::BeginChild("##Backups", ImVec2(0, 150), true);
            for (const auto& b : backups)
                ImGui::TextWrapped("%s -> %s", b.Name.c_str(), b.BackupPath.c_str());
            ImGui::EndChild();
        }
    }
}
