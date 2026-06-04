#include "StdInc.h"
#include "FakeSteamInterfaces.h"
#include "Logger.h"
#include "SteamStorageLocal.h"

class FakeSteamRemoteStorage final
{
public:
    virtual bool FileWrite(const char* file, const void* data, int32_t size)
    {
        Logger::Info(std::string("SteamRemoteStorage::FileWrite ") + (file ? file : "null"));
        return SteamStorageLocal::WriteFile(file, data, size);
    }

    virtual int32_t FileRead(const char* file, void* data, int32_t size)
    {
        Logger::Info(std::string("SteamRemoteStorage::FileRead ") + (file ? file : "null"));
        return SteamStorageLocal::ReadFile(file, data, size);
    }

    virtual SteamAPICall_t FileWriteAsync(const char* file, const void* data, uint32_t size)
    {
        Logger::Info(std::string("SteamRemoteStorage::FileWriteAsync ") + (file ? file : "null"));
        SteamStorageLocal::WriteFile(file, data, static_cast<int32_t>(size));
        return 1;
    }

    virtual SteamAPICall_t FileReadAsync(const char* file, uint32_t offset, uint32_t size)
    {
        NSR_UNUSED(file);
        NSR_UNUSED(offset);
        NSR_UNUSED(size);
        Logger::Info("SteamRemoteStorage::FileReadAsync");
        return 1;
    }

    virtual bool FileReadAsyncComplete(SteamAPICall_t call, void* data, uint32_t size)
    {
        NSR_UNUSED(call);
        NSR_UNUSED(data);
        NSR_UNUSED(size);
        Logger::Info("SteamRemoteStorage::FileReadAsyncComplete");
        return false;
    }

    virtual bool FileForget(const char* file)
    {
        NSR_UNUSED(file);
        Logger::Info("SteamRemoteStorage::FileForget");
        return true;
    }

    virtual bool FileDelete(const char* file)
    {
        Logger::Info(std::string("SteamRemoteStorage::FileDelete ") + (file ? file : "null"));
        return SteamStorageLocal::DeleteFileLocal(file);
    }

    virtual SteamAPICall_t FileShare(const char* file)
    {
        NSR_UNUSED(file);
        Logger::Info("SteamRemoteStorage::FileShare");
        return 1;
    }

    virtual bool SetSyncPlatforms(const char* file, int platforms)
    {
        NSR_UNUSED(file);
        NSR_UNUSED(platforms);
        Logger::Info("SteamRemoteStorage::SetSyncPlatforms");
        return true;
    }

    virtual int32_t GetFileSize(const char* file)
    {
        Logger::Info(std::string("SteamRemoteStorage::GetFileSize ") + (file ? file : "null"));
        return SteamStorageLocal::Size(file);
    }

    virtual int64_t GetFileTimestamp(const char* file)
    {
        NSR_UNUSED(file);
        Logger::Info("SteamRemoteStorage::GetFileTimestamp");
        return 0;
    }

    virtual int GetSyncPlatforms(const char* file)
    {
        NSR_UNUSED(file);
        Logger::Info("SteamRemoteStorage::GetSyncPlatforms");
        return 0;
    }

    virtual int32_t GetFileCount()
    {
        Logger::Info("SteamRemoteStorage::GetFileCount");
        return 0;
    }

    virtual const char* GetFileNameAndSize(int index, int32_t* size)
    {
        NSR_UNUSED(index);
        Logger::Info("SteamRemoteStorage::GetFileNameAndSize");
        if (size) *size = 0;
        return "";
    }

    virtual bool GetQuota(uint64_t* total, uint64_t* available)
    {
        Logger::Info("SteamRemoteStorage::GetQuota");
        if (total) *total = 1024ull * 1024ull * 1024ull;
        if (available) *available = 1024ull * 1024ull * 1024ull;
        return true;
    }

    virtual bool IsCloudEnabledForAccount()
    {
        Logger::Info("SteamRemoteStorage::IsCloudEnabledForAccount");
        return true;
    }

    virtual bool IsCloudEnabledForApp()
    {
        Logger::Info("SteamRemoteStorage::IsCloudEnabledForApp");
        return true;
    }

    virtual void SetCloudEnabledForApp(bool enabled)
    {
        NSR_UNUSED(enabled);
        Logger::Info("SteamRemoteStorage::SetCloudEnabledForApp");
    }

    virtual bool UGCDownload()
    {
        Logger::Info("SteamRemoteStorage::UGCDownload");
        return false;
    }
};

static FakeSteamRemoteStorage g_Interface;

void* FakeSteamInterfaces::RemoteStorage()
{
    Logger::Info("SteamRemoteStorage: local save interface returned");
    return &g_Interface;
}