#pragma once
#include "StdInc.h"

namespace NS2Offsets
{
    enum class OffsetStatus
    {
        NotImplemented,
        Initialized,
        Failed
    };

    struct OffsetEntry
    {
        const char* Name;
        uintptr_t Address;
        OffsetStatus Status;
    };

    constexpr uintptr_t IdaBase = 0x140000000;

    uintptr_t GetModuleBase();
    uintptr_t Rebase(uintptr_t idaAddress);

    OffsetEntry* Find(const std::string& name);
    std::vector<OffsetEntry> GetAll();

    void SetInitialized(const std::string& name);
    void SetFailed(const std::string& name);

    const char* StatusName(OffsetStatus status);
    void LogAll();
}