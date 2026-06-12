#include "StdInc.h"
#include "RuntimeAssetTelemetry.h"
#include "Logger.h"
#include "imgui.h"

#include <mutex>
#include <chrono>

namespace
{
    std::mutex g_Mutex;
    std::vector<RuntimeAssetTelemetry::Event> g_Events;

    double Now()
    {
        using namespace std::chrono;
        static auto start = steady_clock::now();
        return duration<double>(steady_clock::now() - start).count();
    }

    bool Interesting(const std::string& s)
    {
        std::string v = s;
        std::transform(v.begin(), v.end(), v.begin(), [](unsigned char c){ return static_cast<char>(std::tolower(c)); });

        return v.find(".xfbin") != std::string::npos ||
               v.find(".cpk") != std::string::npos ||
               v.find(".nud") != std::string::npos ||
               v.find(".nut") != std::string::npos ||
               v.find(".dds") != std::string::npos ||
               v.find(".adx") != std::string::npos ||
               v.find(".awb") != std::string::npos ||
               v.find(".acb") != std::string::npos ||
               v.find(".lua") != std::string::npos ||
               v.find("\\data\\") != std::string::npos ||
               v.find("/data/") != std::string::npos;
    }
}

namespace RuntimeAssetTelemetry
{
    bool Init()
    {
        Clear();
        Logger::Info("RuntimeAssetTelemetry initialized");
        return true;
    }

    void Shutdown()
    {
        Clear();
    }

    std::string EventTypeName(EventType type)
    {
        switch (type)
        {
        case EventType::FileOpen: return "FileOpen";
        case EventType::FileRead: return "FileRead";
        case EventType::FileWrite: return "FileWrite";
        case EventType::FindFile: return "FindFile";
        case EventType::Xfbin: return "XFBIN";
        case EventType::Cpk: return "CPK";
        case EventType::Model: return "Model";
        case EventType::Texture: return "Texture";
        case EventType::Sound: return "Sound";
        case EventType::Lua: return "Lua";
        default: return "Unknown";
        }
    }

    void Push(EventType type, const std::string& name, const std::string& detail, uint64_t size)
    {
        if ((type == EventType::FileOpen || type == EventType::FindFile) && !Interesting(name))
            return;

        Event e{};
        e.Type = type;
        e.Name = name;
        e.Detail = detail;
        e.Size = size;
        e.ThreadId = GetCurrentThreadId();
        e.TimeSeconds = Now();

        {
            std::lock_guard<std::mutex> lock(g_Mutex);
            g_Events.push_back(e);
            if (g_Events.size() > 2000)
                g_Events.erase(g_Events.begin(), g_Events.begin() + 250);
        }

        Logger::Info("[RUNTIME] " + EventTypeName(type) + " | " + name + " | " + detail);
    }

    std::vector<Event> Snapshot()
    {
        std::lock_guard<std::mutex> lock(g_Mutex);
        return g_Events;
    }

    void Clear()
    {
        std::lock_guard<std::mutex> lock(g_Mutex);
        g_Events.clear();
    }

    int Count(EventType type)
    {
        std::lock_guard<std::mutex> lock(g_Mutex);
        int c = 0;
        for (const auto& e : g_Events)
            if (e.Type == type)
                c++;
        return c;
    }

    void DrawDebugPanel()
    {
        ImGui::Text("Runtime Asset Telemetry");
        ImGui::Separator();

        if (ImGui::Button("Clear Runtime Events", ImVec2(180, 24)))
            Clear();

        ImGui::SameLine();
        ImGui::TextDisabled("Open %d | XFBIN %d | CPK %d | Texture %d | Model %d | Sound %d | Lua %d",
            Count(EventType::FileOpen), Count(EventType::Xfbin), Count(EventType::Cpk),
            Count(EventType::Texture), Count(EventType::Model), Count(EventType::Sound), Count(EventType::Lua));

        auto events = Snapshot();
        ImGui::BeginChild("##RuntimeEvents", ImVec2(0, 260), true);
        int start = static_cast<int>(events.size()) - 300;
        if (start < 0) start = 0;

        for (int i = start; i < static_cast<int>(events.size()); ++i)
        {
            const auto& e = events[i];
            ImGui::TextWrapped("[%.2f] [%s] T%u %s %s",
                e.TimeSeconds, EventTypeName(e.Type).c_str(), e.ThreadId, e.Name.c_str(), e.Detail.c_str());
        }
        ImGui::EndChild();
    }
}
