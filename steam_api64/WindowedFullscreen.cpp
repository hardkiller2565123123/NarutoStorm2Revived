#include "StdInc.h"
#include "WindowedFullscreen.h"
#include "Logger.h"

static HWND g_GameWindow = nullptr;
static WNDPROC g_OriginalWndProc = nullptr;
static bool g_Applied = false;

static LRESULT CALLBACK GameWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_ACTIVATEAPP:
        // Keep game thinking it is active when alt-tabbed.
        return CallWindowProcA(g_OriginalWndProc, hwnd, msg, TRUE, lParam);

    case WM_ACTIVATE:
        // Prevent inactive/minimized focus behavior.
        return CallWindowProcA(g_OriginalWndProc, hwnd, msg, WA_ACTIVE, lParam);

    case WM_KILLFOCUS:
        // Swallow focus loss so the game is less likely to pause/freeze.
        return 0;

    case WM_SYSCOMMAND:
        if ((wParam & 0xFFF0) == SC_MINIMIZE)
            return 0;
        break;
    }

    return CallWindowProcA(g_OriginalWndProc, hwnd, msg, wParam, lParam);
}

static BOOL CALLBACK EnumWindowsCallback(HWND hwnd, LPARAM)
{
    DWORD windowProcessId = 0;
    GetWindowThreadProcessId(hwnd, &windowProcessId);

    if (windowProcessId != GetCurrentProcessId())
        return TRUE;

    if (!IsWindowVisible(hwnd))
        return TRUE;

    char title[256]{};
    GetWindowTextA(hwnd, title, sizeof(title));

    if (strlen(title) <= 0)
        return TRUE;

    if (strstr(title, "NartuoStorm2Revived Debug Console"))
        return TRUE;

    if (strstr(title, "NARUTO") ||
        strstr(title, "Naruto") ||
        strstr(title, "NSUNS") ||
        strstr(title, "Storm"))
    {
        g_GameWindow = hwnd;
        return FALSE;
    }

    g_GameWindow = hwnd;
    return FALSE;
}

static HWND FindGameWindow()
{
    g_GameWindow = nullptr;
    EnumWindows(EnumWindowsCallback, 0);
    return g_GameWindow;
}

static void HookGameWindowProc(HWND hwnd)
{
    if (!hwnd || g_OriginalWndProc)
        return;

    g_OriginalWndProc = reinterpret_cast<WNDPROC>(
        SetWindowLongPtrA(hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(GameWindowProc)));

    if (g_OriginalWndProc)
        Logger::Info("Game window procedure hooked for background keep-alive");
    else
        Logger::Error("Failed to hook game window procedure");
}

static void ApplyBorderlessFullscreen(HWND hwnd)
{
    if (!hwnd || g_Applied)
        return;

    HMONITOR monitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);

    MONITORINFO monitorInfo{};
    monitorInfo.cbSize = sizeof(MONITORINFO);

    if (!GetMonitorInfoA(monitor, &monitorInfo))
        return;

    LONG_PTR style = GetWindowLongPtrA(hwnd, GWL_STYLE);
    LONG_PTR exStyle = GetWindowLongPtrA(hwnd, GWL_EXSTYLE);

    style &= ~WS_CAPTION;
    style &= ~WS_THICKFRAME;
    style &= ~WS_MINIMIZEBOX;
    style &= ~WS_MAXIMIZEBOX;
    style &= ~WS_SYSMENU;

    exStyle &= ~WS_EX_DLGMODALFRAME;
    exStyle &= ~WS_EX_CLIENTEDGE;
    exStyle &= ~WS_EX_STATICEDGE;
    exStyle &= ~WS_EX_WINDOWEDGE;

    SetWindowLongPtrA(hwnd, GWL_STYLE, style);
    SetWindowLongPtrA(hwnd, GWL_EXSTYLE, exStyle);

    const RECT& rect = monitorInfo.rcMonitor;

    SetWindowPos(
        hwnd,
        HWND_TOP,
        rect.left,
        rect.top,
        rect.right - rect.left,
        rect.bottom - rect.top,
        SWP_NOOWNERZORDER |
        SWP_FRAMECHANGED |
        SWP_SHOWWINDOW);

    HookGameWindowProc(hwnd);

    g_Applied = true;

    Logger::Info("Windowed fullscreen applied to game window");
    Logger::Info("Background keep-alive enabled for game window");
}

static DWORD WINAPI WindowedFullscreenThread(LPVOID)
{
    Logger::Info("Windowed fullscreen thread started");

    for (int i = 0; i < 120; i++)
    {
        HWND hwnd = FindGameWindow();

        if (hwnd)
        {
            ApplyBorderlessFullscreen(hwnd);
            return 0;
        }

        Sleep(500);
    }

    Logger::Error("Failed to find game window for windowed fullscreen");
    return 0;
}

namespace WindowedFullscreen
{
    void Init()
    {
        CreateThread(
            nullptr,
            0,
            WindowedFullscreenThread,
            nullptr,
            0,
            nullptr);
    }
}