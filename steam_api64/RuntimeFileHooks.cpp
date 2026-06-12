#include "StdInc.h"
#include "RuntimeFileHooks.h"
#include "RuntimeAssetTelemetry.h"
#include "Logger.h"
#include "MinHook.h"

namespace
{
    using CreateFileA_t = HANDLE(WINAPI*)(LPCSTR, DWORD, DWORD, LPSECURITY_ATTRIBUTES, DWORD, DWORD, HANDLE);
    using CreateFileW_t = HANDLE(WINAPI*)(LPCWSTR, DWORD, DWORD, LPSECURITY_ATTRIBUTES, DWORD, DWORD, HANDLE);
    using ReadFile_t = BOOL(WINAPI*)(HANDLE, LPVOID, DWORD, LPDWORD, LPOVERLAPPED);
    using WriteFile_t = BOOL(WINAPI*)(HANDLE, LPCVOID, DWORD, LPDWORD, LPOVERLAPPED);
    using FindFirstFileA_t = HANDLE(WINAPI*)(LPCSTR, LPWIN32_FIND_DATAA);
    using FindFirstFileW_t = HANDLE(WINAPI*)(LPCWSTR, LPWIN32_FIND_DATAW);
    using FindNextFileA_t = BOOL(WINAPI*)(HANDLE, LPWIN32_FIND_DATAA);
    using FindNextFileW_t = BOOL(WINAPI*)(HANDLE, LPWIN32_FIND_DATAW);

    CreateFileA_t oCreateFileA = nullptr;
    CreateFileW_t oCreateFileW = nullptr;
    ReadFile_t oReadFile = nullptr;
    WriteFile_t oWriteFile = nullptr;
    FindFirstFileA_t oFindFirstFileA = nullptr;
    FindFirstFileW_t oFindFirstFileW = nullptr;
    FindNextFileA_t oFindNextFileA = nullptr;
    FindNextFileW_t oFindNextFileW = nullptr;

    std::string WideToUtf8(LPCWSTR w)
    {
        if (!w) return "";
        int len = WideCharToMultiByte(CP_UTF8, 0, w, -1, nullptr, 0, nullptr, nullptr);
        if (len <= 0) return "";
        std::string out(len - 1, 0);
        WideCharToMultiByte(CP_UTF8, 0, w, -1, out.data(), len, nullptr, nullptr);
        return out;
    }

    RuntimeAssetTelemetry::EventType Guess(const std::string& path)
    {
        std::string s = path;
        std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c){ return static_cast<char>(std::tolower(c)); });

        if (s.find(".xfbin") != std::string::npos) return RuntimeAssetTelemetry::EventType::Xfbin;
        if (s.find(".cpk") != std::string::npos) return RuntimeAssetTelemetry::EventType::Cpk;
        if (s.find(".nud") != std::string::npos) return RuntimeAssetTelemetry::EventType::Model;
        if (s.find(".nut") != std::string::npos || s.find(".dds") != std::string::npos) return RuntimeAssetTelemetry::EventType::Texture;
        if (s.find(".adx") != std::string::npos || s.find(".awb") != std::string::npos || s.find(".acb") != std::string::npos) return RuntimeAssetTelemetry::EventType::Sound;
        if (s.find(".lua") != std::string::npos) return RuntimeAssetTelemetry::EventType::Lua;
        return RuntimeAssetTelemetry::EventType::FileOpen;
    }

    HANDLE WINAPI hkCreateFileA(LPCSTR n, DWORD a, DWORD s, LPSECURITY_ATTRIBUTES sec, DWORD c, DWORD f, HANDLE t)
    {
        HANDLE h = oCreateFileA(n, a, s, sec, c, f, t);
        if (n) RuntimeAssetTelemetry::Push(Guess(n), n, "CreateFileA");
        return h;
    }

    HANDLE WINAPI hkCreateFileW(LPCWSTR n, DWORD a, DWORD s, LPSECURITY_ATTRIBUTES sec, DWORD c, DWORD f, HANDLE t)
    {
        HANDLE h = oCreateFileW(n, a, s, sec, c, f, t);
        if (n)
        {
            std::string p = WideToUtf8(n);
            RuntimeAssetTelemetry::Push(Guess(p), p, "CreateFileW");
        }
        return h;
    }

    BOOL WINAPI hkReadFile(HANDLE h, LPVOID b, DWORD want, LPDWORD got, LPOVERLAPPED ov)
    {
        BOOL ok = oReadFile(h, b, want, got, ov);
        if (ok && got && *got > 0)
            RuntimeAssetTelemetry::Push(RuntimeAssetTelemetry::EventType::FileRead, "ReadFile", "", *got);
        return ok;
    }

    BOOL WINAPI hkWriteFile(HANDLE h, LPCVOID b, DWORD want, LPDWORD got, LPOVERLAPPED ov)
    {
        BOOL ok = oWriteFile(h, b, want, got, ov);
        if (ok && got && *got > 0)
            RuntimeAssetTelemetry::Push(RuntimeAssetTelemetry::EventType::FileWrite, "WriteFile", "", *got);
        return ok;
    }

    HANDLE WINAPI hkFindFirstFileA(LPCSTR n, LPWIN32_FIND_DATAA d)
    {
        HANDLE h = oFindFirstFileA(n, d);
        if (n) RuntimeAssetTelemetry::Push(RuntimeAssetTelemetry::EventType::FindFile, n, "FindFirstFileA");
        return h;
    }

    HANDLE WINAPI hkFindFirstFileW(LPCWSTR n, LPWIN32_FIND_DATAW d)
    {
        HANDLE h = oFindFirstFileW(n, d);
        if (n) RuntimeAssetTelemetry::Push(RuntimeAssetTelemetry::EventType::FindFile, WideToUtf8(n), "FindFirstFileW");
        return h;
    }

    BOOL WINAPI hkFindNextFileA(HANDLE h, LPWIN32_FIND_DATAA d)
    {
        BOOL ok = oFindNextFileA(h, d);
        if (ok && d) RuntimeAssetTelemetry::Push(RuntimeAssetTelemetry::EventType::FindFile, d->cFileName, "FindNextFileA");
        return ok;
    }

    BOOL WINAPI hkFindNextFileW(HANDLE h, LPWIN32_FIND_DATAW d)
    {
        BOOL ok = oFindNextFileW(h, d);
        if (ok && d) RuntimeAssetTelemetry::Push(RuntimeAssetTelemetry::EventType::FindFile, WideToUtf8(d->cFileName), "FindNextFileW");
        return ok;
    }

    template<typename Fn>
    bool HookApi(const char* module, const char* name, void* detour, Fn* original)
    {
        HMODULE mod = GetModuleHandleA(module);
        if (!mod) mod = LoadLibraryA(module);
        if (!mod) return false;

        void* target = reinterpret_cast<void*>(GetProcAddress(mod, name));
        if (!target) return false;

        if (MH_CreateHook(target, detour, reinterpret_cast<void**>(original)) != MH_OK)
            return false;
        if (MH_EnableHook(target) != MH_OK)
            return false;

        Logger::Info(std::string("RuntimeFileHooks hooked ") + module + "!" + name);
        return true;
    }
}

namespace RuntimeFileHooks
{
    bool Init()
    {
        bool ok = true;
        ok &= HookApi("kernel32.dll", "CreateFileA", reinterpret_cast<void*>(&hkCreateFileA), &oCreateFileA);
        ok &= HookApi("kernel32.dll", "CreateFileW", reinterpret_cast<void*>(&hkCreateFileW), &oCreateFileW);
        ok &= HookApi("kernel32.dll", "ReadFile", reinterpret_cast<void*>(&hkReadFile), &oReadFile);
        ok &= HookApi("kernel32.dll", "WriteFile", reinterpret_cast<void*>(&hkWriteFile), &oWriteFile);
        ok &= HookApi("kernel32.dll", "FindFirstFileA", reinterpret_cast<void*>(&hkFindFirstFileA), &oFindFirstFileA);
        ok &= HookApi("kernel32.dll", "FindFirstFileW", reinterpret_cast<void*>(&hkFindFirstFileW), &oFindFirstFileW);
        ok &= HookApi("kernel32.dll", "FindNextFileA", reinterpret_cast<void*>(&hkFindNextFileA), &oFindNextFileA);
        ok &= HookApi("kernel32.dll", "FindNextFileW", reinterpret_cast<void*>(&hkFindNextFileW), &oFindNextFileW);
        Logger::Info(std::string("RuntimeFileHooks initialized ") + (ok ? "OK" : "WITH FAILURES"));
        return ok;
    }

    void Shutdown()
    {
        Logger::Info("RuntimeFileHooks shutdown");
    }
}
