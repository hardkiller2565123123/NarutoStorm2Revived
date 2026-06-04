#pragma once

#include "StdInc.h"

namespace Logger
{
    bool Init();
    void Shutdown();

    void Info(const std::string& text);
    void Error(const std::string& text);

    void ExportInitialized(const char* exportName);
}