#include "StdInc.h"
#include "NS2Offsets.h"
#include "Logger.h"

namespace
{
    NS2Offsets::OffsetEntry g_Offsets[] =
    {
        // File I/O hooks
        { "CreateFileA", 0x140772108, NS2Offsets::OffsetStatus::NotImplemented },
        { "CreateFileW", 0x140772110, NS2Offsets::OffsetStatus::NotImplemented },
        { "ReadFile", 0x140772148, NS2Offsets::OffsetStatus::NotImplemented },
        { "WriteFile", 0x140772160, NS2Offsets::OffsetStatus::NotImplemented },
        { "CloseHandle", 0x140772168, NS2Offsets::OffsetStatus::NotImplemented },
        { "FindFirstFileA", 0x140772280, NS2Offsets::OffsetStatus::NotImplemented },
        { "FindFirstFileW", 0x140772290, NS2Offsets::OffsetStatus::NotImplemented },
        { "FindNextFileW", 0x140772350, NS2Offsets::OffsetStatus::NotImplemented },
        { "GetFileSize", 0x140772140, NS2Offsets::OffsetStatus::NotImplemented },
        { "GetFileSizeEx", 0x140772360, NS2Offsets::OffsetStatus::NotImplemented },

        // Steam hooks
        { "SteamAPI_Init", 0x1409310BC, NS2Offsets::OffsetStatus::NotImplemented },
        { "SteamAPI_RunCallbacks", 0x140772C90, NS2Offsets::OffsetStatus::NotImplemented },
        { "SteamAPI_RegisterCallback", 0x140772C80, NS2Offsets::OffsetStatus::NotImplemented },
        { "SteamAPI_UnregisterCallback", 0x140772C78, NS2Offsets::OffsetStatus::NotImplemented },
        { "SteamAPI_RegisterCallResult", 0x140772C50, NS2Offsets::OffsetStatus::NotImplemented },
        { "SteamAPI_UnregisterCallResult", 0x140772C48, NS2Offsets::OffsetStatus::NotImplemented },
        { "SteamAPI_Shutdown", 0x140772CB8, NS2Offsets::OffsetStatus::NotImplemented },

        // Asset strings / future XREF targets
        { "aXfbin", 0x14079FBA4, NS2Offsets::OffsetStatus::NotImplemented },
        { "aEXfbin", 0x140887960, NS2Offsets::OffsetStatus::NotImplemented },
        { "aXfbinRead", 0x1408933A8, NS2Offsets::OffsetStatus::NotImplemented },
        { "aXfbinLoadRequest", 0x140890EF8, NS2Offsets::OffsetStatus::NotImplemented },

        { "aReadcpk", 0x14079FDB0, NS2Offsets::OffsetStatus::NotImplemented },
        { "aCriCpkAnalyzer", 0x14088A590, NS2Offsets::OffsetStatus::NotImplemented },
        { "aCpkmode", 0x14088A7B8, NS2Offsets::OffsetStatus::NotImplemented },
        { "aCpkc", 0x14088A878, NS2Offsets::OffsetStatus::NotImplemented },

        { "aNuccchunkmodel", 0x1408903E0, NS2Offsets::OffsetStatus::NotImplemented },
        { "aModel", 0x1408D05B8, NS2Offsets::OffsetStatus::NotImplemented },
        { "aTexture", 0x1408D06C8, NS2Offsets::OffsetStatus::NotImplemented },
        { "aTextureRef", 0x1408D0718, NS2Offsets::OffsetStatus::NotImplemented },
        { "aTexturesize", 0x14088C248, NS2Offsets::OffsetStatus::NotImplemented },
        { "aBindtexture", 0x1408CF440, NS2Offsets::OffsetStatus::NotImplemented },

        { "aAdx", 0x1408AFE08, NS2Offsets::OffsetStatus::NotImplemented },
        { "aCriAdxDecoder", 0x1408BF030, NS2Offsets::OffsetStatus::NotImplemented },
        { "aCcloadsound", 0x14081E678, NS2Offsets::OffsetStatus::NotImplemented },
        { "aSoundLoadThread", 0x140887818, NS2Offsets::OffsetStatus::NotImplemented },

        // Lua strings
        { "aLuaFunctionExport", 0x1408A6BD0, NS2Offsets::OffsetStatus::NotImplemented },
        { "aLua51", 0x1408A6BF0, NS2Offsets::OffsetStatus::NotImplemented },
        { "aLuaDebug", 0x1408A7C48, NS2Offsets::OffsetStatus::NotImplemented },
        { "aLuaPath", 0x1408A8008, NS2Offsets::OffsetStatus::NotImplemented },
        { "aLuaCpath", 0x1408A8088, NS2Offsets::OffsetStatus::NotImplemented },

        // Gameplay / player strings
        { "aCcgameUpdateAllSubManager", 0x14082E8E8, NS2Offsets::OffsetStatus::NotImplemented },
        { "aCcplayerCtrl", 0x140803250, NS2Offsets::OffsetStatus::NotImplemented },
        { "aCcplayerDraw", 0x1408032C0, NS2Offsets::OffsetStatus::NotImplemented },
        { "aBattlesequence", 0x14082DB48, NS2Offsets::OffsetStatus::NotImplemented },
        { "aBattleDebug", 0x14083B788, NS2Offsets::OffsetStatus::NotImplemented },

        // Function candidates from XREFs.
        // ccGame::UpdateAllSubManager xref candidate.
        { "Game_UpdateAllSubManager_Candidate_1", 0x140419EC8, NS2Offsets::OffsetStatus::NotImplemented },

        // ccGame::UpdateAllSubManager xref candidate.
        { "Game_UpdateAllSubManager_Candidate_2", 0x1404483F4, NS2Offsets::OffsetStatus::NotImplemented },

        // Scene/data pointers
        { "ccSceneBootRoot", 0x140977930, NS2Offsets::OffsetStatus::NotImplemented },
        { "ccSceneGameRoot", 0x1409779C0, NS2Offsets::OffsetStatus::NotImplemented },
        { "ccSceneAdventure", 0x140977B70, NS2Offsets::OffsetStatus::NotImplemented },
        { "ccSceneBattle", 0x140977FF0, NS2Offsets::OffsetStatus::NotImplemented },
        { "ccSceneBattleMain", 0x1409783E0, NS2Offsets::OffsetStatus::NotImplemented },
        { "ccSceneBattleRoot", 0x140978470, NS2Offsets::OffsetStatus::NotImplemented },
        { "ccSceneCharacterSelect", 0x14097AFC0, NS2Offsets::OffsetStatus::NotImplemented },
        { "ccSceneNetwork", 0x14097C1C0, NS2Offsets::OffsetStatus::NotImplemented },
        { "ccSceneTitle", 0x14097D600, NS2Offsets::OffsetStatus::NotImplemented },
        { "ccSceneOption", 0x14097DB10, NS2Offsets::OffsetStatus::NotImplemented },
        { "ccSceneKeyConfig", 0x14097DA80, NS2Offsets::OffsetStatus::NotImplemented },
        { "ccSceneDInputSetting", 0x14097DBA0, NS2Offsets::OffsetStatus::NotImplemented },
    };
}

namespace NS2Offsets
{
    uintptr_t GetModuleBase()
    {
        return reinterpret_cast<uintptr_t>(GetModuleHandleA(nullptr));
    }

    uintptr_t Rebase(uintptr_t idaAddress)
    {
        if (!idaAddress)
            return 0;

        return GetModuleBase() + (idaAddress - IdaBase);
    }

    OffsetEntry* Find(const std::string& name)
    {
        for (auto& entry : g_Offsets)
        {
            if (_stricmp(entry.Name, name.c_str()) == 0)
                return &entry;
        }

        return nullptr;
    }

    std::vector<OffsetEntry> GetAll()
    {
        return std::vector<OffsetEntry>(
            std::begin(g_Offsets),
            std::end(g_Offsets));
    }

    void SetInitialized(const std::string& name)
    {
        OffsetEntry* entry = Find(name);

        if (entry)
            entry->Status = OffsetStatus::Initialized;
    }

    void SetFailed(const std::string& name)
    {
        OffsetEntry* entry = Find(name);

        if (entry)
            entry->Status = OffsetStatus::Failed;
    }

    const char* StatusName(OffsetStatus status)
    {
        switch (status)
        {
        case OffsetStatus::Initialized:
            return "Initialized";

        case OffsetStatus::Failed:
            return "Failed";

        case OffsetStatus::NotImplemented:
        default:
            return "Not Implemented";
        }
    }

    void LogAll()
    {
        Logger::Info("========== NS2 Offsets ==========");
        Logger::Info("Total offsets: " + std::to_string(GetAll().size()));

        for (const auto& entry : g_Offsets)
        {
            char buffer[256]{};

            sprintf_s(
                buffer,
                "%s | 0x%p | %s",
                entry.Name,
                reinterpret_cast<void*>(Rebase(entry.Address)),
                StatusName(entry.Status));

            Logger::Info(buffer);
        }

        Logger::Info("=================================");
    }
}