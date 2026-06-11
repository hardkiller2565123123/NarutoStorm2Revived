#include "DX11OverlayInternal.h"

void DrawGraphicsPage()
{
    auto& s = NS2Config::Screen();

    if (g_SelectedSubPage == 0)
    {
        ImGui::Text("Graphics > Display");
        ImGui::Separator();

        ImGui::InputInt("Width", &s.Width);
        HelpMarker("Game window/backbuffer width saved to config.ini.");

        ImGui::InputInt("Height", &s.Height);
        HelpMarker("Game window/backbuffer height saved to config.ini.");

        ImGui::InputInt("Aspect X", &s.AspectX);
        HelpMarker("Horizontal aspect ratio value. Usually 16 for widescreen.");

        ImGui::InputInt("Aspect Y", &s.AspectY);
        HelpMarker("Vertical aspect ratio value. Usually 9 for widescreen.");

        ImGui::Separator();
        ImGui::Text("Presets");

        ResolutionPreset("1280x720", 1280, 720);
        ImGui::SameLine();
        ResolutionPreset("1600x900", 1600, 900);
        ImGui::SameLine();
        ResolutionPreset("1920x1080", 1920, 1080);

        ResolutionPreset("2560x1440", 2560, 1440);
        ImGui::SameLine();
        ResolutionPreset("3840x2160", 3840, 2160);

        ImGui::Separator();

        IniCheckbox("Windowed", s.Windowed, "Uses the game's windowed mode setting.");
        IniCheckbox("VSync", s.VSync, "Syncs frame presentation to the monitor refresh rate.");
        IniCheckbox("Show Cursor", s.ShowCursor, "Controls the game's cursor visibility setting.");

        ImGui::Separator();

        if (ImGui::Button("Save Config", ImVec2(135.0f, 24.0f)))
            NS2Config::Save();

        ImGui::SameLine();

        if (ImGui::Button("Reload Config", ImVec2(135.0f, 24.0f)))
            NS2Config::Load();

        ImGui::SameLine();

        if (ImGui::Button("Reset Graphics", ImVec2(145.0f, 24.0f)))
            NS2Config::ResetGraphics();
    }
    else if (g_SelectedSubPage == 1)
    {
        ImGui::Text("Graphics > Anti-Aliasing");
        ImGui::Separator();

        IniCheckbox("FXAA", s.FXAA, "Fast post-process anti-aliasing. Cheap and usually safe.");
        IniCheckbox("SMAA", s.SMAA, "Sharper post-process anti-aliasing. May not work in all builds.");
        IniCheckbox("MSAA", s.MSAA, "Multi-sample anti-aliasing setting from config.ini.");

        ImGui::SliderInt("SSAA", &s.SSAA, 0, 4);
        HelpMarker("Super-sampling level. Higher values can heavily reduce performance.");

        ImGui::Separator();

        if (ImGui::Button("Disable All AA", ImVec2(150.0f, 24.0f)))
        {
            s.FXAA = 0;
            s.SMAA = 0;
            s.MSAA = 0;
            s.SSAA = 0;
        }

        ImGui::SameLine();

        if (ImGui::Button("Recommended", ImVec2(150.0f, 24.0f)))
        {
            s.FXAA = 1;
            s.SMAA = 0;
            s.MSAA = 1;
            s.SSAA = 0;
        }

        ImGui::Separator();

        if (ImGui::Button("Save Config", ImVec2(135.0f, 24.0f)))
            NS2Config::Save();
    }
    else if (g_SelectedSubPage == 2)
    {
        ImGui::Text("Graphics > Quality");
        ImGui::Separator();

        ImGui::SliderInt("Shadow Quality", &s.ShadowQuality, 0, 3);
        HelpMarker("Controls shadow detail. Higher values may cost performance.");

        IniCheckbox("Mipmaps", s.MipMap, "Improves distant texture filtering and stability.");
        IniCheckbox("Motion Blur", s.MotionBlur, "Enables or disables camera/object motion blur.");
        IniCheckbox("Glare", s.Glare, "Controls bright light glare/bloom-style effects.");

        ImGui::Separator();
        ImGui::Text("Quick Presets");

        if (ImGui::Button("Performance", ImVec2(135.0f, 24.0f)))
        {
            s.ShadowQuality = 0;
            s.MotionBlur = 0;
            s.Glare = 0;
            s.FXAA = 1;
            s.SMAA = 0;
            s.MSAA = 0;
            s.SSAA = 0;
        }

        ImGui::SameLine();

        if (ImGui::Button("Balanced", ImVec2(135.0f, 24.0f)))
        {
            s.ShadowQuality = 1;
            s.MotionBlur = 1;
            s.Glare = 1;
            s.FXAA = 1;
            s.SMAA = 0;
            s.MSAA = 1;
            s.SSAA = 0;
        }

        ImGui::SameLine();

        if (ImGui::Button("Quality", ImVec2(135.0f, 24.0f)))
        {
            s.ShadowQuality = 3;
            s.MotionBlur = 1;
            s.Glare = 1;
            s.FXAA = 1;
            s.SMAA = 1;
            s.MSAA = 1;
            s.SSAA = 1;
        }

        ImGui::Separator();

        if (ImGui::Button("Save Config", ImVec2(135.0f, 24.0f)))
            NS2Config::Save();
    }
    else if (g_SelectedSubPage == 3)
    {
        ImGui::Text("Graphics > Overlay");
        ImGui::Separator();

        ImGui::Checkbox("Show top-left status", &g_ShowStatusBar);
        HelpMarker("Shows or hides the small NS2 Revived status line.");

        ImGui::Checkbox("Show project name", &g_StatusShowName);
        HelpMarker("Adds NS2 Revived text to the status line.");

        ImGui::Checkbox("Show build date", &g_StatusShowBuild);
        HelpMarker("Adds build date/time to the status line.");

        ImGui::Checkbox("Show FPS", &g_StatusShowFps);
        HelpMarker("Adds FPS to the status line.");

        ImGui::Checkbox("Show F1 hint", &g_StatusShowHotkey);
        HelpMarker("Adds the F1 hotkey hint to the status line.");

        ImGui::Checkbox("Draw ImGui mouse cursor", &g_DrawMouseCursor);
        HelpMarker("Draws ImGui software cursor while the overlay menu is open.");

        ImGui::Checkbox("Block game input while menu is open", &g_BlockGameInputWhenOpen);
        HelpMarker("Prevents game input from firing while using the menu.");

        ImGui::Separator();

        if (ImGui::Button("FPS Only", ImVec2(120.0f, 24.0f)))
        {
            g_StatusShowName = false;
            g_StatusShowBuild = false;
            g_StatusShowFps = true;
            g_StatusShowHotkey = false;
        }

        ImGui::SameLine();

        if (ImGui::Button("Full Status", ImVec2(120.0f, 24.0f)))
        {
            g_StatusShowName = true;
            g_StatusShowBuild = true;
            g_StatusShowFps = true;
            g_StatusShowHotkey = true;
        }
    }
    else
    {
        ImGui::Text("Graphics > Advanced");
        ImGui::Separator();

        ImGui::Text("Config Path:");
        ImGui::TextWrapped("%s", NS2Config::GetPath().c_str());

        ImGui::Separator();
        ImGui::Text("Renderer: D3D11");
        ImGui::Text("HWND: 0x%p", g_GameWindow);
        ImGui::Text("Device: 0x%p", g_Device);
        ImGui::Text("Context: 0x%p", g_Context);
        ImGui::Text("RTV: 0x%p", g_RenderTargetView);
    }
}
