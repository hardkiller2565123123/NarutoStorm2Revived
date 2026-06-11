#include "DX11OverlayInternal.h"

void DrawCameraPage()
{
    if (g_SelectedSubPage == 0)
    {
        ImGui::Text("Camera > Free Camera");
        ImGui::Separator();

        PlannedCheckbox("Enable free camera", &g_CameraFreeCam, "Needs camera address mapping; UI and state are ready.");
        PlannedCheckbox("Use free camera during cutscenes", &g_CameraAllowDuringCutscenes, "Needs cutscene camera ownership patch.");
        PlannedCheckbox("Remove HUD", &g_CameraHideHud, "Needs HUD draw flag or UI visibility address.");

        ImGui::SliderFloat("FOV", &g_CameraFov, 10.0f, 140.0f);
        ImGui::SliderFloat("Speed", &g_CameraSpeed, 0.05f, 10.0f);
        ImGui::SliderFloat("Boost", &g_CameraBoost, 1.0f, 20.0f);
        ImGui::SliderFloat("Smoothing", &g_CameraSmooth, 0.0f, 1.0f);
    }
    else if (g_SelectedSubPage == 1)
    {
        ImGui::Text("Camera > Coordinates");
        ImGui::Separator();

        ImGui::InputFloat3("Position", g_CameraPos);
        ImGui::InputFloat3("Rotation", g_CameraRot);

        if (ImGui::Button("Copy Camera Position", ImVec2(190, 24)))
        {
            g_CameraCopyQueued = true;
            Logger::Info("Camera page: copy camera position requested");
        }

        ImGui::SameLine();

        if (ImGui::Button("Reset Camera", ImVec2(140, 24)))
        {
            g_CameraPos[0] = g_CameraPos[1] = g_CameraPos[2] = 0.0f;
            g_CameraRot[0] = g_CameraRot[1] = g_CameraRot[2] = 0.0f;
        }
    }
    else
    {
        ImGui::Text("Camera > Viewer Tools");
        ImGui::Separator();

        PlannedCheckbox("Hitbox viewer", &g_DebugShowRenderHandles, "Needs collision/hitbox data address mapping.");
        PlannedCheckbox("Hurtbox viewer", &g_DebugShowWindowState, "Needs hurtbox structure mapping.");
        PlannedCheckbox("Collision viewer", &g_DebugShowSwapchainInfo, "Needs stage collision mesh decode.");
        PlannedCheckbox("Spawn point viewer", &g_DebugShowRefreshRate, "Needs match/stage spawn data mapping.");
        PlannedCheckbox("Event viewer", &g_DebugShowImGuiStats, "Needs event/script state hook.");
    }
}
