#pragma once
#include "StdInc.h"

namespace OverlayConsole
{
    struct CommandResult
    {
        bool Handled = false;
        std::string Message;
    };

    bool Init();
    void Shutdown();

    void AddLine(const std::string& line);
    void Clear();

    const std::vector<std::string>& GetLines();

    CommandResult Execute(const std::string& commandLine);
}