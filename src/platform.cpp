#include "platform.h"

#include <cstdio>
#include <iostream>

#if defined(_WIN32)
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #include <windows.h>
    #ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
        #define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
    #endif
#endif

using namespace std;

namespace platform {

namespace {

constexpr size_t MAX_INPUT_CHARACTERS = 8192;

}  // namespace

void setup_console()
{
#if defined(_WIN32)
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    HANDLE output = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    if (GetConsoleMode(output, &mode)) SetConsoleMode(output, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
#endif
}

bool read_console_line(string& line)
{
    line.clear();
#if defined(_WIN32)
    // Кириллица в консоли Windows корректно читается только через ReadConsoleW (UTF-16)
    HANDLE input = GetStdHandle(STD_INPUT_HANDLE);
    DWORD mode = 0;
    if (GetConsoleMode(input, &mode)) {
        wstring wide(MAX_INPUT_CHARACTERS, L'\0');
        DWORD read = 0;
        if (!ReadConsoleW(input, wide.data(), static_cast<DWORD>(wide.size() - 1), &read, nullptr) || read == 0) return false;
        wide.resize(read);
        while (!wide.empty() && (wide.back() == L'\n' || wide.back() == L'\r')) wide.pop_back();
        if (wide.empty()) return true;
        int length = WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), static_cast<int>(wide.size()), nullptr, 0, nullptr, nullptr);
        line.resize(static_cast<size_t>(length));
        WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), static_cast<int>(wide.size()), line.data(), length, nullptr, nullptr);
        return true;
    }
#endif
    if (!getline(cin, line)) return false;
    if (!line.empty() && line.back() == '\r') line.pop_back();
    return true;
}

filesystem::path to_path(const string& utf8_path)
{
    return filesystem::u8path(utf8_path);
}

string to_utf8(const filesystem::path& path)
{
    return path.u8string();
}

filesystem::path executable_directory()
{
#if defined(_WIN32)
    wchar_t buffer[MAX_PATH];
    DWORD length = GetModuleFileNameW(nullptr, buffer, MAX_PATH);
    if (length > 0 && length < MAX_PATH) return filesystem::path(buffer).parent_path();
#else
    error_code error;
    filesystem::path self = filesystem::read_symlink("/proc/self/exe", error);
    if (!error) return self.parent_path();
#endif
    return filesystem::current_path();
}

}  // namespace platform
