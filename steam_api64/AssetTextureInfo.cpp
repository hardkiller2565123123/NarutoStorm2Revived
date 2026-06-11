#include "StdInc.h"
#include "AssetTextureInfo.h"
#include "AssetPreviewResolver.h"

#include <fstream>
#include <sstream>

namespace
{
    template<typename T>
    T ReadLE(const std::vector<unsigned char>& b, size_t off)
    {
        if (off + sizeof(T) > b.size()) return T{};
        T v{};
        std::memcpy(&v, b.data() + off, sizeof(T));
        return v;
    }

    bool ReadHead(const std::string& path, std::vector<unsigned char>& out)
    {
        out.clear();
        std::ifstream f(path, std::ios::binary);
        if (!f.is_open()) return false;
        out.resize(512);
        f.read(reinterpret_cast<char*>(out.data()), static_cast<std::streamsize>(out.size()));
        out.resize(static_cast<size_t>(f.gcount()));
        return true;
    }
}

namespace AssetTextureInfo
{
    bool Inspect(const AssetBrowser::AssetEntry& asset, TextureInfo& outInfo)
    {
        outInfo = TextureInfo{};

        std::string path;
        if (!AssetPreviewResolver::ResolveReadablePath(asset, path, true))
        {
            outInfo.Status = "No readable texture file.";
            return false;
        }

        std::vector<unsigned char> b;
        if (!ReadHead(path, b))
        {
            outInfo.Status = "Could not read texture.";
            return false;
        }

        if (b.size() >= 128 && std::memcmp(b.data(), "DDS ", 4) == 0)
        {
            outInfo.Valid = true;
            outInfo.Height = static_cast<int>(ReadLE<uint32_t>(b, 12));
            outInfo.Width = static_cast<int>(ReadLE<uint32_t>(b, 16));
            outInfo.MipCount = static_cast<int>(ReadLE<uint32_t>(b, 28));
            char fourcc[5]{};
            std::memcpy(fourcc, b.data() + 84, 4);
            outInfo.Format = fourcc;
            outInfo.Status = "DDS metadata loaded.";
            return true;
        }

        if (b.size() >= 4 && std::memcmp(b.data(), "NTP3", 4) == 0)
        {
            outInfo.Valid = true;
            outInfo.Format = "NUT/NTP3";
            outInfo.Status = "NUT header detected. Full NUT parser can be added next.";
            return true;
        }

        outInfo.Status = "Unknown texture format.";
        return false;
    }

    std::string BuildText(const TextureInfo& info)
    {
        std::ostringstream ss;
        ss << "Valid: " << (info.Valid ? "true" : "false") << "\\n";
        ss << "Status: " << info.Status << "\\n";
        ss << "Format: " << info.Format << "\\n";
        ss << "Size: " << info.Width << "x" << info.Height << "\\n";
        ss << "Mipmaps: " << info.MipCount << "\\n";
        return ss.str();
    }
}
