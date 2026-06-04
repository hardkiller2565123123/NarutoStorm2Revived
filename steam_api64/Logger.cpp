#include "StdInc.h"
#include "Logger.h"

static std::ofstream g_Log;
static std::mutex g_LogMutex;

static bool g_ConsoleReady = false;
static bool g_ConsoleAllocatedByUs = false;
static bool g_LoggerInitialized = false;

static std::string ToLowerString(std::string text)
{
    for (char& c : text)
        c = static_cast<char>(tolower(static_cast<unsigned char>(c)));

    return text;
}

static std::string GetProcessPath()
{
    char exePath[MAX_PATH]{};
    GetModuleFileNameA(nullptr, exePath, MAX_PATH);
    return exePath;
}

static std::string GetProcessName()
{
    std::string path = GetProcessPath();

    size_t slash = path.find_last_of("\\/");

    if (slash != std::string::npos)
        path = path.substr(slash + 1);

    return ToLowerString(path);
}

static std::string GetGameDirectory()
{
    std::string path = GetProcessPath();

    size_t slash = path.find_last_of("\\/");

    if (slash != std::string::npos)
        return path.substr(0, slash + 1);

    return "";
}

static bool IsRealGameProcess()
{
    std::string exe = GetProcessName();

    return exe == "nsuns2.exe";
}

static void InitConsole()
{
    if (g_ConsoleReady)
        return;

    HWND existingConsole = GetConsoleWindow();

    if (!existingConsole)
    {
        if (AllocConsole())
            g_ConsoleAllocatedByUs = true;
    }

    FILE* dummy = nullptr;

    freopen_s(&dummy, "CONOUT$", "w", stdout);
    freopen_s(&dummy, "CONOUT$", "w", stderr);
    freopen_s(&dummy, "CONIN$", "r", stdin);

    SetConsoleTitleA("NartuoStorm2Revived Debug Console");

    g_ConsoleReady = true;
}

bool Logger::Init()
{
    if (g_LoggerInitialized)
        return true;

    g_LoggerInitialized = true;

    std::string gameDir = GetGameDirectory();
    std::string logsDir = gameDir + "logs";

    CreateDirectoryA(logsDir.c_str(), nullptr);

    std::string logPath = logsDir + "\\NartuoStorm2Revived.log";

    DeleteFileA(logPath.c_str());

    g_Log.open(logPath, std::ios::out | std::ios::trunc);

    if (!g_Log.is_open())
        return false;

    if (IsRealGameProcess())
        InitConsole();

    Info("Logger initialized");
    Info("Fresh log created");
    Info("Process: " + GetProcessPath());
    Info("Process name: " + GetProcessName());
    Info("Log path: " + logPath);

    return true;
}

void Logger::Shutdown()
{
    Info("Logger shutdown");

    {
        std::lock_guard<std::mutex> lock(g_LogMutex);

        if (g_Log.is_open())
            g_Log.close();
    }

    if (g_ConsoleReady && g_ConsoleAllocatedByUs)
        FreeConsole();

    g_ConsoleReady = false;
    g_ConsoleAllocatedByUs = false;
    g_LoggerInitialized = false;
}

void Logger::Info(const std::string& text)
{
    std::lock_guard<std::mutex> lock(g_LogMutex);

    std::string line = "[INFO] " + text;

    if (g_Log.is_open())
    {
        g_Log << line << std::endl;
        g_Log.flush();
    }

    if (g_ConsoleReady)
        std::cout << line << std::endl;
}

void Logger::Error(const std::string& text)
{
    std::lock_guard<std::mutex> lock(g_LogMutex);

    std::string line = "[ERROR] " + text;

    if (g_Log.is_open())
    {
        g_Log << line << std::endl;
        g_Log.flush();
    }

    if (g_ConsoleReady)
        std::cout << line << std::endl;
}

void Logger::ExportInitialized(const char* exportName)
{
    Info(std::string(exportName) + ": initialized");
}