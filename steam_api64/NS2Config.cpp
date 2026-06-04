#include "StdInc.h"
#include "NS2Config.h"
#include "Logger.h"

namespace
{
    std::string g_ConfigPath;
    NS2Config::ScreenSettings g_Screen;
    std::map<std::string, std::string> g_Keys;

    std::string GetGameFolder()
    {
        char path[MAX_PATH]{};
        GetModuleFileNameA(nullptr, path, MAX_PATH);

        std::filesystem::path exePath(path);
        return exePath.parent_path().string();
    }

    int ReadInt(const char* section, const char* key, int fallback)
    {
        return GetPrivateProfileIntA(section, key, fallback, g_ConfigPath.c_str());
    }

    std::string ReadString(const char* section, const char* key, const char* fallback)
    {
        char buffer[256]{};

        GetPrivateProfileStringA(
            section,
            key,
            fallback,
            buffer,
            sizeof(buffer),
            g_ConfigPath.c_str());

        return buffer;
    }

    void WriteInt(const char* section, const char* key, int value)
    {
        WritePrivateProfileStringA(
            section,
            key,
            std::to_string(value).c_str(),
            g_ConfigPath.c_str());
    }

    void WriteString(const char* section, const char* key, const std::string& value)
    {
        WritePrivateProfileStringA(
            section,
            key,
            value.c_str(),
            g_ConfigPath.c_str());
    }

    void LoadControls()
    {
        const char* keys[] =
        {
            "BUTTON_START", "BUTTON_SELECT",
            "BUTTON_R_UP", "BUTTON_R_DOWN", "BUTTON_R_LEFT", "BUTTON_R_RIGHT",
            "BUTTON_L3", "BUTTON_R3",
            "BUTTON_L1", "BUTTON_R1",
            "BUTTON_L2", "BUTTON_R2",
            "BUTTON_LS_UP", "BUTTON_LS_DOWN", "BUTTON_LS_LEFT", "BUTTON_LS_RIGHT",
            "BUTTON_RS_UP", "BUTTON_RS_DOWN", "BUTTON_RS_LEFT", "BUTTON_RS_RIGHT",
            "BUTTON_L_UP", "BUTTON_L_DOWN", "BUTTON_L_LEFT", "BUTTON_L_RIGHT"
        };

        for (const char* key : keys)
            g_Keys[key] = ReadString("controlSettingKB", key, "");
    }

    void SaveControls()
    {
        for (const auto& item : g_Keys)
            WriteString("controlSettingKB", item.first.c_str(), item.second);
    }
}

namespace NS2Config
{
    bool Init()
    {
        g_ConfigPath = GetGameFolder() + "\\config.ini";
        Logger::Info("NS2Config path: " + g_ConfigPath);
        return Load();
    }

    bool Load()
    {
        g_Screen.Width = ReadInt("screenSetting", "width", 1920);
        g_Screen.Height = ReadInt("screenSetting", "height", 1080);
        g_Screen.AspectX = ReadInt("screenSetting", "aspectx", 16);
        g_Screen.AspectY = ReadInt("screenSetting", "aspecty", 9);
        g_Screen.SSAA = ReadInt("screenSetting", "ssaa", 0);
        g_Screen.Windowed = ReadInt("screenSetting", "windowed", 1);
        g_Screen.VSync = ReadInt("screenSetting", "vsync", 0);
        g_Screen.ShowCursor = ReadInt("screenSetting", "showCursor", 1);
        g_Screen.FPS = ReadInt("screenSetting", "fps", 1);
        g_Screen.FXAA = ReadInt("screenSetting", "fxaa", 1);
        g_Screen.SMAA = ReadInt("screenSetting", "smaa", 0);
        g_Screen.MSAA = ReadInt("screenSetting", "msaa", 1);
        g_Screen.MipMap = ReadInt("screenSetting", "mipmap", 0);
        g_Screen.ShadowQuality = ReadInt("screenSetting", "shadowQuality", 1);
        g_Screen.MotionBlur = ReadInt("screenSetting", "motionBlur", 1);
        g_Screen.Glare = ReadInt("screenSetting", "glare", 1);

        LoadControls();

        Logger::Info("NS2Config loaded");
        return true;
    }

    bool Save()
    {
        WriteInt("screenSetting", "width", g_Screen.Width);
        WriteInt("screenSetting", "height", g_Screen.Height);
        WriteInt("screenSetting", "aspectx", g_Screen.AspectX);
        WriteInt("screenSetting", "aspecty", g_Screen.AspectY);
        WriteInt("screenSetting", "ssaa", g_Screen.SSAA);
        WriteInt("screenSetting", "windowed", g_Screen.Windowed);
        WriteInt("screenSetting", "vsync", g_Screen.VSync);
        WriteInt("screenSetting", "showCursor", g_Screen.ShowCursor);
        WriteInt("screenSetting", "fps", g_Screen.FPS);
        WriteInt("screenSetting", "fxaa", g_Screen.FXAA);
        WriteInt("screenSetting", "smaa", g_Screen.SMAA);
        WriteInt("screenSetting", "msaa", g_Screen.MSAA);
        WriteInt("screenSetting", "mipmap", g_Screen.MipMap);
        WriteInt("screenSetting", "shadowQuality", g_Screen.ShadowQuality);
        WriteInt("screenSetting", "motionBlur", g_Screen.MotionBlur);
        WriteInt("screenSetting", "glare", g_Screen.Glare);

        SaveControls();

        Logger::Info("NS2Config saved");
        return true;
    }

    const std::string& GetPath()
    {
        return g_ConfigPath;
    }

    ScreenSettings& Screen()
    {
        return g_Screen;
    }

    std::string GetKeyBind(const std::string& name)
    {
        auto it = g_Keys.find(name);
        return it == g_Keys.end() ? "" : it->second;
    }

    void SetKeyBind(const std::string& name, const std::string& value)
    {
        g_Keys[name] = value;
    }

    void ResetGraphics()
    {
        g_Screen = ScreenSettings{};
        Save();
    }

    void ResetControls()
    {
        g_Keys.clear();

        SetKeyBind("BUTTON_START", "KB_ENTER");
        SetKeyBind("BUTTON_SELECT", "KB_BS");
        SetKeyBind("BUTTON_R_UP", "KB_O");
        SetKeyBind("BUTTON_R_DOWN", "KB_L");
        SetKeyBind("BUTTON_R_LEFT", "KB_K");
        SetKeyBind("BUTTON_R_RIGHT", "KB_SEMICOLON");
        SetKeyBind("BUTTON_L1", "KB_Q");
        SetKeyBind("BUTTON_R1", "KB_P");
        SetKeyBind("BUTTON_L2", "KB_E");
        SetKeyBind("BUTTON_R2", "KB_I");
        SetKeyBind("BUTTON_LS_UP", "KB_W");
        SetKeyBind("BUTTON_LS_DOWN", "KB_S");
        SetKeyBind("BUTTON_LS_LEFT", "KB_A");
        SetKeyBind("BUTTON_LS_RIGHT", "KB_D");
        SetKeyBind("BUTTON_L_UP", "KB_UP");
        SetKeyBind("BUTTON_L_DOWN", "KB_DOWN");
        SetKeyBind("BUTTON_L_LEFT", "KB_LEFT");
        SetKeyBind("BUTTON_L_RIGHT", "KB_RIGHT");

        Save();
    }
}