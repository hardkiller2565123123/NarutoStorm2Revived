#include "StdInc.h"
#include "XfbinInspector.h"
#include "Logger.h"
#include "AssetPreviewResolver.h"

#include <fstream>
#include <sstream>

namespace
{
    std::string Lower(std::string s)
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

    bool IsTextChar(unsigned char c)
    {
        return c == '\n' ||
            c == '\r' ||
            c == '\t' ||
            (c >= 32 && c < 127);
    }

    std::vector<unsigned char> ReadFile(const std::string& path, size_t maxBytes)
    {
        std::vector<unsigned char> out;

        std::ifstream file(path, std::ios::binary);

        if (!file.is_open())
            return out;

        file.seekg(0, std::ios::end);
        size_t size = static_cast<size_t>(std::max<std::streamoff>(0, file.tellg()));
        file.seekg(0, std::ios::beg);

        size = std::min(size, maxBytes);
        out.resize(size);

        if (size)
            file.read(reinterpret_cast<char*>(out.data()), static_cast<std::streamsize>(size));

        return out;
    }

    std::vector<std::string> ExtractStrings(const std::vector<unsigned char>& bytes)
    {
        std::vector<std::string> out;
        std::string current;

        for (unsigned char c : bytes)
        {
            if (IsTextChar(c))
            {
                current.push_back(static_cast<char>(c));
            }
            else
            {
                if (current.size() >= 4)
                    out.push_back(current);

                current.clear();
            }
        }

        if (current.size() >= 4)
            out.push_back(current);

        if (out.size() > 2000)
            out.resize(2000);

        return out;
    }

    void AddUnique(std::vector<std::string>& list, const std::string& value)
    {
        if (value.empty())
            return;

        if (std::find(list.begin(), list.end(), value) == list.end())
            list.push_back(value);
    }

    bool Contains(const std::string& s, const std::string& needle)
    {
        return Lower(s).find(Lower(needle)) != std::string::npos;
    }

    bool LooksLikeTexture(const std::string& s)
    {
        std::string l = Lower(s);
        return l.find(".dds") != std::string::npos ||
            l.find(".nut") != std::string::npos ||
            l.find("texture") != std::string::npos ||
            l.find("tex") != std::string::npos;
    }

    bool LooksLikeModel(const std::string& s)
    {
        std::string l = Lower(s);
        return l.find(".nud") != std::string::npos ||
            l.find("model") != std::string::npos ||
            l.find("mesh") != std::string::npos;
    }

    bool LooksLikeMaterial(const std::string& s)
    {
        std::string l = Lower(s);
        return l.find("material") != std::string::npos ||
            l.find("mat") != std::string::npos ||
            l.find("shader") != std::string::npos;
    }

    bool LooksLikeAnimation(const std::string& s)
    {
        std::string l = Lower(s);
        return l.find("anm") != std::string::npos ||
            l.find("animation") != std::string::npos ||
            l.find("motion") != std::string::npos ||
            l.find(".mot") != std::string::npos;
    }
}

namespace XfbinInspector
{
    bool Inspect(const AssetBrowser::AssetEntry& asset, XfbinInfo& outInfo)
    {
        outInfo = XfbinInfo{};

        if (asset.Extension != ".xfbin" && asset.Extension != ".nud")
        {
            outInfo.Valid = false;
            outInfo.Status = "Not an XFBIN/NUD asset";
            return false;
        }

        std::string readablePath;

        if (!AssetPreviewResolver::ResolveReadablePath(asset, readablePath, true))
        {
            outInfo.Valid = false;
            outInfo.Status = "Could not resolve readable asset path. CPK entry may need extractor tool.";
            return false;
        }

        std::vector<unsigned char> bytes = ReadFile(readablePath, 16 * 1024 * 1024);

        

        if (bytes.empty())
        {
            outInfo.Valid = false;
            outInfo.Status = "Could not read file";
            return false;
        }

        if (bytes.size() >= 4)
            outInfo.Magic.assign(reinterpret_cast<const char*>(bytes.data()), reinterpret_cast<const char*>(bytes.data()) + 4);

        outInfo.Strings = ExtractStrings(bytes);

        for (const auto& s : outInfo.Strings)
        {
            std::string l = Lower(s);

            if (Contains(l, "nucc") || Contains(l, "chunk"))
                AddUnique(outInfo.Chunks, s);

            if (LooksLikeTexture(s))
                AddUnique(outInfo.TextureRefs, s);

            if (LooksLikeModel(s))
                AddUnique(outInfo.ModelRefs, s);

            if (LooksLikeMaterial(s))
                AddUnique(outInfo.MaterialRefs, s);

            if (LooksLikeAnimation(s))
                AddUnique(outInfo.AnimationRefs, s);

            if (Contains(l, ".lua") || Contains(l, "lua"))
                AddUnique(outInfo.LuaRefs, s);
        }

        outInfo.Valid = true;
        outInfo.Status = "Quick XFBIN inspection complete";

        return true;
    }

    std::string BuildReport(const XfbinInfo& info)
    {
        std::ostringstream out;

        out << "XFBIN Inspector\n";
        out << "===============\n";
        out << "Valid: " << (info.Valid ? "true" : "false") << "\n";
        out << "Status: " << info.Status << "\n";
        out << "Magic: " << info.Magic << "\n";
        out << "Strings: " << info.Strings.size() << "\n";
        out << "Chunks: " << info.Chunks.size() << "\n";
        out << "Textures: " << info.TextureRefs.size() << "\n";
        out << "Models: " << info.ModelRefs.size() << "\n";
        out << "Materials: " << info.MaterialRefs.size() << "\n";
        out << "Animations: " << info.AnimationRefs.size() << "\n";
        out << "Lua refs: " << info.LuaRefs.size() << "\n\n";

        auto dump = [&](const char* title, const std::vector<std::string>& list)
            {
                out << title << "\n";
                out << "----------------\n";

                for (const auto& item : list)
                    out << "- " << item << "\n";

                out << "\n";
            };

        dump("Chunks", info.Chunks);
        dump("Texture Refs", info.TextureRefs);
        dump("Model Refs", info.ModelRefs);
        dump("Material Refs", info.MaterialRefs);
        dump("Animation Refs", info.AnimationRefs);
        dump("Lua Refs", info.LuaRefs);

        return out.str();
    }
}