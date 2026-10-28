#pragma once

#include <cstddef>
#include <cstdint>

#if defined(_WIN32)
    #define CRYPTO_EXPORT extern "C" __declspec(dllexport)
#else
    #define CRYPTO_EXPORT extern "C" __attribute__((visibility("default")))
#endif

struct ConstBuffer {
    const uint8_t* data;
    size_t size;
};

struct MutBuffer {
    uint8_t* data;
    size_t size;
};

struct AlgorithmInfo {
    const char* algorithm_name;
    const char* description;
    size_t key_size;
};

constexpr int OPERATION_ENCRYPT = 0;
constexpr int OPERATION_DECRYPT = 1;

constexpr int CRYPTO_OK = 0;
constexpr int CRYPTO_ERROR_INVALID_ARGUMENT = 1;
constexpr int CRYPTO_ERROR_INVALID_KEY = 2;
constexpr int CRYPTO_ERROR_BUFFER_TOO_SMALL = 3;
constexpr int CRYPTO_ERROR_CORRUPTED_DATA = 4;
constexpr int CRYPTO_ERROR_RANDOM_FAILURE = 5;
constexpr int CRYPTO_ERROR_INTERNAL = 6;

// Выходной буфер выделяет вызывающая сторона размером не меньше get_output_size();
// после успешного вызова output->size содержит фактическую длину результата.
using GetAlgorithmInfoFunction = const AlgorithmInfo* (*)();
using GetOutputSizeFunction = size_t (*)(size_t input_size, int operation_type);
using CryptFunction = int (*)(ConstBuffer key, ConstBuffer input, MutBuffer* output);
using GenerateKeyFunction = int (*)(MutBuffer* key);
// Только для тестирования: шифрование с заданным вручную вектором инициализации
using EncryptWithIvFunction = int (*)(ConstBuffer key, ConstBuffer iv, ConstBuffer input, MutBuffer* output);
