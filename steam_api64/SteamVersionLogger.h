#pragma once
#include "StdInc.h"

namespace SteamVersionLogger
{
    void LogExportRequest(const char* name);
    void LogInterfaceRequest(const char* source, const char* version);
    bool Contains(const std::string& text, const char* needle);
}