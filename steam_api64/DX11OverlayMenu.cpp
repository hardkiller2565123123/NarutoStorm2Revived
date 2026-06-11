#include "DX11OverlayInternal.h"

void DrawDropdownPanel()
{
    if (!g_MenuOpen || g_SelectedTab < 0)
        return;

    ImGuiViewport* viewport = ImGui::GetMainViewport();

    float x = 95.0f + static_cast<float>(g_SelectedTab) * 88.0f;
    float y = 31.0f * g_MenuAnimation;
    float panelWidth = std::min(viewport->Size.x - x - 12.0f, 1220.0f);
    float panelHeight = std::min(viewport->Size.y - y - 44.0f, 650.0f);

    if (panelWidth < 760.0f)
    {
        x = 12.0f;
        panelWidth = viewport->Size.x - 24.0f;
    }

    if (panelHeight < 360.0f)
        panelHeight = viewport->Size.y - y - 20.0f;

    ImGui::SetNextWindowPos(ImVec2(x, y), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(panelWidth, panelHeight), ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.90f);

    ImGui::Begin(
        "##NS2ShipwrightDropdown",
        nullptr,
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoCollapse);

    ImGui::BeginChild("##NS2LeftList", ImVec2(220.0f, 0.0f), true);
    ImGui::Text("Options");
    ImGui::Separator();

    switch (g_SelectedTab)
    {
    case TAB_GRAPHICS:
        DrawLeftButton("Display", 0);
        DrawLeftButton("Anti-Aliasing", 1);
        DrawLeftButton("Quality", 2);
        DrawLeftButton("Overlay", 3);
        DrawLeftButton("Advanced", 4);
        break;

    case TAB_CONTROLS:
        DrawLeftButton("General", 0);
        DrawLeftButton("Mouse", 1);
        DrawLeftButton("Binds", 2);
        DrawLeftButton("Advanced", 3);
        break;

    case TAB_GAME:
        DrawLeftButton("General", 0);
        DrawLeftButton("Saves", 1);
        DrawLeftButton("Information", 2);
        break;

    case TAB_CAMERA:
        DrawLeftButton("Free Camera", 0);
        DrawLeftButton("Coordinates", 1);
        DrawLeftButton("Viewer Tools", 2);
        break;

    case TAB_ASSETS:
        DrawLeftButton("Browser", 0);
        DrawLeftButton("Stages", 1);
        DrawLeftButton("Characters", 2);
        DrawLeftButton("Models", 3);
        DrawLeftButton("Textures", 4);
        DrawLeftButton("Audio", 5);
        DrawLeftButton("Packages", 6);
        DrawLeftButton("Mod Overrides", 7);
        DrawLeftButton("Lua Scripts", 8);
        DrawLeftButton("Lua Autorun", 9);
        DrawLeftButton("Lua Overrides", 10);
        DrawLeftButton("Lua Console", 11);
        DrawLeftButton("Lua Functions", 12);
        DrawLeftButton("Tools", 13);
        break;

    case TAB_MODS:
        DrawLeftButton("Loader", 0);
        DrawLeftButton("Folders", 1);
        break;

    case TAB_PATCHES:
        DrawLeftButton("Display", 0);
        DrawLeftButton("Gameplay", 1);
        DrawLeftButton("Safety", 2);
        DrawLeftButton("Status", 3);
        break;

    case TAB_DEBUG:
        DrawLeftButton("Performance", 0);
        DrawLeftButton("Render", 1);
        DrawLeftButton("Steam", 2);
        DrawLeftButton("Network", 3);
        DrawLeftButton("Storage", 4);
        DrawLeftButton("Mods", 5);
        DrawLeftButton("Tools", 6);
        break;
    }

    ImGui::EndChild();
    ImGui::SameLine();

    ImGui::BeginChild("##NS2PageArea", ImVec2(0.0f, 0.0f), true);

    switch (g_SelectedTab)
    {
    case TAB_GRAPHICS: DrawGraphicsPage(); break;
    case TAB_CONTROLS: DrawControlsPage(); break;
    case TAB_GAME: DrawGamePage(); break;
    case TAB_CAMERA: DrawCameraPage(); break;
    case TAB_ASSETS: DrawAssetsPage(); break;
    case TAB_MODS: DrawModsPage(); break;
    case TAB_PATCHES: DrawPatchesPage(); break;
    case TAB_DEBUG: DrawDebugPage(); break;
    }

    ImGui::EndChild();
    ImGui::End();
}

void DrawAnimatedTopMenu()
{
    float target = g_MenuOpen ? 1.0f : 0.0f;
    float speed = ImGui::GetIO().DeltaTime * 14.0f;
    g_MenuAnimation = AnimateToward(g_MenuAnimation, target, speed);

    if (g_MenuAnimation < 0.01f && !g_MenuOpen)
    {
        g_MenuAnimation = 0.0f;
        g_SelectedTab = TAB_NONE;
        return;
    }

    ImGuiViewport* viewport = ImGui::GetMainViewport();

    const float barHeight = 30.0f;
    const float y = -barHeight + (barHeight * g_MenuAnimation);

    ImGui::SetNextWindowPos(ImVec2(0.0f, y), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, barHeight), ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.92f);

    ImGui::Begin(
        "##NS2AnimatedTopMenuBar",
        nullptr,
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoScrollWithMouse |
        ImGuiWindowFlags_NoCollapse);

    ImGui::SetCursorPos(ImVec2(8.0f, 5.0f));

    ImGui::Text("NS2 Revived");
    ImGui::SameLine(95.0f);

    DrawTabButton("Graphics", TAB_GRAPHICS); ImGui::SameLine();
    DrawTabButton("Controls", TAB_CONTROLS); ImGui::SameLine();
    DrawTabButton("Game", TAB_GAME); ImGui::SameLine();
    DrawTabButton("Camera", TAB_CAMERA); ImGui::SameLine();
    DrawTabButton("Assets", TAB_ASSETS); ImGui::SameLine();
    DrawTabButton("Mods", TAB_MODS); ImGui::SameLine();
    DrawTabButton("Patches", TAB_PATCHES); ImGui::SameLine();
    DrawTabButton("Debug", TAB_DEBUG);

    float rightTextX = viewport->Size.x - 240.0f;

    if (rightTextX > ImGui::GetCursorPosX() + 20.0f)
        ImGui::SameLine(rightTextX);

    ImGui::Text("FPS %d | LAT 0 ms", g_CurrentFps);

    ImGui::End();

    DrawDropdownPanel();
}
