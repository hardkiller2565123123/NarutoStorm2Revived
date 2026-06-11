#include "DX11OverlayInternal.h"

void DrawDebugPage()
{
    if (g_SelectedSubPage == 0)
    {
        ImGui::Text("Debug > Performance");
        HelpMarker("Runtime performance counters for the overlay and hooked renderer.");
        ImGui::Separator();

        ImGui::Checkbox("Show FPS graph", &g_DebugFpsGraph);
        ImGui::Checkbox("Show frame time graph", &g_DebugFrameTimeGraph);
        ImGui::Checkbox("Show min / avg / max FPS", &g_DebugShowMinAvgMax);
        ImGui::Checkbox("Show Present counter", &g_DebugShowPresentCount);
        ImGui::Checkbox("Show ResizeBuffers counter", &g_DebugShowResizeCount);
        ImGui::Checkbox("Show ImGui stats", &g_DebugShowImGuiStats);

        if (g_DebugFpsGraph)
        {
            ImGui::PlotLines(
                "FPS",
                g_FpsHistory,
                ARRAYSIZE(g_FpsHistory),
                g_FpsHistoryOffset,
                nullptr,
                0.0f,
                144.0f,
                ImVec2(460.0f, 120.0f));
        }

        ImGui::Text("Current FPS: %d", g_CurrentFps);
        ImGui::Text("Frame time: %.3f ms", g_CurrentFps > 0 ? 1000.0f / static_cast<float>(g_CurrentFps) : 0.0f);
    }
    else if (g_SelectedSubPage == 1)
    {
        ImGui::Text("Debug > Render");
        HelpMarker("Renderer, swapchain, and render-target diagnostic information.");
        ImGui::Separator();

        ImGui::Checkbox("Show render handles", &g_DebugShowRenderHandles);
        ImGui::Checkbox("Show swapchain info", &g_DebugShowSwapchainInfo);
        ImGui::Checkbox("Show window state", &g_DebugShowWindowState);
        ImGui::Checkbox("Show refresh rate", &g_DebugShowRefreshRate);

        if (ImGui::Button("Force recreate render target", ImVec2(220.0f, 24.0f)))
        {
            CleanupRenderTarget();
            Logger::Info("DX11 overlay: render target manually recreated");
        }

        if (g_DebugShowRenderHandles)
        {
            ImGui::Separator();
            ImGui::Text("HWND: 0x%p", g_GameWindow);
            ImGui::Text("Device: 0x%p", g_Device);
            ImGui::Text("Context: 0x%p", g_Context);
            ImGui::Text("RTV: 0x%p", g_RenderTargetView);
        }
    }
    else if (g_SelectedSubPage == 2)
    {
        ImGui::Text("Debug > Steam");
        HelpMarker("Steam proxy/emulation debug options.");
        ImGui::Separator();

        ImGui::Checkbox("Log Steam interface calls", &g_DebugLogSteamInterfaces);
        ImGui::Checkbox("Log callbacks", &g_DebugLogCallbacks);
        ImGui::Checkbox("Log call results", &g_DebugLogCallResults);
        ImGui::Checkbox("Show fake SteamID", &g_DebugShowSteamId);
        ImGui::Checkbox("Show lobby state", &g_DebugShowLobbyState);

        ImGui::Separator();
        ImGui::Text("Steam proxy: loaded");
        ImGui::Text("Remote storage: local");
        ImGui::Text("Stats: emulated");
        ImGui::Text("Matchmaking: emulated");

        if (ImGui::Button("Dump Steam registry", ImVec2(180.0f, 24.0f)))
            Logger::Info("DX11 overlay: dump Steam registry requested");
    }
    else if (g_SelectedSubPage == 3)
    {
        ImGui::Text("Debug > Network");
        HelpMarker("Network hook diagnostics for DNS and socket-related code.");
        ImGui::Separator();

        ImGui::Checkbox("Log DNS requests", &g_DebugLogDns);
        ImGui::Checkbox("Log socket connects", &g_DebugLogSocketConnects);
        ImGui::Checkbox("Show last blocked host/IP", &g_DebugShowLastBlockedAddress);
        ImGui::Checkbox("Show network hook status", &g_DebugShowNetworkHookStatus);
        ImGui::Checkbox("Force block traffic", &g_DebugForceBlockTraffic);
        ImGui::Checkbox("LAN only", &g_DebugLanOnly);

        ImGui::Separator();
        ImGui::Text("connect hook: enabled");
        ImGui::Text("WSAConnect hook: enabled");
        ImGui::Text("getaddrinfo hook: enabled");
        ImGui::Text("GetAddrInfoW hook: enabled");
    }
    else if (g_SelectedSubPage == 4)
    {
        ImGui::Text("Debug > Storage");
        HelpMarker("Local storage and save debug options.");
        ImGui::Separator();

        ImGui::Checkbox("Log storage reads", &g_DebugLogStorageReads);
        ImGui::Checkbox("Log storage writes", &g_DebugLogStorageWrites);
        ImGui::Checkbox("Show storage folder", &g_DebugShowStorageFolder);
        ImGui::Checkbox("Show last save file", &g_DebugShowLastSaveFile);

        ImGui::Separator();
        ImGui::Text("Storage mode: local");
        ImGui::Text("Remote storage: emulated");
    }
    else if (g_SelectedSubPage == 5)
    {
        ImGui::Text("Debug > Mods");
        HelpMarker("Mod loader diagnostics.");
        ImGui::Separator();

        ImGui::Checkbox("Show mods folder", &g_DebugShowModsFolder);
        ImGui::Checkbox("Log mod errors", &g_DebugLogModErrors);
        ImGui::Checkbox("Show loaded mod count", &g_DebugShowLoadedModCount);

        ImGui::Separator();
        ImGui::Text("Mods folder: NartuoStorm2Revived\\mods");
        ImGui::Text("Loaded mods: 0");
    }
    else
    {
        ImGui::Text("Debug > Tools");
        HelpMarker("Emergency/debug actions.");
        ImGui::Separator();

        if (ImGui::Button("Write test log", ImVec2(180.0f, 24.0f)))
            Logger::Info("DX11 overlay: test log from debug tools");

        if (ImGui::Button("Emergency disable overlay", ImVec2(220.0f, 24.0f)))
            g_OverlayEnabled = false;
    }
}
