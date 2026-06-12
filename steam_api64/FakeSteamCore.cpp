#include "StdInc.h"
#include "FakeSteamCore.h"
#include "FakeSteamInterfaces.h"
#include "Logger.h"
#include "SteamAuth.h"
#include "SteamCallbackManager.h"
#include "SteamCallResultManager.h"
#include "SteamConfig.h"
#include "SteamIDManager.h"
#include "SteamStatsLocal.h"
#include "SteamStorageLocal.h"

#include <cctype>
#include <ctime>

extern "C" void* __cdecl NS2Revived_SteamNetworking();
extern "C" void* __cdecl NS2Revived_SteamGameServerNetworking();
extern "C" void* __cdecl NS2Revived_SteamNetworkingUtils();
extern "C" void* __cdecl NS2Revived_SteamNetworkingMessages();
extern "C" void* __cdecl NS2Revived_SteamNetworkingSockets();
extern "C" void* __cdecl NS2Revived_SteamGameServerNetworkingSockets();

namespace
{
    constexpr HSteamUser kLocalUser = 1;
    constexpr HSteamPipe kLocalPipe = 1;
    constexpr AppId_t kStorm2AppId = 543870;

    bool g_Initialized = false;
    std::string g_InstallPath;
    std::string g_UserDataFolder;
    std::vector<unsigned char> g_LastEncryptedTicket;

#pragma pack(push, 1)
    struct OfflineSteamIPAddress
    {
        union
        {
            uint32_t m_unIPv4;
            uint8_t m_rgubIPv6[16];
            uint64_t m_ipv6Qword[2];
        };

        int m_eType;
    };
#pragma pack(pop)

    std::string ExeFolder()
    {
        char path[MAX_PATH]{};
        GetModuleFileNameA(nullptr, path, MAX_PATH);
        return std::filesystem::path(path).parent_path().string();
    }

    void CopyString(char* out, int outSize, const std::string& value)
    {
        if (!out || outSize <= 0)
            return;

        strncpy_s(out, static_cast<size_t>(outSize), value.c_str(), _TRUNCATE);
    }

    std::string UpperInterfaceName(std::string value)
    {
        std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
            return static_cast<char>(std::toupper(c));
        });
        return value;
    }

    bool HasInterface(const std::string& value, const char* token)
    {
        return value.find(token) != std::string::npos;
    }

    void* RouteVersion(const char* version)
    {
        std::string v = version ? version : "";
        std::string u = UpperInterfaceName(v);

        if (HasInterface(u, "STEAMNETWORKINGMESSAGES")) return NS2Revived_SteamNetworkingMessages();
        if (HasInterface(u, "STEAMNETWORKINGSOCKETS")) return NS2Revived_SteamNetworkingSockets();
        if (HasInterface(u, "STEAMNETWORKINGUTILS")) return NS2Revived_SteamNetworkingUtils();
        if (HasInterface(u, "STEAMNETWORKING")) return NS2Revived_SteamNetworking();

        if (HasInterface(u, "STEAMCLIENT")) return FakeSteamCore::Client();
        if (HasInterface(u, "STEAMGAMESERVERSTATS")) return FakeSteamCore::GameServerStats();
        if (HasInterface(u, "STEAMGAMESERVER")) return FakeSteamCore::GameServer();
        if (HasInterface(u, "STEAMUSERSTATS")) return FakeSteamInterfaces::UserStats();
        if (HasInterface(u, "STEAMUSER")) return FakeSteamCore::User();
        if (HasInterface(u, "STEAMUTILS")) return FakeSteamCore::Utils();
        if (HasInterface(u, "STEAMAPPS")) return FakeSteamCore::Apps();

        if (HasInterface(u, "STEAMFRIENDS")) return FakeSteamInterfaces::Friends();
        if (HasInterface(u, "STEAMMATCHMAKINGSERVERS")) return FakeSteamInterfaces::MatchmakingServers();
        if (HasInterface(u, "STEAMMATCHMAKING")) return FakeSteamInterfaces::Matchmaking();
        if (HasInterface(u, "STEAMREMOTESTORAGE")) return FakeSteamInterfaces::RemoteStorage();
        if (HasInterface(u, "STEAMSCREENSHOTS")) return FakeSteamInterfaces::Screenshots();
        if (HasInterface(u, "STEAMHTTP")) return FakeSteamInterfaces::HTTP();
        if (HasInterface(u, "STEAMCONTROLLER")) return FakeSteamInterfaces::Controller();
        if (HasInterface(u, "STEAMUGC")) return FakeSteamInterfaces::UGC();
        if (HasInterface(u, "STEAMAPPLIST")) return FakeSteamInterfaces::AppList();
        if (HasInterface(u, "STEAMMUSICREMOTE")) return FakeSteamInterfaces::MusicRemote();
        if (HasInterface(u, "STEAMMUSIC")) return FakeSteamInterfaces::Music();
        if (HasInterface(u, "STEAMHTMLSURFACE")) return FakeSteamInterfaces::HTMLSurface();
        if (HasInterface(u, "STEAMINVENTORY")) return FakeSteamInterfaces::Inventory();
        if (HasInterface(u, "STEAMVIDEO")) return FakeSteamInterfaces::Video();
        if (HasInterface(u, "STEAMPARENTALSETTINGS")) return FakeSteamInterfaces::ParentalSettings();
        if (HasInterface(u, "STEAMGAMESEARCH")) return FakeSteamInterfaces::GameSearch();
        if (HasInterface(u, "STEAMINPUT")) return FakeSteamInterfaces::Input();
        if (HasInterface(u, "STEAMPARTIES")) return FakeSteamInterfaces::Parties();
        if (HasInterface(u, "STEAMREMOTEPLAY")) return FakeSteamInterfaces::RemotePlay();

        return nullptr;
    }

    class OfflineSteamUser final
    {
    public:
        virtual HSteamUser GetHSteamUser() { return kLocalUser; }
        virtual bool BLoggedOn() { return true; }
        virtual CSteamID GetSteamID() { return SteamIDManager::GetLocalSteamID(); }

        virtual int InitiateGameConnection(void* authBlob, int maxAuthBlob, CSteamID server, uint32_t ip, uint16_t port, bool secure)
        {
            NSR_UNUSED(server);
            NSR_UNUSED(ip);
            NSR_UNUSED(port);
            NSR_UNUSED(secure);

            uint32_t size = 0;
            SteamAuth::GetAuthSessionTicket(authBlob, maxAuthBlob, &size);
            return static_cast<int>(size);
        }

        virtual void TerminateGameConnection(uint32_t ip, uint16_t port)
        {
            NSR_UNUSED(ip);
            NSR_UNUSED(port);
        }

        virtual void TrackAppUsageEvent(uint64_t gameID, int event, const char* extra)
        {
            NSR_UNUSED(gameID);
            NSR_UNUSED(event);
            NSR_UNUSED(extra);
        }

        virtual bool GetUserDataFolder(char* buffer, int bufferSize)
        {
            CopyString(buffer, bufferSize, g_UserDataFolder);
            return buffer && bufferSize > 0;
        }

        virtual void StartVoiceRecording() {}
        virtual void StopVoiceRecording() {}

        virtual int GetAvailableVoice(uint32_t* compressed, uint32_t* uncompressed, uint32_t sampleRate)
        {
            NSR_UNUSED(sampleRate);
            if (compressed) *compressed = 0;
            if (uncompressed) *uncompressed = 0;
            return 2;
        }

        virtual int GetVoice(bool wantCompressed, void* dest, uint32_t destSize, uint32_t* written, bool wantUncompressed, void* uncompressedDest, uint32_t uncompressedSize, uint32_t* uncompressedWritten, uint32_t sampleRate)
        {
            NSR_UNUSED(wantCompressed);
            NSR_UNUSED(dest);
            NSR_UNUSED(destSize);
            NSR_UNUSED(wantUncompressed);
            NSR_UNUSED(uncompressedDest);
            NSR_UNUSED(uncompressedSize);
            NSR_UNUSED(sampleRate);
            if (written) *written = 0;
            if (uncompressedWritten) *uncompressedWritten = 0;
            return 2;
        }

        virtual int DecompressVoice(const void* compressed, uint32_t compressedSize, void* dest, uint32_t destSize, uint32_t* written, uint32_t sampleRate)
        {
            NSR_UNUSED(compressed);
            NSR_UNUSED(compressedSize);
            NSR_UNUSED(dest);
            NSR_UNUSED(destSize);
            NSR_UNUSED(sampleRate);
            if (written) *written = 0;
            return 2;
        }

        virtual uint32_t GetVoiceOptimalSampleRate() { return 48000; }
        virtual uint32_t GetAuthSessionTicket(void* ticket, int maxTicket, uint32_t* ticketSize, const void* identity)
        {
            NSR_UNUSED(identity);
            return SteamAuth::GetAuthSessionTicket(ticket, maxTicket, ticketSize);
        }
        virtual uint32_t GetAuthTicketForWebApi(const char* identity)
        {
            NSR_UNUSED(identity);
            return SteamAuth::GetAuthSessionTicket(nullptr, 0, nullptr);
        }
        virtual int BeginAuthSession(const void* ticket, int ticketSize, CSteamID steamID) { return SteamAuth::BeginAuthSession(ticket, ticketSize, steamID); }
        virtual void EndAuthSession(CSteamID steamID) { SteamAuth::EndAuthSession(steamID); }
        virtual void CancelAuthTicket(uint32_t ticketHandle) { SteamAuth::CancelAuthTicket(ticketHandle); }
        virtual int UserHasLicenseForApp(CSteamID steamID, AppId_t appID)
        {
            NSR_UNUSED(steamID);
            NSR_UNUSED(appID);
            return 0;
        }
        virtual bool BIsBehindNAT() { return false; }
        virtual void AdvertiseGame(CSteamID server, uint32_t ip, uint16_t port)
        {
            NSR_UNUSED(server);
            NSR_UNUSED(ip);
            NSR_UNUSED(port);
        }
        virtual SteamAPICall_t RequestEncryptedAppTicket(void* data, int dataSize)
        {
            const char prefix[] = "NS2REVIVED_ENCRYPTED_APP_TICKET";
            g_LastEncryptedTicket.assign(prefix, prefix + sizeof(prefix));
            if (data && dataSize > 0)
            {
                const auto* bytes = static_cast<const unsigned char*>(data);
                g_LastEncryptedTicket.insert(g_LastEncryptedTicket.end(), bytes, bytes + dataSize);
            }

            struct Result { int m_eResult; };
            Result result{ 1 };
            return SteamCallResultManager::CreateCallResult(154, &result, sizeof(result), true);
        }
        virtual bool GetEncryptedAppTicket(void* ticket, int maxTicket, uint32_t* ticketSize)
        {
            if (g_LastEncryptedTicket.empty())
                RequestEncryptedAppTicket(nullptr, 0);

            if (ticketSize)
                *ticketSize = static_cast<uint32_t>(g_LastEncryptedTicket.size());

            if (!ticket || maxTicket < static_cast<int>(g_LastEncryptedTicket.size()))
                return false;

            memcpy(ticket, g_LastEncryptedTicket.data(), g_LastEncryptedTicket.size());
            return true;
        }
        virtual int GetGameBadgeLevel(int series, bool foil)
        {
            NSR_UNUSED(series);
            NSR_UNUSED(foil);
            return 0;
        }
        virtual int GetPlayerSteamLevel() { return 1; }
        virtual SteamAPICall_t RequestStoreAuthURL(const char* redirect)
        {
            NSR_UNUSED(redirect);
            struct Result { char m_szURL[512]; };
            Result result{};
            strcpy_s(result.m_szURL, "about:blank");
            return SteamCallResultManager::CreateCallResult(165, &result, sizeof(result), true);
        }
        virtual bool BIsPhoneVerified() { return true; }
        virtual bool BIsTwoFactorEnabled() { return false; }
        virtual bool BIsPhoneIdentifying() { return false; }
        virtual bool BIsPhoneRequiringVerification() { return false; }
        virtual SteamAPICall_t GetMarketEligibility() { return SteamCallResultManager::CreateCallResult(166); }
        virtual SteamAPICall_t GetDurationControl() { return SteamCallResultManager::CreateCallResult(167); }
        virtual bool BSetDurationControlOnlineState(int state)
        {
            NSR_UNUSED(state);
            return true;
        }
    };

    class OfflineSteamApps final
    {
    public:
        virtual bool BIsSubscribed() { return true; }
        virtual bool BIsLowViolence() { return false; }
        virtual bool BIsCybercafe() { return false; }
        virtual bool BIsVACBanned() { return false; }
        virtual const char* GetCurrentGameLanguage() { return "english"; }
        virtual const char* GetAvailableGameLanguages() { return "english"; }
        virtual bool BIsSubscribedApp(AppId_t appID) { NSR_UNUSED(appID); return true; }
        virtual bool BIsDlcInstalled(AppId_t appID) { NSR_UNUSED(appID); return true; }
        virtual uint32_t GetEarliestPurchaseUnixTime(AppId_t appID) { NSR_UNUSED(appID); return 1262304000u; }
        virtual bool BIsSubscribedFromFreeWeekend() { return false; }
        virtual int GetDLCCount() { return 0; }
        virtual bool BGetDLCDataByIndex(int index, AppId_t* appID, bool* available, char* name, int nameSize)
        {
            NSR_UNUSED(index);
            if (appID) *appID = 0;
            if (available) *available = false;
            if (name && nameSize > 0) name[0] = '\0';
            return false;
        }
        virtual void InstallDLC(AppId_t appID) { NSR_UNUSED(appID); }
        virtual void UninstallDLC(AppId_t appID) { NSR_UNUSED(appID); }
        virtual void RequestAppProofOfPurchaseKey(AppId_t appID) { NSR_UNUSED(appID); }
        virtual bool GetCurrentBetaName(char* name, int nameSize)
        {
            CopyString(name, nameSize, "public");
            return name && nameSize > 0;
        }
        virtual bool MarkContentCorrupt(bool missingFilesOnly) { NSR_UNUSED(missingFilesOnly); return false; }
        virtual uint32_t GetInstalledDepots(AppId_t appID, uint32_t* depots, uint32_t maxDepots)
        {
            NSR_UNUSED(appID);
            if (depots && maxDepots > 0) depots[0] = kStorm2AppId;
            return maxDepots > 0 ? 1u : 0u;
        }
        virtual uint32_t GetAppInstallDir(AppId_t appID, char* folder, uint32_t folderSize)
        {
            NSR_UNUSED(appID);
            CopyString(folder, static_cast<int>(folderSize), g_InstallPath);
            return static_cast<uint32_t>(g_InstallPath.size());
        }
        virtual bool BIsAppInstalled(AppId_t appID) { NSR_UNUSED(appID); return true; }
        virtual CSteamID GetAppOwner() { return SteamIDManager::GetLocalSteamID(); }
        virtual const char* GetLaunchQueryParam(const char* key) { NSR_UNUSED(key); return ""; }
        virtual bool GetDlcDownloadProgress(AppId_t appID, uint64_t* downloaded, uint64_t* total)
        {
            NSR_UNUSED(appID);
            if (downloaded) *downloaded = 0;
            if (total) *total = 0;
            return false;
        }
        virtual int GetAppBuildId() { return 1; }
        virtual void RequestAllProofOfPurchaseKeys() {}
        virtual SteamAPICall_t GetFileDetails(const char* file)
        {
            NSR_UNUSED(file);
            return SteamCallResultManager::CreateCallResult(1023);
        }
        virtual int GetLaunchCommandLine(char* commandLine, int size)
        {
            if (commandLine && size > 0) commandLine[0] = '\0';
            return 0;
        }
        virtual bool BIsSubscribedFromFamilySharing() { return false; }
        virtual bool BIsTimedTrial(uint32_t* allowed, uint32_t* played)
        {
            if (allowed) *allowed = 0;
            if (played) *played = 0;
            return false;
        }
        virtual bool SetDlcContext(AppId_t appID) { NSR_UNUSED(appID); return true; }
    };

    class OfflineSteamGameServer final
    {
    public:
        virtual bool InitGameServer(uint32_t ip, uint16_t gamePort, uint16_t queryPort, uint32_t flags, AppId_t appID, const char* version)
        {
            NSR_UNUSED(ip);
            NSR_UNUSED(gamePort);
            NSR_UNUSED(queryPort);
            NSR_UNUSED(flags);
            NSR_UNUSED(appID);
            NSR_UNUSED(version);
            m_LoggedOn = true;
            return true;
        }

        virtual void SetProduct(const char* product) { m_Product = product ? product : ""; }
        virtual void SetGameDescription(const char* description) { m_Description = description ? description : ""; }
        virtual void SetModDir(const char* modDir) { m_ModDir = modDir ? modDir : ""; }
        virtual void SetDedicatedServer(bool dedicated) { m_Dedicated = dedicated; }
        virtual void LogOn(const char* token) { NSR_UNUSED(token); m_LoggedOn = true; }
        virtual void LogOnAnonymous() { m_LoggedOn = true; }
        virtual void LogOff() { m_LoggedOn = false; }
        virtual bool BLoggedOn() { return m_LoggedOn; }
        virtual bool BSecure() { return false; }
        virtual CSteamID GetSteamID() { return SteamIDManager::GetLocalSteamID(); }
        virtual bool WasRestartRequested() { return false; }
        virtual void SetMaxPlayerCount(int playersMax) { m_MaxPlayers = playersMax; }
        virtual void SetBotPlayerCount(int botPlayers) { m_BotPlayers = botPlayers; }
        virtual void SetServerName(const char* serverName) { m_ServerName = serverName ? serverName : ""; }
        virtual void SetMapName(const char* mapName) { m_MapName = mapName ? mapName : ""; }
        virtual void SetPasswordProtected(bool passwordProtected) { m_PasswordProtected = passwordProtected; }
        virtual void SetSpectatorPort(uint16_t spectatorPort) { m_SpectatorPort = spectatorPort; }
        virtual void SetSpectatorServerName(const char* spectatorServerName) { m_SpectatorServerName = spectatorServerName ? spectatorServerName : ""; }
        virtual void ClearAllKeyValues() { m_KeyValues.clear(); }
        virtual void SetKeyValue(const char* key, const char* value)
        {
            if (!key || !key[0])
                return;

            m_KeyValues[key] = value ? value : "";
        }
        virtual void SetGameTags(const char* tags) { m_GameTags = tags ? tags : ""; }
        virtual void SetGameData(const char* data) { m_GameData = data ? data : ""; }
        virtual void SetRegion(const char* region) { m_Region = region ? region : ""; }
        virtual void SetAdvertiseServerActive(bool active)
        {
            m_AdvertiseActive = false;
            NSR_UNUSED(active);
        }
        virtual uint32_t GetAuthSessionTicket(void* ticket, int maxTicket, uint32_t* ticketSize, const void* identity)
        {
            NSR_UNUSED(identity);
            return SteamAuth::GetAuthSessionTicket(ticket, maxTicket, ticketSize);
        }
        virtual int BeginAuthSession(const void* ticket, int ticketSize, CSteamID steamID) { return SteamAuth::BeginAuthSession(ticket, ticketSize, steamID); }
        virtual void EndAuthSession(CSteamID steamID) { SteamAuth::EndAuthSession(steamID); }
        virtual void CancelAuthTicket(uint32_t ticketHandle) { SteamAuth::CancelAuthTicket(ticketHandle); }
        virtual int UserHasLicenseForApp(CSteamID steamID, AppId_t appID)
        {
            NSR_UNUSED(steamID);
            NSR_UNUSED(appID);
            return 0;
        }
        virtual bool RequestUserGroupStatus(CSteamID steamIDUser, CSteamID steamIDGroup)
        {
            struct Result
            {
                CSteamID m_SteamIDUser;
                CSteamID m_SteamIDGroup;
                bool m_bMember;
                bool m_bOfficer;
            };

            Result result{ steamIDUser, steamIDGroup, false, false };
            SteamCallbackManager::PushCallback(208, &result, sizeof(result));
            return true;
        }
        virtual void GetGameplayStats()
        {
            struct Result
            {
                int m_eResult;
                int32_t m_nRank;
                uint32_t m_unTotalConnects;
                uint32_t m_unTotalMinutesPlayed;
            };

            Result result{ 1, 0, 0, 0 };
            SteamCallbackManager::PushCallback(207, &result, sizeof(result));
        }
        virtual SteamAPICall_t GetServerReputation()
        {
            struct Result
            {
                int m_eResult;
                uint32_t m_unReputationScore;
                bool m_bBanned;
                uint32_t m_unBannedIP;
                uint16_t m_usBannedPort;
                uint64_t m_ulBannedGameID;
                uint32_t m_unBanExpires;
            };

            Result result{ 1, 100, false, 0, 0, 0, 0 };
            return SteamCallResultManager::CreateCallResult(209, &result, sizeof(result), true);
        }
        virtual OfflineSteamIPAddress GetPublicIP()
        {
            OfflineSteamIPAddress ip{};
            ip.m_unIPv4 = 0x7F000001u;
            ip.m_eType = 0;
            return ip;
        }
        virtual bool HandleIncomingPacket(const void* data, int dataSize, uint32_t sourceIP, uint16_t sourcePort)
        {
            NSR_UNUSED(data);
            NSR_UNUSED(dataSize);
            NSR_UNUSED(sourceIP);
            NSR_UNUSED(sourcePort);
            return false;
        }
        virtual int GetNextOutgoingPacket(void* out, int maxOut, uint32_t* netAddress, uint16_t* port)
        {
            NSR_UNUSED(out);
            NSR_UNUSED(maxOut);
            if (netAddress) *netAddress = 0;
            if (port) *port = 0;
            return 0;
        }
        virtual SteamAPICall_t AssociateWithClan(CSteamID clanID)
        {
            NSR_UNUSED(clanID);
            struct Result { int m_eResult; };
            Result result{ 1 };
            return SteamCallResultManager::CreateCallResult(210, &result, sizeof(result), true);
        }
        virtual SteamAPICall_t ComputeNewPlayerCompatibility(CSteamID newPlayer)
        {
            struct Result
            {
                int m_eResult;
                int m_cPlayersThatDontLikeCandidate;
                int m_cPlayersThatCandidateDoesntLike;
                int m_cClanPlayersThatDontLikeCandidate;
                CSteamID m_SteamIDCandidate;
            };

            Result result{ 1, 0, 0, 0, newPlayer };
            return SteamCallResultManager::CreateCallResult(211, &result, sizeof(result), true);
        }
        virtual bool SendUserConnectAndAuthenticate(uint32_t ipClient, const void* authBlob, uint32_t authBlobSize, CSteamID* steamIDUser)
        {
            NSR_UNUSED(ipClient);
            NSR_UNUSED(authBlob);
            NSR_UNUSED(authBlobSize);

            CSteamID id = SteamIDManager::GetLocalSteamID();
            if (steamIDUser)
                *steamIDUser = id;

            struct Result
            {
                CSteamID m_SteamID;
                CSteamID m_OwnerSteamID;
            };

            Result result{ id, id };
            SteamCallbackManager::PushCallback(201, &result, sizeof(result));
            return true;
        }
        virtual CSteamID CreateUnauthenticatedUserConnection()
        {
            return ++m_NextUnauthenticatedUser;
        }
        virtual void SendUserDisconnect(CSteamID steamIDUser) { NSR_UNUSED(steamIDUser); }
        virtual bool BUpdateUserData(CSteamID steamIDUser, const char* playerName, uint32_t score)
        {
            NSR_UNUSED(steamIDUser);
            NSR_UNUSED(playerName);
            NSR_UNUSED(score);
            return true;
        }
        virtual void SetMasterServerHeartbeatInterval_DEPRECATED(int interval) { NSR_UNUSED(interval); }
        virtual void ForceMasterServerHeartbeat_DEPRECATED() {}

    private:
        bool m_LoggedOn = true;
        bool m_Dedicated = false;
        bool m_PasswordProtected = false;
        bool m_AdvertiseActive = false;
        int m_MaxPlayers = 8;
        int m_BotPlayers = 0;
        uint16_t m_SpectatorPort = 0;
        CSteamID m_NextUnauthenticatedUser = 76561199006070000ull;
        std::string m_Product = "NarutoStorm2Revived";
        std::string m_Description = "Naruto Storm 2 Revived";
        std::string m_ModDir;
        std::string m_ServerName = "Offline Session";
        std::string m_MapName;
        std::string m_SpectatorServerName;
        std::string m_GameTags;
        std::string m_GameData;
        std::string m_Region;
        std::map<std::string, std::string> m_KeyValues;
    };

    class OfflineSteamGameServerStats final
    {
    public:
        virtual SteamAPICall_t RequestUserStats(CSteamID steamIDUser)
        {
            struct Result
            {
                int m_eResult;
                CSteamID m_steamIDUser;
            };

            Result result{ 1, steamIDUser };
            return SteamCallResultManager::CreateCallResult(1800, &result, sizeof(result), true);
        }
        virtual bool GetUserStat(CSteamID steamIDUser, const char* name, int32_t* data)
        {
            NSR_UNUSED(steamIDUser);
            return SteamStatsLocal::GetInt(name, data);
        }
        virtual bool GetUserStat(CSteamID steamIDUser, const char* name, float* data)
        {
            NSR_UNUSED(steamIDUser);
            return SteamStatsLocal::GetFloat(name, data);
        }
        virtual bool GetUserAchievement(CSteamID steamIDUser, const char* name, bool* achieved)
        {
            NSR_UNUSED(steamIDUser);
            return SteamStatsLocal::GetAchievement(name, achieved);
        }
        virtual bool SetUserStat(CSteamID steamIDUser, const char* name, int32_t data)
        {
            NSR_UNUSED(steamIDUser);
            return SteamStatsLocal::SetInt(name, data);
        }
        virtual bool SetUserStat(CSteamID steamIDUser, const char* name, float data)
        {
            NSR_UNUSED(steamIDUser);
            return SteamStatsLocal::SetFloat(name, data);
        }
        virtual bool UpdateUserAvgRateStat(CSteamID steamIDUser, const char* name, float countThisSession, double sessionLength)
        {
            NSR_UNUSED(steamIDUser);
            if (sessionLength <= 0.0)
                return SteamStatsLocal::SetFloat(name, countThisSession);

            return SteamStatsLocal::SetFloat(name, static_cast<float>(countThisSession / sessionLength));
        }
        virtual bool SetUserAchievement(CSteamID steamIDUser, const char* name)
        {
            NSR_UNUSED(steamIDUser);
            return SteamStatsLocal::SetAchievement(name);
        }
        virtual bool ClearUserAchievement(CSteamID steamIDUser, const char* name)
        {
            NSR_UNUSED(steamIDUser);
            return SteamStatsLocal::ClearAchievement(name);
        }
        virtual SteamAPICall_t StoreUserStats(CSteamID steamIDUser)
        {
            SteamStatsLocal::Save();

            struct Result
            {
                int m_eResult;
                CSteamID m_steamIDUser;
            };

            Result result{ 1, steamIDUser };
            return SteamCallResultManager::CreateCallResult(1801, &result, sizeof(result), true);
        }
    };

    class OfflineSteamUtils final
    {
    public:
        virtual uint32_t GetSecondsSinceAppActive() { return static_cast<uint32_t>(GetTickCount64() / 1000); }
        virtual uint32_t GetSecondsSinceComputerActive() { return static_cast<uint32_t>(GetTickCount64() / 1000); }
        virtual int GetConnectedUniverse() { return 1; }
        virtual uint32_t GetServerRealTime() { return static_cast<uint32_t>(std::time(nullptr)); }
        virtual const char* GetIPCountry() { return "US"; }
        virtual bool GetImageSize(int image, uint32_t* width, uint32_t* height)
        {
            NSR_UNUSED(image);
            if (width) *width = 0;
            if (height) *height = 0;
            return false;
        }
        virtual bool GetImageRGBA(int image, uint8_t* dest, int destSize)
        {
            NSR_UNUSED(image);
            NSR_UNUSED(dest);
            NSR_UNUSED(destSize);
            return false;
        }
        virtual bool GetCSERIPPort(uint32_t* ip, uint16_t* port)
        {
            if (ip) *ip = 0x0100007F;
            if (port) *port = 0;
            return true;
        }
        virtual uint8_t GetCurrentBatteryPower() { return 255; }
        virtual uint32_t GetAppID() { return kStorm2AppId; }
        virtual void SetOverlayNotificationPosition(int pos) { NSR_UNUSED(pos); }
        virtual bool IsAPICallCompleted(SteamAPICall_t call, bool* failed)
        {
            if (failed) *failed = false;
            return SteamCallResultManager::IsCompleted(call);
        }
        virtual int GetAPICallFailureReason(SteamAPICall_t call)
        {
            NSR_UNUSED(call);
            return 0;
        }
        virtual bool GetAPICallResult(SteamAPICall_t call, void* callback, int callbackSize, int callbackExpected, bool* failed)
        {
            if (failed) *failed = false;
            int id = 0;
            bool ok = SteamCallResultManager::GetData(call, callback, callbackSize, &id);
            return ok && (callbackExpected == 0 || id == callbackExpected);
        }
        virtual void RunFrame() { SteamCallbackManager::RunCallbacks(); }
        virtual uint32_t GetIPCCallCount() { return 0; }
        virtual void SetWarningMessageHook(void* hook) { NSR_UNUSED(hook); }
        virtual bool IsOverlayEnabled() { return false; }
        virtual bool BOverlayNeedsPresent() { return false; }
        virtual SteamAPICall_t CheckFileSignature(const char* file)
        {
            NSR_UNUSED(file);
            return SteamCallResultManager::CreateCallResult(705);
        }
        virtual bool ShowGamepadTextInput(int mode, int lineMode, const char* description, uint32_t maxChars, const char* existing)
        {
            NSR_UNUSED(mode);
            NSR_UNUSED(lineMode);
            NSR_UNUSED(description);
            NSR_UNUSED(maxChars);
            NSR_UNUSED(existing);
            return false;
        }
        virtual uint32_t GetEnteredGamepadTextLength() { return 0; }
        virtual bool GetEnteredGamepadTextInput(char* text, uint32_t textSize)
        {
            if (text && textSize > 0) text[0] = '\0';
            return false;
        }
        virtual const char* GetSteamUILanguage() { return "english"; }
        virtual bool IsSteamRunningInVR() { return false; }
        virtual void SetOverlayNotificationInset(int x, int y) { NSR_UNUSED(x); NSR_UNUSED(y); }
        virtual bool IsSteamInBigPictureMode() { return false; }
        virtual void StartVRDashboard() {}
        virtual bool IsVRHeadsetStreamingEnabled() { return false; }
        virtual void SetVRHeadsetStreamingEnabled(bool enabled) { NSR_UNUSED(enabled); }
        virtual bool IsSteamChinaLauncher() { return false; }
        virtual bool InitFilterText(uint32_t options) { NSR_UNUSED(options); return true; }
        virtual int FilterText(int context, CSteamID source, const char* input, char* output, uint32_t outputSize)
        {
            NSR_UNUSED(context);
            NSR_UNUSED(source);
            CopyString(output, static_cast<int>(outputSize), input ? input : "");
            return 0;
        }
        virtual int GetIPv6ConnectivityState(int protocol) { NSR_UNUSED(protocol); return 0; }
        virtual bool IsSteamRunningOnSteamDeck() { return false; }
        virtual bool ShowFloatingGamepadTextInput(int mode, int x, int y, int w, int h)
        {
            NSR_UNUSED(mode); NSR_UNUSED(x); NSR_UNUSED(y); NSR_UNUSED(w); NSR_UNUSED(h);
            return false;
        }
        virtual void SetGameLauncherMode(bool launcherMode) { NSR_UNUSED(launcherMode); }
        virtual bool DismissFloatingGamepadTextInput() { return true; }
    };

    class OfflineSteamClient final
    {
    public:
        virtual HSteamPipe CreateSteamPipe() { return kLocalPipe; }
        virtual bool BReleaseSteamPipe(HSteamPipe pipe) { NSR_UNUSED(pipe); return true; }
        virtual HSteamUser ConnectToGlobalUser(HSteamPipe pipe) { NSR_UNUSED(pipe); return kLocalUser; }
        virtual HSteamUser CreateLocalUser(HSteamPipe* pipe, int accountType)
        {
            NSR_UNUSED(accountType);
            if (pipe) *pipe = kLocalPipe;
            return kLocalUser;
        }
        virtual void ReleaseUser(HSteamPipe pipe, HSteamUser user) { NSR_UNUSED(pipe); NSR_UNUSED(user); }
        virtual void* GetISteamUser(HSteamUser user, HSteamPipe pipe, const char* version) { NSR_UNUSED(user); NSR_UNUSED(pipe); NSR_UNUSED(version); return FakeSteamCore::User(); }
        virtual void* GetISteamGameServer(HSteamUser user, HSteamPipe pipe, const char* version) { NSR_UNUSED(user); NSR_UNUSED(pipe); NSR_UNUSED(version); return FakeSteamCore::GameServer(); }
        virtual void SetLocalIPBinding(const void* ip, uint16_t port) { NSR_UNUSED(ip); NSR_UNUSED(port); }
        virtual void* GetISteamFriends(HSteamUser user, HSteamPipe pipe, const char* version) { NSR_UNUSED(user); NSR_UNUSED(pipe); NSR_UNUSED(version); return FakeSteamInterfaces::Friends(); }
        virtual void* GetISteamUtils(HSteamPipe pipe, const char* version) { NSR_UNUSED(pipe); NSR_UNUSED(version); return FakeSteamCore::Utils(); }
        virtual void* GetISteamMatchmaking(HSteamUser user, HSteamPipe pipe, const char* version) { NSR_UNUSED(user); NSR_UNUSED(pipe); NSR_UNUSED(version); return FakeSteamInterfaces::Matchmaking(); }
        virtual void* GetISteamMatchmakingServers(HSteamUser user, HSteamPipe pipe, const char* version) { NSR_UNUSED(user); NSR_UNUSED(pipe); NSR_UNUSED(version); return FakeSteamInterfaces::MatchmakingServers(); }
        virtual void* GetISteamGenericInterface(HSteamUser user, HSteamPipe pipe, const char* version) { NSR_UNUSED(user); NSR_UNUSED(pipe); return RouteVersion(version); }
        virtual void* GetISteamUserStats(HSteamUser user, HSteamPipe pipe, const char* version) { NSR_UNUSED(user); NSR_UNUSED(pipe); NSR_UNUSED(version); return FakeSteamInterfaces::UserStats(); }
        virtual void* GetISteamGameServerStats(HSteamUser user, HSteamPipe pipe, const char* version) { NSR_UNUSED(user); NSR_UNUSED(pipe); NSR_UNUSED(version); return FakeSteamCore::GameServerStats(); }
        virtual void* GetISteamApps(HSteamUser user, HSteamPipe pipe, const char* version) { NSR_UNUSED(user); NSR_UNUSED(pipe); NSR_UNUSED(version); return FakeSteamCore::Apps(); }
        virtual void* GetISteamNetworking(HSteamUser user, HSteamPipe pipe, const char* version) { NSR_UNUSED(user); NSR_UNUSED(pipe); NSR_UNUSED(version); return NS2Revived_SteamNetworking(); }
        virtual void* GetISteamRemoteStorage(HSteamUser user, HSteamPipe pipe, const char* version) { NSR_UNUSED(user); NSR_UNUSED(pipe); NSR_UNUSED(version); return FakeSteamInterfaces::RemoteStorage(); }
        virtual void* GetISteamScreenshots(HSteamUser user, HSteamPipe pipe, const char* version) { NSR_UNUSED(user); NSR_UNUSED(pipe); NSR_UNUSED(version); return FakeSteamInterfaces::Screenshots(); }
        virtual void* GetISteamGameSearch(HSteamUser user, HSteamPipe pipe, const char* version) { NSR_UNUSED(user); NSR_UNUSED(pipe); NSR_UNUSED(version); return FakeSteamInterfaces::GameSearch(); }
        virtual void RunFrame() { SteamCallbackManager::RunCallbacks(); }
        virtual uint32_t GetIPCCallCount() { return 0; }
        virtual void SetWarningMessageHook(void* hook) { NSR_UNUSED(hook); }
        virtual bool BShutdownIfAllPipesClosed() { return false; }
        virtual void* GetISteamHTTP(HSteamUser user, HSteamPipe pipe, const char* version) { NSR_UNUSED(user); NSR_UNUSED(pipe); NSR_UNUSED(version); return FakeSteamInterfaces::HTTP(); }
        virtual void* DEPRECATED_GetISteamUnifiedMessages(HSteamUser user, HSteamPipe pipe, const char* version) { NSR_UNUSED(user); NSR_UNUSED(pipe); NSR_UNUSED(version); return nullptr; }
        virtual void* GetISteamController(HSteamUser user, HSteamPipe pipe, const char* version) { NSR_UNUSED(user); NSR_UNUSED(pipe); NSR_UNUSED(version); return FakeSteamInterfaces::Controller(); }
        virtual void* GetISteamUGC(HSteamUser user, HSteamPipe pipe, const char* version) { NSR_UNUSED(user); NSR_UNUSED(pipe); NSR_UNUSED(version); return FakeSteamInterfaces::UGC(); }
        virtual void* GetISteamAppList(HSteamUser user, HSteamPipe pipe, const char* version) { NSR_UNUSED(user); NSR_UNUSED(pipe); NSR_UNUSED(version); return FakeSteamInterfaces::AppList(); }
        virtual void* GetISteamMusic(HSteamUser user, HSteamPipe pipe, const char* version) { NSR_UNUSED(user); NSR_UNUSED(pipe); NSR_UNUSED(version); return FakeSteamInterfaces::Music(); }
        virtual void* GetISteamMusicRemote(HSteamUser user, HSteamPipe pipe, const char* version) { NSR_UNUSED(user); NSR_UNUSED(pipe); NSR_UNUSED(version); return FakeSteamInterfaces::MusicRemote(); }
        virtual void* GetISteamHTMLSurface(HSteamUser user, HSteamPipe pipe, const char* version) { NSR_UNUSED(user); NSR_UNUSED(pipe); NSR_UNUSED(version); return FakeSteamInterfaces::HTMLSurface(); }
        virtual void DEPRECATED_Set_SteamAPI_CPostAPIResultInProcess(void* fn) { NSR_UNUSED(fn); }
        virtual void DEPRECATED_Remove_SteamAPI_CPostAPIResultInProcess(void* fn) { NSR_UNUSED(fn); }
        virtual void Set_SteamAPI_CCheckCallbackRegisteredInProcess(void* fn) { NSR_UNUSED(fn); }
        virtual void* GetISteamInventory(HSteamUser user, HSteamPipe pipe, const char* version) { NSR_UNUSED(user); NSR_UNUSED(pipe); NSR_UNUSED(version); return FakeSteamInterfaces::Inventory(); }
        virtual void* GetISteamVideo(HSteamUser user, HSteamPipe pipe, const char* version) { NSR_UNUSED(user); NSR_UNUSED(pipe); NSR_UNUSED(version); return FakeSteamInterfaces::Video(); }
        virtual void* GetISteamParentalSettings(HSteamUser user, HSteamPipe pipe, const char* version) { NSR_UNUSED(user); NSR_UNUSED(pipe); NSR_UNUSED(version); return FakeSteamInterfaces::ParentalSettings(); }
        virtual void* GetISteamInput(HSteamUser user, HSteamPipe pipe, const char* version) { NSR_UNUSED(user); NSR_UNUSED(pipe); NSR_UNUSED(version); return FakeSteamInterfaces::Input(); }
        virtual void* GetISteamParties(HSteamUser user, HSteamPipe pipe, const char* version) { NSR_UNUSED(user); NSR_UNUSED(pipe); NSR_UNUSED(version); return FakeSteamInterfaces::Parties(); }
        virtual void* GetISteamRemotePlay(HSteamUser user, HSteamPipe pipe, const char* version) { NSR_UNUSED(user); NSR_UNUSED(pipe); NSR_UNUSED(version); return FakeSteamInterfaces::RemotePlay(); }
        virtual void DestroyAllInterfaces() {}
    };

    OfflineSteamClient g_Client;
    OfflineSteamUser g_User;
    OfflineSteamUtils g_Utils;
    OfflineSteamApps g_Apps;
    OfflineSteamGameServer g_GameServer;
    OfflineSteamGameServerStats g_GameServerStats;
}

namespace FakeSteamCore
{
    bool Init()
    {
        if (g_Initialized)
            return true;

        g_Initialized = true;
        g_InstallPath = ExeFolder();
        g_UserDataFolder = (std::filesystem::path(g_InstallPath) / "NartuoStorm2Revived" / "userdata").string();

        try
        {
            std::filesystem::create_directories(g_UserDataFolder);
        }
        catch (...)
        {
        }

        Logger::Info("FakeSteamCore initialized fully offline");
        return true;
    }

    void Shutdown()
    {
        g_Initialized = false;
        g_LastEncryptedTicket.clear();
    }

    HSteamUser UserHandle() { return kLocalUser; }
    HSteamPipe PipeHandle() { return kLocalPipe; }
    void* Client() { Init(); return &g_Client; }
    void* User() { Init(); return &g_User; }
    void* Utils() { Init(); return &g_Utils; }
    void* Apps() { Init(); return &g_Apps; }
    void* GameServer() { Init(); return &g_GameServer; }
    void* GameServerStats() { Init(); return &g_GameServerStats; }
    const char* InstallPath() { Init(); return g_InstallPath.c_str(); }
}

void* FakeSteamInterfaces::Client() { return FakeSteamCore::Client(); }
void* FakeSteamInterfaces::User() { return FakeSteamCore::User(); }
void* FakeSteamInterfaces::Utils() { return FakeSteamCore::Utils(); }
void* FakeSteamInterfaces::Apps() { return FakeSteamCore::Apps(); }
void* FakeSteamInterfaces::GameServer() { return FakeSteamCore::GameServer(); }
void* FakeSteamInterfaces::GameServerStats() { return FakeSteamCore::GameServerStats(); }
