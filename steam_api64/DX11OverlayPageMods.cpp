#include "DX11OverlayInternal.h"

void DrawModsPage()
{
    if (g_SelectedSubPage == 0)
    {
        ImGui::Text("Mods > Loader");
        HelpMarker("Basic toggles for the local mod loader layer.");
        ImGui::Separator();

        ImGui::Checkbox("Enable mod loading", &g_ModsEnabled);
        HelpMarker("Turns the local mod loader layer on or off.");

        ImGui::Checkbox("Load DLL mods", &g_ModsLoadDll);
        HelpMarker("Allows DLL-based mods from the mods folder to be loaded by the framework when supported.");

        ImGui::Checkbox("Verbose mod logging", &g_ModsVerboseLog);
        HelpMarker("Writes extra mod loader details to the log.");
    }
    else
    {
        ImGui::Text("Mods > Folder");
        HelpMarker("Folder actions for the local mods directory.");
        ImGui::Separator();

        ImGui::Text("NartuoStorm2Revived\\mods");
        HelpMarker("Default relative mods folder used by this overlay/framework build.");

        if (ImGui::Button("Reload mods", ImVec2(170.0f, 25.0f)))
            Logger::Info("DX11 overlay: reload mods requested");

        if (ImGui::Button("Open mods folder", ImVec2(170.0f, 25.0f)))
            Logger::Info("DX11 overlay: open mods folder requested");
    }
}
