#pragma once
#include "StdInc.h"

namespace SteamPersonaManager
{
    void Init();

    const char* GetPersonaName();
    void SetPersonaName(const std::string& name);

    bool SetRichPresence(const std::string& key, const std::string& value);
    const char* GetRichPresence(const std::string& key);
    void ClearRichPresence();
}