#include <algorithm>
#include <array>
#include <numeric>

#include "crypto_api.h"
#include "random_bytes.h"
#include "secure_memory.h"

using namespace std;

namespace {

constexpr size_t MAX_SIDE = 8;
// Формат ключа: [число столбцов C][число строк R][перестановка столбцов, 8 байт][перестановка строк, 8 байт].
// Байт i перестановки — номер (с нуля) места, на которое переходит i-й столбец (строка); неиспользуемые байты равны 0.
constexpr size_t KEY_SIZE = 2 + 2 * MAX_SIDE;
constexpr uint8_t FILLER = 'X';
constexpr uint8_t ALTERNATIVE_FILLER = 'Y';

constexpr AlgorithmInfo TRANSPOSITION_INFO = {
    "Двойная табличная перестановка", "перестановка строк и столбцов таблицы до 8x8", KEY_SIZE
};

struct TranspositionKey {
    size_t columns;
    size_t rows;
    array<size_t, MAX_SIDE> column_target;
    array<size_t, MAX_SIDE> column_source;
    array<size_t, MAX_SIDE> row_target;
    array<size_t, MAX_SIDE> row_source;
};

bool parse_permutation(const uint8_t* bytes, size_t length, array<size_t, MAX_SIDE>& target,
                       array<size_t, MAX_SIDE>& source)
{
    array<bool, MAX_SIDE> used{};
    for (size_t i = 0; i < length; ++i) {
        if (bytes[i] >= length || used[bytes[i]]) return false;
        used[bytes[i]] = true;
        target[i] = bytes[i];
        source[bytes[i]] = i;
    }
    return all_of(bytes + length, bytes + MAX_SIDE, [](uint8_t unused) { return unused == 0; });
}

bool parse_key(ConstBuffer key, TranspositionKey& parsed)
{
    if (key.data == nullptr || key.size != KEY_SIZE) return false;
    parsed.columns = key.data[0];
    parsed.rows = key.data[1];
    if (parsed.columns == 0 || parsed.columns > MAX_SIDE || parsed.rows == 0 || parsed.rows > MAX_SIDE) return false;
    return parse_permutation(key.data + 2, parsed.columns, parsed.column_target, parsed.column_source) &&
           parse_permutation(key.data + 2 + MAX_SIDE, parsed.rows, parsed.row_target, parsed.row_source);
}

size_t target_index(const TranspositionKey& key, size_t cell, bool encrypt)
{
    size_t row = cell / key.columns;
    size_t column = cell % key.columns;
    return encrypt ? key.row_target[row] * key.columns + key.column_target[column]
                   : key.row_source[row] * key.columns + key.column_source[column];
}

}  // namespace

CRYPTO_EXPORT const AlgorithmInfo* get_algorithm_info()
{
    return &TRANSPOSITION_INFO;
}

// Точный размер зависит от ключа, поэтому возвращается верхняя граница
CRYPTO_EXPORT size_t get_output_size(size_t input_size, int operation_type)
{
    return operation_type == OPERATION_ENCRYPT ? input_size + MAX_SIDE * MAX_SIDE : input_size;
}

CRYPTO_EXPORT int encrypt(ConstBuffer key, ConstBuffer input, MutBuffer* output)
{
    if (output == nullptr || (input.data == nullptr && input.size > 0)) return CRYPTO_ERROR_INVALID_ARGUMENT;
    TranspositionKey parsed{};
    if (!parse_key(key, parsed)) return CRYPTO_ERROR_INVALID_KEY;

    // Дополнение всегда занимает от 1 до C*R байт; заполнитель отличается от последнего байта данных,
    // поэтому при расшифровании граница определяется однозначно
    const size_t block_size = parsed.columns * parsed.rows;
    const size_t padded_size = (input.size / block_size + 1) * block_size;
    if (output->data == nullptr || output->size < padded_size) return CRYPTO_ERROR_BUFFER_TOO_SMALL;

    const uint8_t filler = (input.size > 0 && input.data[input.size - 1] == FILLER) ? ALTERNATIVE_FILLER : FILLER;
    for (size_t block = 0; block < padded_size; block += block_size) {
        for (size_t cell = 0; cell < block_size; ++cell) {
            size_t position = block + cell;
            output->data[block + target_index(parsed, cell, true)] =
                position < input.size ? input.data[position] : filler;
        }
    }
    output->size = padded_size;
    return CRYPTO_OK;
}

CRYPTO_EXPORT int decrypt(ConstBuffer key, ConstBuffer input, MutBuffer* output)
{
    if (output == nullptr || input.data == nullptr) return CRYPTO_ERROR_INVALID_ARGUMENT;
    TranspositionKey parsed{};
    if (!parse_key(key, parsed)) return CRYPTO_ERROR_INVALID_KEY;

    const size_t block_size = parsed.columns * parsed.rows;
    if (input.size == 0 || input.size % block_size != 0) return CRYPTO_ERROR_CORRUPTED_DATA;
    if (output->data == nullptr || output->size < input.size) return CRYPTO_ERROR_BUFFER_TOO_SMALL;

    for (size_t block = 0; block < input.size; block += block_size) {
        for (size_t cell = 0; cell < block_size; ++cell) {
            output->data[block + target_index(parsed, cell, false)] = input.data[block + cell];
        }
    }

    const uint8_t filler = output->data[input.size - 1];
    if (filler != FILLER && filler != ALTERNATIVE_FILLER) {
        secure_wipe(output->data, input.size);
        return CRYPTO_ERROR_CORRUPTED_DATA;
    }
    const uint8_t* data_end = output->data + input.size;
    const uint8_t* padding_start = find_if(make_reverse_iterator(data_end),
                                           make_reverse_iterator(data_end - block_size),
                                           [filler](uint8_t byte) { return byte != filler; }).base();
    output->size = static_cast<size_t>(padding_start - output->data);
    return CRYPTO_OK;
}

CRYPTO_EXPORT int generate_key(MutBuffer* key)
{
    if (key == nullptr || key->data == nullptr || key->size < KEY_SIZE) return CRYPTO_ERROR_INVALID_ARGUMENT;

    fill(key->data, key->data + KEY_SIZE, uint8_t{ 0 });
    key->data[0] = MAX_SIDE;
    key->data[1] = MAX_SIDE;

    // Перемешивание Фишера–Йетса на криптостойком генераторе
    for (uint8_t* permutation : { key->data + 2, key->data + 2 + MAX_SIDE }) {
        iota(permutation, permutation + MAX_SIDE, uint8_t{ 0 });
        for (size_t i = MAX_SIDE - 1; i > 0; --i) {
            size_t j = 0;
            if (!random_index(i + 1, j)) {
                secure_wipe(key->data, KEY_SIZE);
                return CRYPTO_ERROR_RANDOM_FAILURE;
            }
            swap(permutation[i], permutation[j]);
        }
    }
    key->size = KEY_SIZE;
    return CRYPTO_OK;
}
