#pragma once
#include "StdInc.h"
#include "RuntimeAssetTelemetry.h"

namespace RuntimeAssetTelemetry
{
    enum class EventType
    {
        FileOpen,
        FileRead,
        FileWrite,
        FindFile,
        Xfbin,
        Cpk,
        Model,
        Texture,
        Sound,
        Lua,
        Unknown
    };

    struct Event
    {
        EventType Type = EventType::Unknown;
        std::string Name;
        std::string Detail;
        uint64_t Size = 0;
        uint32_t ThreadId = 0;
        double TimeSeconds = 0.0;
    };

    bool Init();
    void Shutdown();

    void Push(EventType type, const std::string& name, const std::string& detail = "", uint64_t size = 0);
    std::vector<Event> Snapshot();
    void Clear();
    int Count(EventType type);
    std::string EventTypeName(EventType type);
    void DrawDebugPanel();
   
}
