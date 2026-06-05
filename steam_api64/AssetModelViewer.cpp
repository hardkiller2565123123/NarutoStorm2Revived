#include "StdInc.h"
#include "AssetModelViewer.h"
#include "Logger.h"
#include "imgui.h"

namespace
{
    struct Vec3
    {
        float x;
        float y;
        float z;

        Vec3() : x(0.0f), y(0.0f), z(0.0f) {}
        Vec3(float px, float py, float pz) : x(px), y(py), z(pz) {}
    };

    float g_Rotation = 0.0f;
    bool g_AutoSpin = true;
    bool g_ShowGrid = true;
    bool g_ShowBounds = true;
    bool g_ShowWireframe = true;
    float g_Zoom = 1.0f;

    ImVec2 Rotate2D(ImVec2 p, float angle)
    {
        float c = cosf(angle);
        float s = sinf(angle);
        return ImVec2(p.x * c - p.y * s, p.x * s + p.y * c);
    }

    void DrawLine3D(ImDrawList* draw, ImVec2 center, const Vec3& a, const Vec3& b, float rot, float scale, ImU32 color)
    {
        ImVec2 ap = Rotate2D(ImVec2(a.x, a.z), rot);
        ImVec2 bp = Rotate2D(ImVec2(b.x, b.z), rot);

        ImVec2 pa(center.x + ap.x * scale, center.y + a.y * scale - ap.y * scale * 0.35f);
        ImVec2 pb(center.x + bp.x * scale, center.y + b.y * scale - bp.y * scale * 0.35f);

        draw->AddLine(pa, pb, color, 1.5f);
    }

    void DrawCube(ImDrawList* draw, ImVec2 center, float scale, float rot, ImU32 color)
    {
        Vec3 v[8] =
        {
            Vec3(-1.0f, -1.0f, -1.0f), Vec3( 1.0f, -1.0f, -1.0f),
            Vec3( 1.0f,  1.0f, -1.0f), Vec3(-1.0f,  1.0f, -1.0f),
            Vec3(-1.0f, -1.0f,  1.0f), Vec3( 1.0f, -1.0f,  1.0f),
            Vec3( 1.0f,  1.0f,  1.0f), Vec3(-1.0f,  1.0f,  1.0f)
        };

        int edges[12][2] =
        {
            {0, 1}, {1, 2}, {2, 3}, {3, 0},
            {4, 5}, {5, 6}, {6, 7}, {7, 4},
            {0, 4}, {1, 5}, {2, 6}, {3, 7}
        };

        for (int i = 0; i < 12; i++)
            DrawLine3D(draw, center, v[edges[i][0]], v[edges[i][1]], rot, scale, color);
    }

    void DrawGrid(ImDrawList* draw, ImVec2 center, float scale, float rot)
    {
        ImU32 color = IM_COL32(45, 95, 145, 95);

        for (int i = -5; i <= 5; i++)
        {
            DrawLine3D(draw, center, Vec3(static_cast<float>(i), -1.15f, -5.0f), Vec3(static_cast<float>(i), -1.15f,  5.0f), rot, scale, color);
            DrawLine3D(draw, center, Vec3(-5.0f, -1.15f, static_cast<float>(i)), Vec3( 5.0f, -1.15f, static_cast<float>(i)), rot, scale, color);
        }
    }
}

namespace AssetModelViewer
{
    void Init()
    {
        Logger::Info("AssetModelViewer initialized");
    }

    void Shutdown()
    {
        Logger::Info("AssetModelViewer shutdown");
    }

    void Draw(const AssetBrowser::AssetEntry& asset, ImVec2 size)
    {
        ImVec2 available = ImGui::GetContentRegionAvail();

        if (size.x <= 0.0f || size.x < available.x)
            size.x = available.x;

        if (size.y <= 0.0f || size.y < available.y)
            size.y = available.y;

        if (size.x < 260.0f)
            size.x = 260.0f;

        if (size.y < 260.0f)
            size.y = 260.0f;

        ImGui::BeginChild("##AssetModelViewer", size, true);

        ImDrawList* draw = ImGui::GetWindowDrawList();
        ImVec2 pos = ImGui::GetCursorScreenPos();
        ImVec2 avail = ImGui::GetContentRegionAvail();

        ImVec2 canvasMin = pos;
        ImVec2 canvasMax(pos.x + avail.x, pos.y + avail.y - 92.0f);

        if (canvasMax.y < canvasMin.y + 140.0f)
            canvasMax.y = canvasMin.y + 140.0f;

        draw->AddRectFilled(canvasMin, canvasMax, IM_COL32(5, 8, 12, 220));
        draw->AddRect(canvasMin, canvasMax, IM_COL32(55, 125, 190, 190));

        ImVec2 center((canvasMin.x + canvasMax.x) * 0.5f, (canvasMin.y + canvasMax.y) * 0.5f + 20.0f);
        float scale = std::min(canvasMax.x - canvasMin.x, canvasMax.y - canvasMin.y) * 0.18f * g_Zoom;

        if (g_AutoSpin)
            g_Rotation += ImGui::GetIO().DeltaTime * 0.75f;

        if (g_ShowGrid)
            DrawGrid(draw, center, scale * 0.45f, g_Rotation);

        if (g_ShowBounds)
            DrawCube(draw, center, scale, g_Rotation, IM_COL32(70, 160, 255, 255));

        if (g_ShowWireframe)
            DrawCube(draw, ImVec2(center.x, center.y - scale * 0.45f), scale * 0.55f, -g_Rotation * 1.25f, IM_COL32(255, 190, 70, 230));

        draw->AddText(ImVec2(canvasMin.x + 8.0f, canvasMin.y + 8.0f), IM_COL32(230, 235, 245, 255), "3D Asset Viewer");
        draw->AddText(ImVec2(canvasMin.x + 8.0f, canvasMin.y + 26.0f), IM_COL32(180, 190, 205, 255), asset.Name.c_str());

        std::string typeLine = std::string("Type: ") + AssetBrowser::TypeName(asset.Type) + " | Ext: " + asset.Extension;
        draw->AddText(ImVec2(canvasMin.x + 8.0f, canvasMax.y - 22.0f), IM_COL32(255, 178, 70, 255), typeLine.c_str());

        ImGui::Dummy(ImVec2(avail.x, (canvasMax.y - canvasMin.y) + 6.0f));

        ImGui::Checkbox("Auto spin", &g_AutoSpin);
        ImGui::SameLine();
        ImGui::Checkbox("Grid", &g_ShowGrid);
        ImGui::SameLine();
        ImGui::Checkbox("Bounds", &g_ShowBounds);
        ImGui::SameLine();
        ImGui::Checkbox("Wireframe", &g_ShowWireframe);

        ImGui::SliderFloat("Zoom", &g_Zoom, 0.35f, 3.0f);

        ImGui::TextWrapped("This is the full-size in-game 3D preview shell. Real NUD/XFBIN mesh decode can be wired into this viewport next.");

        ImGui::EndChild();
    }
}
