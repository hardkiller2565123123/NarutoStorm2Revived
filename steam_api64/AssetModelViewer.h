#pragma once
#include "StdInc.h"
#include "AssetBrowser.h"
#include "imgui.h"

namespace AssetModelViewer
{
    void Init();
    void Shutdown();
    void Draw(const AssetBrowser::AssetEntry& asset, ImVec2 size);
}
