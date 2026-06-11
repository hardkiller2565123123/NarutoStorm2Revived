#include "StdInc.h"
#include "CpkArchive.h"
#include "Logger.h"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <map>

namespace
{
    struct UtfValue
    {
        int Type = -1;
        uint64_t U64 = 0;
        double F64 = 0.0;
        std::string String;
        std::vector<unsigned char> Data;
        bool HasValue = false;
    };

    struct UtfColumn
    {
        uint8_t Flags = 0;
        std::string Name;
    };

    struct UtfTable
    {
        std::vector<UtfColumn> Columns;
        std::vector<std::map<std::string, UtfValue>> Rows;
    };

    uint16_t ReadBE16(const std::vector<unsigned char>& b, size_t off)
    {
        if (off + 2 > b.size())
            return 0;

        return static_cast<uint16_t>((b[off] << 8) | b[off + 1]);
    }

    uint32_t ReadBE32(const std::vector<unsigned char>& b, size_t off)
    {
        if (off + 4 > b.size())
            return 0;

        return
            (static_cast<uint32_t>(b[off]) << 24) |
            (static_cast<uint32_t>(b[off + 1]) << 16) |
            (static_cast<uint32_t>(b[off + 2]) << 8) |
            static_cast<uint32_t>(b[off + 3]);
    }

    uint64_t ReadBE64(const std::vector<unsigned char>& b, size_t off)
    {
        uint64_t hi = ReadBE32(b, off);
        uint64_t lo = ReadBE32(b, off + 4);
        return (hi << 32) | lo;
    }

    uint32_t ReadLE32(std::ifstream& f)
    {
        unsigned char b[4]{};
        f.read(reinterpret_cast<char*>(b), 4);
        return
            static_cast<uint32_t>(b[0]) |
            (static_cast<uint32_t>(b[1]) << 8) |
            (static_cast<uint32_t>(b[2]) << 16) |
            (static_cast<uint32_t>(b[3]) << 24);
    }

    uint64_t ReadLE64(std::ifstream& f)
    {
        unsigned char b[8]{};
        f.read(reinterpret_cast<char*>(b), 8);

        uint64_t v = 0;
        for (int i = 7; i >= 0; --i)
        {
            v <<= 8;
            v |= b[i];
        }

        return v;
    }

    std::string Normalize(std::string s)
    {
        std::replace(s.begin(), s.end(), '\\', '/');

        while (!s.empty() && s.front() == '/')
            s.erase(s.begin());

        return s;
    }

    std::string ToLower(std::string s)
    {
        std::transform(
            s.begin(),
            s.end(),
            s.begin(),
            [](unsigned char c)
            {
                return static_cast<char>(std::tolower(c));
            });

        return s;
    }

    bool FileExists(const std::filesystem::path& p)
    {
        std::error_code ec;
        return std::filesystem::exists(p, ec);
    }

    std::string ReadCStringFromBlock(
        const std::vector<unsigned char>& b,
        size_t off)
    {
        if (off >= b.size())
            return "";

        std::string out;

        for (size_t i = off; i < b.size(); ++i)
        {
            if (b[i] == 0)
                break;

            out.push_back(static_cast<char>(b[i]));
        }

        return out;
    }

    std::vector<unsigned char> DecryptUtf(const std::vector<unsigned char>& input)
    {
        std::vector<unsigned char> result(input.size());

        uint32_t m = 0x0000655f;
        uint32_t t = 0x00004115;

        for (size_t i = 0; i < input.size(); ++i)
        {
            result[i] = static_cast<unsigned char>(input[i] ^ static_cast<unsigned char>(m & 0xff));
            m *= t;
        }

        return result;
    }

    bool ReadUtfPacketAt(
        std::ifstream& file,
        uint64_t offset,
        std::vector<unsigned char>& outPacket)
    {
        outPacket.clear();

        file.seekg(static_cast<std::streamoff>(offset), std::ios::beg);

        char magic[4]{};
        file.read(magic, 4);

        if (std::memcmp(magic, "CPK ", 4) != 0 &&
            std::memcmp(magic, "TOC ", 4) != 0 &&
            std::memcmp(magic, "ITOC", 4) != 0 &&
            std::memcmp(magic, "ETOC", 4) != 0)
        {
            return false;
        }

        ReadLE32(file);
        uint64_t size = ReadLE64(file);

        if (size == 0 || size > 512ull * 1024ull * 1024ull)
            return false;

        outPacket.resize(static_cast<size_t>(size));
        file.read(reinterpret_cast<char*>(outPacket.data()), static_cast<std::streamsize>(outPacket.size()));

        if (outPacket.size() >= 4 &&
            !(outPacket[0] == '@' && outPacket[1] == 'U' && outPacket[2] == 'T' && outPacket[3] == 'F'))
        {
            outPacket = DecryptUtf(outPacket);
        }

        return outPacket.size() >= 4 &&
            outPacket[0] == '@' &&
            outPacket[1] == 'U' &&
            outPacket[2] == 'T' &&
            outPacket[3] == 'F';
    }

    bool ParseUtf(
        const std::vector<unsigned char>& packet,
        UtfTable& out)
    {
        out = UtfTable{};

        if (packet.size() < 0x20)
            return false;

        if (!(packet[0] == '@' && packet[1] == 'U' && packet[2] == 'T' && packet[3] == 'F'))
            return false;

        const size_t base = 0;

        uint32_t rowsOffset = ReadBE32(packet, 0x08) + 8;
        uint32_t stringsOffset = ReadBE32(packet, 0x0C) + 8;
        uint32_t dataOffset = ReadBE32(packet, 0x10) + 8;

        uint16_t numColumns = ReadBE16(packet, 0x18);
        uint16_t rowLength = ReadBE16(packet, 0x1A);
        uint32_t numRows = ReadBE32(packet, 0x1C);

        size_t pos = 0x20;

        for (uint16_t i = 0; i < numColumns; ++i)
        {
            if (pos >= packet.size())
                return false;

            UtfColumn col{};
            col.Flags = packet[pos++];

            uint32_t nameOff = ReadBE32(packet, pos);
            pos += 4;

            col.Name = ReadCStringFromBlock(packet, stringsOffset + nameOff);
            out.Columns.push_back(col);
        }

        for (uint32_t row = 0; row < numRows; ++row)
        {
            std::map<std::string, UtfValue> rowData;
            size_t rowPos = rowsOffset + row * rowLength;

            for (const auto& col : out.Columns)
            {
                uint8_t storage = col.Flags & 0xF0;
                uint8_t type = col.Flags & 0x0F;

                UtfValue val{};
                val.Type = type;

                if (storage == 0x00 || storage == 0x10)
                {
                    val.HasValue = false;
                    rowData[col.Name] = val;
                    continue;
                }

                if (storage == 0x30)
                {
                    val.HasValue = false;
                    rowData[col.Name] = val;
                    continue;
                }

                if (storage != 0x50)
                {
                    val.HasValue = false;
                    rowData[col.Name] = val;
                    continue;
                }

                if (rowPos >= packet.size())
                    return false;

                val.HasValue = true;

                switch (type)
                {
                case 0x00:
                case 0x01:
                    val.U64 = packet[rowPos];
                    rowPos += 1;
                    break;

                case 0x02:
                case 0x03:
                    val.U64 = ReadBE16(packet, rowPos);
                    rowPos += 2;
                    break;

                case 0x04:
                case 0x05:
                    val.U64 = ReadBE32(packet, rowPos);
                    rowPos += 4;
                    break;

                case 0x06:
                case 0x07:
                    val.U64 = ReadBE64(packet, rowPos);
                    rowPos += 8;
                    break;

                case 0x08:
                {
                    uint32_t raw = ReadBE32(packet, rowPos);
                    float f{};
                    std::memcpy(&f, &raw, sizeof(float));
                    val.F64 = f;
                    rowPos += 4;
                    break;
                }

                case 0x0A:
                {
                    uint32_t strOff = ReadBE32(packet, rowPos);
                    rowPos += 4;
                    val.String = ReadCStringFromBlock(packet, stringsOffset + strOff);
                    break;
                }

                case 0x0B:
                {
                    uint32_t blobOff = ReadBE32(packet, rowPos);
                    uint32_t blobSize = ReadBE32(packet, rowPos + 4);
                    rowPos += 8;

                    size_t start = dataOffset + blobOff;

                    if (start < packet.size())
                    {
                        size_t size = std::min<size_t>(blobSize, packet.size() - start);
                        val.Data.assign(packet.begin() + start, packet.begin() + start + size);
                    }

                    break;
                }

                default:
                    val.HasValue = false;
                    break;
                }

                rowData[col.Name] = val;
            }

            out.Rows.push_back(rowData);
        }

        return true;
    }

    bool HasValue(
        const std::map<std::string, UtfValue>& row,
        const std::string& key)
    {
        auto it = row.find(key);
        return it != row.end() && it->second.HasValue;
    }

    uint64_t GetU64(
        const std::map<std::string, UtfValue>& row,
        const std::string& key,
        uint64_t fallback = 0)
    {
        auto it = row.find(key);

        if (it == row.end() || !it->second.HasValue)
            return fallback;

        return it->second.U64;
    }

    std::string GetString(
        const std::map<std::string, UtfValue>& row,
        const std::string& key)
    {
        auto it = row.find(key);

        if (it == row.end() || !it->second.HasValue)
            return "";

        return it->second.String;
    }

    std::vector<unsigned char> DecompressCrilayla(
        const std::vector<unsigned char>& input)
    {
        if (input.size() < 0x20)
            return {};

        if (std::memcmp(input.data(), "CRILAYLA", 8) != 0)
            return input;

        uint32_t uncompressedSize =
            static_cast<uint32_t>(input[8]) |
            (static_cast<uint32_t>(input[9]) << 8) |
            (static_cast<uint32_t>(input[10]) << 16) |
            (static_cast<uint32_t>(input[11]) << 24);

        uint32_t headerOffset =
            static_cast<uint32_t>(input[12]) |
            (static_cast<uint32_t>(input[13]) << 8) |
            (static_cast<uint32_t>(input[14]) << 16) |
            (static_cast<uint32_t>(input[15]) << 24);

        std::vector<unsigned char> result(uncompressedSize + 0x100);

        if (headerOffset + 0x10 + 0x100 <= input.size())
            std::memcpy(result.data(), input.data() + headerOffset + 0x10, 0x100);

        int inputOffset = static_cast<int>(input.size()) - 0x100 - 1;
        int outputEnd = 0x100 + static_cast<int>(uncompressedSize) - 1;
        int bytesOutput = 0;

        unsigned char bitPool = 0;
        int bitsLeft = 0;

        auto getBits = [&](int bitCount) -> uint16_t
            {
                uint16_t outBits = 0;
                int produced = 0;

                while (produced < bitCount)
                {
                    if (bitsLeft == 0)
                    {
                        if (inputOffset < 0)
                            return outBits;

                        bitPool = input[inputOffset--];
                        bitsLeft = 8;
                    }

                    int bitsThisRound = std::min(bitsLeft, bitCount - produced);

                    outBits <<= bitsThisRound;
                    outBits |= static_cast<uint16_t>(
                        (bitPool >> (bitsLeft - bitsThisRound)) &
                        ((1 << bitsThisRound) - 1));

                    bitsLeft -= bitsThisRound;
                    produced += bitsThisRound;
                }

                return outBits;
            };

        const int vleLens[4] = { 2, 3, 5, 8 };

        while (bytesOutput < static_cast<int>(uncompressedSize))
        {
            if (getBits(1) > 0)
            {
                int backreferenceOffset =
                    outputEnd - bytesOutput + getBits(13) + 3;

                int backreferenceLength = 3;
                int level = 0;

                for (; level < 4; ++level)
                {
                    int thisLevel = getBits(vleLens[level]);
                    backreferenceLength += thisLevel;

                    if (thisLevel != ((1 << vleLens[level]) - 1))
                        break;
                }

                if (level == 4)
                {
                    int thisLevel = 0;

                    do
                    {
                        thisLevel = getBits(8);
                        backreferenceLength += thisLevel;
                    } while (thisLevel == 255);
                }

                for (int i = 0; i < backreferenceLength; ++i)
                {
                    if (backreferenceOffset < 0 ||
                        backreferenceOffset >= static_cast<int>(result.size()) ||
                        outputEnd - bytesOutput < 0)
                    {
                        break;
                    }

                    result[outputEnd - bytesOutput] = result[backreferenceOffset--];
                    bytesOutput++;
                }
            }
            else
            {
                if (outputEnd - bytesOutput < 0)
                    break;

                result[outputEnd - bytesOutput] = static_cast<unsigned char>(getBits(8));
                bytesOutput++;
            }
        }

        return result;
    }

    bool ReadBytesAt(
        const std::string& path,
        uint64_t offset,
        uint64_t size,
        std::vector<unsigned char>& out)
    {
        out.clear();

        if (size == 0 || size > 1024ull * 1024ull * 1024ull)
            return false;

        std::ifstream file(path, std::ios::binary);

        if (!file.is_open())
            return false;

        file.seekg(static_cast<std::streamoff>(offset), std::ios::beg);

        out.resize(static_cast<size_t>(size));
        file.read(reinterpret_cast<char*>(out.data()), static_cast<std::streamsize>(out.size()));

        return static_cast<size_t>(file.gcount()) == out.size();
    }

    uint64_t AlignValue(uint64_t value, uint64_t align)
    {
        if (align == 0)
            return value;

        uint64_t rem = value % align;

        if (rem == 0)
            return value;

        return value + (align - rem);
    }

    bool BuildEntriesFromToc(
        const UtfTable& toc,
        uint64_t addOffset,
        std::vector<CpkArchive::Entry>& out)
    {
        for (const auto& row : toc.Rows)
        {
            std::string dir = GetString(row, "DirName");
            std::string file = GetString(row, "FileName");

            if (file.empty())
                continue;

            uint64_t fileOffset = GetU64(row, "FileOffset", 0) + addOffset;
            uint64_t fileSize = GetU64(row, "FileSize", 0);
            uint64_t extractSize = GetU64(row, "ExtractSize", fileSize);

            CpkArchive::Entry e{};
            e.Name = file;
            e.Path = Normalize(dir.empty() ? file : dir + "/" + file);
            e.Offset = fileOffset;
            e.Size = fileSize;
            e.ExtractedSize = extractSize;
            e.Compressed = extractSize > fileSize;
            e.Heuristic = false;
            e.Extractable = fileSize > 0;

            out.push_back(e);
        }

        return !out.empty();
    }

    bool BuildEntriesFromItoc(
        const UtfTable& itoc,
        uint64_t contentOffset,
        uint64_t align,
        std::vector<CpkArchive::Entry>& out)
    {
        if (itoc.Rows.empty())
            return false;

        const auto& row0 = itoc.Rows[0];

        auto parseDataTable = [&](const std::string& key, std::map<int, std::pair<uint64_t, uint64_t>>& sizes)
            {
                auto it = row0.find(key);

                if (it == row0.end() || it->second.Data.empty())
                    return;

                UtfTable dataTable{};

                if (!ParseUtf(it->second.Data, dataTable))
                    return;

                for (const auto& row : dataTable.Rows)
                {
                    int id = static_cast<int>(GetU64(row, "ID", 0));
                    uint64_t fileSize = GetU64(row, "FileSize", 0);
                    uint64_t extractSize = GetU64(row, "ExtractSize", fileSize);

                    sizes[id] = std::make_pair(fileSize, extractSize);
                }
            };

        std::map<int, std::pair<uint64_t, uint64_t>> sizes;

        parseDataTable("DataL", sizes);
        parseDataTable("DataH", sizes);

        uint64_t currentOffset = contentOffset;

        for (const auto& pair : sizes)
        {
            int id = pair.first;
            uint64_t fileSize = pair.second.first;
            uint64_t extractSize = pair.second.second;

            CpkArchive::Entry e{};
            e.Name = std::to_string(id);
            e.Path = std::to_string(id);
            e.Offset = currentOffset;
            e.Size = fileSize;
            e.ExtractedSize = extractSize;
            e.Compressed = extractSize > fileSize;
            e.Heuristic = false;
            e.Extractable = fileSize > 0;

            out.push_back(e);

            currentOffset = AlignValue(currentOffset + fileSize, align);
        }

        return !out.empty();
    }
}

namespace CpkArchive
{
    bool Load(const std::string& cpkPath, ArchiveInfo& outInfo)
    {
        outInfo = ArchiveInfo{};
        outInfo.FullPath = cpkPath;

        if (!FileExists(cpkPath))
        {
            outInfo.Status = "CPK file not found";
            return false;
        }

        std::ifstream file(cpkPath, std::ios::binary);

        if (!file.is_open())
        {
            outInfo.Status = "Could not open CPK";
            return false;
        }

        char magic[4]{};
        file.read(magic, 4);

        if (std::memcmp(magic, "CPK ", 4) != 0)
        {
            outInfo.Status = "Not a CPK file";
            return false;
        }

        std::vector<unsigned char> cpkPacket;

        if (!ReadUtfPacketAt(file, 0, cpkPacket))
        {
            outInfo.Status = "Failed to read CPK UTF header";
            return false;
        }

        UtfTable cpkUtf{};

        if (!ParseUtf(cpkPacket, cpkUtf) || cpkUtf.Rows.empty())
        {
            outInfo.Status = "Failed to parse CPK UTF header";
            return false;
        }

        const auto& header = cpkUtf.Rows[0];

        uint64_t tocOffset = GetU64(header, "TocOffset", UINT64_MAX);
        uint64_t itocOffset = GetU64(header, "ItocOffset", UINT64_MAX);
        uint64_t contentOffset = GetU64(header, "ContentOffset", 0);
        uint64_t align = GetU64(header, "Align", 0x800);

        bool loaded = false;

        if (tocOffset != UINT64_MAX && tocOffset > 0)
        {
            std::vector<unsigned char> tocPacket;

            if (ReadUtfPacketAt(file, tocOffset, tocPacket))
            {
                UtfTable toc{};

                if (ParseUtf(tocPacket, toc))
                {
                    uint64_t addOffset = 0;

                    if (contentOffset < tocOffset)
                        addOffset = contentOffset;
                    else
                        addOffset = tocOffset;

                    loaded = BuildEntriesFromToc(toc, addOffset, outInfo.Entries);
                }
            }
        }

        if (!loaded && itocOffset != UINT64_MAX && itocOffset > 0)
        {
            std::vector<unsigned char> itocPacket;

            if (ReadUtfPacketAt(file, itocOffset, itocPacket))
            {
                UtfTable itoc{};

                if (ParseUtf(itocPacket, itoc))
                    loaded = BuildEntriesFromItoc(itoc, contentOffset, align, outInfo.Entries);
            }
        }

        outInfo.Valid = true;

        if (outInfo.Entries.empty())
            outInfo.Status = "CPK valid but no file entries parsed";
        else
            outInfo.Status = "CPK parsed natively";

        Logger::Info(
            "CpkArchive native loaded: " +
            cpkPath +
            " entries=" +
            std::to_string(outInfo.Entries.size()));

        return true;
    }

    bool ExtractEntry(
        const std::string& cpkPath,
        const std::string& entryPath,
        const std::string& outputPath)
    {
        ArchiveInfo info{};

        if (!Load(cpkPath, info))
            return false;

        std::string wanted = ToLower(Normalize(entryPath));

        const Entry* found = nullptr;

        for (const auto& e : info.Entries)
        {
            std::string path = ToLower(Normalize(e.Path));
            std::string name = ToLower(e.Name);

            if (path == wanted || name == wanted)
            {
                found = &e;
                break;
            }
        }

        if (!found)
        {
            Entry marker{};
            marker.Name = std::filesystem::path(entryPath).filename().string();
            marker.Path = Normalize(entryPath);
            marker.Heuristic = true;
            marker.Extractable = false;
            DumpFile(cpkPath, marker, outputPath + ".notfound.txt");
            return false;
        }

        return DumpFile(cpkPath, *found, outputPath);
    }

    bool DumpFile(
        const std::string& cpkPath,
        const Entry& entry,
        const std::string& outputPath)
    {
        if (!entry.Extractable || entry.Size == 0)
        {
            std::filesystem::create_directories(std::filesystem::path(outputPath).parent_path());

            std::ofstream marker(outputPath + ".cpkentry.txt", std::ios::binary | std::ios::trunc);

            if (!marker.is_open())
                return false;

            marker << "CPK entry is indexed but not extractable.\n";
            marker << "Archive: " << cpkPath << "\n";
            marker << "Entry: " << entry.Path << "\n";
            marker << "Name: " << entry.Name << "\n";
            marker << "Offset: " << entry.Offset << "\n";
            marker << "Size: " << entry.Size << "\n";
            marker << "ExtractedSize: " << entry.ExtractedSize << "\n";
            marker << "Heuristic: " << (entry.Heuristic ? "true" : "false") << "\n";
            return true;
        }

        std::vector<unsigned char> bytes;

        if (!ReadBytesAt(cpkPath, entry.Offset, entry.Size, bytes))
            return false;

        if (bytes.size() >= 8 && std::memcmp(bytes.data(), "CRILAYLA", 8) == 0)
            bytes = DecompressCrilayla(bytes);

        std::filesystem::path out = outputPath;
        std::filesystem::create_directories(out.parent_path());

        std::ofstream file(out, std::ios::binary | std::ios::trunc);

        if (!file.is_open())
            return false;

        file.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));

        Logger::Info(
            "CPK native dumped: " +
            entry.Path +
            " -> " +
            out.string());

        return true;
    }
}