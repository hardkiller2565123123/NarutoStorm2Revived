#include "StdInc.h"
#include "FakeSteamInterfaces.h"
#include "Logger.h"
#include "SteamLobbyManager.h"
#include "SteamSessionManager.h"
#include "SteamCallResultManager.h"
#include "SteamCallbackManager.h"

static CSteamID g_LastLobbyListResult = 0;

class FakeSteamMatchmaking final
{
public:
    virtual int GetFavoriteGameCount()
    {
        Logger::Info("SteamMatchmaking::GetFavoriteGameCount");
        return 0;
    }

    virtual bool GetFavoriteGame(
        int game,
        AppId_t* appID,
        uint32_t* ip,
        uint16_t* connPort,
        uint16_t* queryPort,
        uint32_t* flags,
        uint32_t* lastPlayed)
    {
        NSR_UNUSED(game);

        Logger::Info("SteamMatchmaking::GetFavoriteGame");

        if (appID) *appID = 543870;
        if (ip) *ip = 0;
        if (connPort) *connPort = 0;
        if (queryPort) *queryPort = 0;
        if (flags) *flags = 0;
        if (lastPlayed) *lastPlayed = 0;

        return false;
    }

    virtual int AddFavoriteGame(
        AppId_t appID,
        uint32_t ip,
        uint16_t connPort,
        uint16_t queryPort,
        uint32_t flags,
        uint32_t lastPlayed)
    {
        NSR_UNUSED(appID);
        NSR_UNUSED(ip);
        NSR_UNUSED(connPort);
        NSR_UNUSED(queryPort);
        NSR_UNUSED(flags);
        NSR_UNUSED(lastPlayed);

        Logger::Info("SteamMatchmaking::AddFavoriteGame");
        return 0;
    }

    virtual bool RemoveFavoriteGame(
        AppId_t appID,
        uint32_t ip,
        uint16_t connPort,
        uint16_t queryPort,
        uint32_t flags)
    {
        NSR_UNUSED(appID);
        NSR_UNUSED(ip);
        NSR_UNUSED(connPort);
        NSR_UNUSED(queryPort);
        NSR_UNUSED(flags);

        Logger::Info("SteamMatchmaking::RemoveFavoriteGame");
        return true;
    }

    virtual SteamAPICall_t RequestLobbyList()
    {
        Logger::Info("SteamMatchmaking::RequestLobbyList");

        if (g_LastLobbyListResult == 0)
            g_LastLobbyListResult = SteamLobbyManager::CreateLobby(8);

        uint32_t lobbyCount = 1;

        SteamCallbackManager::PushCallback(510, &lobbyCount, sizeof(lobbyCount));

        return SteamCallResultManager::CreateCallResult(
            510,
            &lobbyCount,
            sizeof(lobbyCount),
            false);
    }

    virtual void AddRequestLobbyListStringFilter(
        const char* key,
        const char* value,
        int comparison)
    {
        NSR_UNUSED(comparison);

        Logger::Info(
            std::string("SteamMatchmaking::AddRequestLobbyListStringFilter ") +
            (key ? key : "null") +
            "=" +
            (value ? value : "null"));
    }

    virtual void AddRequestLobbyListNumericalFilter(
        const char* key,
        int value,
        int comparison)
    {
        NSR_UNUSED(key);
        NSR_UNUSED(value);
        NSR_UNUSED(comparison);

        Logger::Info("SteamMatchmaking::AddRequestLobbyListNumericalFilter");
    }

    virtual void AddRequestLobbyListNearValueFilter(
        const char* key,
        int value)
    {
        NSR_UNUSED(key);
        NSR_UNUSED(value);

        Logger::Info("SteamMatchmaking::AddRequestLobbyListNearValueFilter");
    }

    virtual void AddRequestLobbyListFilterSlotsAvailable(int slotsAvailable)
    {
        NSR_UNUSED(slotsAvailable);
        Logger::Info("SteamMatchmaking::AddRequestLobbyListFilterSlotsAvailable");
    }

    virtual void AddRequestLobbyListDistanceFilter(int distanceFilter)
    {
        NSR_UNUSED(distanceFilter);
        Logger::Info("SteamMatchmaking::AddRequestLobbyListDistanceFilter");
    }

    virtual void AddRequestLobbyListResultCountFilter(int maxResults)
    {
        NSR_UNUSED(maxResults);
        Logger::Info("SteamMatchmaking::AddRequestLobbyListResultCountFilter");
    }

    virtual void AddRequestLobbyListCompatibleMembersFilter(CSteamID lobbyID)
    {
        NSR_UNUSED(lobbyID);
        Logger::Info("SteamMatchmaking::AddRequestLobbyListCompatibleMembersFilter");
    }

    virtual CSteamID GetLobbyByIndex(int index)
    {
        Logger::Info("SteamMatchmaking::GetLobbyByIndex");

        if (index != 0)
            return 0;

        if (g_LastLobbyListResult == 0)
            g_LastLobbyListResult = SteamLobbyManager::CreateLobby(8);

        return g_LastLobbyListResult;
    }

    virtual SteamAPICall_t CreateLobby(int lobbyType, int maxMembers)
    {
        NSR_UNUSED(lobbyType);

        Logger::Info("SteamMatchmaking::CreateLobby");

        CSteamID lobbyID = SteamLobbyManager::CreateLobby(maxMembers);
        SteamSessionManager::JoinLobby(lobbyID);

        g_LastLobbyListResult = lobbyID;

        struct LobbyCreatedResult
        {
            int m_eResult;
            uint64_t m_ulSteamIDLobby;
        };

        LobbyCreatedResult result{};
        result.m_eResult = 1;
        result.m_ulSteamIDLobby = lobbyID;

        SteamCallbackManager::PushCallback(513, &result, sizeof(result));

        Logger::Info("SteamMatchmaking created lobby id=" + std::to_string(lobbyID));

        return SteamCallResultManager::CreateCallResult(
            513,
            &result,
            sizeof(result),
            false);
    }

    virtual SteamAPICall_t JoinLobby(CSteamID lobbyID)
    {
        Logger::Info("SteamMatchmaking::JoinLobby " + std::to_string(lobbyID));

        SteamLobbyManager::JoinLobby(lobbyID);
        SteamSessionManager::JoinLobby(lobbyID);

        struct LobbyEnterResult
        {
            uint64_t m_ulSteamIDLobby;
            uint32_t m_rgfChatPermissions;
            bool m_bLocked;
            uint32_t m_EChatRoomEnterResponse;
        };

        LobbyEnterResult result{};
        result.m_ulSteamIDLobby = lobbyID;
        result.m_rgfChatPermissions = 0;
        result.m_bLocked = false;
        result.m_EChatRoomEnterResponse = 1;

        SteamCallbackManager::PushCallback(504, &result, sizeof(result));

        return SteamCallResultManager::CreateCallResult(
            504,
            &result,
            sizeof(result),
            false);
    }

    virtual void LeaveLobby(CSteamID lobbyID)
    {
        Logger::Info("SteamMatchmaking::LeaveLobby " + std::to_string(lobbyID));

        SteamLobbyManager::LeaveLobby(lobbyID);
        SteamSessionManager::LeaveLobby(lobbyID);
    }

    virtual bool InviteUserToLobby(CSteamID lobbyID, CSteamID invitee)
    {
        NSR_UNUSED(lobbyID);
        NSR_UNUSED(invitee);

        Logger::Info("SteamMatchmaking::InviteUserToLobby");
        return false;
    }

    virtual int GetNumLobbyMembers(CSteamID lobbyID)
    {
        Logger::Info("SteamMatchmaking::GetNumLobbyMembers");
        return SteamLobbyManager::GetMemberCount(lobbyID);
    }

    virtual CSteamID GetLobbyMemberByIndex(CSteamID lobbyID, int member)
    {
        Logger::Info("SteamMatchmaking::GetLobbyMemberByIndex");
        return SteamLobbyManager::GetMemberByIndex(lobbyID, member);
    }

    virtual const char* GetLobbyData(CSteamID lobbyID, const char* key)
    {
        Logger::Info(std::string("SteamMatchmaking::GetLobbyData ") + (key ? key : "null"));
        return SteamLobbyManager::GetData(lobbyID, key);
    }

    virtual bool SetLobbyData(CSteamID lobbyID, const char* key, const char* value)
    {
        Logger::Info(std::string("SteamMatchmaking::SetLobbyData ") + (key ? key : "null"));
        return SteamLobbyManager::SetData(lobbyID, key, value);
    }

    virtual int GetLobbyDataCount(CSteamID lobbyID)
    {
        NSR_UNUSED(lobbyID);
        Logger::Info("SteamMatchmaking::GetLobbyDataCount");
        return 0;
    }

    virtual bool GetLobbyDataByIndex(
        CSteamID lobbyID,
        int index,
        char* key,
        int keySize,
        char* value,
        int valueSize)
    {
        NSR_UNUSED(lobbyID);
        NSR_UNUSED(index);

        Logger::Info("SteamMatchmaking::GetLobbyDataByIndex");

        if (key && keySize > 0)
            key[0] = '\0';

        if (value && valueSize > 0)
            value[0] = '\0';

        return false;
    }

    virtual bool DeleteLobbyData(CSteamID lobbyID, const char* key)
    {
        NSR_UNUSED(lobbyID);
        NSR_UNUSED(key);

        Logger::Info("SteamMatchmaking::DeleteLobbyData");
        return true;
    }

    virtual const char* GetLobbyMemberData(
        CSteamID lobbyID,
        CSteamID user,
        const char* key)
    {
        NSR_UNUSED(lobbyID);
        NSR_UNUSED(user);
        NSR_UNUSED(key);

        Logger::Info("SteamMatchmaking::GetLobbyMemberData");
        return "";
    }

    virtual void SetLobbyMemberData(
        CSteamID lobbyID,
        const char* key,
        const char* value)
    {
        NSR_UNUSED(lobbyID);
        NSR_UNUSED(key);
        NSR_UNUSED(value);

        Logger::Info("SteamMatchmaking::SetLobbyMemberData");
    }

    virtual bool SendLobbyChatMsg(
        CSteamID lobbyID,
        const void* msgBody,
        int msgBodySize)
    {
        NSR_UNUSED(lobbyID);
        NSR_UNUSED(msgBody);
        NSR_UNUSED(msgBodySize);

        Logger::Info("SteamMatchmaking::SendLobbyChatMsg");
        return true;
    }

    virtual int GetLobbyChatEntry(
        CSteamID lobbyID,
        int chatID,
        CSteamID* user,
        void* data,
        int dataSize,
        int* chatEntryType)
    {
        NSR_UNUSED(lobbyID);
        NSR_UNUSED(chatID);

        Logger::Info("SteamMatchmaking::GetLobbyChatEntry");

        if (user)
            *user = 0;

        if (data && dataSize > 0)
            reinterpret_cast<char*>(data)[0] = '\0';

        if (chatEntryType)
            *chatEntryType = 0;

        return 0;
    }

    virtual bool RequestLobbyData(CSteamID lobbyID)
    {
        NSR_UNUSED(lobbyID);
        Logger::Info("SteamMatchmaking::RequestLobbyData");
        return true;
    }

    virtual void SetLobbyGameServer(
        CSteamID lobbyID,
        uint32_t gameServerIP,
        uint16_t gameServerPort,
        CSteamID steamIDGameServer)
    {
        NSR_UNUSED(lobbyID);
        NSR_UNUSED(gameServerIP);
        NSR_UNUSED(gameServerPort);
        NSR_UNUSED(steamIDGameServer);

        Logger::Info("SteamMatchmaking::SetLobbyGameServer");
    }

    virtual bool GetLobbyGameServer(
        CSteamID lobbyID,
        uint32_t* gameServerIP,
        uint16_t* gameServerPort,
        CSteamID* steamIDGameServer)
    {
        Logger::Info("SteamMatchmaking::GetLobbyGameServer");

        if (gameServerIP)
            *gameServerIP = 0x7F000001;

        if (gameServerPort)
            *gameServerPort = 47584;

        if (steamIDGameServer)
            *steamIDGameServer = lobbyID;

        return true;
    }

    virtual bool SetLobbyMemberLimit(CSteamID lobbyID, int maxMembers)
    {
        NSR_UNUSED(lobbyID);
        NSR_UNUSED(maxMembers);

        Logger::Info("SteamMatchmaking::SetLobbyMemberLimit");
        return true;
    }

    virtual int GetLobbyMemberLimit(CSteamID lobbyID)
    {
        NSR_UNUSED(lobbyID);

        Logger::Info("SteamMatchmaking::GetLobbyMemberLimit");
        return 8;
    }

    virtual bool SetLobbyType(CSteamID lobbyID, int lobbyType)
    {
        NSR_UNUSED(lobbyID);
        NSR_UNUSED(lobbyType);

        Logger::Info("SteamMatchmaking::SetLobbyType");
        return true;
    }

    virtual bool SetLobbyJoinable(CSteamID lobbyID, bool joinable)
    {
        NSR_UNUSED(lobbyID);
        NSR_UNUSED(joinable);

        Logger::Info("SteamMatchmaking::SetLobbyJoinable");
        return true;
    }

    virtual CSteamID GetLobbyOwner(CSteamID lobbyID)
    {
        Logger::Info("SteamMatchmaking::GetLobbyOwner");
        return lobbyID;
    }

    virtual bool SetLobbyOwner(CSteamID lobbyID, CSteamID newOwner)
    {
        NSR_UNUSED(lobbyID);
        NSR_UNUSED(newOwner);

        Logger::Info("SteamMatchmaking::SetLobbyOwner");
        return true;
    }

    virtual bool SetLinkedLobby(CSteamID lobbyID, CSteamID linkedLobbyID)
    {
        NSR_UNUSED(lobbyID);
        NSR_UNUSED(linkedLobbyID);

        Logger::Info("SteamMatchmaking::SetLinkedLobby");
        return true;
    }
};

static FakeSteamMatchmaking g_Interface;

void* FakeSteamInterfaces::Matchmaking()
{
    Logger::Info("SteamMatchmaking: fixed SteamMatchMaking009 standalone interface returned");
    return &g_Interface;
}