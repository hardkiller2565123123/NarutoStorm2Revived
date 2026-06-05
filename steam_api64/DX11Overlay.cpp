#include "StdInc.h"
#include "DX11Overlay.h"
#include "Logger.h"
#include "NS2Config.h"
#include "AssetBrowser.h"
#include "AssetPreview.h"
#include "AssetModelViewer.h"
#include "CpkArchive.h"
#include "ModRedirector.h"
#include "AssetPreloadManager.h"

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

static bool g_DebugOverlayEnabled = true;
static bool g_DebugShowOnlyFps = false;
static bool g_DebugShowBuildDate = true;
static bool g_DebugShowF1Hint = true;
static bool g_DebugDrawMouseCursor = true;
static bool g_DebugBlockInput = true;

static bool g_DebugVerboseLog = false;

static bool g_DebugFpsGraph = true;
static bool g_DebugFrameTimeGraph = true;
static bool g_DebugShowMinAvgMax = true;
static bool g_DebugShowPresentCount = true;
static bool g_DebugShowResizeCount = true;
static bool g_DebugShowImGuiStats = true;

static bool g_DebugShowRenderHandles = true;
static bool g_DebugShowSwapchainInfo = true;
static bool g_DebugShowWindowState = true;
static bool g_DebugShowRefreshRate = true;

static bool g_DebugLogSteamInterfaces = false;
static bool g_DebugLogCallbacks = false;
static bool g_DebugLogCallResults = false;
static bool g_DebugShowSteamId = true;
static bool g_DebugShowLobbyState = true;

static bool g_DebugLogDns = true;
static bool g_DebugLogSocketConnects = true;
static bool g_DebugShowLastBlockedAddress = true;
static bool g_DebugShowNetworkHookStatus = true;
static bool g_DebugForceBlockTraffic = true;
static bool g_DebugLanOnly = false;

static bool g_DebugLogStorageReads = true;
static bool g_DebugLogStorageWrites = true;
static bool g_DebugShowStorageFolder = true;
static bool g_DebugShowLastSaveFile = true;

static bool g_DebugShowModsFolder = true;
static bool g_DebugLogModErrors = true;
static bool g_DebugShowLoadedModCount = true;

static float g_FpsHistory[180]{};
static int g_FpsHistoryOffset = 0;

// ------------------------------------------------------------
// Game / Camera / Patch helper state
// ------------------------------------------------------------

static bool g_GameSkipIntro = false;
static bool g_GameAutoSkipCutscenes = false;
static bool g_GameUnlockAllTemporary = false;
static bool g_GameUnlockAllPersistent = false;
static bool g_GameSaveBackupEnabled = true;
static bool g_GameUseCustomSaveFolder = false;

static bool g_CameraFreeCam = false;
static bool g_CameraHideHud = false;
static bool g_CameraAllowDuringCutscenes = false;
static bool g_CameraCopyQueued = false;

static float g_CameraFov = 60.0f;
static float g_CameraSpeed = 1.0f;
static float g_CameraBoost = 4.0f;
static float g_CameraSmooth = 0.25f;
static float g_CameraPos[3] = { 0.0f, 0.0f, 0.0f };
static float g_CameraRot[3] = { 0.0f, 0.0f, 0.0f };

static bool ActiveCheckbox(const char* label, bool* value, const char* help)
{
    bool changed = ImGui::Checkbox(label, value);

    ImGui::SameLine();
    ImGui::TextDisabled("[ACTIVE]");

    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("%s", help);

    return changed;
}

static bool PlannedCheckbox(const char* label, bool* value, const char* reason)
{
    bool changed = ImGui::Checkbox(label, value);

    ImGui::SameLine();
    ImGui::TextDisabled("[PLANNED]");

    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("%s", reason);

    return changed;
}

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

static void DrawAssetLoadingToast()
{
    bool scanning = AssetBrowser::IsScanning();
    bool preloading = AssetPreloadManager::IsPreloading();

    if (!scanning && !preloading)
        return;

    ImGuiViewport* viewport = ImGui::GetMainViewport();

    ImGui::SetNextWindowPos(
        ImVec2(viewport->Size.x - 430.0f, 36.0f),
        ImGuiCond_Always);

    ImGui::SetNextWindowSize(ImVec2(410.0f, 128.0f), ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.90f);

    ImGui::Begin(
        "##NS2AssetLoadingToast",
        nullptr,
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoFocusOnAppearing |
        ImGuiWindowFlags_NoNav);

    const char* title = scanning ? "Reading game assets..." : "Loading asset previews...";

    int progress = scanning
        ? AssetBrowser::GetScanProgress()
        : AssetPreloadManager::GetProgress();

    ImGui::Text("%s", title);
    ImGui::SameLine();
    ImGui::Text("%d%%", progress);

    ImGui::ProgressBar(progress / 100.0f, ImVec2(-1.0f, 16.0f));

    if (preloading)
    {
        ImGui::Text(
            "Loaded %d / %d",
            AssetPreloadManager::GetLoadedCount(),
            AssetPreloadManager::GetTotalCount());
    }

    ImGui::TextWrapped(
        "First load may lag while NS2 Revived opens and caches every asset preview. "
        "After this, clicking assets should be smoother.");

    std::string current = scanning
        ? AssetBrowser::GetScanStatus()
        : AssetPreloadManager::GetCurrentAsset();

    if (!current.empty())
        ImGui::TextDisabled("%s", current.c_str());

    ImGui::End();
}

static void UpdateHotkeys()
{
    bool f1Down = (GetAsyncKeyState(VK_F1) & 0x8000) != 0;

    if (f1Down && !g_F1WasDown)
    {
        g_MenuOpen = !g_MenuOpen;

        if (!g_MenuOpen)
            g_SelectedTab = -1;
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

static bool IniCheckbox(const char* label, int& value)
{
    bool checked = value != 0;

    if (ImGui::Checkbox(label, &checked))
    {
        value = checked ? 1 : 0;
        return true;
    }

    return false;
}

static void HelpMarker(const char* text)
{
    ImGui::SameLine();
    ImGui::TextDisabled("(?)");

    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(420.0f);
        ImGui::TextUnformatted(text);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}


static void ResolutionPreset(const char* label, int width, int height)
{
    auto& s = NS2Config::Screen();

    if (ImGui::Button(label, ImVec2(110.0f, 24.0f)))
    {
        s.Width = width;
        s.Height = height;
    }
}

static bool IniCheckbox(const char* label, int& value, const char* help)
{
    bool checked = value != 0;
    bool changed = ImGui::Checkbox(label, &checked);

    HelpMarker(help);

    if (changed)
        value = checked ? 1 : 0;

    return changed;
}

static void DrawGraphicsPage()
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
        HelpMarker("Shows or hides the small NS2 Revived build/FPS text.");

        ImGui::Checkbox("Show build date", &g_StatusShowBuild);
        HelpMarker("Shows the compile date/time in the status overlay.");

        ImGui::Checkbox("Show FPS", &g_StatusShowFps);
        HelpMarker("Shows current FPS in the overlay status line.");

        ImGui::Checkbox("Show F1 hint", &g_StatusShowHotkey);
        HelpMarker("Shows the F1 menu hint in the status line.");

        ImGui::Checkbox("Draw mouse cursor", &g_DrawMouseCursor);
        HelpMarker("Draws ImGui's own cursor while the menu is open.");

        ImGui::Checkbox("Block game input while menu is open", &g_BlockGameInputWhenOpen);
        HelpMarker("Prevents the game from receiving mouse/keyboard input while using the menu.");

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

        ImGui::Text("Runtime Renderer Info");
        ImGui::Text("Renderer: D3D11");
        ImGui::Text("HWND: 0x%p", g_GameWindow);
        ImGui::Text("Device: 0x%p", g_Device);
        ImGui::Text("Context: 0x%p", g_Context);
        ImGui::Text("RTV: 0x%p", g_RenderTargetView);

        ImGui::Separator();

        if (ImGui::Button("Force Recreate Render Target", ImVec2(240.0f, 24.0f)))
        {
            CleanupRenderTarget();
            Logger::Info("DX11 overlay: graphics page forced render target recreate");
        }

        if (ImGui::Button("Save Config", ImVec2(135.0f, 24.0f)))
            NS2Config::Save();

        ImGui::SameLine();

        if (ImGui::Button("Reload Config", ImVec2(135.0f, 24.0f)))
            NS2Config::Load();

        ImGui::SameLine();

        if (ImGui::Button("Reset Graphics", ImVec2(145.0f, 24.0f)))
            NS2Config::ResetGraphics();
    }
}


static void DrawControlsPage()
{
    if (g_SelectedSubPage == 0)
    {
        ImGui::Text("Controls > General");
        HelpMarker("General input settings for the overlay. These options only affect the overlay/menu layer, not the game's own keybind system.");
        ImGui::Separator();

        ImGui::Checkbox("Controller enabled", &g_ControlsControllerEnabled);
        HelpMarker("Allows controller input to be used by overlay features that support controller navigation.");

        ImGui::Checkbox("Keyboard navigation", &g_ControlsKeyboardNav);
        HelpMarker("Allows ImGui keyboard navigation inside the menu. Useful if mouse input is unreliable.");
    }
    else if (g_SelectedSubPage == 1)
    {
        ImGui::Text("Controls > Mouse");
        HelpMarker("Mouse behavior while the F1 overlay menu is open.");
        ImGui::Separator();

        ImGui::Checkbox("Capture mouse when menu is open", &g_ControlsCaptureMouse);
        HelpMarker("Keeps mouse interaction focused on the overlay while the menu is open.");

        ImGui::Checkbox("Draw ImGui mouse cursor", &g_DrawMouseCursor);
        HelpMarker("Draws ImGui's software cursor while the menu is open. This is also available in Graphics > Overlay because it is a visible overlay option.");

        ImGui::Checkbox("Block game input while menu is open", &g_BlockGameInputWhenOpen);
        HelpMarker("Prevents the game from receiving mouse/keyboard input while using the menu.");
    }
    else if (g_SelectedSubPage == 2)
    {
        ImGui::Text("Controls > Binds");
        HelpMarker("Hotkey and input bind information. The menu hotkey is currently hardcoded to F1.");
        ImGui::Separator();

        ImGui::Text("Open menu: F1");
        HelpMarker("Press F1 once to open the overlay menu and press it again to close it.");

        if (ImGui::Button("Refresh controllers", ImVec2(190.0f, 25.0f)))
            Logger::Info("DX11 overlay: refresh controllers requested");
        HelpMarker("Requests a controller refresh. This is useful after plugging in a controller while the game is already running.");

        if (ImGui::Button("Reset binds", ImVec2(190.0f, 25.0f)))
            Logger::Info("DX11 overlay: reset binds requested");
        HelpMarker("Resets overlay bind state back to defaults. The F1 menu toggle remains the default menu key.");
    }
    else
    {
        ImGui::Text("Controls > Advanced");
        HelpMarker("Advanced input safety options for preventing accidental game control while using the overlay.");
        ImGui::Separator();

        ImGui::Checkbox("Block all game input while menu is open", &g_ControlsBlockInput);
        HelpMarker("Master input-blocking toggle for the Controls page. The active WndProc block uses the Graphics > Overlay input-block option.");
    }
}


static void DrawModsPage()
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
        HelpMarker("Writes extra mod loader details to the log, including detected files and load errors.");
    }
    else
    {
        ImGui::Text("Mods > Folder");
        HelpMarker("Folder actions for the local mods directory.");
        ImGui::Separator();

        ImGui::Text("NarutoStorm2Revived\\mods");
        HelpMarker("Default relative mods folder used by this overlay/framework build.");

        if (ImGui::Button("Reload mods", ImVec2(170.0f, 25.0f)))
            Logger::Info("DX11 overlay: reload mods requested");
        HelpMarker("Requests a mod list refresh. This does not unload already-loaded native DLLs.");

        if (ImGui::Button("Open mods folder", ImVec2(170.0f, 25.0f)))
            Logger::Info("DX11 overlay: open mods folder requested");
        HelpMarker("Opens or logs the mods folder location depending on what shell/file-browser support is implemented in your project.");
    }
}


static void DrawDebugPage()
{
    if (g_SelectedSubPage == 0)
    {
        ImGui::Text("Debug > Performance");
        HelpMarker("Runtime performance counters for the overlay and hooked renderer.");
        ImGui::Separator();

        ImGui::Checkbox("Show FPS graph", &g_DebugFpsGraph);
        HelpMarker("Draws a rolling FPS graph using the overlay's measured frame rate history.");

        ImGui::Checkbox("Show frame time graph", &g_DebugFrameTimeGraph);
        HelpMarker("Reserved toggle for frame-time graph output. Current frame-time text is shown below.");

        ImGui::Checkbox("Show min / avg / max FPS", &g_DebugShowMinAvgMax);
        HelpMarker("Reserved toggle for extended FPS statistics once min/average/max tracking is wired in.");

        ImGui::Checkbox("Show Present counter", &g_DebugShowPresentCount);
        HelpMarker("Reserved toggle for counting IDXGISwapChain::Present calls.");

        ImGui::Checkbox("Show ResizeBuffers counter", &g_DebugShowResizeCount);
        HelpMarker("Reserved toggle for counting ResizeBuffers calls. Useful when diagnosing resize/flicker issues.");

        ImGui::Checkbox("Show ImGui stats", &g_DebugShowImGuiStats);
        HelpMarker("Shows internal ImGui information such as frame rate and draw data counts when expanded later.");

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
            HelpMarker("Rolling FPS history. The newest value wraps through the graph buffer.");
        }

        ImGui::Text("Current FPS: %d", g_CurrentFps);
        HelpMarker("Current measured FPS from the Present hook.");

        ImGui::Text("Frame time: %.3f ms", g_CurrentFps > 0 ? 1000.0f / static_cast<float>(g_CurrentFps) : 0.0f);
        HelpMarker("Approximate milliseconds per frame based on current FPS.");
    }
    else if (g_SelectedSubPage == 1)
    {
        ImGui::Text("Debug > Render");
        HelpMarker("Renderer, swapchain, and render-target diagnostic information.");
        ImGui::Separator();

        ImGui::Checkbox("Show render handles", &g_DebugShowRenderHandles);
        HelpMarker("Shows raw D3D11 pointers for the game window, device, context, and render target view.");

        ImGui::Checkbox("Show swapchain info", &g_DebugShowSwapchainInfo);
        HelpMarker("Reserved toggle for displaying swapchain format, dimensions, and flags.");

        ImGui::Checkbox("Show window state", &g_DebugShowWindowState);
        HelpMarker("Reserved toggle for displaying window focus, minimized, and foreground state.");

        ImGui::Checkbox("Show refresh rate", &g_DebugShowRefreshRate);
        HelpMarker("Reserved toggle for displaying monitor/swapchain refresh-rate information.");

        if (ImGui::Button("Force recreate render target", ImVec2(220.0f, 24.0f)))
        {
            CleanupRenderTarget();
            Logger::Info("DX11 overlay: render target manually recreated");
        }
        HelpMarker("Releases the overlay render target so it can be recreated on the next frame. Useful after resize issues.");

        if (g_DebugShowRenderHandles)
        {
            ImGui::Separator();
            ImGui::Text("HWND: 0x%p", g_GameWindow);
            HelpMarker("Game window handle detected from the swapchain.");

            ImGui::Text("Device: 0x%p", g_Device);
            HelpMarker("D3D11 device used by the overlay renderer.");

            ImGui::Text("Context: 0x%p", g_Context);
            HelpMarker("Immediate D3D11 context used for ImGui draw calls.");

            ImGui::Text("RTV: 0x%p", g_RenderTargetView);
            HelpMarker("Render target view created from the swapchain backbuffer.");
        }
    }
    else if (g_SelectedSubPage == 2)
    {
        ImGui::Text("Debug > Steam");
        HelpMarker("Steam proxy/emulation debug options. These are logging/inspection toggles only.");
        ImGui::Separator();

        ImGui::Checkbox("Log Steam interface calls", &g_DebugLogSteamInterfaces);
        HelpMarker("Logs when Steam interface wrappers are requested or used.");

        ImGui::Checkbox("Log callbacks", &g_DebugLogCallbacks);
        HelpMarker("Logs Steam callback activity from your proxy/emulation layer when wired in.");

        ImGui::Checkbox("Log call results", &g_DebugLogCallResults);
        HelpMarker("Logs Steam call-result activity when the proxy/emulation layer supports it.");

        ImGui::Checkbox("Show fake SteamID", &g_DebugShowSteamId);
        HelpMarker("Shows the local/fake SteamID used by the emulation layer when available.");

        ImGui::Checkbox("Show lobby state", &g_DebugShowLobbyState);
        HelpMarker("Shows local lobby/session state when your matchmaking layer provides it.");

        ImGui::Separator();
        ImGui::Text("Steam proxy: loaded");
        HelpMarker("Status line for the Steam proxy layer.");

        ImGui::Text("Remote storage: local");
        HelpMarker("Indicates that Steam remote storage is redirected to local storage.");

        ImGui::Text("Stats: emulated");
        HelpMarker("Indicates that Steam stats/achievements are handled by the local emulation layer.");

        ImGui::Text("Matchmaking: emulated");
        HelpMarker("Indicates that matchmaking calls are handled locally or by your replacement layer.");

        if (ImGui::Button("Dump Steam registry", ImVec2(180.0f, 24.0f)))
            Logger::Info("DX11 overlay: dump Steam registry requested");
        HelpMarker("Writes the current Steam proxy/emulation state to the log when implemented.");
    }
    else if (g_SelectedSubPage == 3)
    {
        ImGui::Text("Debug > Network");
        HelpMarker("Network hook diagnostics for DNS and socket-related code.");
        ImGui::Separator();

        ImGui::Checkbox("Log DNS requests", &g_DebugLogDns);
        HelpMarker("Logs hostname lookup attempts captured by DNS hooks.");

        ImGui::Checkbox("Log socket connects", &g_DebugLogSocketConnects);
        HelpMarker("Logs outbound socket connection attempts captured by connect/WSAConnect hooks.");

        ImGui::Checkbox("Show last blocked host/IP", &g_DebugShowLastBlockedAddress);
        HelpMarker("Shows the last host or IP blocked by your network filtering layer when available.");

        ImGui::Checkbox("Show network hook status", &g_DebugShowNetworkHookStatus);
        HelpMarker("Shows whether the expected network hooks are currently installed.");

        ImGui::Checkbox("Force block traffic", &g_DebugForceBlockTraffic);
        HelpMarker("Debug override for forcing external traffic blocking in your network layer.");

        ImGui::Checkbox("LAN only", &g_DebugLanOnly);
        HelpMarker("Debug toggle for allowing only LAN/private-address traffic when your network layer supports it.");

        ImGui::Separator();
        ImGui::Text("connect hook: enabled");
        HelpMarker("Status for the connect hook.");

        ImGui::Text("WSAConnect hook: enabled");
        HelpMarker("Status for the WSAConnect hook.");

        ImGui::Text("getaddrinfo hook: enabled");
        HelpMarker("Status for the ANSI getaddrinfo hook.");

        ImGui::Text("GetAddrInfoW hook: enabled");
        HelpMarker("Status for the wide-character GetAddrInfoW hook.");
    }
    else if (g_SelectedSubPage == 4)
    {
        ImGui::Text("Debug > Storage");
        HelpMarker("Local storage and save/stat debug tools.");
        ImGui::Separator();

        ImGui::Checkbox("Show storage folder", &g_DebugShowStorageFolder);
        HelpMarker("Shows the folder used by the local replacement storage layer.");

        ImGui::Checkbox("Log file reads", &g_DebugLogStorageReads);
        HelpMarker("Logs local storage read activity when wired into the storage layer.");

        ImGui::Checkbox("Log file writes", &g_DebugLogStorageWrites);
        HelpMarker("Logs local storage write activity when wired into the storage layer.");

        ImGui::Checkbox("Show last save file", &g_DebugShowLastSaveFile);
        HelpMarker("Shows the most recent save/stat file touched by the storage layer.");

        ImGui::Separator();
        ImGui::Text("Storage: local remote storage");
        HelpMarker("Steam remote-storage replacement is using local files.");

        ImGui::Text("Last save: STORM2.S");
        HelpMarker("Example last-save display. Replace with live state when your storage manager exposes it.");

        if (ImGui::Button("Force save stats", ImVec2(160.0f, 24.0f)))
            Logger::Info("DX11 overlay: force save stats requested");
        HelpMarker("Requests a manual stat/save flush when your storage layer supports it.");

        ImGui::SameLine();

        if (ImGui::Button("Dump storage files", ImVec2(170.0f, 24.0f)))
            Logger::Info("DX11 overlay: dump storage files requested");
        HelpMarker("Writes known local storage files to the log when implemented.");
    }
    else if (g_SelectedSubPage == 5)
    {
        ImGui::Text("Debug > Mods");
        HelpMarker("Debug display for the mod loader state.");
        ImGui::Separator();

        ImGui::Checkbox("Show mods folder", &g_DebugShowModsFolder);
        HelpMarker("Shows the resolved mods folder in debug output.");

        ImGui::Checkbox("Log mod errors", &g_DebugLogModErrors);
        HelpMarker("Logs detailed errors from the mod loader.");

        ImGui::Checkbox("Show loaded mod count", &g_DebugShowLoadedModCount);
        HelpMarker("Shows how many mods the loader detected or loaded when that state is exposed.");

        ImGui::Separator();
        ImGui::Text("Mods folder: NarutoStorm2Revived\\mods");
        HelpMarker("Current default relative mods folder.");

        ImGui::Text("Loaded mods: 0");
        HelpMarker("Placeholder until the mod loader reports a live count.");
    }
    else if (g_SelectedSubPage == 6)
    {
        ImGui::Text("Debug > Tools");
        HelpMarker("Manual debug actions for logging and state dumps.");
        ImGui::Separator();

        if (ImGui::Button("Write test log", ImVec2(180.0f, 24.0f)))
            Logger::Info("DX11 overlay: test log from debug tools");
        HelpMarker("Writes one test line to the log to confirm logging works.");

        if (ImGui::Button("Dump overlay state", ImVec2(180.0f, 24.0f)))
            Logger::Info("DX11 overlay: dump overlay state requested");
        HelpMarker("Writes overlay menu/input/render state to the log when implemented.");

        if (ImGui::Button("Dump Steam state", ImVec2(180.0f, 24.0f)))
            Logger::Info("DX11 overlay: dump Steam state requested");
        HelpMarker("Writes Steam proxy/emulation state to the log when implemented.");

        if (ImGui::Button("Dump network state", ImVec2(180.0f, 24.0f)))
            Logger::Info("DX11 overlay: dump network state requested");
        HelpMarker("Writes network hook/filter state to the log when implemented.");

        if (ImGui::Button("Emergency disable overlay", ImVec2(220.0f, 24.0f)))
            g_OverlayEnabled = false;
        HelpMarker("Turns off overlay/status drawing immediately. Press F1 again after re-enabling through code/config if needed.");
    }
}


static void DrawAssetsPage();
static void DrawGamePage()
{
    if (g_SelectedSubPage == 0)
    {
        ImGui::Text("Game > General");
        ImGui::Separator();

        ImGui::TextWrapped("General game tools live here. Patch-style toggles were moved to the Patches tab so options are not duplicated.");

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
        ImGui::TextWrapped("Unlocks, intro skip, cutscene skip, borderless, ultrawide, and similar patch features are only in Patches now.");
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
        ImGui::TextWrapped("NartuoStorm2Revived\\\\mods");

        ImGui::Separator();

        ImGui::Text("Asset cache:");
        ImGui::TextWrapped("NartuoStorm2Revived\\\\cache\\\\assets_cache.json");

        ImGui::Text("Asset preload:");
        ImGui::Text("%s", AssetPreloadManager::IsPreloading() ? "loading" : "idle/complete");
    }
}
;
// duplicate g_CameraRot removed
static bool g_PatchBorderlessFullscreen = true;
static bool g_PatchNativeUltrawide = false;
static bool g_PatchMultiMonitor = false;
static bool g_PatchDisableIntro = false;
static bool g_PatchAutoSkipCutscenes = false;
static bool g_PatchUnlockAllTemp = false;
static bool g_PatchUnlockAllPermanent = false;
static bool g_PatchHighRefresh = false;
static bool g_PatchBackgroundPauseFix = true;
static bool g_PatchSkipOnlineChecks = true;


static void DrawCameraPage()
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
        PlannedCheckbox("Event viewer", &g_DebugShowImGuiStats, "Needs event script/state hook.");
    }
}

static void DrawPatchesPage()
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
        ImGui::TextWrapped("UI is preserved and ready, but the actual patch still needs addresses/hooks before it should touch memory.");

        ImGui::Separator();

        ImGui::Checkbox("Only enable safe patches by default", &g_PatchSkipOnlineChecks);
        HelpMarker("Keeps current safe/offline patch helpers enabled, while planned memory patches stay informational until implemented.");

        ImGui::TextWrapped("No duplicate patch options are shown in Game anymore.");
    }
    else
    {
        ImGui::Text("Patches > Status");
        ImGui::Separator();

        ImGui::Text("Borderless fullscreen: %s", g_PatchBorderlessFullscreen ? "enabled" : "disabled");
        ImGui::Text("Background helper: %s", g_PatchBackgroundPauseFix ? "enabled" : "disabled");
        ImGui::Text("Offline check skip: %s", g_PatchSkipOnlineChecks ? "enabled" : "disabled");

        ImGui::Separator();
        ImGui::TextWrapped("ACTIVE means backed by current framework code. PLANNED means the option is ready but does not patch memory yet.");
    }
}


static char g_AssetSearch[128]{};
static int g_AssetTypeFilter = 0;
static int g_SelectedAssetIndex = -1;
static int g_AssetPreviewTab = 0;
static bool g_AssetOverridesOnly = false;
static bool g_AssetDumpedOnly = false;
static bool g_AssetAutoPreview = true;
static bool g_AssetAutoDumpBeforePreview = false;
static bool g_AssetFullPathLabels = true;
static AssetPreview::PreviewState g_CurrentAssetPreview{};
static bool g_AssetScannedOnce = false;
static int g_AssetCategoryPage = 0;
static bool g_AssetStartedAsyncScan = false;

static void DrawModelPreviewShell(const ImVec2& size)
{
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 p = ImGui::GetCursorScreenPos();
    ImVec2 center(p.x + size.x * 0.5f, p.y + size.y * 0.48f);
    float s = std::min(size.x, size.y) * 0.28f;
    ImU32 blue = IM_COL32(70, 160, 255, 255);
    ImU32 orange = IM_COL32(255, 150, 40, 255);
    ImU32 gray = IM_COL32(120, 120, 120, 180);

    dl->AddRectFilled(p, ImVec2(p.x + size.x, p.y + size.y), IM_COL32(4, 8, 12, 230));
    dl->AddRect(p, ImVec2(p.x + size.x, p.y + size.y), IM_COL32(40, 90, 150, 220));

    ImVec2 a(center.x - s, center.y - s);
    ImVec2 b(center.x + s, center.y - s);
    ImVec2 c(center.x + s, center.y + s);
    ImVec2 d(center.x - s, center.y + s);
    ImVec2 off(s * 0.45f, -s * 0.32f);
    ImVec2 a2(a.x + off.x, a.y + off.y), b2(b.x + off.x, b.y + off.y), c2(c.x + off.x, c.y + off.y), d2(d.x + off.x, d.y + off.y);

    dl->AddLine(a,b,blue,2); dl->AddLine(b,c,blue,2); dl->AddLine(c,d,blue,2); dl->AddLine(d,a,blue,2);
    dl->AddLine(a2,b2,blue,2); dl->AddLine(b2,c2,blue,2); dl->AddLine(c2,d2,blue,2); dl->AddLine(d2,a2,blue,2);
    dl->AddLine(a,a2,blue,2); dl->AddLine(b,b2,blue,2); dl->AddLine(c,c2,blue,2); dl->AddLine(d,d2,blue,2);
    dl->AddLine(ImVec2(center.x, center.y), ImVec2(center.x + s * 1.5f, center.y), orange, 2);
    dl->AddLine(ImVec2(center.x, center.y), ImVec2(center.x, center.y - s * 1.5f), orange, 2);
    dl->AddText(ImVec2(p.x + 10, p.y + 10), IM_COL32(220,220,220,255), "3D Preview Shell");
    dl->AddText(ImVec2(p.x + 10, p.y + size.y - 24), IM_COL32(255,170,40,255), "NUD/XFBIN mesh decode is staged; wire renderer is ready.");
    ImGui::Dummy(size);
}

static void DrawAssetPreviewPanel(const AssetBrowser::AssetEntry& asset)
{
    const char* tabs[] = { "Info", "3D View", "Hex", "Strings", "Edit", "Dump" };
    for (int i = 0; i < ARRAYSIZE(tabs); ++i)
    {
        if (i > 0) ImGui::SameLine();
        if (ImGui::Selectable(tabs[i], g_AssetPreviewTab == i, 0, ImVec2(86, 22)))
            g_AssetPreviewTab = i;
    }
    ImGui::Separator();

    if (g_AssetAutoPreview && !g_CurrentAssetPreview.Valid)
        AssetPreloadManager::ForceBuildPreview(asset, g_CurrentAssetPreview);

    if (g_AssetPreviewTab == 0)
    {
        ImGui::Text("Name: %s", asset.Name.c_str());
        ImGui::Text("Type: %s", AssetBrowser::TypeName(asset.Type));
        ImGui::Text("Source: %s", AssetBrowser::SourceName(asset.Source));
        ImGui::Text("Size: %.2f MB", (double)asset.Size / 1024.0 / 1024.0);
        ImGui::Text("Extension: %s", asset.Extension.c_str());
        ImGui::Text("Virtual: %s", asset.VirtualPath.c_str());
        ImGui::TextWrapped("Path: %s", asset.FullPath.c_str());
        if (asset.IsCpkEntry)
        {
            ImGui::TextColored(ImVec4(1.0f,0.65f,0.2f,1.0f), "CPK indexed entry");
            ImGui::TextWrapped("Archive: %s", asset.ArchivePath.c_str());
            ImGui::TextWrapped("Entry: %s", asset.ArchiveEntryPath.c_str());
        }
        if (asset.HasModOverride || asset.IsLooseOverride)
            ImGui::TextColored(ImVec4(0.4f,1.0f,0.4f,1.0f), "Override active: %s", asset.OverrideFullPath.c_str());
    }
    else if (g_AssetPreviewTab == 1)
    {
        AssetModelViewer::Draw(asset, ImVec2(0.0f, 0.0f));
    }
    else if (g_AssetPreviewTab == 2)
    {
        if (g_CurrentAssetPreview.Bytes.empty())
        {
            std::vector<unsigned char> bytes;
            AssetPreview::ReadBytes(asset.IsCpkEntry ? asset.ArchivePath : asset.FullPath, bytes, 1024 * 1024);
            ImGui::TextWrapped("%s", AssetPreview::BuildHex(bytes, 65536).c_str());
        }
        else
        {
            ImGui::TextWrapped("%s", AssetPreview::BuildHex(g_CurrentAssetPreview.Bytes, 65536).c_str());
        }
    }
    else if (g_AssetPreviewTab == 3)
    {
        ImGui::TextWrapped("%s", g_CurrentAssetPreview.Text.c_str());
    }
    else if (g_AssetPreviewTab == 4)
    {
        static int editOffset = 0;
        static int editValue = 0;
        ImGui::TextColored(ImVec4(1.0f,0.65f,0.2f,1.0f), "Safe Hex Edit");
        ImGui::TextWrapped("Edits are saved only to Dump\Edited, never directly to the original game file.");
        ImGui::InputInt("Offset", &editOffset);
        ImGui::InputInt("Value", &editValue);
        if (ImGui::Button("Write Edited Copy", ImVec2(170,24)))
            Logger::Info("Asset hex edit requested for " + asset.RelativePath);
        ImGui::SameLine();
        if (ImGui::Button("Dump First", ImVec2(120,24)))
            AssetBrowser::DumpAsset(asset, false);
    }
    else
    {
        ImGui::Text("Dump folder:");
        ImGui::TextWrapped("%s", AssetBrowser::GetDumpFolder().c_str());
        if (!asset.DumpFullPath.empty())
            ImGui::TextWrapped("Dumped file: %s", asset.DumpFullPath.c_str());
        else
            ImGui::TextDisabled("This asset is not marked as dumped yet.");
    }
}

static void DrawAssetsPage()
{
    if (!g_AssetStartedAsyncScan)
    {
        AssetBrowser::LoadCache();
        AssetBrowser::StartAsyncScan(false);
        g_AssetStartedAsyncScan = true;
    }

    const auto& assets = AssetBrowser::GetAssets();
    AssetBrowser::AssetStats stats = AssetBrowser::GetStats();

    const char* pageNames[] = { "Browser", "Stages", "Characters", "Models", "Textures", "Audio", "Packages", "Mod Overrides", "Tools" };
    ImGui::Text("Assets > %s", pageNames[g_SelectedSubPage >= 0 && g_SelectedSubPage < ARRAYSIZE(pageNames) ? g_SelectedSubPage : 0]);
    ImGui::Separator();

    if (ImGui::Button("Async Rescan", ImVec2(120, 24))) AssetBrowser::StartAsyncScan(true);
    ImGui::SameLine();
    if (ImGui::Button("Save Cache", ImVec2(110, 24))) AssetBrowser::SaveCache();
    ImGui::SameLine();
    if (ImGui::Button("Load Cache", ImVec2(110, 24))) AssetBrowser::LoadCache();
    ImGui::SameLine();
    if (ImGui::Button("Dump Selected", ImVec2(130, 24)) && g_SelectedAssetIndex >= 0 && g_SelectedAssetIndex < (int)assets.size()) AssetBrowser::DumpAsset(assets[g_SelectedAssetIndex], true);
    ImGui::SameLine();
    if (ImGui::Button("Export CSV", ImVec2(110, 24))) AssetBrowser::ExportCsv("NartuoStorm2Revived/Dump/asset_index.csv");

    ImGui::Text("Total %d | Game %d | Mods %d | Dumped %d | Overrides %d | CPK %d | XFBIN %d | Models %d | Textures %d | Audio %d",
        stats.TotalAssets, stats.GameAssets, stats.ModAssets, stats.DumpedAssets, stats.OverrideAssets, stats.PackageAssets, stats.XfbinAssets, stats.ModelAssets, stats.TextureAssets, stats.AudioAssets);

    if (AssetBrowser::IsScanning())
    {
        ImGui::TextColored(ImVec4(1.0f,0.65f,0.2f,1.0f), "Reading assets in background: %d%% - %s", AssetBrowser::GetScanProgress(), AssetBrowser::GetScanStatus().c_str());
        ImGui::ProgressBar(AssetBrowser::GetScanProgress() / 100.0f, ImVec2(-1, 12));
    }
    else if (AssetBrowser::WasCacheLoaded())
    {
        ImGui::TextDisabled("Loaded from cache first, then refreshed in background like shader compilation.");
    }

    const char* filters[] = { "All", "Package", "Model", "Texture", "Animation", "Audio", "Stage", "Character", "Data", "Unknown" };
    ImGui::SetNextItemWidth(280); ImGui::InputText("Search", g_AssetSearch, sizeof(g_AssetSearch));
    ImGui::SameLine(); ImGui::SetNextItemWidth(190); ImGui::Combo("Type", &g_AssetTypeFilter, filters, ARRAYSIZE(filters));
    ImGui::Checkbox("Overrides only", &g_AssetOverridesOnly); ImGui::SameLine();
    ImGui::Checkbox("Dumped only", &g_AssetDumpedOnly); ImGui::SameLine();
    ImGui::Checkbox("Auto preview", &g_AssetAutoPreview); ImGui::SameLine();
    ImGui::Checkbox("Auto dump before preview", &g_AssetAutoDumpBeforePreview);

    int forcedType = 0;
    if (g_SelectedSubPage == 1) forcedType = 6;      // Stages
    else if (g_SelectedSubPage == 2) forcedType = 7; // Characters
    else if (g_SelectedSubPage == 3) forcedType = 2; // Models
    else if (g_SelectedSubPage == 4) forcedType = 3; // Textures
    else if (g_SelectedSubPage == 5) forcedType = 5; // Audio
    else if (g_SelectedSubPage == 6) forcedType = 1; // Packages

    ImGui::Separator();
    float previewWidth = 520.0f;
    float listWidth = ImGui::GetContentRegionAvail().x - previewWidth - 10.0f;
    if (listWidth < 420.0f) listWidth = 420.0f;
    ImGui::BeginChild("##AssetList", ImVec2(listWidth, 0.0f), true);

    std::string search = g_AssetSearch;
    std::transform(search.begin(), search.end(), search.begin(), [](unsigned char c){ return (char)std::tolower(c); });
    for (int i = 0; i < (int)assets.size(); ++i)
    {
        const auto& asset = assets[i];
        int activeType = forcedType ? forcedType : g_AssetTypeFilter;
        if (activeType > 0 && _stricmp(AssetBrowser::TypeName(asset.Type), filters[activeType]) != 0) continue;
        if (g_SelectedSubPage == 7 && !(asset.HasModOverride || asset.IsLooseOverride || asset.Source == AssetBrowser::AssetSource::Mod)) continue;
        if (g_AssetOverridesOnly && !(asset.HasModOverride || asset.IsLooseOverride || asset.Source == AssetBrowser::AssetSource::Mod)) continue;
        if (g_AssetDumpedOnly && !asset.IsDumped && asset.Source != AssetBrowser::AssetSource::Dump) continue;
        if (!search.empty())
        {
            std::string hay = asset.RelativePath + " " + asset.VirtualPath + " " + asset.Name;
            std::transform(hay.begin(), hay.end(), hay.begin(), [](unsigned char c){ return (char)std::tolower(c); });
            if (hay.find(search) == std::string::npos) continue;
        }
        std::string label = "[" + std::string(AssetBrowser::SourceName(asset.Source)) + "] [" + AssetBrowser::TypeName(asset.Type) + "] " + (g_AssetFullPathLabels ? asset.RelativePath : asset.Name);
        if (asset.HasModOverride || asset.IsLooseOverride) label += "  [OVERRIDE]";
        if (asset.IsDumped) label += "  [DUMPED]";
        if (asset.IsCpkEntry) label += "  [CPK]";
        if (ImGui::Selectable(label.c_str(), g_SelectedAssetIndex == i))
        {
            g_SelectedAssetIndex = i;
            g_AssetPreviewTab = 1;
            if (g_AssetAutoPreview) AssetPreloadManager::ForceBuildPreview(asset, g_CurrentAssetPreview);
        }
    }
    ImGui::EndChild();

    ImGui::SameLine();
    ImGui::BeginChild("##AssetDetails", ImVec2(0.0f, 0.0f), true);
    if (g_SelectedAssetIndex >= 0 && g_SelectedAssetIndex < (int)assets.size())
    {
        const auto& asset = assets[g_SelectedAssetIndex];
        if (ImGui::Button("Dump", ImVec2(100, 24))) AssetBrowser::DumpAsset(asset, true);
        ImGui::SameLine();
        if (ImGui::Button("XFBIN Info", ImVec2(110, 24))) AssetBrowser::DumpXfbinInfo(asset);
        ImGui::SameLine();
        if (ImGui::Button("Log", ImVec2(70, 24))) Logger::Info("Selected asset: " + asset.RelativePath);
        DrawAssetPreviewPanel(asset);
    }
    else
    {
        ImGui::Text("Select an asset from the list.");
        ImGui::TextDisabled("Preview updates automatically. The 3D View tab shows the object viewer shell and will use decoded meshes when NUD/XFBIN decoding is wired.");
    }
    ImGui::EndChild();
}

static void DrawDropdownPanel()
{
    if (!g_MenuOpen || g_SelectedTab < 0)
        return;

    ImGuiViewport* viewport = ImGui::GetMainViewport();
    float x = 95.0f + static_cast<float>(g_SelectedTab) * 88.0f;
    float y = 31.0f * g_MenuAnimation;
    float panelWidth = std::min(viewport->Size.x - x - 12.0f, 1220.0f);
    float panelHeight = std::min(viewport->Size.y - y - 44.0f, 650.0f);
    if (panelWidth < 760.0f) panelWidth = viewport->Size.x - 24.0f, x = 12.0f;
    if (panelHeight < 360.0f) panelHeight = viewport->Size.y - y - 20.0f;

    ImGui::SetNextWindowPos(ImVec2(x, y), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(panelWidth, panelHeight), ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.90f);

    ImGui::Begin("##NS2ShipwrightDropdown", nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse);

    ImGui::BeginChild("##NS2LeftList", ImVec2(220.0f, 0.0f), true);
    ImGui::Text("Options");
    ImGui::Separator();

    if (g_SelectedTab == 0) { DrawLeftButton("Display",0); DrawLeftButton("Anti-Aliasing",1); DrawLeftButton("Quality",2); DrawLeftButton("Overlay",3); DrawLeftButton("Advanced",4); }
    else if (g_SelectedTab == 1) { DrawLeftButton("General",0); DrawLeftButton("Mouse",1); DrawLeftButton("Binds",2); DrawLeftButton("Advanced",3); }
    else if (g_SelectedTab == 5) { DrawLeftButton("General",0); DrawLeftButton("Saves",1); DrawLeftButton("Information",2); }
    else if (g_SelectedTab == 6) { DrawLeftButton("Free Camera",0); DrawLeftButton("Coordinates",1); DrawLeftButton("Viewers",2); }
    else if (g_SelectedTab == 4) { DrawLeftButton("Browser",0); DrawLeftButton("Stages",1); DrawLeftButton("Characters",2); DrawLeftButton("Models",3); DrawLeftButton("Textures",4); DrawLeftButton("Audio",5); DrawLeftButton("Packages",6); DrawLeftButton("Mod Overrides",7); DrawLeftButton("Tools",8); }
    else if (g_SelectedTab == 2) { DrawLeftButton("Loader",0); DrawLeftButton("Folder",1); }
    else if (g_SelectedTab == 7) { DrawLeftButton("Display",0); DrawLeftButton("Gameplay",1); DrawLeftButton("Safety",2); DrawLeftButton("Status",3); }
    else if (g_SelectedTab == 3) { DrawLeftButton("Performance",0); DrawLeftButton("Render",1); DrawLeftButton("Steam",2); DrawLeftButton("Network",3); DrawLeftButton("Storage",4); DrawLeftButton("Mods",5); DrawLeftButton("Tools",6); }

    ImGui::EndChild();
    ImGui::SameLine();
    ImGui::BeginChild("##NS2RightContent", ImVec2(0.0f, 0.0f), true);

    switch (g_SelectedTab)
    {
    case 0: DrawGraphicsPage(); break;
    case 1: DrawControlsPage(); break;
    case 5: DrawGamePage(); break;
    case 6: DrawCameraPage(); break;
    case 4: DrawAssetsPage(); break;
    case 2: DrawModsPage(); break;
    case 7: DrawPatchesPage(); break;
    case 3: DrawDebugPage(); break;
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

    ImGui::Begin("##NS2AnimatedTopMenuBar", nullptr,
        ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoCollapse);

    ImGui::SetCursorPos(ImVec2(8.0f, 5.0f));
    ImGui::Text("NS2 Revived");
    ImGui::SameLine(95.0f);
    DrawTabButton("Graphics", 0); ImGui::SameLine();
    DrawTabButton("Controls", 1); ImGui::SameLine();
    DrawTabButton("Game", 5); ImGui::SameLine();
    DrawTabButton("Camera", 6); ImGui::SameLine();
    DrawTabButton("Assets", 4); ImGui::SameLine();
    DrawTabButton("Mods", 2); ImGui::SameLine();
    DrawTabButton("Patches", 7); ImGui::SameLine();
    DrawTabButton("Debug", 3);

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
    DrawAssetLoadingToast();

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
