#include "StdInc.h"
#include "AssetHexEditor.h"
#include "AssetPreviewResolver.h"
#include "AssetOverrideManager.h"
#include "AssetNotification.h"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <iomanip>

namespace
{
    AssetHexEditor::Session g_Session;

    bool ReadFile(const std::string& path, std::vector<unsigned char>& out)
    {
        out.clear();

        std::ifstream file(path, std::ios::binary);

        if (!file.is_open())
            return false;

        file.seekg(0, std::ios::end);
        size_t size = static_cast<size_t>(std::max<std::streamoff>(0, file.tellg()));
        file.seekg(0, std::ios::beg);

        if (size > 64 * 1024 * 1024)
            size = 64 * 1024 * 1024;

        out.resize(size);

        if (size)
            file.read(reinterpret_cast<char*>(out.data()), static_cast<std::streamsize>(size));

        return true;
    }

    bool WriteFile(const std::string& path, const std::vector<unsigned char>& bytes)
    {
        std::filesystem::create_directories(std::filesystem::path(path).parent_path());

        std::ofstream file(path, std::ios::binary | std::ios::trunc);

        if (!file.is_open())
            return false;

        file.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
        return true;
    }
}

namespace AssetHexEditor
{
    bool Init() { return true; }
    void Shutdown() { Clear(); }

    bool LoadAsset(const AssetBrowser::AssetEntry& asset, bool makeOverrideCopy)
    {
        Clear();

        std::string path;

        if (!AssetPreviewResolver::ResolveReadablePath(asset, path, true))
        {
            g_Session.Status = "Could not resolve readable asset.";
            return false;
        }

        if (makeOverrideCopy)
        {
            AssetOverrideManager::CreateOverrideFromFile(asset, path);
            path = AssetOverrideManager::GetOverridePath(asset);
        }

        if (!ReadFile(path, g_Session.Bytes))
        {
            g_Session.Status = "Failed to read file.";
            return false;
        }

        g_Session.Loaded = true;
        g_Session.Dirty = false;
        g_Session.SourcePath = path;
        g_Session.WorkingPath = path;
        g_Session.Status = "Loaded " + path;
        return true;
    }

    bool Save()
    {
        if (!g_Session.Loaded || g_Session.WorkingPath.empty())
            return false;

        if (!WriteFile(g_Session.WorkingPath, g_Session.Bytes))
        {
            g_Session.Status = "Save failed.";
            return false;
        }

        g_Session.Dirty = false;
        g_Session.Status = "Saved " + g_Session.WorkingPath;
        AssetNotification::PushSuccess("Hex Saved", g_Session.WorkingPath);
        return true;
    }

    bool SaveAs(const std::string& path)
    {
        if (!g_Session.Loaded)
            return false;

        if (!WriteFile(path, g_Session.Bytes))
            return false;

        g_Session.WorkingPath = path;
        g_Session.Dirty = false;
        g_Session.Status = "Saved as " + path;
        return true;
    }

    void Revert()
    {
        if (g_Session.WorkingPath.empty())
            return;

        std::vector<unsigned char> bytes;

        if (ReadFile(g_Session.WorkingPath, bytes))
        {
            g_Session.Bytes = bytes;
            g_Session.Dirty = false;
            g_Session.Status = "Reverted.";
        }
    }

    void Clear()
    {
        g_Session = Session{};
    }

    Session& GetSession()
    {
        return g_Session;
    }

    int FindString(const std::string& text, size_t startOffset)
    {
        if (!g_Session.Loaded || text.empty())
            return -1;

        for (size_t i = startOffset; i + text.size() <= g_Session.Bytes.size(); ++i)
        {
            if (std::memcmp(g_Session.Bytes.data() + i, text.data(), text.size()) == 0)
                return static_cast<int>(i);
        }

        return -1;
    }

    std::string BuildHexView(size_t startOffset, size_t maxBytes)
    {
        std::ostringstream ss;

        if (!g_Session.Loaded)
            return "";

        size_t end = std::min(g_Session.Bytes.size(), startOffset + maxBytes);

        for (size_t i = startOffset; i < end; i += 16)
        {
            ss << std::hex << std::setw(8) << std::setfill('0') << i << "  ";

            for (size_t j = 0; j < 16; ++j)
            {
                if (i + j < end)
                    ss << std::setw(2) << static_cast<int>(g_Session.Bytes[i + j]) << " ";
                else
                    ss << "   ";
            }

            ss << " ";

            for (size_t j = 0; j < 16 && i + j < end; ++j)
            {
                unsigned char c = g_Session.Bytes[i + j];
                ss << ((c >= 32 && c < 127) ? static_cast<char>(c) : '.');
            }

            ss << "\n";
        }

        return ss.str();
    }
}
