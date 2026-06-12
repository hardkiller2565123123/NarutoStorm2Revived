#include "StdInc.h"
#include "FakeSteamInterfaces.h"
#include "Logger.h"
#include "SteamPersonaManager.h"

class FakeSteamFriends final
{
public:
    virtual const char* GetPersonaName()
    {
        Logger::Info("SteamFriends::GetPersonaName");
        return SteamPersonaManager::GetPersonaName();
    }

    virtual SteamAPICall_t SetPersonaName(const char* name)
    {
        Logger::Info("SteamFriends::SetPersonaName");

        if (name)
            SteamPersonaManager::SetPersonaName(name);

        return 0;
    }

    virtual int GetPersonaState()
    {
        Logger::Info("SteamFriends::GetPersonaState");
        return 1;
    }

    virtual int GetFriendCount(int flags)
    {
        NSR_UNUSED(flags);
        Logger::Info("SteamFriends::GetFriendCount");
        return SteamPersonaManager::GetFriendCount();
    }

    virtual CSteamID GetFriendByIndex(int index, int flags)
    {
        NSR_UNUSED(flags);
        Logger::Info("SteamFriends::GetFriendByIndex");
        return SteamPersonaManager::GetFriendByIndex(index);
    }

    virtual int GetFriendRelationship(CSteamID id)
    {
        NSR_UNUSED(id);
        Logger::Info("SteamFriends::GetFriendRelationship");
        return 0;
    }

    virtual int GetFriendPersonaState(CSteamID id)
    {
        NSR_UNUSED(id);
        Logger::Info("SteamFriends::GetFriendPersonaState");
        return 1;
    }

    virtual const char* GetFriendPersonaName(CSteamID id)
    {
        Logger::Info("SteamFriends::GetFriendPersonaName");
        return SteamPersonaManager::GetFriendPersonaName(id);
    }

    virtual bool GetFriendGamePlayed(CSteamID id, void* info)
    {
        NSR_UNUSED(id);
        Logger::Info("SteamFriends::GetFriendGamePlayed");

        if (info)
            memset(info, 0, 64);

        return false;
    }

    virtual const char* GetFriendPersonaNameHistory(CSteamID id, int index)
    {
        NSR_UNUSED(id);
        NSR_UNUSED(index);
        Logger::Info("SteamFriends::GetFriendPersonaNameHistory");
        return "";
    }

    virtual int GetFriendSteamLevel(CSteamID id)
    {
        NSR_UNUSED(id);
        Logger::Info("SteamFriends::GetFriendSteamLevel");
        return 0;
    }

    virtual const char* GetPlayerNickname(CSteamID id)
    {
        NSR_UNUSED(id);
        Logger::Info("SteamFriends::GetPlayerNickname");
        return "";
    }

    virtual int GetFriendsGroupCount()
    {
        Logger::Info("SteamFriends::GetFriendsGroupCount");
        return 0;
    }

    virtual int16_t GetFriendsGroupIDByIndex(int index)
    {
        NSR_UNUSED(index);
        Logger::Info("SteamFriends::GetFriendsGroupIDByIndex");
        return -1;
    }

    virtual const char* GetFriendsGroupName(int16_t group)
    {
        NSR_UNUSED(group);
        Logger::Info("SteamFriends::GetFriendsGroupName");
        return "";
    }

    virtual int GetFriendsGroupMembersCount(int16_t group)
    {
        NSR_UNUSED(group);
        Logger::Info("SteamFriends::GetFriendsGroupMembersCount");
        return 0;
    }

    virtual void GetFriendsGroupMembersList(
        int16_t group,
        CSteamID* outMembers,
        int count)
    {
        NSR_UNUSED(group);
        Logger::Info("SteamFriends::GetFriendsGroupMembersList");

        if (outMembers && count > 0)
            memset(outMembers, 0, sizeof(CSteamID) * count);
    }

    virtual bool HasFriend(CSteamID id, int flags)
    {
        NSR_UNUSED(flags);
        Logger::Info("SteamFriends::HasFriend");
        for (int i = 0; i < SteamPersonaManager::GetFriendCount(); ++i)
        {
            if (SteamPersonaManager::GetFriendByIndex(i) == id)
                return true;
        }

        return false;
    }

    virtual int GetClanCount()
    {
        Logger::Info("SteamFriends::GetClanCount");
        return 0;
    }

    virtual CSteamID GetClanByIndex(int index)
    {
        NSR_UNUSED(index);
        Logger::Info("SteamFriends::GetClanByIndex");
        return 0;
    }

    virtual const char* GetClanName(CSteamID id)
    {
        NSR_UNUSED(id);
        Logger::Info("SteamFriends::GetClanName");
        return "";
    }

    virtual const char* GetClanTag(CSteamID id)
    {
        NSR_UNUSED(id);
        Logger::Info("SteamFriends::GetClanTag");
        return "";
    }

    virtual bool GetClanActivityCounts(
        CSteamID id,
        int* online,
        int* inGame,
        int* chatting)
    {
        NSR_UNUSED(id);
        Logger::Info("SteamFriends::GetClanActivityCounts");

        if (online) *online = 0;
        if (inGame) *inGame = 0;
        if (chatting) *chatting = 0;

        return false;
    }

    virtual SteamAPICall_t DownloadClanActivityCounts(CSteamID* clans, int count)
    {
        NSR_UNUSED(clans);
        NSR_UNUSED(count);
        Logger::Info("SteamFriends::DownloadClanActivityCounts");
        return 0;
    }

    virtual int GetFriendCountFromSource(CSteamID source)
    {
        NSR_UNUSED(source);
        Logger::Info("SteamFriends::GetFriendCountFromSource");
        return 0;
    }

    virtual CSteamID GetFriendFromSourceByIndex(CSteamID source, int index)
    {
        NSR_UNUSED(source);
        NSR_UNUSED(index);
        Logger::Info("SteamFriends::GetFriendFromSourceByIndex");
        return 0;
    }

    virtual bool IsUserInSource(CSteamID user, CSteamID source)
    {
        NSR_UNUSED(user);
        NSR_UNUSED(source);
        Logger::Info("SteamFriends::IsUserInSource");
        return false;
    }

    virtual void SetInGameVoiceSpeaking(CSteamID user, bool speaking)
    {
        NSR_UNUSED(user);
        NSR_UNUSED(speaking);
        Logger::Info("SteamFriends::SetInGameVoiceSpeaking");
    }

    virtual void ActivateGameOverlay(const char* dialog)
    {
        NSR_UNUSED(dialog);
        Logger::Info("SteamFriends::ActivateGameOverlay");
    }

    virtual void ActivateGameOverlayToUser(const char* dialog, CSteamID id)
    {
        NSR_UNUSED(dialog);
        NSR_UNUSED(id);
        Logger::Info("SteamFriends::ActivateGameOverlayToUser");
    }

    virtual void ActivateGameOverlayToWebPage(const char* url)
    {
        NSR_UNUSED(url);
        Logger::Info("SteamFriends::ActivateGameOverlayToWebPage");
    }

    virtual void ActivateGameOverlayToStore(AppId_t appId, int flag)
    {
        NSR_UNUSED(appId);
        NSR_UNUSED(flag);
        Logger::Info("SteamFriends::ActivateGameOverlayToStore");
    }

    virtual void SetPlayedWith(CSteamID id)
    {
        NSR_UNUSED(id);
        Logger::Info("SteamFriends::SetPlayedWith");
    }

    virtual void ActivateGameOverlayInviteDialog(CSteamID lobby)
    {
        NSR_UNUSED(lobby);
        Logger::Info("SteamFriends::ActivateGameOverlayInviteDialog");
    }

    virtual int GetSmallFriendAvatar(CSteamID id)
    {
        NSR_UNUSED(id);
        Logger::Info("SteamFriends::GetSmallFriendAvatar");
        return 0;
    }

    virtual int GetMediumFriendAvatar(CSteamID id)
    {
        NSR_UNUSED(id);
        Logger::Info("SteamFriends::GetMediumFriendAvatar");
        return 0;
    }

    virtual int GetLargeFriendAvatar(CSteamID id)
    {
        NSR_UNUSED(id);
        Logger::Info("SteamFriends::GetLargeFriendAvatar");
        return 0;
    }

    virtual bool RequestUserInformation(CSteamID id, bool nameOnly)
    {
        NSR_UNUSED(id);
        NSR_UNUSED(nameOnly);
        Logger::Info("SteamFriends::RequestUserInformation");
        return false;
    }

    virtual SteamAPICall_t RequestClanOfficerList(CSteamID clan)
    {
        NSR_UNUSED(clan);
        Logger::Info("SteamFriends::RequestClanOfficerList");
        return 0;
    }

    virtual CSteamID GetClanOwner(CSteamID clan)
    {
        NSR_UNUSED(clan);
        Logger::Info("SteamFriends::GetClanOwner");
        return 0;
    }

    virtual int GetClanOfficerCount(CSteamID clan)
    {
        NSR_UNUSED(clan);
        Logger::Info("SteamFriends::GetClanOfficerCount");
        return 0;
    }

    virtual CSteamID GetClanOfficerByIndex(CSteamID clan, int index)
    {
        NSR_UNUSED(clan);
        NSR_UNUSED(index);
        Logger::Info("SteamFriends::GetClanOfficerByIndex");
        return 0;
    }

    virtual uint32_t GetUserRestrictions()
    {
        Logger::Info("SteamFriends::GetUserRestrictions");
        return 0;
    }

    virtual bool SetRichPresence(const char* key, const char* value)
    {
        Logger::Info("SteamFriends::SetRichPresence");

        if (key)
            SteamPersonaManager::SetRichPresence(key, value ? value : "");

        return true;
    }

    virtual void ClearRichPresence()
    {
        Logger::Info("SteamFriends::ClearRichPresence");
        SteamPersonaManager::ClearRichPresence();
    }

    virtual const char* GetFriendRichPresence(CSteamID id, const char* key)
    {
        NSR_UNUSED(id);
        Logger::Info("SteamFriends::GetFriendRichPresence");
        return SteamPersonaManager::GetRichPresence(key ? key : "");
    }

    virtual int GetFriendRichPresenceKeyCount(CSteamID id)
    {
        NSR_UNUSED(id);
        Logger::Info("SteamFriends::GetFriendRichPresenceKeyCount");
        return SteamPersonaManager::GetRichPresenceKeyCount();
    }

    virtual const char* GetFriendRichPresenceKeyByIndex(CSteamID id, int index)
    {
        NSR_UNUSED(id);
        Logger::Info("SteamFriends::GetFriendRichPresenceKeyByIndex");
        return SteamPersonaManager::GetRichPresenceKeyByIndex(index);
    }

    virtual void RequestFriendRichPresence(CSteamID id)
    {
        NSR_UNUSED(id);
        Logger::Info("SteamFriends::RequestFriendRichPresence");
    }

    virtual bool InviteUserToGame(CSteamID id, const char* connectString)
    {
        NSR_UNUSED(id);
        NSR_UNUSED(connectString);
        Logger::Info("SteamFriends::InviteUserToGame");
        return false;
    }

    virtual int GetCoplayFriendCount()
    {
        Logger::Info("SteamFriends::GetCoplayFriendCount");
        return 0;
    }

    virtual CSteamID GetCoplayFriend(int index)
    {
        NSR_UNUSED(index);
        Logger::Info("SteamFriends::GetCoplayFriend");
        return 0;
    }

    virtual int GetFriendCoplayTime(CSteamID id)
    {
        NSR_UNUSED(id);
        Logger::Info("SteamFriends::GetFriendCoplayTime");
        return 0;
    }

    virtual AppId_t GetFriendCoplayGame(CSteamID id)
    {
        NSR_UNUSED(id);
        Logger::Info("SteamFriends::GetFriendCoplayGame");
        return 0;
    }

    virtual SteamAPICall_t JoinClanChatRoom(CSteamID clan)
    {
        NSR_UNUSED(clan);
        Logger::Info("SteamFriends::JoinClanChatRoom");
        return 0;
    }

    virtual bool LeaveClanChatRoom(CSteamID clan)
    {
        NSR_UNUSED(clan);
        Logger::Info("SteamFriends::LeaveClanChatRoom");
        return true;
    }

    virtual int GetClanChatMemberCount(CSteamID clan)
    {
        NSR_UNUSED(clan);
        Logger::Info("SteamFriends::GetClanChatMemberCount");
        return 0;
    }

    virtual CSteamID GetChatMemberByIndex(CSteamID clan, int user)
    {
        NSR_UNUSED(clan);
        NSR_UNUSED(user);
        Logger::Info("SteamFriends::GetChatMemberByIndex");
        return 0;
    }

    virtual bool SendClanChatMessage(CSteamID chat, const char* text)
    {
        NSR_UNUSED(chat);
        NSR_UNUSED(text);
        Logger::Info("SteamFriends::SendClanChatMessage");
        return false;
    }

    virtual int GetClanChatMessage(
        CSteamID chat,
        int message,
        void* text,
        int maxText,
        int* entryType,
        CSteamID* chatter)
    {
        NSR_UNUSED(chat);
        NSR_UNUSED(message);
        Logger::Info("SteamFriends::GetClanChatMessage");

        if (text && maxText > 0)
            reinterpret_cast<char*>(text)[0] = '\0';

        if (entryType)
            *entryType = 0;

        if (chatter)
            *chatter = 0;

        return 0;
    }

    virtual bool IsClanChatAdmin(CSteamID chat, CSteamID user)
    {
        NSR_UNUSED(chat);
        NSR_UNUSED(user);
        Logger::Info("SteamFriends::IsClanChatAdmin");
        return false;
    }

    virtual bool IsClanChatWindowOpenInSteam(CSteamID chat)
    {
        NSR_UNUSED(chat);
        Logger::Info("SteamFriends::IsClanChatWindowOpenInSteam");
        return false;
    }

    virtual bool OpenClanChatWindowInSteam(CSteamID chat)
    {
        NSR_UNUSED(chat);
        Logger::Info("SteamFriends::OpenClanChatWindowInSteam");
        return false;
    }

    virtual bool CloseClanChatWindowInSteam(CSteamID chat)
    {
        NSR_UNUSED(chat);
        Logger::Info("SteamFriends::CloseClanChatWindowInSteam");
        return true;
    }

    virtual bool SetListenForFriendsMessages(bool enabled)
    {
        NSR_UNUSED(enabled);
        Logger::Info("SteamFriends::SetListenForFriendsMessages");
        return true;
    }

    virtual bool ReplyToFriendMessage(CSteamID id, const char* message)
    {
        NSR_UNUSED(id);
        NSR_UNUSED(message);
        Logger::Info("SteamFriends::ReplyToFriendMessage");
        return false;
    }

    virtual int GetFriendMessage(
        CSteamID id,
        int message,
        void* data,
        int dataSize,
        int* entryType)
    {
        NSR_UNUSED(id);
        NSR_UNUSED(message);
        Logger::Info("SteamFriends::GetFriendMessage");

        if (data && dataSize > 0)
            reinterpret_cast<char*>(data)[0] = '\0';

        if (entryType)
            *entryType = 0;

        return 0;
    }

    virtual SteamAPICall_t GetFollowerCount(CSteamID id)
    {
        NSR_UNUSED(id);
        Logger::Info("SteamFriends::GetFollowerCount");
        return 0;
    }

    virtual SteamAPICall_t IsFollowing(CSteamID id)
    {
        NSR_UNUSED(id);
        Logger::Info("SteamFriends::IsFollowing");
        return 0;
    }

    virtual SteamAPICall_t EnumerateFollowingList(uint32_t start)
    {
        NSR_UNUSED(start);
        Logger::Info("SteamFriends::EnumerateFollowingList");
        return 0;
    }
};

static FakeSteamFriends g_Interface;

void* FakeSteamInterfaces::Friends()
{
    Logger::Info("SteamFriends: fixed standalone emulated interface returned");
    return &g_Interface;
}
