#include "StdInc.h"
#include "OverlayConsole.h"

#include "Logger.h"
#include "AssetBrowser.h"
#include "AssetPreloadManager.h"
#include "LuaManager.h"

namespace
{
    std::vector<std::string> g_Lines;

    void Push(const std::string& line)
    {
        g_Lines.push_back(line);

        if (g_Lines.size() > 400)
            g_Lines.erase(g_Lines.begin(), g_Lines.begin() + 100);
    }

    std::string Lower(std::string s)
    {
        std::transform(s.begin(), s.end(), s.begin(),
            [](unsigned char c)
            {
                return static_cast<char>(std::tolower(c));
            });

        return s;
    }
}

namespace OverlayConsole
{
    bool Init()
    {
        Push("NS2 Revived console ready. Type help.");
        Logger::Info("OverlayConsole initialized");
        return true;
    }

    void Shutdown()
    {
        g_Lines.clear();
        Logger::Info("OverlayConsole shutdown");
    }

    void AddLine(const std::string& line)
    {
        Push(line);
    }

    void Clear()
    {
        g_Lines.clear();
    }

    const std::vector<std::string>& GetLines()
    {
        return g_Lines;
    }

    CommandResult Execute(const std::string& commandLine)
    {
        CommandResult result{};
        std::string cmd = Lower(commandLine);

        Push("> " + commandLine);

        if (cmd == "help")
        {
            result.Handled = true;
            result.Message =
                "Commands: help, fps, asset_rescan, asset_preload, asset_cache_save, "
                "asset_cache_load, lua_rescan, lua_dump, lua_clear, mods_reload, clear";
        }
        else if (cmd == "fps")
        {
            result.Handled = true;
            result.Message = "FPS command received.";
        }
        else if (cmd == "asset_rescan")
        {
            AssetBrowser::StartAsyncScan(true);
            result.Handled = true;
            result.Message = "Asset rescan started.";
        }
        else if (cmd == "asset_preload")
        {
            AssetPreloadManager::StartFullPreload(false);
            result.Handled = true;
            result.Message = "Asset preload started.";
        }
        else if (cmd == "asset_cache_save")
        {
            AssetBrowser::SaveCache();
            result.Handled = true;
            result.Message = "Asset cache saved.";
        }
        else if (cmd == "asset_cache_load")
        {
            AssetBrowser::LoadCache();
            result.Handled = true;
            result.Message = "Asset cache loaded.";
        }
        else if (cmd == "lua_rescan")
        {
            LuaManager::Rescan();
            result.Handled = true;
            result.Message = "Lua rescan complete.";
        }
        else if (cmd == "lua_dump")
        {
            LuaManager::DumpKnownGamePaths();
            result.Handled = true;
            result.Message = "Known Lua paths dumped.";
        }
        else if (cmd == "lua_clear")
        {
            LuaManager::ClearConsoleLog();
            result.Handled = true;
            result.Message = "Lua console log cleared.";
        }
        else if (cmd == "mods_reload")
        {
            result.Handled = true;
            result.Message = "Mod reload requested.";
            Logger::Info("OverlayConsole: mods_reload requested");
        }
        else if (cmd == "clear")
        {
            Clear();
            result.Handled = true;
            result.Message = "";
        }
        else
        {
            result.Handled = false;
            result.Message = "Unknown command. Type help.";
        }

        if (!result.Message.empty())
            Push(result.Message);

        return result;
    }
}