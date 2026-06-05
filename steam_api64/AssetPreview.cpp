#include "StdInc.h"
#include "AssetPreview.h"
#include "CpkArchive.h"
#include "Logger.h"

#include <fstream>
#include <sstream>
#include <iomanip>

namespace
{
    std::string ToLower(std::string s)
    {
        std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c){ return (char)std::tolower(c); });
        return s;
    }

    template<typename T>
    T ReadLE(const std::vector<unsigned char>& b, size_t off)
    {
        if (off + sizeof(T) > b.size()) return T{};
        T v{};
        std::memcpy(&v, b.data() + off, sizeof(T));
        return v;
    }

    std::string GetBestPath(const AssetBrowser::AssetEntry& asset, bool preferDumped)
    {
        if (preferDumped && !asset.DumpFullPath.empty() && std::filesystem::exists(asset.DumpFullPath))
            return asset.DumpFullPath;
        return asset.FullPath;
    }

    bool IsLikelyText(const std::vector<unsigned char>& b)
    {
        if (b.empty()) return false;
        size_t printable = 0;
        for (unsigned char c : b)
        {
            if (c == '\n' || c == '\r' || c == '\t' || (c >= 32 && c < 127))
                printable++;
        }
        return printable > b.size() * 8 / 10;
    }

    void FillDdsInfo(const std::string& path, AssetPreview::PreviewState& state)
    {
        std::vector<unsigned char> b;
        AssetPreview::ReadBytes(path, b, 256);
        state.Kind = AssetPreview::PreviewKind::TextureInfo;
        state.Title = "DDS Texture Info";
        if (b.size() < 128 || std::memcmp(b.data(), "DDS ", 4) != 0)
        {
            state.Status = "Not a DDS header or unsupported texture container";
            return;
        }

        uint32_t height = ReadLE<uint32_t>(b, 12);
        uint32_t width = ReadLE<uint32_t>(b, 16);
        uint32_t mip = ReadLE<uint32_t>(b, 28);
        char fourcc[5]{};
        std::memcpy(fourcc, b.data() + 84, 4);

        state.Width = (int)width;
        state.Height = (int)height;
        state.MipCount = (int)mip;
        state.Format = fourcc;
        state.Valid = true;

        std::ostringstream ss;
        ss << "DDS Texture\n";
        ss << "Path: " << path << "\n";
        ss << "Size: " << width << "x" << height << "\n";
        ss << "Mipmaps: " << mip << "\n";
        ss << "Format: " << fourcc << "\n";
        ss << "Note: This is metadata preview. D3D texture preview can be wired with CreateDDSTextureFromFile later.\n";
        state.Text = ss.str();
        state.Status = "DDS metadata loaded";
    }

    void FillXfbinInfo(const std::string& path, AssetPreview::PreviewState& state)
    {
        std::vector<unsigned char> b;
        AssetPreview::ReadBytes(path, b, 1024 * 1024);
        state.Kind = AssetPreview::PreviewKind::XfbinInfo;
        state.Title = "XFBIN Info";
        state.Bytes = b;

        std::ostringstream ss;
        ss << "XFBIN Quick Viewer\n";
        ss << "Path: " << path << "\n";
        ss << "Bytes loaded: " << b.size() << "\n";
        if (b.size() >= 4)
            ss << "Magic: " << std::string((const char*)b.data(), (const char*)b.data() + 4) << "\n";

        ss << "\nDetected markers:\n";
        std::string text((const char*)b.data(), (const char*)b.data() + b.size());
        const char* terms[] = { "nuccChunk", "nucc", "NUCC", "texture", "material", "model", "stage", "player", "camera", "anm" };
        std::vector<std::string> found;
        for (const char* t : terms)
        {
            size_t pos = 0;
            while ((pos = text.find(t, pos)) != std::string::npos && found.size() < 128)
            {
                size_t begin = pos;
                size_t end = pos;
                while (begin > 0 && (unsigned char)text[begin - 1] >= 32 && (unsigned char)text[begin - 1] < 127) begin--;
                while (end < text.size() && (unsigned char)text[end] >= 32 && (unsigned char)text[end] < 127) end++;
                std::string m = text.substr(begin, end - begin);
                if (!m.empty() && std::find(found.begin(), found.end(), m) == found.end())
                    found.push_back(m);
                pos += std::strlen(t);
            }
        }
        for (const auto& s : found)
            ss << "- " << s << "\n";

        state.Text = ss.str();
        state.Valid = true;
        state.Status = "XFBIN metadata scan loaded";
    }
}

namespace AssetPreview
{
    bool ReadBytes(const std::string& path, std::vector<unsigned char>& outBytes, size_t maxBytes)
    {
        outBytes.clear();
        std::ifstream file(path, std::ios::binary);
        if (!file.is_open()) return false;
        file.seekg(0, std::ios::end);
        size_t len = (size_t)std::max<std::streamoff>(0, file.tellg());
        file.seekg(0, std::ios::beg);
        len = std::min(len, maxBytes);
        outBytes.resize(len);
        if (len)
            file.read((char*)outBytes.data(), (std::streamsize)len);
        return true;
    }

    std::string BuildHex(const std::vector<unsigned char>& bytes, size_t maxBytes)
    {
        std::ostringstream ss;
        size_t count = std::min(bytes.size(), maxBytes);
        for (size_t i = 0; i < count; i += 16)
        {
            ss << std::hex << std::setw(8) << std::setfill('0') << i << "  ";
            for (size_t j = 0; j < 16; ++j)
            {
                if (i + j < count) ss << std::setw(2) << (int)bytes[i + j] << ' ';
                else ss << "   ";
            }
            ss << " ";
            for (size_t j = 0; j < 16 && i + j < count; ++j)
            {
                unsigned char c = bytes[i + j];
                ss << ((c >= 32 && c < 127) ? (char)c : '.');
            }
            ss << "\n";
        }
        return ss.str();
    }

    bool BuildPreview(const AssetBrowser::AssetEntry& asset, PreviewState& outState, bool preferDumped)
    {
        outState = PreviewState{};
        std::string path = GetBestPath(asset, preferDumped);
        outState.Title = asset.Name;

        std::string ext = ToLower(asset.Extension);
        if (ext == ".cpk")
        {
            CpkArchive::ArchiveInfo info;
            CpkArchive::Load(path, info);
            outState.Kind = PreviewKind::CpkInfo;
            std::ostringstream ss;
            ss << "CPK Archive\n" << asset.FullPath << "\n";
            ss << "Status: " << info.Status << "\n";
            ss << "Indexed entries: " << info.Entries.size() << "\n\n";
            for (size_t i = 0; i < std::min<size_t>(info.Entries.size(), 200); ++i)
                ss << info.Entries[i].Path << "\n";
            outState.Text = ss.str();
            outState.Valid = info.Valid;
            outState.Status = info.Status;
            return true;
        }

        if (ext == ".dds")
        {
            FillDdsInfo(path, outState);
            return true;
        }

        if (ext == ".xfbin")
        {
            FillXfbinInfo(path, outState);
            return true;
        }

        if (ext == ".nud")
        {
            ReadBytes(path, outState.Bytes, 1024 * 1024);
            outState.Kind = PreviewKind::ModelShell;
            outState.Title = "NUD Model Shell";
            outState.Text = "NUD model selected. Wireframe preview shell is available in the overlay. Full mesh decode/render requires NUD vertex/index parser wiring.";
            outState.Valid = true;
            outState.Status = "Model preview shell loaded";
            return true;
        }

        ReadBytes(path, outState.Bytes, 512 * 1024);
        if (IsLikelyText(outState.Bytes))
        {
            outState.Kind = PreviewKind::Text;
            outState.Text.assign((const char*)outState.Bytes.data(), (const char*)outState.Bytes.data() + outState.Bytes.size());
        }
        else
        {
            outState.Kind = PreviewKind::Hex;
            outState.Text = BuildHex(outState.Bytes, 8192);
        }
        outState.Valid = true;
        outState.Status = "Preview loaded";
        return true;
    }
}
