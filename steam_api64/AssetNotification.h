#pragma once
#include "StdInc.h"

namespace AssetNotification
{
    enum class Type { Info, Success, Warning, Error };

    bool Init();
    void Shutdown();

    void Push(Type type, const std::string& title, const std::string& message, float seconds = 4.0f);
    void PushInfo(const std::string& title, const std::string& message);
    void PushSuccess(const std::string& title, const std::string& message);
    void PushWarning(const std::string& title, const std::string& message);
    void PushError(const std::string& title, const std::string& message);

    void DrawToasts();
}
