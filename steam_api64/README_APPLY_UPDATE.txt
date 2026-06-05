NS2 Revived Steam + Asset Compile-Safe Rewrite

Apply this update by copying these files into your steam_api64 project and replacing the existing files.

IMPORTANT:
- Do not paste these under your old files. Replace the old files fully.
- The Steam manager files are included both at the archive root and inside /Steam for convenience.
- Use whichever layout matches your project. Your current includes use root names like #include "SteamIDManager.h", so the root copies are likely the ones you want.

What this fixes:
- SteamIDManager::GetSteamID64 missing
- SteamIDManager::GetLocalSteamID missing
- SteamIDManager::ToUint64 ambiguous overloads
- SteamCallResultManager::CreateCallResult missing / overload mismatch
- SteamLobbyManager::GetData / SetData / GetMemberCount / GetMemberByIndex missing
- SteamLobbyManager::CreateLobby overload mismatch
- SteamSessionManager::LeaveLobby no-argument overload
- string-to-const-char* return issues for persona/lobby data
- duplicate/recursive lobby/session join behavior

What stayed intact:
- AssetBrowser/AssetPreview/CpkArchive/ModRedirector files are included from your current uploaded versions.
- DX11Overlay.cpp and dllmain.cpp are included so you can keep the current asset viewer/async asset scan direction.

If Visual Studio still reports duplicate function bodies:
- Make sure you did a full file replacement.
- Make sure only one .cpp for each manager is in the project.
- Remove any extra generated Steam manager copies from previous update folders.
