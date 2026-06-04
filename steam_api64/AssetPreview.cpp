#include "StdInc.h"
#include "AssetPreview.h"
#include <fstream>
#include <sstream>
#include <iomanip>

namespace
{
    std::string Lower(std::string s)
    {
        std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c){ return static_cast<char>(std::tolower(c)); });
        return s;
    }

    bool IsTextExt(const std::string& ext)
    {
        return ext == ".txt" || ext == ".ini" || ext == ".json" || ext == ".xml" || ext == ".csv" || ext == ".lua" || ext == ".log";
    }

    bool IsModelExt(const std::string& ext)
    {
        return ext == ".xfbin" || ext == ".nud" || ext == ".mdl" || ext == ".model";
    }

    uint32_t LE32(const std::vector<uint8_t>& d, size_t p)
    {
        if (p + 4 > d.size()) return 0;
        return static_cast<uint32_t>(d[p]) |
            (static_cast<uint32_t>(d[p + 1]) << 8) |
            (static_cast<uint32_t>(d[p + 2]) << 16) |
            (static_cast<uint32_t>(d[p + 3]) << 24);
    }

    std::string MakeHex(const std::vector<uint8_t>& d, size_t maxBytes)
    {
        std::ostringstream oss;
        size_t count = std::min(d.size(), maxBytes);
        for (size_t i = 0; i < count; i += 16)
        {
            oss << std::hex << std::setw(8) << std::setfill('0') << i << "  ";
            for (size_t j = 0; j < 16; ++j)
            {
                if (i + j < count)
                    oss << std::setw(2) << static_cast<int>(d[i + j]) << ' ';
                else
                    oss << "   ";
            }
            oss << " ";
            for (size_t j = 0; j < 16 && i + j < count; ++j)
            {
                uint8_t c = d[i + j];
                oss << (c >= 32 && c <= 126 ? static_cast<char>(c) : '.');
            }
            oss << "\n";
        }
        return oss.str();
    }

    std::string MakeDdsInfo(const std::vector<uint8_t>& d)
    {
        if (d.size() < 128 || memcmp(d.data(), "DDS ", 4) != 0)
            return {};
        uint32_t height = LE32(d, 12);
        uint32_t width = LE32(d, 16);
        uint32_t mipmaps = LE32(d, 28);
        uint32_t fourCC = LE32(d, 84);
        char cc[5] = {};
        memcpy(cc, &fourCC, 4);
        std::ostringstream oss;
        oss << "DDS Texture\n";
        oss << "Size: " << width << "x" << height << "\n";
        oss << "Mipmaps: " << mipmaps << "\n";
        oss << "FourCC: " << cc << "\n";
        return oss.str();
    }
}

namespace AssetPreview
{
    bool LoadPreview(const std::string& path, PreviewInfo& outInfo, size_t maxBytes)
    {
        outInfo = {};
        outInfo.Path = path;
        outInfo.Extension = Lower(std::filesystem::path(path).extension().string());
        outInfo.IsText = IsTextExt(outInfo.Extension);
        outInfo.IsDds = outInfo.Extension == ".dds";
        outInfo.IsModelLike = IsModelExt(outInfo.Extension);

        std::ifstream file(path, std::ios::binary);
        if (!file)
            return false;

        file.seekg(0, std::ios::end);
        size_t size = static_cast<size_t>(file.tellg());
        file.seekg(0, std::ios::beg);
        size_t readSize = std::min(size, maxBytes);
        std::vector<uint8_t> data(readSize);
        if (readSize) file.read(reinterpret_cast<char*>(data.data()), readSize);

        if (outInfo.IsText)
            outInfo.Text.assign(reinterpret_cast<const char*>(data.data()), data.size());

        if (outInfo.IsDds)
            outInfo.DdsInfo = MakeDdsInfo(data);

        outInfo.Hex = MakeHex(data, std::min<size_t>(readSize, 4096));
        outInfo.Loaded = true;
        return true;
    }

    void DrawModelPreviewPlaceholder(const char* label, float width, float height)
    {
        ImVec2 p = ImGui::GetCursorScreenPos();
        ImDrawList* draw = ImGui::GetWindowDrawList();
        ImVec2 size(width, height);
        draw->AddRectFilled(p, ImVec2(p.x + size.x, p.y + size.y), IM_COL32(8, 10, 14, 215));
        draw->AddRect(p, ImVec2(p.x + size.x, p.y + size.y), IM_COL32(60, 120, 200, 220));

        float cx = p.x + size.x * 0.5f;
        float cy = p.y + size.y * 0.52f;
        float s = std::min(size.x, size.y) * 0.28f;
        float t = static_cast<float>(ImGui::GetTime());
        float a = t * 0.85f;
        auto proj = [&](float x, float y, float z) -> ImVec2
        {
            float ca = cosf(a), sa = sinf(a);
            float rx = x * ca - z * sa;
            float rz = x * sa + z * ca;
            float f = 1.0f / (2.3f + rz * 0.45f);
            return ImVec2(cx + rx * s * f * 2.2f, cy - y * s * f * 2.2f);
        };
        float pts[8][3] = {{-1,-1,-1},{1,-1,-1},{1,1,-1},{-1,1,-1},{-1,-1,1},{1,-1,1},{1,1,1},{-1,1,1}};
        int edges[12][2] = {{0,1},{1,2},{2,3},{3,0},{4,5},{5,6},{6,7},{7,4},{0,4},{1,5},{2,6},{3,7}};
        for (auto& e : edges)
            draw->AddLine(proj(pts[e[0]][0], pts[e[0]][1], pts[e[0]][2]), proj(pts[e[1]][0], pts[e[1]][1], pts[e[1]][2]), IM_COL32(80, 165, 255, 255), 1.5f);
        draw->AddText(ImVec2(p.x + 10, p.y + 8), IM_COL32(220, 230, 255, 255), label);
        draw->AddText(ImVec2(p.x + 10, p.y + size.y - 34), IM_COL32(170, 180, 195, 255), "Model decode not wired yet: showing 3D viewer shell");
        ImGui::Dummy(size);
    }

    void DrawPreview(const PreviewInfo& info, float width, float height)
    {
        if (!info.Loaded)
        {
            ImGui::TextDisabled("No preview loaded.");
            return;
        }
        if (info.IsDds && !info.DdsInfo.empty())
        {
            ImGui::TextUnformatted(info.DdsInfo.c_str());
            ImGui::Separator();
        }
        if (info.IsModelLike)
        {
            DrawModelPreviewPlaceholder(std::filesystem::path(info.Path).filename().string().c_str(), width, 180.0f);
            ImGui::Separator();
        }
        if (info.IsText)
        {
            ImGui::BeginChild("##AssetPreviewText", ImVec2(width, height), true);
            ImGui::TextUnformatted(info.Text.c_str());
            ImGui::EndChild();
        }
        else
        {
            ImGui::BeginChild("##AssetPreviewHex", ImVec2(width, height), true);
            ImGui::TextUnformatted(info.Hex.c_str());
            ImGui::EndChild();
        }
    }
}
