#include "StdInc.h"
#include "DX11Overlay.h"
#include "Logger.h"

#include "imgui.h"
#include "imgui_impl_dx11.h"
#include "imgui_impl_win32.h"

#include <d3d11.h>
#include <dxgi.h>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

typedef HRESULT(__stdcall* PresentFn)(IDXGISwapChain* swapChain, UINT syncInterval, UINT flags);
typedef HRESULT(__stdcall* ResizeBuffersFn)(IDXGISwapChain* swapChain, UINT bufferCount, UINT width, UINT height, DXGI_FORMAT newFormat, UINT swapChainFlags);

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

static PresentFn g_OriginalPresent = nullptr;
static ResizeBuffersFn g_OriginalResizeBuffers = nullptr;

static bool g_HookReady = false;
static bool g_PresentSeen = false;
static bool g_ImGuiReady = false;
static bool g_MenuOpen = false;
static bool g_F1WasDown = false;

static HWND g_GameWindow = nullptr;
static WNDPROC g_OriginalWndProc = nullptr;

static ID3D11Device* g_Device = nullptr;
static ID3D11DeviceContext* g_Context = nullptr;
static ID3D11RenderTargetView* g_RenderTargetView = nullptr;

static LARGE_INTEGER g_Frequency{};
static LARGE_INTEGER g_LastFpsTime{};
static int g_FrameCounter = 0;
static int g_CurrentFps = 0;

static int g_SelectedTab = -1;
static int g_SelectedSubPage = 0;
static float g_MenuAnimation = 0.0f;

static bool g_OverlayEnabled = true;
static bool g_ShowStatusBar = true;
static bool g_StatusShowName = true;
static bool g_StatusShowBuild = true;
static bool g_StatusShowFps = true;
static bool g_StatusShowHotkey = true;
static bool g_DrawMouseCursor = true;
static bool g_BlockGameInputWhenOpen = true;

static bool g_GraphicsWindowedFullscreen = true;
static bool g_GraphicsShowFps = true;
static bool g_GraphicsVsync = false;
static bool g_GraphicsKeepOverlayAboveUi = true;
static bool g_GraphicsFixResizeFlicker = true;
static bool g_GraphicsBorderlessFix = true;

static bool g_ControlsControllerEnabled = true;
static bool g_ControlsCaptureMouse = true;
static bool g_ControlsBlockInput = true;
static bool g_ControlsKeyboardNav = true;

static bool g_NetworkBlockTraffic = true;
static bool g_NetworkCustomServer = false;
static bool g_NetworkLogSockets = true;
static bool g_NetworkLogDns = true;
static bool g_NetworkLanOnly = false;

static bool g_SteamStorageLocal = true;
static bool g_SteamStatsEmulated = true;
static bool g_SteamFriendsEmulated = true;
static bool g_SteamMatchmakingEmulated = true;
static bool g_SteamLogInterfaces = false;

static bool g_ModsEnabled = true;
static bool g_ModsLoadDll = true;
static bool g_ModsVerboseLog = true;

static bool g_DebugShowHandles = true;
static bool g_DebugVerboseLog = false;
static bool g_DebugFpsGraph = true;
static bool g_DebugFrameStats = true;
static bool g_DebugRenderInfo = true;

static float g_FpsHistory[180]{};
static int g_FpsHistoryOffset = 0;

static void UpdateFps()
{
    if (g_Frequency.QuadPart == 0)
    {
        QueryPerformanceFrequency(&g_Frequency);
        QueryPerformanceCounter(&g_LastFpsTime);
        return;
    }

    g_FrameCounter++;

    LARGE_INTEGER now{};
    QueryPerformanceCounter(&now);

    double elapsed =
        static_cast<double>(now.QuadPart - g_LastFpsTime.QuadPart) /
        static_cast<double>(g_Frequency.QuadPart);

    if (elapsed >= 1.0)
    {
        g_CurrentFps = static_cast<int>(
            static_cast<double>(g_FrameCounter) / elapsed + 0.5);

        g_FpsHistory[g_FpsHistoryOffset] = static_cast<float>(g_CurrentFps);
        g_FpsHistoryOffset = (g_FpsHistoryOffset + 1) % ARRAYSIZE(g_FpsHistory);

        g_FrameCounter = 0;
        g_LastFpsTime = now;
    }
}

static void UpdateHotkeys()
{
    bool f1Down = (GetAsyncKeyState(VK_F1) & 0x8000) != 0;

    if (f1Down && !g_F1WasDown)
    {
        g_MenuOpen = !g_MenuOpen;

        if (!g_MenuOpen)
            g_SelectedTab = -1;

        Logger::Info(
            std::string("DX11 overlay: F1 toggled menu ") +
            (g_MenuOpen ? "open" : "closed"));
    }

    g_F1WasDown = f1Down;
}

static void CleanupRenderTarget()
{
    SAFE_RELEASE(g_RenderTargetView);
}

static bool CreateRenderTarget(IDXGISwapChain* swapChain)
{
    if (g_RenderTargetView)
        return true;

    if (!swapChain || !g_Device)
        return false;

    ID3D11Texture2D* backBuffer = nullptr;

    HRESULT hr = swapChain->GetBuffer(
        0,
        __uuidof(ID3D11Texture2D),
        reinterpret_cast<void**>(&backBuffer));

    if (FAILED(hr) || !backBuffer)
    {
        Logger::Error("DX11 overlay: failed to get backbuffer");
        return false;
    }

    hr = g_Device->CreateRenderTargetView(
        backBuffer,
        nullptr,
        &g_RenderTargetView);

    backBuffer->Release();

    if (FAILED(hr) || !g_RenderTargetView)
    {
        Logger::Error("DX11 overlay: failed to create render target view");
        return false;
    }

    Logger::Info("DX11 overlay: render target created");
    return true;
}

static LRESULT CALLBACK OverlayWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (g_MenuOpen && g_ImGuiReady)
    {
        ImGuiIO& io = ImGui::GetIO();
        io.MouseDrawCursor = g_DrawMouseCursor;

        if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam))
            return TRUE;

        if (g_BlockGameInputWhenOpen)
        {
            switch (msg)
            {
            case WM_MOUSEMOVE:
            case WM_LBUTTONDOWN:
            case WM_LBUTTONUP:
            case WM_RBUTTONDOWN:
            case WM_RBUTTONUP:
            case WM_MBUTTONDOWN:
            case WM_MBUTTONUP:
            case WM_MOUSEWHEEL:
            case WM_MOUSEHWHEEL:
            case WM_KEYDOWN:
            case WM_KEYUP:
            case WM_CHAR:
                return TRUE;
            }
        }
    }

    if (g_OriginalWndProc)
        return CallWindowProcA(g_OriginalWndProc, hwnd, msg, wParam, lParam);

    return DefWindowProcA(hwnd, msg, wParam, lParam);
}

static bool HookGameWindow()
{
    if (!g_GameWindow || g_OriginalWndProc)
        return true;

    g_OriginalWndProc = reinterpret_cast<WNDPROC>(
        SetWindowLongPtrA(
            g_GameWindow,
            GWLP_WNDPROC,
            reinterpret_cast<LONG_PTR>(OverlayWndProc)));

    if (!g_OriginalWndProc)
    {
        Logger::Error("DX11 overlay: failed to hook game window proc");
        return false;
    }

    Logger::Info("DX11 overlay: game window proc hooked");
    return true;
}

static void ApplyStyle()
{
    ImGui::StyleColorsDark();

    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 4.0f;
    style.FrameRounding = 3.0f;
    style.GrabRounding = 3.0f;
    style.ScrollbarRounding = 4.0f;
    style.WindowBorderSize = 1.0f;
    style.FrameBorderSize = 0.0f;
    style.WindowPadding = ImVec2(8.0f, 7.0f);
    style.FramePadding = ImVec2(7.0f, 3.0f);
    style.ItemSpacing = ImVec2(7.0f, 5.0f);

    ImVec4* colors = style.Colors;
    colors[ImGuiCol_WindowBg] = ImVec4(0.035f, 0.038f, 0.042f, 0.92f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.020f, 0.022f, 0.026f, 0.55f);
    colors[ImGuiCol_Button] = ImVec4(0.12f, 0.30f, 0.52f, 1.00f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.18f, 0.42f, 0.72f, 1.00f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.22f, 0.50f, 0.86f, 1.00f);
    colors[ImGuiCol_CheckMark] = ImVec4(0.32f, 0.62f, 1.00f, 1.00f);
    colors[ImGuiCol_Header] = ImVec4(0.14f, 0.34f, 0.58f, 1.00f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.18f, 0.44f, 0.74f, 1.00f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.22f, 0.50f, 0.86f, 1.00f);
}

static bool InitImGui(IDXGISwapChain* swapChain)
{
    if (g_ImGuiReady)
        return true;

    if (!swapChain)
        return false;

    HRESULT hr = swapChain->GetDevice(
        __uuidof(ID3D11Device),
        reinterpret_cast<void**>(&g_Device));

    if (FAILED(hr) || !g_Device)
    {
        Logger::Error("DX11 overlay: failed to get D3D11 device");
        return false;
    }

    g_Device->GetImmediateContext(&g_Context);

    if (!g_Context)
    {
        Logger::Error("DX11 overlay: failed to get D3D11 context");
        SAFE_RELEASE(g_Device);
        return false;
    }

    DXGI_SWAP_CHAIN_DESC desc{};
    hr = swapChain->GetDesc(&desc);

    if (FAILED(hr) || !desc.OutputWindow)
    {
        Logger::Error("DX11 overlay: failed to get game window from swapchain");
        SAFE_RELEASE(g_Context);
        SAFE_RELEASE(g_Device);
        return false;
    }

    g_GameWindow = desc.OutputWindow;

    if (!CreateRenderTarget(swapChain))
    {
        SAFE_RELEASE(g_Context);
        SAFE_RELEASE(g_Device);
        return false;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.IniFilename = nullptr;
    io.LogFilename = nullptr;
    io.MouseDrawCursor = true;

    ApplyStyle();

    ImGui_ImplWin32_Init(g_GameWindow);
    ImGui_ImplDX11_Init(g_Device, g_Context);

    HookGameWindow();

    g_ImGuiReady = true;

    Logger::Info("DX11 overlay: ImGui initialized in-game");
    return true;
}

static float AnimateToward(float current, float target, float speed)
{
    float delta = target - current;
    current += delta * speed;

    if (fabsf(delta) < 0.001f)
        current = target;

    return current;
}

static void DrawAlwaysStatusBar()
{
    if (!g_OverlayEnabled || !g_ShowStatusBar)
        return;

    ImGui::SetNextWindowPos(ImVec2(6.0f, 5.0f), ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.42f);

    ImGui::Begin(
        "##NS2StatusOnly",
        nullptr,
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_AlwaysAutoResize |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoFocusOnAppearing |
        ImGuiWindowFlags_NoNav);

    std::string text;

    if (g_StatusShowName)
        text += "NS2 Revived";

    if (g_StatusShowBuild)
    {
        if (!text.empty()) text += " | ";
        text += "Build ";
        text += __DATE__;
        text += " ";
        text += __TIME__;
    }

    if (g_StatusShowFps)
    {
        if (!text.empty()) text += " | ";
        text += "FPS ";
        text += std::to_string(g_CurrentFps);
    }

    if (g_StatusShowHotkey)
    {
        if (!text.empty()) text += " | ";
        text += "F1";
    }

    ImGui::TextUnformatted(text.c_str());

    ImGui::End();
}

static void DrawTabButton(const char* label, int tab)
{
    bool selected = g_SelectedTab == tab;

    if (selected)
    {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.15f, 0.38f, 0.68f, 1.00f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.20f, 0.48f, 0.82f, 1.00f));
    }

    if (ImGui::Button(label, ImVec2(86.0f, 22.0f)))
    {
        if (selected)
            g_SelectedTab = -1;
        else
        {
            g_SelectedTab = tab;
            g_SelectedSubPage = 0;
        }
    }

    if (selected)
        ImGui::PopStyleColor(2);
}

static void DrawLeftButton(const char* label, int page)
{
    bool selected = g_SelectedSubPage == page;

    if (selected)
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.17f, 0.38f, 0.66f, 1.0f));

    if (ImGui::Button(label, ImVec2(178.0f, 25.0f)))
        g_SelectedSubPage = page;

    if (selected)
        ImGui::PopStyleColor();
}

static void DrawGraphicsPage()
{
    if (g_SelectedSubPage == 0)
    {
        ImGui::Text("Graphics > General");
        ImGui::Separator();
        ImGui::Checkbox("Windowed fullscreen helper", &g_GraphicsWindowedFullscreen);
        ImGui::Checkbox("Show FPS status", &g_GraphicsShowFps);
        ImGui::Checkbox("VSync", &g_GraphicsVsync);
        ImGui::Text("Renderer: D3D11");
    }
    else if (g_SelectedSubPage == 1)
    {
        ImGui::Text("Graphics > Fixes");
        ImGui::Separator();
        ImGui::Checkbox("Fix window resize flicker", &g_GraphicsFixResizeFlicker);
        ImGui::Checkbox("Improve fullscreen borderless handling", &g_GraphicsBorderlessFix);
        ImGui::Checkbox("Keep overlay above game UI", &g_GraphicsKeepOverlayAboveUi);
    }
    else if (g_SelectedSubPage == 2)
    {
        ImGui::Text("Graphics > Enhancements");
        ImGui::Separator();
        ImGui::Checkbox("Sharper overlay text", &g_GraphicsShowFps);
        ImGui::Checkbox("Smooth menu animation", &g_GraphicsKeepOverlayAboveUi);
        ImGui::Checkbox("Compact top status", &g_ShowStatusBar);
    }
    else
    {
        ImGui::Text("Graphics > Advanced");
        ImGui::Separator();
        ImGui::Text("Device: 0x%p", g_Device);
        ImGui::Text("Context: 0x%p", g_Context);
        ImGui::Text("RTV: 0x%p", g_RenderTargetView);
    }
}

static void DrawControlsPage()
{
    if (g_SelectedSubPage == 0)
    {
        ImGui::Text("Controls > General");
        ImGui::Separator();
        ImGui::Checkbox("Controller enabled", &g_ControlsControllerEnabled);
        ImGui::Checkbox("Keyboard navigation", &g_ControlsKeyboardNav);
    }
    else if (g_SelectedSubPage == 1)
    {
        ImGui::Text("Controls > Mouse");
        ImGui::Separator();
        ImGui::Checkbox("Draw ImGui mouse cursor", &g_DrawMouseCursor);
        ImGui::Checkbox("Capture mouse when menu is open", &g_ControlsCaptureMouse);
        ImGui::Checkbox("Block game input while menu is open", &g_BlockGameInputWhenOpen);
    }
    else if (g_SelectedSubPage == 2)
    {
        ImGui::Text("Controls > Binds");
        ImGui::Separator();
        ImGui::Text("Open menu: F1");
        ImGui::Button("Refresh controllers", ImVec2(190.0f, 25.0f));
        ImGui::Button("Reset binds", ImVec2(190.0f, 25.0f));
    }
    else
    {
        ImGui::Text("Controls > Advanced");
        ImGui::Separator();
        ImGui::Checkbox("Block all game input while menu is open", &g_ControlsBlockInput);
    }
}

static void DrawNetworkPage()
{
    if (g_SelectedSubPage == 0)
    {
        ImGui::Text("Network > General");
        ImGui::Separator();
        ImGui::Checkbox("Block game network traffic", &g_NetworkBlockTraffic);
        ImGui::Checkbox("Use custom server redirect", &g_NetworkCustomServer);
        ImGui::Text("Mode: Local Steam emulation");
    }
    else if (g_SelectedSubPage == 1)
    {
        ImGui::Text("Network > Logging");
        ImGui::Separator();
        ImGui::Checkbox("Log socket connections", &g_NetworkLogSockets);
        ImGui::Checkbox("Log DNS requests", &g_NetworkLogDns);
    }
    else if (g_SelectedSubPage == 2)
    {
        ImGui::Text("Network > Custom Server");
        ImGui::Separator();
        ImGui::Checkbox("LAN only", &g_NetworkLanOnly);
        ImGui::Text("Custom master server: pending");
    }
    else
    {
        ImGui::Text("Network > Advanced");
        ImGui::Separator();
        ImGui::Text("Winsock hooks active");
        ImGui::Text("Traffic policy controlled by NetworkHooks");
    }
}

static void DrawSteamPage()
{
    if (g_SelectedSubPage == 0)
    {
        ImGui::Text("Steam > Interfaces");
        ImGui::Separator();
        ImGui::Checkbox("Local remote storage", &g_SteamStorageLocal);
        ImGui::Checkbox("Emulated stats", &g_SteamStatsEmulated);
        ImGui::Checkbox("Emulated friends", &g_SteamFriendsEmulated);
        ImGui::Checkbox("Emulated matchmaking", &g_SteamMatchmakingEmulated);
    }
    else if (g_SelectedSubPage == 1)
    {
        ImGui::Text("Steam > Logging");
        ImGui::Separator();
        ImGui::Checkbox("Log Steam interface calls", &g_SteamLogInterfaces);
        ImGui::Checkbox("Verbose logging", &g_DebugVerboseLog);
    }
    else
    {
        ImGui::Text("Steam > Status");
        ImGui::Separator();
        ImGui::Text("Steam proxy: loaded");
        ImGui::Text("Remote storage: local");
        ImGui::Text("Stats: emulated");
        ImGui::Text("Matchmaking: emulated");
    }
}

static void DrawModsPage()
{
    if (g_SelectedSubPage == 0)
    {
        ImGui::Text("Mods > Loader");
        ImGui::Separator();
        ImGui::Checkbox("Enable mod loading", &g_ModsEnabled);
        ImGui::Checkbox("Load DLL mods", &g_ModsLoadDll);
        ImGui::Checkbox("Verbose mod logging", &g_ModsVerboseLog);
    }
    else
    {
        ImGui::Text("Mods > Folder");
        ImGui::Separator();
        ImGui::Text("NartuoStorm2Revived\\mods");
        ImGui::Button("Reload mods", ImVec2(170.0f, 25.0f));
        ImGui::Button("Open mods folder", ImVec2(170.0f, 25.0f));
    }
}

static void DrawDebugPage()
{
    if (g_SelectedSubPage == 0)
    {
        ImGui::Text("Debug > Overlay");
        ImGui::Separator();

        ImGui::Checkbox("Overlay enabled", &g_OverlayEnabled);
        ImGui::Checkbox("Show top-left status", &g_ShowStatusBar);

        if (ImGui::BeginCombo("Status content", "Customize"))
        {
            ImGui::Checkbox("Show NS2 Revived", &g_StatusShowName);
            ImGui::Checkbox("Show build date", &g_StatusShowBuild);
            ImGui::Checkbox("Show FPS", &g_StatusShowFps);
            ImGui::Checkbox("Show F1 hint", &g_StatusShowHotkey);
            ImGui::EndCombo();
        }

        if (ImGui::Button("FPS only", ImVec2(120.0f, 24.0f)))
        {
            g_StatusShowName = false;
            g_StatusShowBuild = false;
            g_StatusShowFps = true;
            g_StatusShowHotkey = false;
        }

        ImGui::SameLine();

        if (ImGui::Button("Full status", ImVec2(120.0f, 24.0f)))
        {
            g_StatusShowName = true;
            g_StatusShowBuild = true;
            g_StatusShowFps = true;
            g_StatusShowHotkey = true;
        }
    }
    else if (g_SelectedSubPage == 1)
    {
        ImGui::Text("Debug > FPS Graph");
        ImGui::Separator();

        ImGui::Checkbox("Show FPS graph", &g_DebugFpsGraph);
        ImGui::Checkbox("Show frame stats", &g_DebugFrameStats);

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

        if (g_DebugFrameStats)
        {
            ImGui::Text("Current FPS: %d", g_CurrentFps);
            ImGui::Text("Frame time: %.3f ms", g_CurrentFps > 0 ? 1000.0f / static_cast<float>(g_CurrentFps) : 0.0f);
        }
    }
    else if (g_SelectedSubPage == 2)
    {
        ImGui::Text("Debug > Render");
        ImGui::Separator();

        ImGui::Checkbox("Show handles", &g_DebugShowHandles);
        ImGui::Checkbox("Verbose logging", &g_DebugVerboseLog);
        ImGui::Checkbox("Show render target info", &g_DebugRenderInfo);

        if (g_DebugShowHandles)
        {
            ImGui::Separator();
            ImGui::Text("HWND: 0x%p", g_GameWindow);
            ImGui::Text("Device: 0x%p", g_Device);
            ImGui::Text("Context: 0x%p", g_Context);
            ImGui::Text("RTV: 0x%p", g_RenderTargetView);
        }
    }
    else
    {
        ImGui::Text("Debug > Tools");
        ImGui::Separator();

        if (ImGui::Button("Write test log", ImVec2(160.0f, 24.0f)))
            Logger::Info("DX11 overlay: test log from debug tools");

        ImGui::Button("Dump overlay state", ImVec2(160.0f, 24.0f));
    }
}

static void DrawDropdownPanel()
{
    if (!g_MenuOpen || g_SelectedTab < 0)
        return;

    float x = 95.0f + static_cast<float>(g_SelectedTab) * 88.0f;
    float y = 31.0f * g_MenuAnimation;

    ImGui::SetNextWindowPos(ImVec2(x, y), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(760.0f, 360.0f), ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.90f);

    ImGui::Begin(
        "##NS2ShipwrightDropdown",
        nullptr,
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoCollapse);

    ImGui::BeginChild("##NS2LeftList", ImVec2(195.0f, 0.0f), true);

    ImGui::Text("Options");
    ImGui::Separator();

    if (g_SelectedTab == 0)
    {
        DrawLeftButton("General", 0);
        DrawLeftButton("Fixes", 1);
        DrawLeftButton("Enhancements", 2);
        DrawLeftButton("Advanced", 3);
    }
    else if (g_SelectedTab == 1)
    {
        DrawLeftButton("General", 0);
        DrawLeftButton("Mouse", 1);
        DrawLeftButton("Binds", 2);
        DrawLeftButton("Advanced", 3);
    }
    else if (g_SelectedTab == 2)
    {
        DrawLeftButton("General", 0);
        DrawLeftButton("Logging", 1);
        DrawLeftButton("Custom Server", 2);
        DrawLeftButton("Advanced", 3);
    }
    else if (g_SelectedTab == 3)
    {
        DrawLeftButton("Interfaces", 0);
        DrawLeftButton("Logging", 1);
        DrawLeftButton("Status", 2);
    }
    else if (g_SelectedTab == 4)
    {
        DrawLeftButton("Loader", 0);
        DrawLeftButton("Folder", 1);
    }
    else if (g_SelectedTab == 5)
    {
        DrawLeftButton("Overlay", 0);
        DrawLeftButton("FPS Graph", 1);
        DrawLeftButton("Render", 2);
        DrawLeftButton("Tools", 3);
    }

    ImGui::EndChild();

    ImGui::SameLine();

    ImGui::BeginChild("##NS2RightContent", ImVec2(0.0f, 0.0f), true);

    switch (g_SelectedTab)
    {
    case 0: DrawGraphicsPage(); break;
    case 1: DrawControlsPage(); break;
    case 2: DrawNetworkPage(); break;
    case 3: DrawSteamPage(); break;
    case 4: DrawModsPage(); break;
    case 5: DrawDebugPage(); break;
    }

    ImGui::EndChild();

    ImGui::End();
}

static void DrawAnimatedTopMenu()
{
    float target = g_MenuOpen ? 1.0f : 0.0f;
    float speed = ImGui::GetIO().DeltaTime * 14.0f;

    g_MenuAnimation = AnimateToward(g_MenuAnimation, target, speed);

    if (g_MenuAnimation < 0.01f && !g_MenuOpen)
    {
        g_MenuAnimation = 0.0f;
        g_SelectedTab = -1;
        return;
    }

    ImGuiViewport* viewport = ImGui::GetMainViewport();

    float barHeight = 30.0f;
    float y = -barHeight + (barHeight * g_MenuAnimation);

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
    DrawTabButton("Graphics", 0);
    ImGui::SameLine();
    DrawTabButton("Controls", 1);
    ImGui::SameLine();
    DrawTabButton("Network", 2);
    ImGui::SameLine();
    DrawTabButton("Steam", 3);
    ImGui::SameLine();
    DrawTabButton("Mods", 4);
    ImGui::SameLine();
    DrawTabButton("Debug", 5);

    ImGui::SameLine(viewport->Size.x - 260.0f);
    ImGui::Text("FPS %d", g_CurrentFps);

    ImGui::End();

    if (g_SelectedTab >= 0)
        DrawDropdownPanel();
}

static void DrawImGui(IDXGISwapChain* swapChain)
{
    if (!g_ImGuiReady || !g_Context)
        return;

    if (!CreateRenderTarget(swapChain))
        return;

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();

    ImGuiIO& io = ImGui::GetIO();
    io.MouseDrawCursor = g_MenuOpen && g_DrawMouseCursor;

    ImGui::NewFrame();

    DrawAlwaysStatusBar();
    DrawAnimatedTopMenu();

    ImGui::Render();

    g_Context->OMSetRenderTargets(1, &g_RenderTargetView, nullptr);
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

static void ShutdownImGui()
{
    if (g_ImGuiReady)
    {
        ImGui_ImplDX11_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
        g_ImGuiReady = false;
    }

    CleanupRenderTarget();

    SAFE_RELEASE(g_Context);
    SAFE_RELEASE(g_Device);

    if (g_GameWindow && g_OriginalWndProc)
    {
        SetWindowLongPtrA(
            g_GameWindow,
            GWLP_WNDPROC,
            reinterpret_cast<LONG_PTR>(g_OriginalWndProc));

        g_OriginalWndProc = nullptr;
    }

    g_GameWindow = nullptr;
}

static HRESULT __stdcall PresentHook(IDXGISwapChain* swapChain, UINT syncInterval, UINT flags)
{
    if (!g_PresentSeen)
    {
        g_PresentSeen = true;
        Logger::Info("DX11 overlay: Present hook hit");
    }

    UpdateFps();
    UpdateHotkeys();

    if (InitImGui(swapChain))
        DrawImGui(swapChain);

    return g_OriginalPresent(swapChain, syncInterval, flags);
}

static HRESULT __stdcall ResizeBuffersHook(
    IDXGISwapChain* swapChain,
    UINT bufferCount,
    UINT width,
    UINT height,
    DXGI_FORMAT newFormat,
    UINT swapChainFlags)
{
    Logger::Info("DX11 overlay: ResizeBuffers hook hit");

    CleanupRenderTarget();

    return g_OriginalResizeBuffers(
        swapChain,
        bufferCount,
        width,
        height,
        newFormat,
        swapChainFlags);
}

static HWND CreateDummyWindow()
{
    WNDCLASSEXA wc{};
    wc.cbSize = sizeof(WNDCLASSEXA);
    wc.lpfnWndProc = DefWindowProcA;
    wc.hInstance = GetModuleHandleA(nullptr);
    wc.lpszClassName = "NS2RevivedDummyDX11Window";

    RegisterClassExA(&wc);

    return CreateWindowExA(
        0,
        wc.lpszClassName,
        "NS2RevivedDummyDX11Window",
        WS_OVERLAPPEDWINDOW,
        0,
        0,
        100,
        100,
        nullptr,
        nullptr,
        wc.hInstance,
        nullptr);
}

namespace DX11Overlay
{
    bool Init()
    {
        if (g_HookReady)
            return true;

        Logger::Info("DX11 overlay: initializing ImGui DX11 hook");

        HWND dummyWindow = CreateDummyWindow();

        if (!dummyWindow)
        {
            Logger::Error("DX11 overlay: failed to create dummy window");
            return false;
        }

        DXGI_SWAP_CHAIN_DESC desc{};
        desc.BufferCount = 1;
        desc.BufferDesc.Width = 100;
        desc.BufferDesc.Height = 100;
        desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        desc.OutputWindow = dummyWindow;
        desc.SampleDesc.Count = 1;
        desc.SampleDesc.Quality = 0;
        desc.Windowed = TRUE;
        desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

        IDXGISwapChain* dummySwapChain = nullptr;
        ID3D11Device* dummyDevice = nullptr;
        ID3D11DeviceContext* dummyContext = nullptr;

        D3D_FEATURE_LEVEL featureLevel{};
        D3D_FEATURE_LEVEL featureLevels[] =
        {
            D3D_FEATURE_LEVEL_11_0,
            D3D_FEATURE_LEVEL_10_1,
            D3D_FEATURE_LEVEL_10_0
        };

        HRESULT hr = D3D11CreateDeviceAndSwapChain(
            nullptr,
            D3D_DRIVER_TYPE_HARDWARE,
            nullptr,
            0,
            featureLevels,
            ARRAYSIZE(featureLevels),
            D3D11_SDK_VERSION,
            &desc,
            &dummySwapChain,
            &dummyDevice,
            &featureLevel,
            &dummyContext);

        if (FAILED(hr) || !dummySwapChain)
        {
            Logger::Error("DX11 overlay: dummy swapchain creation failed");
            DestroyWindow(dummyWindow);
            return false;
        }

        void** vtable = *reinterpret_cast<void***>(dummySwapChain);

        void* presentAddress = vtable[8];
        void* resizeBuffersAddress = vtable[13];

        MH_STATUS presentStatus = MH_CreateHook(
            presentAddress,
            &PresentHook,
            reinterpret_cast<void**>(&g_OriginalPresent));

        if (presentStatus != MH_OK &&
            presentStatus != MH_ERROR_ALREADY_CREATED)
        {
            Logger::Error("DX11 overlay: MH_CreateHook Present failed");

            dummySwapChain->Release();
            dummyDevice->Release();
            dummyContext->Release();
            DestroyWindow(dummyWindow);

            return false;
        }

        MH_STATUS resizeStatus = MH_CreateHook(
            resizeBuffersAddress,
            &ResizeBuffersHook,
            reinterpret_cast<void**>(&g_OriginalResizeBuffers));

        if (resizeStatus != MH_OK &&
            resizeStatus != MH_ERROR_ALREADY_CREATED)
        {
            Logger::Error("DX11 overlay: MH_CreateHook ResizeBuffers failed");

            dummySwapChain->Release();
            dummyDevice->Release();
            dummyContext->Release();
            DestroyWindow(dummyWindow);

            return false;
        }

        MH_EnableHook(presentAddress);
        MH_EnableHook(resizeBuffersAddress);

        Logger::Info("DX11 overlay: Present and ResizeBuffers hooks enabled for ImGui drawing");

        dummySwapChain->Release();
        dummyDevice->Release();
        dummyContext->Release();
        DestroyWindow(dummyWindow);

        g_HookReady = true;

        return true;
    }

    void Shutdown()
    {
        ShutdownImGui();
        Logger::Info("DX11 overlay: shutdown");
    }
}