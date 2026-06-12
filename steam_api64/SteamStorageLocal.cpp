#include "StdInc.h"
#include "SteamStorageLocal.h"
#include "Logger.h"

#include <chrono>

static std::string GetStorageRoot()
{
    char path[MAX_PATH]{};
    GetModuleFileNameA(nullptr, path, MAX_PATH);

    std::string base = path;
    size_t slash = base.find_last_of("\\/");

    if (slash != std::string::npos)
        base = base.substr(0, slash + 1);

    std::string root = base + "NartuoStorm2Revived\\storage\\";
    CreateDirectoryA((base + "NartuoStorm2Revived").c_str(), nullptr);
    CreateDirectoryA(root.c_str(), nullptr);

    return root;
}

static std::string SafePath(const char* name)
{
    std::string file = name ? name : "unknown.bin";

    for (char& c : file)
    {
        if (c == '/' || c == '\\' || c == ':' || c == '*' || c == '?' || c == '"' || c == '<' || c == '>' || c == '|')
            c = '_';
    }

    return GetStorageRoot() + file;
}

static std::vector<std::filesystem::directory_entry> ListStoredFiles()
{
    std::vector<std::filesystem::directory_entry> files;
    std::error_code ec;
    std::filesystem::path root = GetStorageRoot();

    if (!std::filesystem::exists(root, ec))
        return files;

    for (const auto& item : std::filesystem::directory_iterator(root, ec))
    {
        if (!ec && item.is_regular_file(ec))
            files.push_back(item);
    }

    std::sort(
        files.begin(),
        files.end(),
        [](const auto& a, const auto& b)
        {
            return a.path().filename().string() < b.path().filename().string();
        });

    return files;
}

namespace SteamStorageLocal
{
    void Init()
    {
        GetStorageRoot();
        Logger::Info("SteamStorageLocal initialized");
    }

    bool WriteFile(const char* name, const void* data, int32_t size)
    {
        if (!data || size < 0)
            return false;

        std::ofstream file(SafePath(name), std::ios::binary | std::ios::trunc);

        if (!file.is_open())
            return false;

        file.write(reinterpret_cast<const char*>(data), size);

        Logger::Info(std::string("SteamStorageLocal wrote ") + (name ? name : "null"));

        return true;
    }

    int32_t ReadFile(const char* name, void* data, int32_t maxSize)
    {
        if (!data || maxSize <= 0)
            return 0;

        std::ifstream file(SafePath(name), std::ios::binary);

        if (!file.is_open())
            return 0;

        file.read(reinterpret_cast<char*>(data), maxSize);

        return static_cast<int32_t>(file.gcount());
    }

    bool Exists(const char* name)
    {
        std::ifstream file(SafePath(name), std::ios::binary);
        return file.good();
    }

    bool DeleteFileLocal(const char* name)
    {
        return DeleteFileA(SafePath(name).c_str()) != FALSE;
    }

    int32_t Size(const char* name)
    {
        std::ifstream file(SafePath(name), std::ios::binary | std::ios::ate);

        if (!file.is_open())
            return 0;

        return static_cast<int32_t>(file.tellg());
    }

    int64_t Timestamp(const char* name)
    {
        std::error_code ec;
        auto time = std::filesystem::last_write_time(SafePath(name), ec);

        if (ec)
            return 0;

        auto seconds = std::chrono::duration_cast<std::chrono::seconds>(
            time.time_since_epoch()).count();

        return static_cast<int64_t>(seconds);
    }

    int32_t FileCount()
    {
        return static_cast<int32_t>(ListStoredFiles().size());
    }

    const char* FileNameAndSize(int index, int32_t* size)
    {
        static std::string s_ReturnName;
        auto files = ListStoredFiles();

        if (index < 0 || index >= static_cast<int>(files.size()))
        {
            if (size) *size = 0;
            s_ReturnName.clear();
            return s_ReturnName.c_str();
        }

        const auto& file = files[index];
        std::error_code ec;

        if (size)
            *size = static_cast<int32_t>(file.file_size(ec));

        s_ReturnName = file.path().filename().string();
        return s_ReturnName.c_str();
    }
}
