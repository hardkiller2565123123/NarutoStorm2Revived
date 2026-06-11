#include "DX11OverlayInternal.h"

void DrawPatchesPage()
{
    if (g_SelectedSubPage == 0)
    {
        ImGui::Text("Patches > Display");
        ImGui::Separator();

        ActiveCheckbox("Borderless fullscreen helper", &g_PatchBorderlessFullscreen, "Uses the existing WindowedFullscreen system. This is the only borderless toggle now.");
        PlannedCheckbox("Native ultrawide support", &g_PatchNativeUltrawide, "Needs aspect ratio/FOV patch addresses. UI is ready.");
        PlannedCheckbox("Multi-monitor support", &g_PatchMultiMonitor, "Needs monitor selection/window placement options.");
        PlannedCheckbox("High refresh fix", &g_PatchHighRefresh, "Needs timing/FPS limiter investigation.");

        ImGui::Separator();
        ImGui::TextWrapped("Display patches live only here. Duplicate display patch controls were removed from Game.");
    }
    else if (g_SelectedSubPage == 1)
    {
        ImGui::Text("Patches > Gameplay");
        ImGui::Separator();

        PlannedCheckbox("Disable intro", &g_PatchDisableIntro, "Needs intro movie/file redirect.");
        PlannedCheckbox("Auto skip cutscenes", &g_PatchAutoSkipCutscenes, "Needs cutscene state hook.");
        PlannedCheckbox("Unlock all temporary", &g_PatchUnlockAllTemp, "Needs unlock table and temporary state layer.");
        PlannedCheckbox("Unlock all permanent", &g_PatchUnlockAllPermanent, "Needs save mutation support.");
        ActiveCheckbox("Background running / pause helper", &g_PatchBackgroundPauseFix, "Existing window keep-alive/background helper should stay enabled.");
        ActiveCheckbox("Skip online checks", &g_PatchSkipOnlineChecks, "Uses the offline/network-blocking framework already loaded by NS2 Revived.");

        ImGui::Separator();
        ImGui::TextWrapped("Gameplay patches live only here. Game tab now focuses on saves and information.");
    }
    else if (g_SelectedSubPage == 2)
    {
        ImGui::Text("Patches > Safety");
        ImGui::Separator();

        ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "[ACTIVE]");
        ImGui::SameLine();
        ImGui::TextWrapped("Feature is backed by current framework code or an existing helper.");

        ImGui::TextColored(ImVec4(1.0f, 0.65f, 0.2f, 1.0f), "[PLANNED]");
        ImGui::SameLine();
        ImGui::TextWrapped("UI is preserved, but the actual patch still needs addresses/hooks before it should touch memory.");

        ImGui::Separator();

        ImGui::Checkbox("Only enable safe patches by default", &g_PatchSkipOnlineChecks);
        HelpMarker("Keeps current safe/offline patch helpers enabled, while planned memory patches stay informational.");
    }
    else
    {
        ImGui::Text("Patches > Status");
        ImGui::Separator();

        ImGui::Text("Borderless fullscreen: %s", g_PatchBorderlessFullscreen ? "enabled" : "disabled");
        ImGui::Text("Background helper: %s", g_PatchBackgroundPauseFix ? "enabled" : "disabled");
        ImGui::Text("Offline check skip: %s", g_PatchSkipOnlineChecks ? "enabled" : "disabled");
    }
}
