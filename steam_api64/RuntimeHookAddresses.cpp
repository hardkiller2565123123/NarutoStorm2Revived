#include "StdInc.h"
#include "RuntimeHookAddresses.h"
#include "Logger.h"
#include "NS2Offsets.h"

namespace
{
    RuntimeHookAddresses::AddressSet g_Addresses{};
    constexpr uintptr_t kIdaBase = 0x140000000;

    uintptr_t ModuleBase()
    {
        return reinterpret_cast<uintptr_t>(GetModuleHandleA(nullptr));
    }
}

namespace RuntimeHookAddresses
{
    uintptr_t Rebase(uintptr_t idaAddress)
    {
        return NS2Offsets::Rebase(idaAddress);
    }

    void Init()
    {
        g_Addresses = AddressSet{};
        g_Addresses.Base = ModuleBase();

        // Put FUNCTION START addresses here after finding XREFs in IDA.
        // Example:
        // g_Addresses.Game_XfbinRead = Rebase(0x140123456);

        g_Addresses.Game_XfbinLoadRequest = 0;
        g_Addresses.Game_XfbinRead = 0;
        g_Addresses.Game_ReadCpk = 0;
        g_Addresses.Game_CriCpkAnalyzer = 0;
        g_Addresses.Game_LoadModel = 0;
        g_Addresses.Game_BindTexture = 0;
        g_Addresses.Game_LoadSound = 0;
        g_Addresses.Game_LuaExecute = 0;

        Logger::Info("RuntimeHookAddresses initialized");
    }

    const AddressSet& Get()
    {
        return g_Addresses;
    }
}
