#include "Paths.h"

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <vector>

#ifdef _WIN32
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
    #include <windows.h>
#endif

namespace fs = std::filesystem;

static fs::path executableDir()
{
#ifdef _WIN32
    wchar_t buffer[MAX_PATH];
    DWORD length = GetModuleFileNameW(nullptr, buffer, MAX_PATH);
    return fs::path(std::wstring(buffer, length)).parent_path();
#else
    return fs::canonical("/proc/self/exe").parent_path();
#endif
}

fs::path resourcePath(const std::string& relativePath)
{
    std::vector<fs::path> candidates = {executableDir() / relativePath};
#ifdef RESOURCE_ROOT
    candidates.push_back(fs::path(RESOURCE_ROOT) / relativePath);
#endif

    for (const fs::path& candidate : candidates) {
        if (fs::exists(candidate))
            return candidate;
    }
    throw std::runtime_error("Resource not found: " + relativePath
                             + " (keep the shaders and assets folders next to the .exe)");
}

fs::path userDataPath(const std::string& fileName)
{
    fs::path folder;
#ifdef _WIN32
    if (const wchar_t* appData = _wgetenv(L"APPDATA"))
        folder = fs::path(appData) / "FloatingIsland";
#else
    if (const char* home = std::getenv("HOME"))
        folder = fs::path(home) / ".floatingisland";
#endif
    if (folder.empty())
        folder = fs::temp_directory_path() / "FloatingIsland";

    std::error_code error;
    fs::create_directories(folder, error);
    return folder / fileName;
}

void showFatalError(const std::string& message)
{
    std::cerr << "Fatal error: " << message << '\n';
#ifdef _WIN32
    const int length = MultiByteToWideChar(CP_UTF8, 0, message.c_str(), -1, nullptr, 0);
    std::wstring wide(length > 0 ? length : 1, L'\0');
    if (length > 0)
        MultiByteToWideChar(CP_UTF8, 0, message.c_str(), -1, wide.data(), length);
    MessageBoxW(nullptr, wide.c_str(), L"Floating Island - error", MB_OK | MB_ICONERROR | MB_SETFOREGROUND);
#endif
}
