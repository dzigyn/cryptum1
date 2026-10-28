#include "crypto_operations.h"

#include <stdexcept>

#include "byte_io.h"
#include "secure_memory.h"

using namespace std;

namespace crypto_operations {

namespace {

void store_length(uint32_t length, uint8_t* bytes)
{
    for (size_t i = 0; i < RECORD_HEADER_SIZE; ++i) bytes[i] = static_cast<uint8_t>(length >> (8 * i));
}

uint32_t load_length(const uint8_t* bytes)
{
    uint32_t length = 0;
    for (size_t i = 0; i < RECORD_HEADER_SIZE; ++i) length |= static_cast<uint32_t>(bytes[i]) << (8 * i);
    return length;
}

ConstBuffer as_const(const vector<uint8_t>& bytes, size_t size)
{
    return ConstBuffer{ bytes.data(), size };
}

void check(int code, const char* operation)
{
    if (code != CRYPTO_OK) throw runtime_error(string(operation) + ": " + describe_error(code));
}

}  // namespace

string describe_error(int code)
{
    switch (code) {
        case CRYPTO_OK: return "нет ошибки";
        case CRYPTO_ERROR_INVALID_ARGUMENT: return "некорректные аргументы функции библиотеки";
        case CRYPTO_ERROR_INVALID_KEY: return "неверный формат или длина ключа";
        case CRYPTO_ERROR_BUFFER_TOO_SMALL: return "недостаточный размер выходного буфера";
        case CRYPTO_ERROR_CORRUPTED_DATA: return "данные повреждены или ключ не подходит";
        case CRYPTO_ERROR_RANDOM_FAILURE: return "сбой генератора случайных чисел";
        case CRYPTO_ERROR_INTERNAL: return "внутренняя ошибка библиотеки";
        default: return "неизвестный код ошибки " + to_string(code);
    }
}

vector<uint8_t> generate_key(const library_loader::CipherLibrary& library)
{
    vector<uint8_t> key(library.info->key_size);
    MutBuffer buffer{ key.data(), key.size() };
    int code = library.generate_key(&buffer);
    if (code != CRYPTO_OK || buffer.size != library.info->key_size) {
        secure_wipe(key);
        throw runtime_error("генерация ключа: " + describe_error(code == CRYPTO_OK ? CRYPTO_ERROR_INTERNAL : code));
    }
    return key;
}

vector<uint8_t> encrypt_bytes(const library_loader::CipherLibrary& library, const vector<uint8_t>& key,
                              const vector<uint8_t>& data)
{
    vector<uint8_t> result(library.get_output_size(data.size(), OPERATION_ENCRYPT));
    MutBuffer output{ result.data(), result.size() };
    check(library.encrypt(as_const(key, key.size()), as_const(data, data.size()), &output), "шифрование");
    result.resize(output.size);
    return result;
}

vector<uint8_t> decrypt_bytes(const library_loader::CipherLibrary& library, const vector<uint8_t>& key,
                              const vector<uint8_t>& data)
{
    if (data.empty()) throw runtime_error("расшифрование: нет данных");
    vector<uint8_t> result(library.get_output_size(data.size(), OPERATION_DECRYPT));
    MutBuffer output{ result.data(), result.size() };
    int code = library.decrypt(as_const(key, key.size()), as_const(data, data.size()), &output);
    if (code != CRYPTO_OK) {
        secure_wipe(result);
        check(code, "расшифрование");
    }
    result.resize(output.size);
    return result;
}

OperationResult encrypt_stream(const library_loader::CipherLibrary& library, const vector<uint8_t>& key,
                               istream& input, ostream& output)
{
    vector<uint8_t> plain(CHUNK_SIZE);
    vector<uint8_t> record(RECORD_HEADER_SIZE + library.get_output_size(CHUNK_SIZE, OPERATION_ENCRYPT));
    OperationResult result;

    try {
        size_t read = 0;
        while ((read = byte_io::read_up_to(input, plain.data(), plain.size())) > 0) {
            MutBuffer cipher{ record.data() + RECORD_HEADER_SIZE, record.size() - RECORD_HEADER_SIZE };
            check(library.encrypt(as_const(key, key.size()), as_const(plain, read), &cipher), "шифрование");

            store_length(static_cast<uint32_t>(cipher.size), record.data());
            byte_io::write_all(output, record.data(), RECORD_HEADER_SIZE + cipher.size);

            result.bytes_read += read;
            result.bytes_written += RECORD_HEADER_SIZE + cipher.size;
            ++result.chunks;
        }
        output.flush();
    } catch (...) {
        secure_wipe(plain);
        throw;
    }
    secure_wipe(plain);
    return result;
}

OperationResult decrypt_stream(const library_loader::CipherLibrary& library, const vector<uint8_t>& key,
                               istream& input, ostream& output)
{
    const size_t max_record = library.get_output_size(CHUNK_SIZE, OPERATION_ENCRYPT);
    vector<uint8_t> record(max_record);
    vector<uint8_t> plain(library.get_output_size(max_record, OPERATION_DECRYPT));
    OperationResult result;

    try {
        uint8_t header[RECORD_HEADER_SIZE];
        size_t header_read = 0;
        while ((header_read = byte_io::read_up_to(input, header, RECORD_HEADER_SIZE)) > 0) {
            if (header_read != RECORD_HEADER_SIZE) throw runtime_error("расшифрование: данные обрезаны");
            uint32_t length = load_length(header);
            if (length == 0 || length > max_record) throw runtime_error("расшифрование: " + describe_error(CRYPTO_ERROR_CORRUPTED_DATA));
            if (byte_io::read_up_to(input, record.data(), length) != length) {
                throw runtime_error("расшифрование: данные обрезаны");
            }

            MutBuffer decrypted{ plain.data(), plain.size() };
            check(library.decrypt(as_const(key, key.size()), as_const(record, length), &decrypted), "расшифрование");
            byte_io::write_all(output, plain.data(), decrypted.size);

            result.bytes_read += RECORD_HEADER_SIZE + length;
            result.bytes_written += decrypted.size;
            ++result.chunks;
        }
        output.flush();
    } catch (...) {
        secure_wipe(plain);
        throw;
    }
    secure_wipe(plain);
    return result;
}

}  // namespace crypto_operations
