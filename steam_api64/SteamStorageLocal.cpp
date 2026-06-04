#include "StdInc.h"
#include "SteamStorageLocal.h"
#include "Logger.h"

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
}