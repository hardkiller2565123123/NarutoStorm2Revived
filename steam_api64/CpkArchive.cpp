#include "StdInc.h"
#include "CpkArchive.h"
#include "Logger.h"
#include <fstream>
#include <sstream>

namespace
{
    std::string Normalize(std::string s)
    {
        std::replace(s.begin(), s.end(), '\\', '/');
        return s;
    }

    bool LooksLikeAssetPath(const std::string& s)
    {
        static const char* exts[] = { ".xfbin", ".nut", ".nud", ".dds", ".acb", ".awb", ".adx", ".bin", ".dat", ".lua", ".txt" };
        std::string lower = s;
        std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c){ return (char)std::tolower(c); });
        if (lower.find('/') == std::string::npos && lower.find('\\') == std::string::npos)
            return false;
        for (const char* e : exts)
        {
            if (lower.find(e) != std::string::npos)
                return true;
        }
        return false;
    }

    std::vector<std::string> ExtractAsciiPaths(const std::vector<char>& data)
    {
        std::vector<std::string> result;
        std::string cur;
        for (char ch : data)
        {
            unsigned char c = (unsigned char)ch;
            if ((c >= 32 && c < 127) || c == '/' || c == '\\')
            {
                cur.push_back((char)c);
                if (cur.size() > 260) cur.erase(cur.begin());
            }
            else
            {
                if (cur.size() >= 5 && LooksLikeAssetPath(cur))
                {
                    cur = Normalize(cur);
                    if (std::find(result.begin(), result.end(), cur) == result.end())
                        result.push_back(cur);
                }
                cur.clear();
            }
        }
        return result;
    }
}

namespace CpkArchive
{
    bool Load(const std::string& path, ArchiveInfo& outInfo)
    {
        outInfo = {};
        outInfo.Path = path;
        outInfo.Name = std::filesystem::path(path).filename().string();

        try
        {
            if (!std::filesystem::exists(path))
            {
                outInfo.Status = "File does not exist";
                return false;
            }

            outInfo.Size = std::filesystem::file_size(path);
            std::ifstream in(path, std::ios::binary);
            if (!in.is_open())
            {
                outInfo.Status = "Failed to open CPK";
                return false;
            }

            std::vector<char> sample;
            const size_t maxRead = (size_t)std::min<uint64_t>(outInfo.Size, 64ull * 1024ull * 1024ull);
            sample.resize(maxRead);
            in.read(sample.data(), (std::streamsize)sample.size());
            sample.resize((size_t)in.gcount());

            if (sample.size() >= 4 && std::memcmp(sample.data(), "CPK ", 4) != 0 && std::memcmp(sample.data(), "CPK", 3) != 0)
            {
                outInfo.Status = "Indexed with heuristic path scanner. Magic was not plain CPK.";
            }
            else
            {
                outInfo.Status = "Indexed with built-in heuristic CPK scanner.";
            }

            auto paths = ExtractAsciiPaths(sample);
            for (const auto& p : paths)
            {
                Entry e{};
                e.Path = p;
                e.Name = std::filesystem::path(p).filename().string();
                e.Offset = 0;
                e.Size = 0;
                e.Heuristic = true;
                outInfo.Entries.push_back(e);
            }

            outInfo.Valid = true;
            return true;
        }
        catch (const std::exception& e)
        {
            outInfo.Status = std::string("Exception: ") + e.what();
            Logger::Error("CpkArchive::Load failed: " + outInfo.Status);
            return false;
        }
    }

    bool DumpFile(const std::string& archivePath, const Entry& entry, const std::string& outPath)
    {
        try
        {
            std::filesystem::create_directories(std::filesystem::path(outPath).parent_path());
            std::ofstream out(outPath, std::ios::binary | std::ios::trunc);
            if (!out.is_open()) return false;
            out << "NS2 Revived CPK indexed entry\n";
            out << "Archive: " << archivePath << "\n";
            out << "Entry: " << entry.Path << "\n";
            out << "Name: " << entry.Name << "\n";
            out << "Offset: " << entry.Offset << "\n";
            out << "Size: " << entry.Size << "\n";
            out << "Note: This is an indexed entry placeholder. Full CPK binary extraction requires a complete TOC decoder.\n";
            return true;
        }
        catch (...) { return false; }
    }

    std::string BuildInfoText(const ArchiveInfo& info)
    {
        std::ostringstream ss;
        ss << "CPK Archive\n";
        ss << "Name: " << info.Name << "\n";
        ss << "Path: " << info.Path << "\n";
        ss << "Size: " << info.Size << "\n";
        ss << "Entries indexed: " << info.Entries.size() << "\n";
        ss << "Status: " << info.Status << "\n";
        return ss.str();
    }
}
