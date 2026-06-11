#include "DX11OverlayInternal.h"

void DrawGamePage()
{
    if (g_SelectedSubPage == 0)
    {
        ImGui::Text("Game > General");
        ImGui::Separator();

        ImGui::TextWrapped("General game tools live here. Patch-style toggles were moved to Patches so options are not duplicated.");

        ImGui::Separator();

        ImGui::Checkbox("Enable save backup helper", &g_GameSaveBackupEnabled);
        HelpMarker("Keeps backup/restore tools enabled for the local save system.");

        ImGui::Checkbox("Use custom save folder", &g_GameUseCustomSaveFolder);
        HelpMarker("Planned hook for redirecting SteamStorageLocal to a custom folder.");

        ImGui::Separator();

        if (ImGui::Button("Backup Save", ImVec2(150, 24)))
            Logger::Info("Game page: backup save requested");

        ImGui::SameLine();

        if (ImGui::Button("Restore Save", ImVec2(150, 24)))
            Logger::Info("Game page: restore save requested");

        ImGui::SameLine();

        if (ImGui::Button("Open Save Folder", ImVec2(170, 24)))
            Logger::Info("Game page: open save folder requested");

        ImGui::Separator();

        ImGui::Text("Useful Status");
        ImGui::Text("Remote storage: local");
        ImGui::Text("Stats: local/emulated");
        ImGui::Text("Game mode: offline-safe");
    }
    else if (g_SelectedSubPage == 1)
    {
        ImGui::Text("Game > Saves");
        ImGui::Separator();

        ActiveCheckbox("Enable save backup helper", &g_GameSaveBackupEnabled, "UI toggle for save backup/restore workflow.");
        PlannedCheckbox("Use custom save folder", &g_GameUseCustomSaveFolder, "Needs SteamStorageLocal path override exposed.");

        if (ImGui::Button("Backup Save", ImVec2(150, 24)))
            Logger::Info("Game page: backup save requested");

        ImGui::SameLine();

        if (ImGui::Button("Restore Save", ImVec2(150, 24)))
            Logger::Info("Game page: restore save requested");

        ImGui::SameLine();

        if (ImGui::Button("Validate Save", ImVec2(150, 24)))
            Logger::Info("Game page: validate save requested");

        ImGui::Separator();
        ImGui::TextWrapped("Unlocks, intro skip, cutscene skip, borderless, ultrawide, and similar patch features are only in Patches.");
    }
    else
    {
        ImGui::Text("Game > Information");
        ImGui::Separator();

        ImGui::Text("Process: NSUNS2.exe");
        ImGui::Text("Renderer: D3D11");
        ImGui::Text("Project: NS2 Revived");
        ImGui::Text("Build: %s %s", __DATE__, __TIME__);

        ImGui::Separator();

        ImGui::Text("Game folder:");
        ImGui::TextWrapped("%s", AssetBrowser::GetGameFolder().c_str());

        ImGui::Text("Dump folder:");
        ImGui::TextWrapped("%s", AssetBrowser::GetDumpFolder().c_str());

        ImGui::Text("Mods folder:");
        ImGui::TextWrapped("NartuoStorm2Revived\\mods");

        ImGui::Separator();

        ImGui::Text("Asset preload:");
        ImGui::Text("%s", AssetPreloadManager::IsPreloading() ? "loading" : "idle/complete");
    }
}
