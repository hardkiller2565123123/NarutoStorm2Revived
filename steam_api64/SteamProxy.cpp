#include "StdInc.h"
#include "SteamProxy.h"
#include "Logger.h"

static HMODULE g_RealSteamModule = nullptr;
static std::string g_RealSteamPath;

static std::string GetProxyDirectory()
{
    HMODULE module = nullptr;

    GetModuleHandleExA(
        GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
        GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        reinterpret_cast<LPCSTR>(&GetProxyDirectory),
        &module);

    char path[MAX_PATH]{};
    GetModuleFileNameA(module, path, MAX_PATH);

    std::string result = path;
    size_t slash = result.find_last_of("\\/");

    if (slash != std::string::npos)
        return result.substr(0, slash + 1);

    return "";
}

static bool FileExists(const std::string& path)
{
    DWORD attr = GetFileAttributesA(path.c_str());

    return attr != INVALID_FILE_ATTRIBUTES &&
        !(attr & FILE_ATTRIBUTE_DIRECTORY);
}


//Common Drives For it to search for OnlineSteam.dll
static std::vector<std::string> BuildSearchPaths()
{
    std::string proxyDir = GetProxyDirectory();

    std::vector<std::string> paths;

    paths.push_back(proxyDir + "OnlineSteam.dll");
    paths.push_back(proxyDir + "NartuoStorm2Revived\\OnlineSteam.dll");

    paths.push_back("D:\\SteamLibrary\\steamapps\\common\\Qwant2\\OnlineSteam.dll");
    paths.push_back("D:\\SteamLibrary\\steamapps\\common\\Qwant2\\NartuoStorm2Revived\\OnlineSteam.dll");

    paths.push_back("C:\\Program Files (x86)\\Steam\\steamapps\\common\\Qwant2\\OnlineSteam.dll");
    paths.push_back("C:\\Program Files (x86)\\Steam\\steamapps\\common\\Qwant2\\NartuoStorm2Revived\\OnlineSteam.dll");

    paths.push_back("C:\\SteamLibrary\\steamapps\\common\\Qwant2\\OnlineSteam.dll");
    paths.push_back("C:\\SteamLibrary\\steamapps\\common\\Qwant2\\NartuoStorm2Revived\\OnlineSteam.dll");

    paths.push_back("E:\\SteamLibrary\\steamapps\\common\\Qwant2\\OnlineSteam.dll");
    paths.push_back("E:\\SteamLibrary\\steamapps\\common\\Qwant2\\NartuoStorm2Revived\\OnlineSteam.dll");

    paths.push_back("F:\\SteamLibrary\\steamapps\\common\\Qwant2\\OnlineSteam.dll");
    paths.push_back("F:\\SteamLibrary\\steamapps\\common\\Qwant2\\NartuoStorm2Revived\\OnlineSteam.dll");

    return paths;
}

namespace SteamProxy
{
    bool Init()
    {
        if (g_RealSteamModule)
            return true;

        std::vector<std::string> paths = BuildSearchPaths();

        for (const std::string& path : paths)
        {
            Logger::Info("Checking Steam DLL path: " + path);

            if (!FileExists(path))
                continue;

            g_RealSteamModule = LoadLibraryA(path.c_str());

            if (g_RealSteamModule)
            {
                g_RealSteamPath = path;
                Logger::Info("Loaded real Steam DLL: " + path);
                return true;
            }

            Logger::Error("LoadLibraryA failed: " + path);
        }

        Logger::Error("Could not find OnlineSteam.dll");

        MessageBoxA(
            nullptr,
            "Could not find OnlineSteam.dll.\n\n"
            "Put OnlineSteam.dll in the same folder as steam_api64.dll,\n"
            "or inside NartuoStorm2Revived.",
            "NartuoStorm2Revived Steam Proxy",
            MB_ICONERROR | MB_OK);

        return false;
    }

    FARPROC GetExport(const char* name)
    {
        if (!Init())
            return nullptr;

        FARPROC proc = GetProcAddress(g_RealSteamModule, name);

        if (!proc)
        {
            Logger::Error(
                std::string("Missing export in OnlineSteam.dll: ") +
                name);
        }

        return proc;
    }

    HMODULE GetRealModule()
    {
        return g_RealSteamModule;
    }

    const std::string& GetRealSteamPath()
    {
        return g_RealSteamPath;
    }
}