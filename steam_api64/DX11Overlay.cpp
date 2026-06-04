#include "StdInc.h"
#include "DX11Overlay.h"
#include "Logger.h"
#include "NS2Config.h"
#include "AssetBrowser.h"
#include "AssetPreview.h"
#include "ModRedirector.h"
#include "CpkArchive.h"

#include "imgui.h"
#include "imgui_impl_dx11.h"
#include "imgui_impl_win32.h"

#include <d3d11.h>
#include <dxgi.h>
#include <shellapi.h>
#include <fstream>
#include <sstream>
#include <iomanip>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "Shell32.lib")

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
static bool g_ConsoleOpen = false;
static bool g_BacktickWasDown = false;

static HWND g_GameWindow = nullptr;
static WNDPROC g_OriginalWndProc = nullptr;

static ID3D11Device* g_Device = nullptr;
static ID3D11DeviceContext* g_Context = nullptr;
static ID3D11RenderTargetView* g_RenderTargetView = nullptr;

static LARGE_INTEGER g_Frequency{};
static LARGE_INTEGER g_LastFpsTime{};
static int g_FrameCounter = 0;
static int g_CurrentFps = 0;
static float g_FpsHistory[240]{};
static float g_FrameTimeHistory[240]{};
static int g_FpsHistoryOffset = 0;
static uint64_t g_PresentCount = 0;
static uint64_t g_ResizeCount = 0;

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
static bool g_GraphicsFixResizeFlicker = true;
static bool g_GraphicsBorderlessFix = true;
static bool g_GraphicsKeepOverlayAboveUi = true;
static float g_OverlayAlpha = 0.90f;
static float g_MenuAnimSpeed = 14.0f;

static bool g_ControlsControllerEnabled = true;
static bool g_ControlsCaptureMouse = true;
static bool g_ControlsBlockInput = true;
static bool g_ControlsKeyboardNav = true;

static bool g_ModsEnabled = true;
static bool g_ModsLoadDll = true;
static bool g_ModsVerboseLog = true;
static bool g_ModRedirectEnabled = true;

static bool g_PatchDisableIntro = false;
static bool g_PatchAutoSkipCutscenes = false;
static bool g_PatchSkipNetworkChecks = true;
static bool g_PatchNativeUltrawide = false;
static bool g_PatchMultiMonitor = false;
static bool g_PatchUnlockAllTemp = false;
static bool g_PatchUnlockAllPersistent = false;
static bool g_PatchSlowMotion = false;
static bool g_PatchFrameStep = false;
static bool g_PatchRemoveHud = false;
static bool g_PatchHitboxViewer = false;
static bool g_PatchHurtboxViewer = false;
static bool g_PatchCollisionViewer = false;
static bool g_PatchSpawnPointViewer = false;
static bool g_PatchEventViewer = false;
static float g_GameSpeed = 1.0f;

static bool g_FreeCameraEnabled = false;
static bool g_FreeCameraCutscenes = false;
static bool g_FreeCameraRemoveHud = false;
static float g_FreeCameraFov = 60.0f;
static float g_FreeCameraSpeed = 1.0f;
static float g_FreeCameraBoost = 4.0f;
static float g_FreeCameraSmoothing = 0.25f;
static float g_FreeCameraPos[3] = { 0.0f, 0.0f, 0.0f };
static float g_FreeCameraRot[3] = { 0.0f, 0.0f, 0.0f };

static bool g_DebugLogSteamInterfaces = false;
static bool g_DebugLogCallbacks = false;
static bool g_DebugLogCallResults = false;
static bool g_DebugLogDns = true;
static bool g_DebugLogSocketConnects = true;
static bool g_DebugVerboseLog = false;

static char g_ConsoleInput[256]{};
static std::vector<std::string> g_ConsoleLines;

static char g_AssetSearch[160]{};
static int g_AssetTypeFilter = 0;
static int g_AssetSourceFilter = 0;
static bool g_AssetOverrideOnly = false;
static bool g_AssetDumpedOnly = false;
static int g_SelectedAssetIndex = -1;
static AssetPreview::PreviewInfo g_CurrentPreview{};
static bool g_HasPreview = false;
static bool g_AutoPreview = true;
static bool g_ShowAssetFullPath = true;

static const char* kTabNames[] = { "Graphics", "Controls", "Game", "Camera", "Assets", "Mods", "Patches", "Debug" };

template <typename T>
static T ClampValue(T v, T minValue, T maxValue)
{
    if (v < minValue) return minValue;
    if (v > maxValue) return maxValue;
    return v;
}

static std::string ToLower(std::string text)
{
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return text;
}

static std::string NormalizeSlashes(std::string text)
{
    std::replace(text.begin(), text.end(), '\\', '/');
    return text;
}

static void ReleaseRenderTarget()
{
    if (g_RenderTargetView)
    {
        g_RenderTargetView->Release();
        g_RenderTargetView = nullptr;
    }
}

static void ReleaseD3D()
{
    ReleaseRenderTarget();
    if (g_Context)
    {
        g_Context->Release();
        g_Context = nullptr;
    }
    if (g_Device)
    {
        g_Device->Release();
        g_Device = nullptr;
    }
}

static void HelpMarker(const char* text)
{
    ImGui::SameLine();
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(460.0f);
        ImGui::TextUnformatted(text);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}

static void StatusBadge(const char* text, bool active)
{
    ImGui::SameLine();
    ImGui::TextColored(active ? ImVec4(0.35f, 0.95f, 0.55f, 1.0f) : ImVec4(1.0f, 0.62f, 0.25f, 1.0f), active ? "[ACTIVE]" : "[PLANNED]");
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("%s", text);
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

static bool BoolOption(const char* label, bool& value, const char* help, bool active)
{
    bool changed = ImGui::Checkbox(label, &value);
    StatusBadge(active ? "This option is wired now." : "This option is visible now, but requires a future address/hook to affect the game.", active);
    HelpMarker(help);
    return changed;
}

static std::string GetGameFolder()
{
    char path[MAX_PATH]{};
    GetModuleFileNameA(nullptr, path, MAX_PATH);
    return std::filesystem::path(path).parent_path().string();
}

static std::filesystem::path GetFrameworkFolder()
{
    return std::filesystem::path(GetGameFolder()) / "NartuoStorm2Revived";
}

static std::filesystem::path GetSaveBackupFolder()
{
    return GetFrameworkFolder() / "SaveBackups";
}

static void OpenFolder(const std::string& folder)
{
    try { std::filesystem::create_directories(folder); } catch (...) {}
    ShellExecuteA(nullptr, "open", folder.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
}

static std::vector<std::filesystem::path> FindKnownSaveFiles()
{
    std::vector<std::filesystem::path> found;
    std::vector<std::string> names = { "STORM2.S", "ns2_userData.dat", "ns2_checkBattleCut.dat" };
    std::filesystem::path roots[] = { GetFrameworkFolder(), std::filesystem::path(GetGameFolder()) };
    for (const auto& root : roots)
    {
        if (!std::filesystem::exists(root)) continue;
        try
        {
            for (const auto& e : std::filesystem::recursive_directory_iterator(root))
            {
                if (!e.is_regular_file()) continue;
                std::string file = e.path().filename().string();
                for (const auto& n : names)
                {
                    if (_stricmp(file.c_str(), n.c_str()) == 0)
                        found.push_back(e.path());
                }
            }
        }
        catch (...) {}
    }
    return found;
}

static void BackupSaves()
{
    auto saves = FindKnownSaveFiles();
    auto out = GetSaveBackupFolder();
    try { std::filesystem::create_directories(out); } catch (...) {}
    int copied = 0;
    for (const auto& s : saves)
    {
        try
        {
            std::filesystem::copy_file(s, out / s.filename(), std::filesystem::copy_options::overwrite_existing);
            ++copied;
        }
        catch (...) {}
    }
    Logger::Info("Save backup copied files=" + std::to_string(copied));
    g_ConsoleLines.push_back("save_backup: copied " + std::to_string(copied) + " file(s)");
}

static void RestoreSaves()
{
    auto backup = GetSaveBackupFolder();
    if (!std::filesystem::exists(backup))
    {
        g_ConsoleLines.push_back("save_restore: backup folder missing");
        return;
    }
    auto saves = FindKnownSaveFiles();
    int copied = 0;
    for (const auto& b : std::filesystem::directory_iterator(backup))
    {
        if (!b.is_regular_file()) continue;
        for (const auto& s : saves)
        {
            if (_stricmp(s.filename().string().c_str(), b.path().filename().string().c_str()) == 0)
            {
                try { std::filesystem::copy_file(b.path(), s, std::filesystem::copy_options::overwrite_existing); ++copied; } catch (...) {}
            }
        }
    }
    Logger::Info("Save restore copied files=" + std::to_string(copied));
    g_ConsoleLines.push_back("save_restore: restored " + std::to_string(copied) + " file(s)");
}

static void UpdateFps()
{
    if (g_Frequency.QuadPart == 0)
    {
        QueryPerformanceFrequency(&g_Frequency);
        QueryPerformanceCounter(&g_LastFpsTime);
        return;
    }

    ++g_FrameCounter;
    LARGE_INTEGER now{};
    QueryPerformanceCounter(&now);
    double elapsed = static_cast<double>(now.QuadPart - g_LastFpsTime.QuadPart) / static_cast<double>(g_Frequency.QuadPart);
    if (elapsed >= 1.0)
    {
        g_CurrentFps = static_cast<int>(static_cast<double>(g_FrameCounter) / elapsed + 0.5);
        g_FpsHistory[g_FpsHistoryOffset] = static_cast<float>(g_CurrentFps);
        g_FrameTimeHistory[g_FpsHistoryOffset] = g_CurrentFps > 0 ? 1000.0f / static_cast<float>(g_CurrentFps) : 0.0f;
        g_FpsHistoryOffset = (g_FpsHistoryOffset + 1) % ARRAYSIZE(g_FpsHistory);
        g_FrameCounter = 0;
        g_LastFpsTime = now;
    }
}

static void ExecuteConsoleCommand(const std::string& command)
{
    if (command.empty()) return;
    g_ConsoleLines.push_back("] " + command);
    std::string c = ToLower(command);
    if (c == "clear") g_ConsoleLines.clear();
    else if (c == "help") g_ConsoleLines.push_back("commands: help, clear, fps, build, scan_assets, dump_assets, dump_selected, mods, save_backup, save_restore");
    else if (c == "fps") g_ConsoleLines.push_back("fps: " + std::to_string(g_CurrentFps));
    else if (c == "build") g_ConsoleLines.push_back(std::string("build: ") + __DATE__ + " " + __TIME__);
    else if (c == "scan_assets") { AssetBrowser::Scan(); g_ConsoleLines.push_back("asset scan complete"); }
    else if (c == "dump_assets") { int n = AssetBrowser::DumpAllAssets(false); g_ConsoleLines.push_back("dumped " + std::to_string(n) + " assets"); }
    else if (c == "dump_selected") { if (g_SelectedAssetIndex >= 0) AssetBrowser::DumpAssetByIndex(g_SelectedAssetIndex, true); g_ConsoleLines.push_back("dump selected requested"); }
    else if (c == "mods") { ModRedirector::Scan(); g_ConsoleLines.push_back("mod redirect scan complete"); }
    else if (c == "save_backup") BackupSaves();
    else if (c == "save_restore") RestoreSaves();
    else g_ConsoleLines.push_back("unknown command: " + command);
}

static void UpdateHotkeys()
{
    bool f1Down = (GetAsyncKeyState(VK_F1) & 0x8000) != 0;
    bool consoleDown = (GetAsyncKeyState(VK_OEM_3) & 0x8000) != 0;

    if (f1Down && !g_F1WasDown)
    {
        g_MenuOpen = !g_MenuOpen;
        if (!g_MenuOpen) g_SelectedTab = -1;
    }
    if (consoleDown && !g_BacktickWasDown)
    {
        g_ConsoleOpen = !g_ConsoleOpen;
        if (g_ConsoleOpen && g_ConsoleLines.empty())
            g_ConsoleLines.push_back("NS2 Revived developer console. Type help.");
    }
    g_F1WasDown = f1Down;
    g_BacktickWasDown = consoleDown;
}

static bool CreateRenderTarget(IDXGISwapChain* swapChain)
{
    if (g_RenderTargetView) return true;
    if (!swapChain || !g_Device) return false;
    ID3D11Texture2D* backBuffer = nullptr;
    HRESULT hr = swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&backBuffer));
    if (FAILED(hr) || !backBuffer) { Logger::Error("DX11 overlay: failed to get backbuffer"); return false; }
    hr = g_Device->CreateRenderTargetView(backBuffer, nullptr, &g_RenderTargetView);
    backBuffer->Release();
    if (FAILED(hr) || !g_RenderTargetView) { Logger::Error("DX11 overlay: failed to create render target view"); return false; }
    Logger::Info("DX11 overlay: render target created");
    return true;
}

static LRESULT CALLBACK OverlayWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if ((g_MenuOpen || g_ConsoleOpen) && g_ImGuiReady)
    {
        ImGuiIO& io = ImGui::GetIO();
        io.MouseDrawCursor = g_DrawMouseCursor;
        if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam)) return TRUE;
        if (g_BlockGameInputWhenOpen)
        {
            switch (msg)
            {
            case WM_MOUSEMOVE: case WM_LBUTTONDOWN: case WM_LBUTTONUP: case WM_RBUTTONDOWN: case WM_RBUTTONUP:
            case WM_MBUTTONDOWN: case WM_MBUTTONUP: case WM_MOUSEWHEEL: case WM_MOUSEHWHEEL:
            case WM_KEYDOWN: case WM_KEYUP: case WM_CHAR:
                return TRUE;
            }
        }
    }
    return g_OriginalWndProc ? CallWindowProcA(g_OriginalWndProc, hwnd, msg, wParam, lParam) : DefWindowProcA(hwnd, msg, wParam, lParam);
}

static bool HookGameWindow()
{
    if (!g_GameWindow || g_OriginalWndProc) return true;
    g_OriginalWndProc = reinterpret_cast<WNDPROC>(SetWindowLongPtrA(g_GameWindow, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(OverlayWndProc)));
    if (!g_OriginalWndProc) { Logger::Error("DX11 overlay: failed to hook game window proc"); return false; }
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
    ImVec4* c = style.Colors;
    c[ImGuiCol_WindowBg] = ImVec4(0.035f, 0.038f, 0.042f, 0.92f);
    c[ImGuiCol_ChildBg] = ImVec4(0.020f, 0.022f, 0.026f, 0.58f);
    c[ImGuiCol_Button] = ImVec4(0.12f, 0.30f, 0.52f, 1.00f);
    c[ImGuiCol_ButtonHovered] = ImVec4(0.18f, 0.42f, 0.72f, 1.00f);
    c[ImGuiCol_ButtonActive] = ImVec4(0.22f, 0.50f, 0.86f, 1.00f);
    c[ImGuiCol_CheckMark] = ImVec4(0.32f, 0.62f, 1.00f, 1.00f);
    c[ImGuiCol_Header] = ImVec4(0.14f, 0.34f, 0.58f, 1.00f);
    c[ImGuiCol_HeaderHovered] = ImVec4(0.18f, 0.44f, 0.74f, 1.00f);
    c[ImGuiCol_HeaderActive] = ImVec4(0.22f, 0.50f, 0.86f, 1.00f);
}

static bool InitImGui(IDXGISwapChain* swapChain)
{
    if (g_ImGuiReady) return true;
    if (!swapChain) return false;
    HRESULT hr = swapChain->GetDevice(__uuidof(ID3D11Device), reinterpret_cast<void**>(&g_Device));
    if (FAILED(hr) || !g_Device) { Logger::Error("DX11 overlay: failed to get D3D11 device"); return false; }
    g_Device->GetImmediateContext(&g_Context);
    if (!g_Context) { Logger::Error("DX11 overlay: failed to get D3D11 context"); ReleaseD3D(); return false; }
    DXGI_SWAP_CHAIN_DESC desc{};
    hr = swapChain->GetDesc(&desc);
    if (FAILED(hr) || !desc.OutputWindow) { Logger::Error("DX11 overlay: failed to get game window"); ReleaseD3D(); return false; }
    g_GameWindow = desc.OutputWindow;
    if (!CreateRenderTarget(swapChain)) { ReleaseD3D(); return false; }
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.IniFilename = nullptr;
    io.LogFilename = nullptr;
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
    if (fabsf(delta) < 0.001f) current = target;
    return current;
}

static void DrawAlwaysStatusBar()
{
    if (!g_OverlayEnabled || !g_ShowStatusBar) return;
    ImGui::SetNextWindowPos(ImVec2(6.0f, 5.0f), ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.42f);
    ImGui::Begin("##NS2StatusOnly", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav);
    std::string text;
    if (g_StatusShowName) text += "NS2 Revived";
    if (g_StatusShowBuild) { if (!text.empty()) text += " | "; text += "Build "; text += __DATE__; text += " "; text += __TIME__; }
    if (g_StatusShowFps) { if (!text.empty()) text += " | "; text += "FPS "; text += std::to_string(g_CurrentFps); }
    if (g_StatusShowHotkey) { if (!text.empty()) text += " | "; text += "F1 Menu  |  ` Console"; }
    ImGui::TextUnformatted(text.c_str());
    ImGui::End();
}

static void DrawDeveloperConsole()
{
    if (!g_ConsoleOpen) return;
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    const float consoleY = 28.0f;
    const float consoleHeight = 145.0f;
    ImGui::SetNextWindowPos(ImVec2(0.0f, consoleY), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, consoleHeight), ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.78f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10.0f, 7.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.02f, 0.025f, 0.03f, 0.78f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.16f, 0.36f, 0.62f, 0.85f));
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.04f, 0.055f, 0.075f, 0.95f));
    ImGui::Begin("##NS2DeveloperConsole", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse);
    ImGui::Text("NS2 Revived Developer Console");
    ImGui::SameLine(viewport->Size.x - 190.0f);
    ImGui::TextDisabled("` close  |  help");
    ImGui::Separator();
    ImGui::BeginChild("##ConsoleHistory", ImVec2(0.0f, 72.0f), false);
    int start = static_cast<int>(g_ConsoleLines.size()) - 5;
    if (start < 0) start = 0;
    for (int i = start; i < static_cast<int>(g_ConsoleLines.size()); ++i) ImGui::TextUnformatted(g_ConsoleLines[i].c_str());
    ImGui::EndChild();
    ImGui::Separator();
    ImGui::TextColored(ImVec4(0.32f, 0.62f, 1.0f, 1.0f), "]");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(-1.0f);
    if (ImGui::InputText("##ConsoleInput", g_ConsoleInput, sizeof(g_ConsoleInput), ImGuiInputTextFlags_EnterReturnsTrue))
    {
        ExecuteConsoleCommand(g_ConsoleInput);
        g_ConsoleInput[0] = '\0';
        ImGui::SetKeyboardFocusHere(-1);
    }
    if (ImGui::IsWindowAppearing()) ImGui::SetKeyboardFocusHere(-1);
    ImGui::End();
    ImGui::PopStyleColor(3);
    ImGui::PopStyleVar(2);
}

static void DrawTabButton(const char* label, int tab)
{
    bool selected = g_SelectedTab == tab;
    if (selected) { ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.15f, 0.38f, 0.68f, 1.0f)); ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.20f, 0.48f, 0.82f, 1.0f)); }
    if (ImGui::Button(label, ImVec2(86.0f, 22.0f))) { if (selected) g_SelectedTab = -1; else { g_SelectedTab = tab; g_SelectedSubPage = 0; } }
    if (selected) ImGui::PopStyleColor(2);
}

static void DrawLeftButton(const char* label, int page)
{
    bool selected = g_SelectedSubPage == page;
    if (selected) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.17f, 0.38f, 0.66f, 1.0f));
    if (ImGui::Button(label, ImVec2(205.0f, 25.0f))) g_SelectedSubPage = page;
    if (selected) ImGui::PopStyleColor();
}

static void ResolutionPreset(const char* label, int width, int height)
{
    auto& s = NS2Config::Screen();
    if (ImGui::Button(label, ImVec2(112.0f, 24.0f))) { s.Width = width; s.Height = height; }
}

static void DrawGraphicsPage()
{
    auto& s = NS2Config::Screen();
    if (g_SelectedSubPage == 0)
    {
        ImGui::Text("Graphics > Display"); ImGui::Separator();
        ImGui::InputInt("Width", &s.Width); HelpMarker("Saved to config.ini width.");
        ImGui::InputInt("Height", &s.Height); HelpMarker("Saved to config.ini height.");
        ImGui::InputInt("Aspect X", &s.AspectX); HelpMarker("Usually 16.");
        ImGui::InputInt("Aspect Y", &s.AspectY); HelpMarker("Usually 9.");
        ImGui::Separator(); ImGui::Text("Resolution Presets");
        ResolutionPreset("1280x720", 1280, 720); ImGui::SameLine(); ResolutionPreset("1600x900", 1600, 900); ImGui::SameLine(); ResolutionPreset("1920x1080", 1920, 1080);
        ResolutionPreset("2560x1440", 2560, 1440); ImGui::SameLine(); ResolutionPreset("3840x2160", 3840, 2160);
        ImGui::Separator();
        IniCheckbox("Windowed", s.Windowed, "Uses the game's windowed mode setting.");
        IniCheckbox("VSync", s.VSync, "Syncs to monitor refresh.");
        IniCheckbox("Show Cursor", s.ShowCursor, "Controls the game's cursor visibility.");
        IniCheckbox("Game FPS Counter", s.FPS, "Controls the game's built-in FPS counter if supported.");
        ImGui::Separator();
        if (ImGui::Button("Save Config", ImVec2(130, 24))) NS2Config::Save(); ImGui::SameLine();
        if (ImGui::Button("Reload", ImVec2(100, 24))) NS2Config::Load(); ImGui::SameLine();
        if (ImGui::Button("Reset Graphics", ImVec2(145, 24))) NS2Config::ResetGraphics();
    }
    else if (g_SelectedSubPage == 1)
    {
        ImGui::Text("Graphics > Anti-Aliasing"); ImGui::Separator();
        IniCheckbox("FXAA", s.FXAA, "Fast post-process anti-aliasing.");
        IniCheckbox("SMAA", s.SMAA, "Sharper post-process anti-aliasing if supported by this build.");
        IniCheckbox("MSAA", s.MSAA, "Multi-sample AA config option.");
        ImGui::SliderInt("SSAA", &s.SSAA, 0, 4); HelpMarker("Super-sampling. Higher costs more performance.");
        if (ImGui::Button("Disable All AA", ImVec2(150, 24))) { s.FXAA = s.SMAA = s.MSAA = s.SSAA = 0; } ImGui::SameLine();
        if (ImGui::Button("Recommended", ImVec2(150, 24))) { s.FXAA = 1; s.SMAA = 0; s.MSAA = 1; s.SSAA = 0; }
    }
    else if (g_SelectedSubPage == 2)
    {
        ImGui::Text("Graphics > Quality"); ImGui::Separator();
        ImGui::SliderInt("Shadow Quality", &s.ShadowQuality, 0, 3); HelpMarker("Config-backed shadow quality.");
        IniCheckbox("Mipmaps", s.MipMap, "Texture mipmap option from config.ini.");
        IniCheckbox("Motion Blur", s.MotionBlur, "Config-backed motion blur.");
        IniCheckbox("Glare", s.Glare, "Config-backed glare/bloom-like option.");
        ImGui::Separator();
        if (ImGui::Button("Performance", ImVec2(125, 24))) { s.ShadowQuality = 0; s.MotionBlur = 0; s.Glare = 0; s.FXAA = 1; s.MSAA = 0; s.SSAA = 0; }
        ImGui::SameLine(); if (ImGui::Button("Balanced", ImVec2(125, 24))) { s.ShadowQuality = 1; s.MotionBlur = 1; s.Glare = 1; s.FXAA = 1; s.MSAA = 1; s.SSAA = 0; }
        ImGui::SameLine(); if (ImGui::Button("Quality", ImVec2(125, 24))) { s.ShadowQuality = 3; s.MotionBlur = 1; s.Glare = 1; s.FXAA = 1; s.SMAA = 1; s.MSAA = 1; s.SSAA = 1; }
    }
    else if (g_SelectedSubPage == 3)
    {
        ImGui::Text("Graphics > Overlay"); ImGui::Separator();
        ImGui::SliderFloat("Overlay Alpha", &g_OverlayAlpha, 0.30f, 1.0f); HelpMarker("Overlay panel opacity.");
        ImGui::SliderFloat("Animation Speed", &g_MenuAnimSpeed, 4.0f, 24.0f); HelpMarker("F1 menu slide speed.");
        ImGui::Checkbox("Show top-left status", &g_ShowStatusBar); HelpMarker("Small NS2 Revived status line.");
        ImGui::Checkbox("Show build date", &g_StatusShowBuild); HelpMarker("Compile date/time in status line.");
        ImGui::Checkbox("Show FPS", &g_StatusShowFps); HelpMarker("FPS in status line.");
        ImGui::Checkbox("Show hotkey hint", &g_StatusShowHotkey); HelpMarker("F1 / console hint.");
        ImGui::Checkbox("Draw mouse cursor", &g_DrawMouseCursor); HelpMarker("ImGui software cursor while menu/console is open.");
        ImGui::Checkbox("Block game input while menu is open", &g_BlockGameInputWhenOpen); HelpMarker("Prevents accidental game input while using overlay.");
        if (ImGui::Button("FPS Only", ImVec2(120, 24))) { g_StatusShowName = false; g_StatusShowBuild = false; g_StatusShowFps = true; g_StatusShowHotkey = false; }
        ImGui::SameLine(); if (ImGui::Button("Full Status", ImVec2(120, 24))) { g_StatusShowName = true; g_StatusShowBuild = true; g_StatusShowFps = true; g_StatusShowHotkey = true; }
    }
    else
    {
        ImGui::Text("Graphics > Runtime Info"); ImGui::Separator();
        ImGui::Text("Renderer: D3D11");
        ImGui::Text("HWND: 0x%p", g_GameWindow);
        ImGui::Text("Device: 0x%p", g_Device);
        ImGui::Text("Context: 0x%p", g_Context);
        ImGui::Text("RTV: 0x%p", g_RenderTargetView);
        ImGui::Text("Config: %s", NS2Config::GetPath().c_str());
        if (ImGui::Button("Force Recreate Render Target", ImVec2(250, 24))) { ReleaseRenderTarget(); Logger::Info("DX11 overlay: forced RTV recreate"); }
    }
}

static void DrawControlsPage()
{
    if (g_SelectedSubPage == 0) { ImGui::Text("Controls > General"); ImGui::Separator(); ImGui::Checkbox("Controller enabled", &g_ControlsControllerEnabled); HelpMarker("Overlay controller navigation flag."); ImGui::Checkbox("Keyboard navigation", &g_ControlsKeyboardNav); HelpMarker("ImGui keyboard navigation."); }
    else if (g_SelectedSubPage == 1) { ImGui::Text("Controls > Mouse"); ImGui::Separator(); ImGui::Checkbox("Capture mouse when menu is open", &g_ControlsCaptureMouse); HelpMarker("Keeps mouse focused on overlay."); ImGui::Checkbox("Draw ImGui mouse cursor", &g_DrawMouseCursor); HelpMarker("Software cursor."); ImGui::Checkbox("Block game input while menu is open", &g_BlockGameInputWhenOpen); HelpMarker("Input safety."); }
    else if (g_SelectedSubPage == 2) { ImGui::Text("Controls > Binds"); ImGui::Separator(); ImGui::Text("Menu: F1"); ImGui::Text("Developer console: `"); ImGui::Text("Frame step key:"); StatusBadge("Needs game update-loop hook.", false); }
    else { ImGui::Text("Controls > Advanced"); ImGui::Separator(); ImGui::Checkbox("Block all game input while menu is open", &g_ControlsBlockInput); HelpMarker("Master overlay input safety flag."); }
}

static void DrawGamePage()
{
    if (g_SelectedSubPage == 0)
    {
        ImGui::Text("Game > General"); ImGui::Separator();
        BoolOption("Force offline mode", g_PatchSkipNetworkChecks, "Keeps the framework in local/offline-oriented mode.", true);
        BoolOption("Temporary unlock all", g_PatchUnlockAllTemp, "Shows the option now. Needs save/inventory address work before it can affect the game.", false);
        BoolOption("Persistent unlock all", g_PatchUnlockAllPersistent, "Requires safe save editing before it can be enabled.", false);
        BoolOption("Auto skip cutscenes", g_PatchAutoSkipCutscenes, "Requires cutscene state/address hook.", false);
    }
    else if (g_SelectedSubPage == 1)
    {
        ImGui::Text("Game > Saves"); ImGui::Separator();
        ImGui::TextWrapped("Backup folder: %s", GetSaveBackupFolder().string().c_str());
        if (ImGui::Button("Backup Saves", ImVec2(160, 24))) BackupSaves(); ImGui::SameLine();
        if (ImGui::Button("Restore Saves", ImVec2(160, 24))) RestoreSaves();
        if (ImGui::Button("Open Backup Folder", ImVec2(190, 24))) OpenFolder(GetSaveBackupFolder().string());
        ImGui::Separator();
        auto saves = FindKnownSaveFiles();
        ImGui::Text("Detected save-related files: %d", static_cast<int>(saves.size()));
        for (const auto& s : saves) ImGui::BulletText("%s", s.string().c_str());
    }
    else
    {
        ImGui::Text("Game > Information"); ImGui::Separator();
        ImGui::Text("Process: NSUNS2.exe");
        ImGui::Text("Framework: NartuoStorm2Revived");
        ImGui::Text("Game folder: %s", GetGameFolder().c_str());
        ImGui::Text("Build: %s %s", __DATE__, __TIME__);
        ImGui::Text("Present Count: %llu", static_cast<unsigned long long>(g_PresentCount));
        ImGui::Text("Resize Count: %llu", static_cast<unsigned long long>(g_ResizeCount));
    }
}

static void DrawCameraPage()
{
    if (g_SelectedSubPage == 0)
    {
        ImGui::Text("Camera > Free Camera"); ImGui::Separator();
        BoolOption("Enable free camera", g_FreeCameraEnabled, "Visible framework option. Needs camera matrix/address hook before it controls the game camera.", false);
        BoolOption("Use during cutscenes", g_FreeCameraCutscenes, "Requires cutscene/camera state hook.", false);
        BoolOption("Remove HUD while free cam is active", g_FreeCameraRemoveHud, "Requires HUD visibility address/hook.", false);
        ImGui::SliderFloat("FOV", &g_FreeCameraFov, 20.0f, 120.0f); StatusBadge("Needs camera FOV hook.", false);
        ImGui::SliderFloat("Speed", &g_FreeCameraSpeed, 0.05f, 20.0f); StatusBadge("Needs camera update hook.", false);
        ImGui::SliderFloat("Boost", &g_FreeCameraBoost, 1.0f, 20.0f); StatusBadge("Needs camera update hook.", false);
        ImGui::SliderFloat("Smoothing", &g_FreeCameraSmoothing, 0.0f, 1.0f); StatusBadge("UI only for now.", false);
    }
    else
    {
        ImGui::Text("Camera > Coordinates"); ImGui::Separator();
        ImGui::InputFloat3("Position", g_FreeCameraPos); StatusBadge("Manual storage only until camera hook exists.", false);
        ImGui::InputFloat3("Rotation", g_FreeCameraRot); StatusBadge("Manual storage only until camera hook exists.", false);
        if (ImGui::Button("Copy Camera Position", ImVec2(190, 24)))
        {
            std::ostringstream oss; oss << g_FreeCameraPos[0] << ", " << g_FreeCameraPos[1] << ", " << g_FreeCameraPos[2];
            ImGui::SetClipboardText(oss.str().c_str());
        }
    }
}

static bool AssetPassesCurrentFilter(const AssetBrowser::AssetEntry& asset)
{
    if (g_AssetOverrideOnly && !asset.HasModOverride) return false;
    if (g_AssetDumpedOnly && !asset.IsDumped) return false;
    if (g_AssetSourceFilter > 0 && static_cast<int>(asset.Source) != g_AssetSourceFilter - 1) return false;
    if (g_AssetTypeFilter > 0)
    {
        const char* filters[] = { "All", "Package", "Model", "Texture", "Animation", "Audio", "Stage", "Character", "Data", "CPK Entry", "Unknown" };
        if (_stricmp(AssetBrowser::TypeName(asset.Type), filters[g_AssetTypeFilter]) != 0) return false;
    }
    if (g_SelectedSubPage >= 1 && g_SelectedSubPage <= 6)
    {
        AssetBrowser::AssetType wanted[] = { AssetBrowser::AssetType::Stage, AssetBrowser::AssetType::Character, AssetBrowser::AssetType::Model, AssetBrowser::AssetType::Texture, AssetBrowser::AssetType::Audio, AssetBrowser::AssetType::Package };
        if (asset.Type != wanted[g_SelectedSubPage - 1]) return false;
    }
    std::string search = ToLower(g_AssetSearch);
    if (!search.empty())
    {
        std::string hay = ToLower(asset.RelativePath + " " + asset.VirtualPath + " " + asset.Name);
        if (hay.find(search) == std::string::npos) return false;
    }
    return true;
}

static void SelectAsset(int index)
{
    g_SelectedAssetIndex = index;
    g_HasPreview = false;
    if (g_AutoPreview)
    {
        const auto& assets = AssetBrowser::GetAssets();
        if (index >= 0 && index < static_cast<int>(assets.size()))
        {
            std::string previewPath = !assets[index].DumpFullPath.empty() ? assets[index].DumpFullPath : assets[index].FullPath;
            if (!previewPath.empty())
                g_HasPreview = AssetPreview::LoadPreview(previewPath, g_CurrentPreview);
        }
    }
}

static void DrawAssetsPage()
{
    const auto& assets = AssetBrowser::GetAssets();
    auto stats = AssetBrowser::GetStats();
    ImGui::Text("Assets > %s", g_SelectedSubPage == 0 ? "Browser" : g_SelectedSubPage == 7 ? "Mod Overrides" : g_SelectedSubPage == 8 ? "Tools" : "Category");
    ImGui::Separator();
    if (ImGui::Button("Rescan", ImVec2(90, 24))) { ModRedirector::Scan(); AssetBrowser::Scan(); }
    ImGui::SameLine(); if (ImGui::Button("Dump Selected", ImVec2(125, 24)) && g_SelectedAssetIndex >= 0) AssetBrowser::DumpAssetByIndex(g_SelectedAssetIndex, true);
    ImGui::SameLine(); if (ImGui::Button("Dump Visible Type", ImVec2(140, 24))) { if (g_SelectedSubPage >= 1 && g_SelectedSubPage <= 6) { AssetBrowser::AssetType types[] = { AssetBrowser::AssetType::Stage, AssetBrowser::AssetType::Character, AssetBrowser::AssetType::Model, AssetBrowser::AssetType::Texture, AssetBrowser::AssetType::Audio, AssetBrowser::AssetType::Package }; AssetBrowser::DumpAssetsByType(types[g_SelectedSubPage - 1], true); } }
    ImGui::SameLine(); if (ImGui::Button("Open Dump", ImVec2(105, 24))) OpenFolder(AssetBrowser::GetDumpFolder());
    ImGui::Text("Total %d | Game %d | Mods %d | Dumped %d | Overrides %d | CPK entries %d", stats.TotalAssets, stats.GameAssets, stats.ModAssets, stats.DumpedAssets, stats.OverrideAssets, stats.PackageInnerAssets);
    ImGui::Separator();
    ImGui::InputText("Search", g_AssetSearch, sizeof(g_AssetSearch));
    const char* typeFilters[] = { "All", "Package", "Model", "Texture", "Animation", "Audio", "Stage", "Character", "Data", "CPK Entry", "Unknown" };
    const char* sourceFilters[] = { "All", "Game", "Mod", "Dump", "CPK" };
    ImGui::Combo("Type", &g_AssetTypeFilter, typeFilters, ARRAYSIZE(typeFilters)); ImGui::SameLine(); ImGui::Combo("Source", &g_AssetSourceFilter, sourceFilters, ARRAYSIZE(sourceFilters));
    ImGui::Checkbox("Overrides only", &g_AssetOverrideOnly); ImGui::SameLine(); ImGui::Checkbox("Dumped only", &g_AssetDumpedOnly); ImGui::SameLine(); ImGui::Checkbox("Auto preview", &g_AutoPreview); ImGui::SameLine(); ImGui::Checkbox("Full path labels", &g_ShowAssetFullPath);
    ImGui::Separator();
    ImGui::BeginChild("##AssetList", ImVec2(520.0f, 0.0f), true);
    for (int i = 0; i < static_cast<int>(assets.size()); ++i)
    {
        const auto& a = assets[i];
        if (!AssetPassesCurrentFilter(a)) continue;
        std::string label = "[" + std::string(AssetBrowser::SourceName(a.Source)) + "] [" + AssetBrowser::TypeName(a.Type) + "] " + (g_ShowAssetFullPath ? a.RelativePath : a.Name);
        if (a.HasModOverride) label += "  <override>";
        if (a.IsDumped) label += "  <dumped>";
        if (ImGui::Selectable(label.c_str(), g_SelectedAssetIndex == i)) SelectAsset(i);
    }
    ImGui::EndChild();
    ImGui::SameLine();
    ImGui::BeginChild("##AssetDetails", ImVec2(0.0f, 0.0f), true);
    if (g_SelectedAssetIndex >= 0 && g_SelectedAssetIndex < static_cast<int>(assets.size()))
    {
        const auto& a = assets[g_SelectedAssetIndex];
        ImGui::Text("Asset Details"); ImGui::Separator();
        ImGui::TextWrapped("Name: %s", a.Name.c_str());
        ImGui::Text("Type: %s", AssetBrowser::TypeName(a.Type)); ImGui::SameLine(); ImGui::Text("Source: %s", AssetBrowser::SourceName(a.Source));
        ImGui::Text("Size: %.2f MB", static_cast<double>(a.Size) / 1024.0 / 1024.0);
        ImGui::TextWrapped("Virtual: %s", a.VirtualPath.c_str());
        ImGui::TextWrapped("Path: %s", a.FullPath.c_str());
        if (a.HasModOverride) ImGui::TextWrapped("Override: %s", a.OverrideFullPath.c_str());
        if (!a.DumpFullPath.empty()) ImGui::TextWrapped("Dumped: %s", a.DumpFullPath.c_str());
        if (a.IsVirtualPackageEntry) { ImGui::Text("CPK Offset: 0x%llX", static_cast<unsigned long long>(a.CpkEntryData.Offset)); ImGui::Text("CPK Size: %llu", static_cast<unsigned long long>(a.CpkEntryData.Size)); }
        ImGui::Separator();
        if (ImGui::Button("Dump", ImVec2(90, 24))) AssetBrowser::DumpAssetByIndex(g_SelectedAssetIndex, true);
        ImGui::SameLine(); if (ImGui::Button("Preview", ImVec2(90, 24))) { std::string p = !a.DumpFullPath.empty() ? a.DumpFullPath : a.FullPath; g_HasPreview = AssetPreview::LoadPreview(p, g_CurrentPreview); }
        ImGui::SameLine(); if (ImGui::Button("Dump XFBIN Info", ImVec2(135, 24))) AssetBrowser::DumpXfbinInfo(a);
        ImGui::SameLine(); if (ImGui::Button("Load CPK Entries", ImVec2(135, 24))) AssetBrowser::LoadCpkEntries(a);
        ImGui::Separator();
        if (AssetBrowser::IsModelPreviewable(a))
        {
            AssetPreview::DrawModelPreviewPlaceholder(a.Name.c_str(), ImGui::GetContentRegionAvail().x, 190.0f);
            ImGui::TextColored(ImVec4(1.0f, 0.62f, 0.25f, 1.0f), "3D mesh decode/rendering is a viewer shell until NUD/XFBIN mesh decoding is wired.");
        }
        if (g_HasPreview) AssetPreview::DrawPreview(g_CurrentPreview, ImGui::GetContentRegionAvail().x, 210.0f);
        else ImGui::TextDisabled("No preview loaded. Press Preview or enable Auto preview.");
    }
    else ImGui::TextDisabled("Select an asset from the list.");
    ImGui::EndChild();
}

static void DrawModsPage()
{
    if (g_SelectedSubPage == 0)
    {
        ImGui::Text("Mods > Loader"); ImGui::Separator();
        ImGui::Checkbox("Enable mod loading", &g_ModsEnabled); HelpMarker("Controls framework mod loader state.");
        ImGui::Checkbox("Load DLL mods", &g_ModsLoadDll); HelpMarker("Allows DLL mods through the existing ModLoader.");
        ImGui::Checkbox("Verbose mod logging", &g_ModsVerboseLog); HelpMarker("Extra mod logs.");
        ImGui::Checkbox("Enable loose file redirects", &g_ModRedirectEnabled); HelpMarker("Enables loose file override matching in ModRedirector.");
        ModRedirector::SetEnabled(g_ModRedirectEnabled);
        if (ImGui::Button("Scan Mods", ImVec2(120, 24))) ModRedirector::Scan(); ImGui::SameLine();
        if (ImGui::Button("Dump Redirects", ImVec2(130, 24))) ModRedirector::DumpToLog();
    }
    else
    {
        ImGui::Text("Mods > Folder / Redirects"); ImGui::Separator();
        ImGui::TextWrapped("Mods folder: %s", ModRedirector::GetModsFolder().c_str());
        if (ImGui::Button("Open Mods Folder", ImVec2(170, 24))) OpenFolder(ModRedirector::GetModsFolder());
        ImGui::Separator();
        const auto& redirects = ModRedirector::GetRedirects();
        ImGui::Text("Redirects: %d", static_cast<int>(redirects.size()));
        ImGui::BeginChild("##Redirects", ImVec2(0, 0), true);
        for (const auto& r : redirects) ImGui::TextWrapped("%s -> %s", r.VirtualPath.c_str(), r.ReplacementPath.c_str());
        ImGui::EndChild();
    }
}

static void DrawPatchesPage()
{
    if (g_SelectedSubPage == 0)
    {
        ImGui::Text("Patches > Display"); ImGui::Separator();
        BoolOption("Fullscreen Borderless", g_GraphicsWindowedFullscreen, "Working helper through WindowedFullscreen/overlay-side window management.", true);
        BoolOption("Native Ultrawide Support", g_PatchNativeUltrawide, "Requires aspect/FOV/render matrix patches for real game-side ultrawide.", false);
        BoolOption("Multi-monitor Support", g_PatchMultiMonitor, "Requires monitor/window placement logic and game resolution handling.", false);
        BoolOption("High Refresh Fix", g_GraphicsBorderlessFix, "Config/window helper is present; full game timing patch requires reverse engineering.", false);
    }
    else if (g_SelectedSubPage == 1)
    {
        ImGui::Text("Patches > Startup / Cutscenes"); ImGui::Separator();
        BoolOption("Disable intro", g_PatchDisableIntro, "Requires file redirect or startup movie/state patch.", false);
        BoolOption("Auto skip cutscenes", g_PatchAutoSkipCutscenes, "Requires cutscene state hook.", false);
        BoolOption("Skip online/network checks", g_PatchSkipNetworkChecks, "Network block/offline layer already exists, full menu skip may need UI state patch.", true);
    }
    else if (g_SelectedSubPage == 2)
    {
        ImGui::Text("Patches > Unlocks"); ImGui::Separator();
        BoolOption("Temporary unlock all for testing", g_PatchUnlockAllTemp, "Needs inventory/progression memory map. Should not write save.", false);
        BoolOption("Permanent unlock all", g_PatchUnlockAllPersistent, "Needs safe save editor / backup before enabling.", false);
    }
    else if (g_SelectedSubPage == 3)
    {
        ImGui::Text("Patches > Gameplay Debug"); ImGui::Separator();
        BoolOption("Slow motion toggle", g_PatchSlowMotion, "Requires game delta-time/update-rate patch.", false);
        ImGui::SliderFloat("Game speed", &g_GameSpeed, 0.05f, 2.0f); StatusBadge("Needs game speed hook.", false);
        BoolOption("Frame step mode", g_PatchFrameStep, "Requires update-loop pause/advance hook.", false);
        BoolOption("Remove HUD", g_PatchRemoveHud, "Requires HUD visibility hook.", false);
    }
    else
    {
        ImGui::Text("Patches > Notes"); ImGui::Separator();
        ImGui::TextWrapped("Active options are wired to current config/window/network/mod systems. Planned options are exposed in the UI so the menu is ready, but they require address scanning or engine hooks before they can safely modify gameplay.");
    }
}

static void DrawDebugPage()
{
    if (g_SelectedSubPage == 0)
    {
        ImGui::Text("Debug > Performance"); ImGui::Separator();
        ImGui::PlotLines("FPS", g_FpsHistory, ARRAYSIZE(g_FpsHistory), g_FpsHistoryOffset, nullptr, 0, 144, ImVec2(460, 100));
        ImGui::PlotLines("Frame Time", g_FrameTimeHistory, ARRAYSIZE(g_FrameTimeHistory), g_FpsHistoryOffset, nullptr, 0, 40, ImVec2(460, 100));
        ImGui::Text("FPS: %d", g_CurrentFps);
        ImGui::Text("Frame time: %.3f ms", g_CurrentFps > 0 ? 1000.0f / static_cast<float>(g_CurrentFps) : 0.0f);
        ImGui::Text("Present: %llu", static_cast<unsigned long long>(g_PresentCount));
        ImGui::Text("ResizeBuffers: %llu", static_cast<unsigned long long>(g_ResizeCount));
    }
    else if (g_SelectedSubPage == 1)
    {
        ImGui::Text("Debug > Render / Game Info"); ImGui::Separator();
        ImGui::Text("Game folder: %s", GetGameFolder().c_str());
        ImGui::Text("HWND: 0x%p", g_GameWindow); ImGui::Text("Device: 0x%p", g_Device); ImGui::Text("Context: 0x%p", g_Context); ImGui::Text("RTV: 0x%p", g_RenderTargetView);
        if (ImGui::Button("Force recreate render target", ImVec2(220, 24))) ReleaseRenderTarget();
    }
    else if (g_SelectedSubPage == 2)
    {
        ImGui::Text("Debug > Viewers"); ImGui::Separator();
        BoolOption("Hitbox Viewer", g_PatchHitboxViewer, "Needs hitbox memory structures or debug draw data.", false);
        BoolOption("Hurtbox Viewer", g_PatchHurtboxViewer, "Needs hurtbox memory structures or debug draw data.", false);
        BoolOption("Collision Viewer", g_PatchCollisionViewer, "Needs collision mesh/scene data.", false);
        BoolOption("Spawn Point Viewer", g_PatchSpawnPointViewer, "Needs stage spawn point data.", false);
        BoolOption("Event Viewer", g_PatchEventViewer, "Needs event/state data.", false);
        ImGui::TextWrapped("These are exposed now as a proper Debug > Viewers page. They are marked planned until the game data sources are found.");
    }
    else if (g_SelectedSubPage == 3)
    {
        ImGui::Text("Debug > Steam"); ImGui::Separator();
        ImGui::Checkbox("Log Steam interface calls", &g_DebugLogSteamInterfaces);
        ImGui::Checkbox("Log callbacks", &g_DebugLogCallbacks);
        ImGui::Checkbox("Log call results", &g_DebugLogCallResults);
        ImGui::Text("Steam proxy: loaded"); ImGui::Text("Remote storage: local"); ImGui::Text("Stats: emulated"); ImGui::Text("Matchmaking: emulated");
    }
    else if (g_SelectedSubPage == 4)
    {
        ImGui::Text("Debug > Network"); ImGui::Separator();
        ImGui::Checkbox("Log DNS requests", &g_DebugLogDns); ImGui::Checkbox("Log socket connects", &g_DebugLogSocketConnects);
        ImGui::Text("connect hook: enabled"); ImGui::Text("WSAConnect hook: enabled"); ImGui::Text("getaddrinfo hook: enabled"); ImGui::Text("GetAddrInfoW hook: enabled");
    }
    else if (g_SelectedSubPage == 5)
    {
        ImGui::Text("Debug > Storage / Config"); ImGui::Separator();
        ImGui::TextWrapped("Config: %s", NS2Config::GetPath().c_str());
        ImGui::TextWrapped("Dump: %s", AssetBrowser::GetDumpFolder().c_str());
        if (ImGui::Button("Save Config", ImVec2(120, 24))) NS2Config::Save(); ImGui::SameLine();
        if (ImGui::Button("Reload Config", ImVec2(120, 24))) NS2Config::Load(); ImGui::SameLine();
        if (ImGui::Button("Open Dump", ImVec2(120, 24))) OpenFolder(AssetBrowser::GetDumpFolder());
    }
    else
    {
        ImGui::Text("Debug > Tools"); ImGui::Separator();
        if (ImGui::Button("Write test log", ImVec2(180, 24))) Logger::Info("DX11 overlay: test log");
        if (ImGui::Button("Dump overlay state", ImVec2(180, 24))) Logger::Info("DX11 overlay: dump requested");
        if (ImGui::Button("Emergency disable overlay", ImVec2(220, 24))) g_OverlayEnabled = false;
    }
}

static void DrawLeftPanelForTab()
{
    if (g_SelectedTab == 0) { DrawLeftButton("Display", 0); DrawLeftButton("Anti-Aliasing", 1); DrawLeftButton("Quality", 2); DrawLeftButton("Overlay", 3); DrawLeftButton("Runtime Info", 4); }
    else if (g_SelectedTab == 1) { DrawLeftButton("General", 0); DrawLeftButton("Mouse", 1); DrawLeftButton("Binds", 2); DrawLeftButton("Advanced", 3); }
    else if (g_SelectedTab == 2) { DrawLeftButton("General", 0); DrawLeftButton("Saves", 1); DrawLeftButton("Information", 2); }
    else if (g_SelectedTab == 3) { DrawLeftButton("Free Camera", 0); DrawLeftButton("Coordinates", 1); }
    else if (g_SelectedTab == 4) { DrawLeftButton("Browser", 0); DrawLeftButton("Stages", 1); DrawLeftButton("Characters", 2); DrawLeftButton("Models", 3); DrawLeftButton("Textures", 4); DrawLeftButton("Audio", 5); DrawLeftButton("Packages", 6); DrawLeftButton("Mod Overrides", 7); DrawLeftButton("Tools", 8); }
    else if (g_SelectedTab == 5) { DrawLeftButton("Loader", 0); DrawLeftButton("Folder", 1); }
    else if (g_SelectedTab == 6) { DrawLeftButton("Display", 0); DrawLeftButton("Startup", 1); DrawLeftButton("Unlocks", 2); DrawLeftButton("Gameplay", 3); DrawLeftButton("Notes", 4); }
    else if (g_SelectedTab == 7) { DrawLeftButton("Performance", 0); DrawLeftButton("Render Info", 1); DrawLeftButton("Viewers", 2); DrawLeftButton("Steam", 3); DrawLeftButton("Network", 4); DrawLeftButton("Storage", 5); DrawLeftButton("Tools", 6); }
}

static void DrawRightPanelForTab()
{
    switch (g_SelectedTab)
    {
    case 0: DrawGraphicsPage(); break;
    case 1: DrawControlsPage(); break;
    case 2: DrawGamePage(); break;
    case 3: DrawCameraPage(); break;
    case 4: DrawAssetsPage(); break;
    case 5: DrawModsPage(); break;
    case 6: DrawPatchesPage(); break;
    case 7: DrawDebugPage(); break;
    }
}

static void DrawDropdownPanel()
{
    if (!g_MenuOpen || g_SelectedTab < 0) return;
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    float x = 95.0f + static_cast<float>(g_SelectedTab) * 88.0f;
    float y = 31.0f * g_MenuAnimation;
    float panelWidth = ClampValue(viewport->Size.x - x - 12.0f, 760.0f, 1180.0f);
    float panelHeight = ClampValue(viewport->Size.y - y - 42.0f, 390.0f, 620.0f);
    ImGui::SetNextWindowPos(ImVec2(x, y), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(panelWidth, panelHeight), ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(g_OverlayAlpha);
    ImGui::Begin("##NS2ShipwrightDropdown", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse);
    ImGui::BeginChild("##NS2LeftList", ImVec2(225.0f, 0.0f), true);
    ImGui::Text("Options"); ImGui::Separator();
    DrawLeftPanelForTab();
    ImGui::EndChild();
    ImGui::SameLine();
    ImGui::BeginChild("##NS2RightContent", ImVec2(0.0f, 0.0f), true);
    DrawRightPanelForTab();
    ImGui::EndChild();
    ImGui::End();
}

static void DrawAnimatedTopMenu()
{
    float target = g_MenuOpen ? 1.0f : 0.0f;
    float speed = ImGui::GetIO().DeltaTime * g_MenuAnimSpeed;
    g_MenuAnimation = AnimateToward(g_MenuAnimation, target, speed);
    if (g_MenuAnimation < 0.01f && !g_MenuOpen) { g_MenuAnimation = 0.0f; g_SelectedTab = -1; return; }
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    float barHeight = 30.0f;
    float y = -barHeight + (barHeight * g_MenuAnimation);
    ImGui::SetNextWindowPos(ImVec2(0.0f, y), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, barHeight), ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.92f);
    ImGui::Begin("##NS2AnimatedTopMenuBar", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoCollapse);
    ImGui::SetCursorPos(ImVec2(8.0f, 5.0f));
    ImGui::Text("NS2 Revived");
    ImGui::SameLine(95.0f);
    for (int i = 0; i < ARRAYSIZE(kTabNames); ++i) { DrawTabButton(kTabNames[i], i); if (i + 1 < ARRAYSIZE(kTabNames)) ImGui::SameLine(); }
    ImGui::SameLine(viewport->Size.x - 260.0f);
    ImGui::Text("FPS %d", g_CurrentFps);
    ImGui::End();
    if (g_SelectedTab >= 0) DrawDropdownPanel();
}

static void DrawImGui(IDXGISwapChain* swapChain)
{
    if (!g_ImGuiReady || !g_Context) return;
    if (!CreateRenderTarget(swapChain)) return;
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGuiIO& io = ImGui::GetIO();
    io.MouseDrawCursor = (g_MenuOpen || g_ConsoleOpen) && g_DrawMouseCursor;
    ImGui::NewFrame();
    DrawAlwaysStatusBar();
    DrawAnimatedTopMenu();
    DrawDeveloperConsole();
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
    ReleaseD3D();
    if (g_GameWindow && g_OriginalWndProc)
    {
        SetWindowLongPtrA(g_GameWindow, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(g_OriginalWndProc));
        g_OriginalWndProc = nullptr;
    }
    g_GameWindow = nullptr;
}

static HRESULT __stdcall PresentHook(IDXGISwapChain* swapChain, UINT syncInterval, UINT flags)
{
    ++g_PresentCount;
    if (!g_PresentSeen) { g_PresentSeen = true; Logger::Info("DX11 overlay: Present hook hit"); }
    UpdateFps();
    UpdateHotkeys();
    if (InitImGui(swapChain)) DrawImGui(swapChain);
    return g_OriginalPresent(swapChain, syncInterval, flags);
}

static HRESULT __stdcall ResizeBuffersHook(IDXGISwapChain* swapChain, UINT bufferCount, UINT width, UINT height, DXGI_FORMAT newFormat, UINT swapChainFlags)
{
    ++g_ResizeCount;
    Logger::Info("DX11 overlay: ResizeBuffers hook hit");
    ReleaseRenderTarget();
    return g_OriginalResizeBuffers(swapChain, bufferCount, width, height, newFormat, swapChainFlags);
}

static HWND CreateDummyWindow()
{
    WNDCLASSEXA wc{};
    wc.cbSize = sizeof(WNDCLASSEXA);
    wc.lpfnWndProc = DefWindowProcA;
    wc.hInstance = GetModuleHandleA(nullptr);
    wc.lpszClassName = "NS2RevivedDummyDX11Window";
    RegisterClassExA(&wc);
    return CreateWindowExA(0, wc.lpszClassName, "NS2RevivedDummyDX11Window", WS_OVERLAPPEDWINDOW, 0, 0, 100, 100, nullptr, nullptr, wc.hInstance, nullptr);
}

namespace DX11Overlay
{
    bool Init()
    {
        if (g_HookReady) return true;
        Logger::Info("DX11 overlay: initializing ImGui DX11 hook");
        HWND dummyWindow = CreateDummyWindow();
        if (!dummyWindow) { Logger::Error("DX11 overlay: failed to create dummy window"); return false; }
        DXGI_SWAP_CHAIN_DESC desc{};
        desc.BufferCount = 1;
        desc.BufferDesc.Width = 100;
        desc.BufferDesc.Height = 100;
        desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        desc.OutputWindow = dummyWindow;
        desc.SampleDesc.Count = 1;
        desc.Windowed = TRUE;
        desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
        IDXGISwapChain* dummySwapChain = nullptr;
        ID3D11Device* dummyDevice = nullptr;
        ID3D11DeviceContext* dummyContext = nullptr;
        D3D_FEATURE_LEVEL featureLevel{};
        D3D_FEATURE_LEVEL featureLevels[] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_1, D3D_FEATURE_LEVEL_10_0 };
        HRESULT hr = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, featureLevels, ARRAYSIZE(featureLevels), D3D11_SDK_VERSION, &desc, &dummySwapChain, &dummyDevice, &featureLevel, &dummyContext);
        if (FAILED(hr) || !dummySwapChain) { Logger::Error("DX11 overlay: dummy swapchain creation failed"); DestroyWindow(dummyWindow); return false; }
        void** vtable = *reinterpret_cast<void***>(dummySwapChain);
        void* presentAddress = vtable[8];
        void* resizeBuffersAddress = vtable[13];
        MH_STATUS presentStatus = MH_CreateHook(presentAddress, &PresentHook, reinterpret_cast<void**>(&g_OriginalPresent));
        if (presentStatus != MH_OK && presentStatus != MH_ERROR_ALREADY_CREATED) { Logger::Error("DX11 overlay: MH_CreateHook Present failed"); dummySwapChain->Release(); dummyDevice->Release(); dummyContext->Release(); DestroyWindow(dummyWindow); return false; }
        MH_STATUS resizeStatus = MH_CreateHook(resizeBuffersAddress, &ResizeBuffersHook, reinterpret_cast<void**>(&g_OriginalResizeBuffers));
        if (resizeStatus != MH_OK && resizeStatus != MH_ERROR_ALREADY_CREATED) { Logger::Error("DX11 overlay: MH_CreateHook ResizeBuffers failed"); dummySwapChain->Release(); dummyDevice->Release(); dummyContext->Release(); DestroyWindow(dummyWindow); return false; }
        MH_EnableHook(presentAddress);
        MH_EnableHook(resizeBuffersAddress);
        Logger::Info("DX11 overlay: Present and ResizeBuffers hooks enabled for ImGui drawing");
        dummySwapChain->Release(); dummyDevice->Release(); dummyContext->Release(); DestroyWindow(dummyWindow);
        g_HookReady = true;
        return true;
    }

    void Shutdown()
    {
        ShutdownImGui();
        Logger::Info("DX11 overlay: shutdown");
    }
}
