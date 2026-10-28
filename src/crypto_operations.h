#pragma once

#include <cstddef>
#include <cstdint>
#include <istream>
#include <ostream>
#include <string>
#include <vector>

#include "library_loader.h"

namespace crypto_operations {

// Данные обрабатываются частями по 1 МиБ, поэтому расход памяти не зависит от размера файла.
// Шифротекст — последовательность записей: [длина записи, 4 байта little-endian][результат encrypt()].
constexpr size_t CHUNK_SIZE = 1u << 20;
constexpr size_t RECORD_HEADER_SIZE = 4;

struct OperationResult {
    uint64_t bytes_read = 0;
    uint64_t bytes_written = 0;
    uint64_t chunks = 0;
};

std::string describe_error(int code);

std::vector<uint8_t> generate_key(const library_loader::CipherLibrary& library);

// Преобразование небольшого буфера целиком (шифрование и расшифрование текста)
std::vector<uint8_t> encrypt_bytes(const library_loader::CipherLibrary& library, const std::vector<uint8_t>& key,
                                   const std::vector<uint8_t>& data);
std::vector<uint8_t> decrypt_bytes(const library_loader::CipherLibrary& library, const std::vector<uint8_t>& key,
                                   const std::vector<uint8_t>& data);

OperationResult encrypt_stream(const library_loader::CipherLibrary& library, const std::vector<uint8_t>& key,
                               std::istream& input, std::ostream& output);

OperationResult decrypt_stream(const library_loader::CipherLibrary& library, const std::vector<uint8_t>& key,
                               std::istream& input, std::ostream& output);

}  // namespace crypto_operations
