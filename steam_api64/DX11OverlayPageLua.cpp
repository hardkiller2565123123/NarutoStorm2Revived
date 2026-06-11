#include "DX11OverlayInternal.h"

void DrawLuaScriptList(LuaManager::ScriptSource sourceFilter)
{
    const auto& scripts = LuaManager::GetScripts();

    std::string search = g_LuaSearch;
    std::transform(search.begin(), search.end(), search.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

    for (int i = 0; i < static_cast<int>(scripts.size()); ++i)
    {
        const auto& script = scripts[i];

        if (sourceFilter != LuaManager::ScriptSource::Unknown && script.Source != sourceFilter)
            continue;

        if (!search.empty())
        {
            std::string hay = script.Name + " " + script.RelativePath + " " + script.VirtualGamePath;
            std::transform(hay.begin(), hay.end(), hay.begin(),
                [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

            if (hay.find(search) == std::string::npos)
                continue;
        }

        std::string label =
            "[" + std::string(LuaManager::SourceName(script.Source)) + "] " +
            (!script.VirtualGamePath.empty() ? script.VirtualGamePath : script.RelativePath);

        if (ImGui::Selectable(label.c_str(), g_SelectedLuaScript == i))
        {
            g_SelectedLuaScript = i;
            LuaManager::ReadScriptText(script, g_LuaScriptText);
        }
    }
}

void DrawLuaPage()
{
    int page = g_SelectedSubPage;

    ImGui::InputText("Search", g_LuaSearch, sizeof(g_LuaSearch));

    if (ImGui::Button("Rescan Lua", ImVec2(130, 24)))
        LuaManager::Rescan();

    ImGui::SameLine();

    if (ImGui::Button("Dump Known Paths", ImVec2(160, 24)))
        LuaManager::DumpKnownGamePaths();

    ImGui::Separator();

    if (page == 8 || page == 0)
    {
        ImGui::Text("Assets > Lua Scripts");
        ImGui::Separator();

        ImGui::BeginChild("##LuaList", ImVec2(420, 0), true);
        DrawLuaScriptList(LuaManager::ScriptSource::Unknown);
        ImGui::EndChild();

        ImGui::SameLine();

        ImGui::BeginChild("##LuaPreview", ImVec2(0, 0), true);

        if (g_SelectedLuaScript >= 0 && g_SelectedLuaScript < static_cast<int>(LuaManager::GetScripts().size()))
        {
            const auto& script = LuaManager::GetScripts()[g_SelectedLuaScript];

            ImGui::Text("Name: %s", script.Name.c_str());
            ImGui::Text("Source: %s", LuaManager::SourceName(script.Source));
            ImGui::TextWrapped("Virtual: %s", script.VirtualGamePath.c_str());
            ImGui::TextWrapped("Path: %s", script.FullPath.empty() ? "(inside game archive / known path only)" : script.FullPath.c_str());
            ImGui::Text("Size: %llu bytes", static_cast<unsigned long long>(script.Size));

            ImGui::Separator();

            if (ImGui::Button("Create Override Copy", ImVec2(190, 24)))
            {
                std::string virtualPath = !script.VirtualGamePath.empty() ? script.VirtualGamePath : script.RelativePath;
                LuaManager::WriteOverrideScript(virtualPath, g_LuaScriptText);
            }

            ImGui::SameLine();

            if (ImGui::Button("Reload Text", ImVec2(120, 24)))
                LuaManager::ReadScriptText(script, g_LuaScriptText);

            ImGui::Separator();
            ImGui::TextWrapped("%s", g_LuaScriptText.c_str());
        }
        else
        {
            ImGui::Text("Select a Lua script.");
            ImGui::TextWrapped("Lua now lives inside Assets because scripts are game assets too.");
        }

        ImGui::EndChild();
    }
    else if (page == 9)
    {
        ImGui::Text("Assets > Lua Autorun");
        ImGui::Separator();

        bool autorun = LuaManager::AutorunEnabled();
        if (ImGui::Checkbox("Enable autorun folder", &autorun))
            LuaManager::SetAutorunEnabled(autorun);

        ImGui::TextWrapped("Folder: %s", LuaManager::GetAutorunFolder().c_str());
        ImGui::Separator();

        DrawLuaScriptList(LuaManager::ScriptSource::Autorun);
    }
    else if (page == 10)
    {
        ImGui::Text("Assets > Lua Overrides");
        ImGui::Separator();

        bool overrides = LuaManager::OverridesEnabled();
        if (ImGui::Checkbox("Enable Lua overrides", &overrides))
            LuaManager::SetOverridesEnabled(overrides);

        ImGui::TextWrapped("Folder: %s", LuaManager::GetOverrideFolder().c_str());
        ImGui::Separator();

        DrawLuaScriptList(LuaManager::ScriptSource::Override);
    }
    else if (page == 11)
    {
        ImGui::Text("Assets > Lua Console");
        ImGui::Separator();

        ImGui::TextWrapped("Lua console is staged. It logs commands now. Real VM execution comes after luaL_loadbuffer/lua_pcall hooks.");

        ImGui::InputText("Command", g_LuaConsoleInput, sizeof(g_LuaConsoleInput));

        if (ImGui::Button("Queue Lua Command", ImVec2(170, 24)))
        {
            LuaManager::QueueConsoleCommand(g_LuaConsoleInput);
            OverlayConsole::Execute(std::string("lua ") + g_LuaConsoleInput);
            g_LuaConsoleInput[0] = '\0';
        }

        ImGui::SameLine();

        if (ImGui::Button("Clear Lua Log", ImVec2(130, 24)))
            LuaManager::ClearConsoleLog();

        ImGui::SameLine();

        if (ImGui::Button("Rescan Lua", ImVec2(120, 24)))
            LuaManager::Rescan();

        ImGui::Separator();

        bool hooksEnabled = LuaHooks::Enabled();
        if (ImGui::Checkbox("Enable Lua hook logger", &hooksEnabled))
            LuaHooks::SetEnabled(hooksEnabled);

        bool redirectsEnabled = LuaHooks::RedirectEnabled();
        if (ImGui::Checkbox("Enable Lua override redirect", &redirectsEnabled))
            LuaHooks::SetRedirectEnabled(redirectsEnabled);

        ImGui::Separator();

        ImGui::Text("Lua Load Log");
        const auto& loadLog = LuaHooks::GetLoadLog();

        ImGui::BeginChild("##LuaHookLoadLog", ImVec2(0, 160), true);
        for (const auto& line : loadLog)
            ImGui::TextWrapped("%s", line.c_str());
        ImGui::EndChild();

        ImGui::Separator();

        ImGui::Text("Lua Console Log");
        const auto& log = LuaManager::GetConsoleLog();

        ImGui::BeginChild("##LuaConsoleLog", ImVec2(0, 0), true);
        for (const auto& line : log)
            ImGui::TextWrapped("%s", line.c_str());
        ImGui::EndChild();
    }
    else
    {
        ImGui::Text("Assets > Lua Functions");
        ImGui::Separator();

        const char* funcs[] =
        {
            "ccLuaTuiPuts",
            "ccLuaNop",
            "ccLuaUiSetEnableI",
            "ccLuaUiSetEnableX",
            "ccLuaUiSetTranslateI",
            "ccLuaUiSetOffsetI",
            "ccLuaUiSetScaleI",
            "ccLuaUiSetOpacityI",
            "ccLuaUiAnmRequestPlayI",
            "ccLuaUiAnmRequestPlayX",
            "ccLuaUiAnmRequestStopI",
            "ccLuaUiAnmRequestPauseI",
            "ccLuaUiFontSetTextMessage",
            "ccLuaUiFontSetTextSpeaker",
            "ccLuaUiFontSetColor",
            "ccLuaUiFontSetAlign",
            "ccLuaUiSeOn"
        };

        ImGui::TextWrapped("Known custom Lua functions found in the game strings:");
        ImGui::Separator();

        for (const char* f : funcs)
            ImGui::BulletText("%s", f);

        static char consoleInput[512]{};

        ImGui::Separator();
        ImGui::Text("Developer Console");

        ImGui::InputText("Console Command", consoleInput, sizeof(consoleInput));

        if (ImGui::Button("Run Command", ImVec2(140, 24)))
        {
            OverlayConsole::Execute(consoleInput);
            consoleInput[0] = '\0';
        }

        ImGui::SameLine();

        if (ImGui::Button("Clear Console", ImVec2(140, 24)))
            OverlayConsole::Clear();

        ImGui::Separator();

        ImGui::BeginChild("##OverlayConsoleLines", ImVec2(0, 220), true);

        const auto& lines = OverlayConsole::GetLines();
        for (const auto& line : lines)
            ImGui::TextWrapped("%s", line.c_str());

        ImGui::EndChild();
    }
}
