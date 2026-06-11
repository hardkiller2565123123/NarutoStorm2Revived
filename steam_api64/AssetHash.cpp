#include "StdInc.h"
#include "AssetHash.h"
#include "AssetPreviewResolver.h"

#include <fstream>
#include <sstream>
#include <iomanip>

namespace
{
    uint32_t g_CrcTable[256];
    bool g_CrcReady = false;

    void InitCrc()
    {
        if (g_CrcReady)
            return;

        for (uint32_t i = 0; i < 256; ++i)
        {
            uint32_t c = i;

            for (int j = 0; j < 8; ++j)
                c = (c & 1) ? (0xEDB88320u ^ (c >> 1)) : (c >> 1);

            g_CrcTable[i] = c;
        }

        g_CrcReady = true;
    }

    std::string Hex64(uint64_t value)
    {
        std::ostringstream ss;
        ss << std::hex << std::uppercase << std::setw(16) << std::setfill('0') << value;
        return ss.str();
    }
}

namespace AssetHash
{
    std::string ToHex32(uint32_t value)
    {
        std::ostringstream ss;
        ss << std::hex << std::uppercase << std::setw(8) << std::setfill('0') << value;
        return ss.str();
    }

    bool ComputeFile(const std::string& path, HashInfo& outInfo)
    {
        outInfo = HashInfo{};

        std::ifstream file(path, std::ios::binary);

        if (!file.is_open())
            return false;

        InitCrc();

        uint32_t crc = 0xFFFFFFFFu;
        uint64_t fnv = 14695981039346656037ull;
        uint64_t total = 0;
        char buffer[64 * 1024];

        while (file)
        {
            file.read(buffer, sizeof(buffer));
            std::streamsize got = file.gcount();

            if (got <= 0)
                break;

            total += static_cast<uint64_t>(got);

            for (std::streamsize i = 0; i < got; ++i)
            {
                unsigned char b = static_cast<unsigned char>(buffer[i]);
                crc = g_CrcTable[(crc ^ b) & 0xFF] ^ (crc >> 8);
                fnv ^= b;
                fnv *= 1099511628211ull;
            }
        }

        outInfo.Valid = true;
        outInfo.CRC32 = crc ^ 0xFFFFFFFFu;
        outInfo.FNV1A64 = Hex64(fnv);
        outInfo.SizeText = std::to_string(total) + " bytes";
        return true;
    }

    bool ComputeAsset(const AssetBrowser::AssetEntry& asset, HashInfo& outInfo)
    {
        std::string path;

        if (!AssetPreviewResolver::ResolveReadablePath(asset, path, false))
            return false;

        return ComputeFile(path, outInfo);
    }

    std::string BuildHashText(const HashInfo& info)
    {
        if (!info.Valid)
            return "Hash unavailable";

        std::ostringstream ss;
        ss << "CRC32: " << ToHex32(info.CRC32) << "\n";
        ss << "FNV1A64: " << info.FNV1A64 << "\n";
        ss << "Size: " << info.SizeText << "\n";
        return ss.str();
    }
}
