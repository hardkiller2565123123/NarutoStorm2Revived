#include "DX11OverlayInternal.h"

void DrawControlsPage()
{
    if (g_SelectedSubPage == 0)
    {
        ImGui::Text("Controls > General");
        HelpMarker("General input settings for the overlay/menu layer.");
        ImGui::Separator();

        ImGui::Checkbox("Controller enabled", &g_ControlsControllerEnabled);
        HelpMarker("Allows controller input to be used by overlay features that support controller navigation.");

        ImGui::Checkbox("Keyboard navigation", &g_ControlsKeyboardNav);
        HelpMarker("Allows ImGui keyboard navigation inside the menu.");

        ImGui::Checkbox("Block game input while menu is open", &g_BlockGameInputWhenOpen);
        HelpMarker("Prevents the game from receiving input while using the menu.");
    }
    else if (g_SelectedSubPage == 1)
    {
        ImGui::Text("Controls > Mouse");
        HelpMarker("Mouse behavior while the F1 overlay menu is open.");
        ImGui::Separator();

        ImGui::Checkbox("Capture mouse when menu is open", &g_ControlsCaptureMouse);
        HelpMarker("Keeps mouse interaction focused on the overlay while the menu is open.");

        ImGui::Checkbox("Draw ImGui mouse cursor", &g_DrawMouseCursor);
        HelpMarker("Draws ImGui software cursor while the menu is open.");
    }
    else if (g_SelectedSubPage == 2)
    {
        ImGui::Text("Controls > Binds");
        HelpMarker("Hotkey and input bind information.");
        ImGui::Separator();

        ImGui::Text("Open menu: F1");
        HelpMarker("Press F1 once to open the overlay menu and press it again to close it.");

        if (ImGui::Button("Refresh controllers", ImVec2(190.0f, 25.0f)))
            Logger::Info("DX11 overlay: refresh controllers requested");

        if (ImGui::Button("Reset binds", ImVec2(190.0f, 25.0f)))
            Logger::Info("DX11 overlay: reset binds requested");
    }
    else
    {
        ImGui::Text("Controls > Advanced");
        HelpMarker("Advanced input safety options.");
        ImGui::Separator();

        ImGui::Checkbox("Master input blocking", &g_ControlsBlockInput);
        HelpMarker("Master toggle for overlay input-blocking behavior.");

        ImGui::TextWrapped("Graphics owns display options. Patches owns game patch toggles. Controls owns input behavior only.");
    }
}
