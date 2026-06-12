#pragma once
#include "StdInc.h"

namespace RuntimeHookAddresses
{
    struct AddressSet
    {
        uintptr_t Base = 0;
        uintptr_t Game_XfbinLoadRequest = 0;
        uintptr_t Game_XfbinRead = 0;
        uintptr_t Game_ReadCpk = 0;
        uintptr_t Game_CriCpkAnalyzer = 0;
        uintptr_t Game_LoadModel = 0;
        uintptr_t Game_BindTexture = 0;
        uintptr_t Game_LoadSound = 0;
        uintptr_t Game_LuaExecute = 0;
    };

    void Init();
    uintptr_t Rebase(uintptr_t idaAddress);
    const AddressSet& Get();
}
