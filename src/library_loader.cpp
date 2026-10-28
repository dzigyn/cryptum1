#include "library_loader.h"

#include <algorithm>
#include <stdexcept>

#include "platform.h"

#if defined(_WIN32)
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #include <windows.h>
#else
    #include <dlfcn.h>
#endif

using namespace std;

namespace library_loader {

namespace {

const vector<SupportedAlgorithm> SUPPORTED_ALGORITHMS = {
    { "atbash", "atbash" },
    { "transposition", "transposition" },
    { "tea", "tea" },
};

void* open_library(const filesystem::path& path)
{
#if defined(_WIN32)
    return LoadLibraryW(path.wstring().c_str());
#else
    return dlopen(path.c_str(), RTLD_NOW | RTLD_LOCAL);
#endif
}

string last_load_error()
{
#if defined(_WIN32)
    DWORD code = GetLastError();
    char* message = nullptr;
    FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                   nullptr, code, MAKELANGID(LANG_ENGLISH, SUBLANG_DEFAULT), reinterpret_cast<char*>(&message), 0,
                   nullptr);
    string text = message != nullptr ? message : "код " + to_string(code);
    if (message != nullptr) LocalFree(message);
    text.erase(find_if(text.rbegin(), text.rend(), [](char c) { return c != '\r' && c != '\n'; }).base(), text.end());
    return text;
#else
    const char* message = dlerror();
    return message != nullptr ? message : "неизвестная ошибка";
#endif
}

// Приведение через void(*)() не вызывает предупреждения о несовместимых типах функций
template <typename Function>
Function find_symbol(void* handle, const char* name)
{
#if defined(_WIN32)
    FARPROC address = GetProcAddress(static_cast<HMODULE>(handle), name);
    return reinterpret_cast<Function>(reinterpret_cast<void (*)()>(address));
#else
    return reinterpret_cast<Function>(dlsym(handle, name));
#endif
}

void close_library(void* handle)
{
#if defined(_WIN32)
    FreeLibrary(static_cast<HMODULE>(handle));
#else
    dlclose(handle);
#endif
}

}  // namespace

const vector<SupportedAlgorithm>& supported_algorithms()
{
    return SUPPORTED_ALGORITHMS;
}

string library_file_name(const string& base_name)
{
#if defined(_WIN32)
    return base_name + ".dll";
#else
    return "lib" + base_name + ".so";
#endif
}

CipherLibrary load_cipher_library(const string& algorithm)
{
    auto entry = find_if(SUPPORTED_ALGORITHMS.begin(), SUPPORTED_ALGORITHMS.end(),
                         [&algorithm](const SupportedAlgorithm& item) { return item.name == algorithm; });
    if (entry == SUPPORTED_ALGORITHMS.end()) throw runtime_error("неизвестный алгоритм «" + algorithm + "»");

    CipherLibrary library;
    library.file_name = library_file_name(entry->library_base_name);
    filesystem::path path = platform::executable_directory() / library.file_name;

    library.handle = open_library(path);
    if (library.handle == nullptr) {
        throw runtime_error("не удалось загрузить библиотеку " + library.file_name + ": " + last_load_error());
    }

    auto get_info = find_symbol<GetAlgorithmInfoFunction>(library.handle, "get_algorithm_info");
    library.get_output_size = find_symbol<GetOutputSizeFunction>(library.handle, "get_output_size");
    library.encrypt = find_symbol<CryptFunction>(library.handle, "encrypt");
    library.decrypt = find_symbol<CryptFunction>(library.handle, "decrypt");
    library.generate_key = find_symbol<GenerateKeyFunction>(library.handle, "generate_key");
    library.encrypt_with_iv = find_symbol<EncryptWithIvFunction>(library.handle, "encrypt_with_iv");

    if (get_info == nullptr || library.get_output_size == nullptr || library.encrypt == nullptr ||
        library.decrypt == nullptr || library.generate_key == nullptr) {
        unload_cipher_library(library);
        throw runtime_error("библиотека " + path.filename().u8string() + " не экспортирует обязательные функции");
    }
    library.info = get_info();
    if (library.info == nullptr || library.info->algorithm_name == nullptr) {
        unload_cipher_library(library);
        throw runtime_error("библиотека " + path.filename().u8string() + " вернула некорректные метаданные");
    }
    return library;
}

void unload_cipher_library(CipherLibrary& library)
{
    if (library.handle != nullptr) close_library(library.handle);
    library = CipherLibrary{};
}

vector<CipherLibrary> load_all_libraries(vector<string>& errors)
{
    vector<CipherLibrary> libraries;
    for (const SupportedAlgorithm& algorithm : SUPPORTED_ALGORITHMS) {
        try {
            libraries.push_back(load_cipher_library(algorithm.name));
        } catch (const runtime_error& error) {
            errors.push_back(error.what());
        }
    }
    return libraries;
}

void unload_all_libraries(vector<CipherLibrary>& libraries)
{
    for_each(libraries.begin(), libraries.end(), unload_cipher_library);
    libraries.clear();
}

}  // namespace library_loader
