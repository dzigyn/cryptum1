#pragma once

#include <string>
#include <vector>

#include "crypto_api.h"

namespace library_loader {

struct CipherLibrary {
    void* handle = nullptr;
    std::string file_name;
    const AlgorithmInfo* info = nullptr;
    GetOutputSizeFunction get_output_size = nullptr;
    CryptFunction encrypt = nullptr;
    CryptFunction decrypt = nullptr;
    GenerateKeyFunction generate_key = nullptr;
    EncryptWithIvFunction encrypt_with_iv = nullptr;  // экспортируется только алгоритмами с вектором инициализации
};

struct SupportedAlgorithm {
    std::string name;
    std::string library_base_name;
};

const std::vector<SupportedAlgorithm>& supported_algorithms();

// atbash -> atbash.dll (Windows) или libatbash.so (Linux)
std::string library_file_name(const std::string& base_name);

// Бросает std::runtime_error, если библиотека не найдена или не экспортирует обязательные функции
CipherLibrary load_cipher_library(const std::string& algorithm);

void unload_cipher_library(CipherLibrary& library);

// Загружает все библиотеки из перечня; причины неудачных загрузок добавляются в errors
std::vector<CipherLibrary> load_all_libraries(std::vector<std::string>& errors);

void unload_all_libraries(std::vector<CipherLibrary>& libraries);

}  // namespace library_loader
