#pragma once
#include "StdInc.h"

namespace NS2Config
{
    struct ScreenSettings
    {
        int Width = 1920;
        int Height = 1080;
        int AspectX = 16;
        int AspectY = 9;
        int SSAA = 0;
        int Windowed = 1;
        int VSync = 0;
        int ShowCursor = 1;
        int FPS = 1;
        int FXAA = 1;
        int SMAA = 0;
        int MSAA = 1;
        int MipMap = 0;
        int ShadowQuality = 1;
        int MotionBlur = 1;
        int Glare = 1;
    };

    bool Init();
    bool Load();
    bool Save();

    const std::string& GetPath();
    ScreenSettings& Screen();

    std::string GetKeyBind(const std::string& name);
    void SetKeyBind(const std::string& name, const std::string& value);

    void ResetGraphics();
    void ResetControls();
}