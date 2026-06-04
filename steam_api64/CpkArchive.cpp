#include "StdInc.h"
#include "CpkArchive.h"
#include "Logger.h"

#include <fstream>
#include <set>

namespace
{
    std::string g_LastError;

    uint16_t ReadBE16(const std::vector<uint8_t>& d, size_t p)
    {
        if (p + 2 > d.size()) return 0;
        return static_cast<uint16_t>((d[p] << 8) | d[p + 1]);
    }

    uint32_t ReadBE32(const std::vector<uint8_t>& d, size_t p)
    {
        if (p + 4 > d.size()) return 0;
        return (static_cast<uint32_t>(d[p]) << 24) |
               (static_cast<uint32_t>(d[p + 1]) << 16) |
               (static_cast<uint32_t>(d[p + 2]) << 8) |
               static_cast<uint32_t>(d[p + 3]);
    }

    uint64_t ReadBE64(const std::vector<uint8_t>& d, size_t p)
    {
        uint64_t hi = ReadBE32(d, p);
        uint64_t lo = ReadBE32(d, p + 4);
        return (hi << 32) | lo;
    }

    std::string ReadCString(const std::vector<uint8_t>& d, size_t p, size_t end)
    {
        std::string s;
        while (p < d.size() && p < end && d[p] != 0)
            s.push_back(static_cast<char>(d[p++]));
        return s;
    }

    std::string Normalize(std::string s)
    {
        std::replace(s.begin(), s.end(), '\\', '/');
        while (!s.empty() && s[0] == '/') s.erase(s.begin());
        return s;
    }

    bool ReadFile(const std::string& path, std::vector<uint8_t>& out)
    {
        std::ifstream f(path, std::ios::binary);
        if (!f) return false;
        f.seekg(0, std::ios::end);
        size_t size = static_cast<size_t>(f.tellg());
        f.seekg(0, std::ios::beg);
        out.resize(size);
        if (size > 0) f.read(reinterpret_cast<char*>(out.data()), size);
        return true;
    }

    enum class UtfType : uint8_t
    {
        U8 = 0x00, S8 = 0x01, U16 = 0x02, S16 = 0x03,
        U32 = 0x04, S32 = 0x05, U64 = 0x06, S64 = 0x07,
        Float = 0x08, String = 0x0A, Bytes = 0x0B
    };

    struct UtfColumn
    {
        std::string Name;
        uint8_t Flags = 0;
        uint8_t Storage = 0;
        UtfType Type = UtfType::U32;
        uint64_t ConstInt = 0;
        std::string ConstString;
    };

    size_t TypeSize(UtfType t)
    {
        switch (t)
        {
        case UtfType::U8:
        case UtfType::S8: return 1;
        case UtfType::U16:
        case UtfType::S16: return 2;
        case UtfType::U32:
        case UtfType::S32:
        case UtfType::Float:
        case UtfType::String: return 4;
        case UtfType::U64:
        case UtfType::S64: return 8;
        default: return 4;
        }
    }

    uint64_t ReadTypedInt(const std::vector<uint8_t>& d, size_t& p, UtfType type)
    {
        uint64_t v = 0;
        switch (type)
        {
        case UtfType::U8:
        case UtfType::S8: v = p < d.size() ? d[p] : 0; p += 1; break;
        case UtfType::U16:
        case UtfType::S16: v = ReadBE16(d, p); p += 2; break;
        case UtfType::U32:
        case UtfType::S32:
        case UtfType::String: v = ReadBE32(d, p); p += 4; break;
        case UtfType::U64:
        case UtfType::S64: v = ReadBE64(d, p); p += 8; break;
        default: p += TypeSize(type); break;
        }
        return v;
    }

    bool ParseUtfAt(const std::vector<uint8_t>& data, size_t tableOffset, std::vector<CpkArchive::CpkEntry>& outEntries, const std::string& archivePath)
    {
        if (tableOffset + 0x20 >= data.size()) return false;
        if (memcmp(data.data() + tableOffset, "@UTF", 4) != 0) return false;

        uint32_t tableSize = ReadBE32(data, tableOffset + 4);
        uint32_t rowsOffset = ReadBE32(data, tableOffset + 8);
        uint32_t stringsOffset = ReadBE32(data, tableOffset + 12);
        uint32_t dataOffset = ReadBE32(data, tableOffset + 16);
        uint16_t columns = ReadBE16(data, tableOffset + 24);
        uint16_t rowWidth = ReadBE16(data, tableOffset + 26);
        uint32_t rows = ReadBE32(data, tableOffset + 28);

        size_t tableEnd = std::min(data.size(), tableOffset + 8 + static_cast<size_t>(tableSize));
        size_t schemaPos = tableOffset + 0x20;
        size_t stringBase = tableOffset + 8 + stringsOffset;
        size_t rowBase = tableOffset + 8 + rowsOffset;

        if (columns == 0 || rows == 0 || columns > 256 || rows > 200000) return false;
        if (stringBase >= data.size() || rowBase >= data.size()) return false;

        std::vector<UtfColumn> cols;
        for (uint16_t i = 0; i < columns && schemaPos < tableEnd; ++i)
        {
            UtfColumn c{};
            c.Flags = data[schemaPos++];
            c.Storage = c.Flags & 0xF0;
            c.Type = static_cast<UtfType>(c.Flags & 0x0F);
            uint32_t nameOff = ReadBE32(data, schemaPos);
            schemaPos += 4;
            c.Name = ReadCString(data, stringBase + nameOff, tableEnd);

            if (c.Storage == 0x30 || c.Storage == 0x10) // constant / zero-ish storage in common UTF tables
            {
                if (c.Type == UtfType::String)
                {
                    uint32_t strOff = ReadBE32(data, schemaPos);
                    schemaPos += 4;
                    c.ConstString = ReadCString(data, stringBase + strOff, tableEnd);
                }
                else
                {
                    c.ConstInt = ReadTypedInt(data, schemaPos, c.Type);
                }
            }

            cols.push_back(c);
        }

        auto getInt = [&](size_t row, const char* name) -> uint64_t
        {
            size_t p = rowBase + row * rowWidth;
            for (const auto& c : cols)
            {
                if (c.Storage == 0x30 || c.Storage == 0x10)
                {
                    if (_stricmp(c.Name.c_str(), name) == 0) return c.ConstInt;
                    continue;
                }
                if (c.Storage == 0x50 || c.Storage == 0x70 || c.Storage == 0x00)
                {
                    size_t valuePos = p;
                    uint64_t v = ReadTypedInt(data, p, c.Type);
                    if (_stricmp(c.Name.c_str(), name) == 0) return v;
                }
            }
            return 0;
        };

        auto getString = [&](size_t row, const char* name) -> std::string
        {
            size_t p = rowBase + row * rowWidth;
            for (const auto& c : cols)
            {
                if ((c.Storage == 0x30 || c.Storage == 0x10) && c.Type == UtfType::String)
                {
                    if (_stricmp(c.Name.c_str(), name) == 0) return c.ConstString;
                    continue;
                }
                if (c.Storage == 0x50 || c.Storage == 0x70 || c.Storage == 0x00)
                {
                    uint64_t v = ReadTypedInt(data, p, c.Type);
                    if (_stricmp(c.Name.c_str(), name) == 0 && c.Type == UtfType::String)
                        return ReadCString(data, stringBase + static_cast<size_t>(v), tableEnd);
                }
            }
            return {};
        };

        size_t before = outEntries.size();
        for (uint32_t r = 0; r < rows; ++r)
        {
            std::string file = getString(r, "FileName");
            std::string dir = getString(r, "DirName");
            if (file.empty()) file = getString(r, "FilePath");
            if (file.empty()) continue;

            CpkArchive::CpkEntry e{};
            e.ArchivePath = archivePath;
            e.Name = file;
            e.Directory = dir;
            e.VirtualPath = Normalize(dir.empty() ? file : (dir + "/" + file));
            e.Size = getInt(r, "FileSize");
            e.ExtractSize = getInt(r, "ExtractSize");
            e.Offset = getInt(r, "FileOffset");
            if (e.Offset == 0) e.Offset = getInt(r, "Offset");
            e.Compressed = e.ExtractSize > e.Size && e.Size > 0;
            e.HasPhysicalData = e.Offset > 0 && e.Size > 0;
            outEntries.push_back(e);
        }

        return outEntries.size() > before;
    }

    void FallbackStringScan(const std::vector<uint8_t>& data, CpkArchive::CpkInfo& info)
    {
        static const std::vector<std::string> exts = { ".xfbin", ".nut", ".nud", ".dds", ".acb", ".awb", ".adx", ".bin", ".dat" };
        std::set<std::string> found;
        std::string current;
        for (uint8_t b : data)
        {
            if (b >= 32 && b <= 126) current.push_back(static_cast<char>(b));
            else
            {
                if (current.size() >= 4)
                {
                    std::string lower = current;
                    std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c){ return static_cast<char>(std::tolower(c)); });
                    for (const auto& ext : exts)
                    {
                        auto pos = lower.rfind(ext);
                        if (pos != std::string::npos)
                        {
                            std::string s = current.substr(0, pos + ext.size());
                            if (s.find('/') != std::string::npos || s.find('\\') != std::string::npos)
                                found.insert(Normalize(s));
                        }
                    }
                }
                current.clear();
            }
        }
        for (const auto& v : found)
        {
            CpkArchive::CpkEntry e{};
            e.ArchivePath = info.Path;
            e.VirtualPath = v;
            e.Name = std::filesystem::path(v).filename().string();
            e.Directory = Normalize(std::filesystem::path(v).parent_path().string());
            e.HasPhysicalData = false;
            info.Entries.push_back(e);
        }
    }
}

namespace CpkArchive
{
    bool Load(const std::string& path, CpkInfo& outInfo)
    {
        g_LastError.clear();
        outInfo = {};
        outInfo.Path = path;
        outInfo.Name = std::filesystem::path(path).filename().string();
        try { outInfo.Size = std::filesystem::file_size(path); } catch (...) {}

        std::vector<uint8_t> data;
        if (!ReadFile(path, data)) { g_LastError = "failed to read CPK"; return false; }
        if (data.size() < 4 || memcmp(data.data(), "CPK ", 4) != 0)
        {
            g_LastError = "not a CPK file";
            return false;
        }

        // Find and parse all @UTF tables. The TOC one contains FileName/DirName rows.
        for (size_t i = 0; i + 4 < data.size(); ++i)
        {
            if (memcmp(data.data() + i, "@UTF", 4) == 0)
                ParseUtfAt(data, i, outInfo.Entries, path);
        }

        if (outInfo.Entries.empty())
            FallbackStringScan(data, outInfo);

        outInfo.Parsed = !outInfo.Entries.empty();
        Logger::Info("CpkArchive loaded " + outInfo.Name + " entries=" + std::to_string(outInfo.Entries.size()));
        return true;
    }

    bool DumpEntry(const CpkEntry& entry, const std::string& dumpRoot, std::string& outPath)
    {
        g_LastError.clear();
        if (!entry.HasPhysicalData)
        {
            g_LastError = "entry has no physical offset; list-only entry";
            return false;
        }

        std::ifstream in(entry.ArchivePath, std::ios::binary);
        if (!in) { g_LastError = "failed to open archive"; return false; }

        std::filesystem::path out = std::filesystem::path(dumpRoot) / "CPK" / std::filesystem::path(entry.ArchivePath).stem() / entry.VirtualPath;
        std::filesystem::create_directories(out.parent_path());

        in.seekg(static_cast<std::streamoff>(entry.Offset), std::ios::beg);
        std::vector<char> buffer(static_cast<size_t>(entry.Size));
        in.read(buffer.data(), buffer.size());

        std::ofstream file(out, std::ios::binary);
        if (!file) { g_LastError = "failed to create output"; return false; }
        file.write(buffer.data(), buffer.size());
        outPath = out.string();
        return true;
    }

    bool DumpArchive(const CpkInfo& info, const std::string& dumpRoot, std::string& outPath)
    {
        g_LastError.clear();
        try
        {
            std::filesystem::path out = std::filesystem::path(dumpRoot) / "Packages" / std::filesystem::path(info.Path).filename();
            std::filesystem::create_directories(out.parent_path());
            std::filesystem::copy_file(info.Path, out, std::filesystem::copy_options::overwrite_existing);
            outPath = out.string();
            return true;
        }
        catch (const std::exception& e)
        {
            g_LastError = e.what();
            return false;
        }
    }

    const char* GetLastErrorText()
    {
        return g_LastError.c_str();
    }
}
