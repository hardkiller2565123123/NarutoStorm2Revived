#include "StdInc.h"
#include "AssetNotification.h"
#include "imgui.h"

#include <mutex>

namespace
{
    struct Toast
    {
        AssetNotification::Type Type = AssetNotification::Type::Info;
        std::string Title;
        std::string Message;
        float TimeLeft = 4.0f;
    };

    std::mutex g_Mutex;
    std::vector<Toast> g_Toasts;

    ImVec4 ColorFor(AssetNotification::Type type)
    {
        switch (type)
        {
        case AssetNotification::Type::Success: return ImVec4(0.25f, 0.85f, 0.35f, 1.0f);
        case AssetNotification::Type::Warning: return ImVec4(1.0f, 0.70f, 0.25f, 1.0f);
        case AssetNotification::Type::Error: return ImVec4(1.0f, 0.30f, 0.25f, 1.0f);
        default: return ImVec4(0.45f, 0.75f, 1.0f, 1.0f);
        }
    }
}

namespace AssetNotification
{
    bool Init()
    {
        return true;
    }

    void Shutdown()
    {
        std::lock_guard<std::mutex> lock(g_Mutex);
        g_Toasts.clear();
    }

    void Push(Type type, const std::string& title, const std::string& message, float seconds)
    {
        std::lock_guard<std::mutex> lock(g_Mutex);

        Toast t{};
        t.Type = type;
        t.Title = title;
        t.Message = message;
        t.TimeLeft = seconds;

        g_Toasts.push_back(t);

        if (g_Toasts.size() > 8)
            g_Toasts.erase(g_Toasts.begin());
    }

    void PushInfo(const std::string& title, const std::string& message) { Push(Type::Info, title, message); }
    void PushSuccess(const std::string& title, const std::string& message) { Push(Type::Success, title, message); }
    void PushWarning(const std::string& title, const std::string& message) { Push(Type::Warning, title, message); }
    void PushError(const std::string& title, const std::string& message) { Push(Type::Error, title, message); }

    void DrawToasts()
    {
        std::lock_guard<std::mutex> lock(g_Mutex);

        if (g_Toasts.empty())
            return;

        ImGuiIO& io = ImGui::GetIO();
        float y = 170.0f;

        for (size_t i = 0; i < g_Toasts.size();)
        {
            Toast& t = g_Toasts[i];
            t.TimeLeft -= io.DeltaTime;

            if (t.TimeLeft <= 0.0f)
            {
                g_Toasts.erase(g_Toasts.begin() + i);
                continue;
            }

            ImGui::SetNextWindowBgAlpha(0.88f);
            ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x - 420.0f, y), ImGuiCond_Always);
            ImGui::SetNextWindowSize(ImVec2(400.0f, 82.0f), ImGuiCond_Always);

            std::string id = "##AssetToast" + std::to_string(i);

            ImGui::Begin(
                id.c_str(),
                nullptr,
                ImGuiWindowFlags_NoTitleBar |
                ImGuiWindowFlags_NoResize |
                ImGuiWindowFlags_NoMove |
                ImGuiWindowFlags_NoSavedSettings |
                ImGuiWindowFlags_NoFocusOnAppearing |
                ImGuiWindowFlags_NoNav);

            ImGui::TextColored(ColorFor(t.Type), "%s", t.Title.c_str());
            ImGui::Separator();
            ImGui::TextWrapped("%s", t.Message.c_str());

            ImGui::End();

            y += 88.0f;
            ++i;
        }
    }
}
