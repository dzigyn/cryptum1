#include <algorithm>

#include "crypto_api.h"

using namespace std;

namespace {

constexpr AlgorithmInfo ATBASH_INFO = { "Атбаш", "зеркальная замена латинских и русских букв", 0 };

constexpr uint8_t CYRILLIC_LEAD_LOW = 0xD0;
constexpr uint8_t CYRILLIC_LEAD_HIGH = 0xD1;
constexpr unsigned CYRILLIC_UPPER_FIRST = 0x0410;
constexpr unsigned CYRILLIC_LOWER_FIRST = 0x0430;
constexpr unsigned CYRILLIC_LAST = 0x044F;
constexpr unsigned CYRILLIC_ALPHABET_SIZE = 32;

uint8_t mirror_latin(uint8_t symbol)
{
    if (symbol >= 'A' && symbol <= 'Z') return static_cast<uint8_t>('Z' - (symbol - 'A'));
    if (symbol >= 'a' && symbol <= 'z') return static_cast<uint8_t>('z' - (symbol - 'a'));
    return symbol;
}

// Русские буквы А..Я, а..я (без Ё) в UTF-8 — два байта: 0xD0/0xD1 и байт продолжения
bool decode_cyrillic(const uint8_t* bytes, const uint8_t* end, unsigned& code_point)
{
    if (end - bytes < 2) return false;
    if ((bytes[0] != CYRILLIC_LEAD_LOW && bytes[0] != CYRILLIC_LEAD_HIGH) || (bytes[1] & 0xC0) != 0x80) return false;
    code_point = (static_cast<unsigned>(bytes[0] & 0x1F) << 6) | (bytes[1] & 0x3F);
    return code_point >= CYRILLIC_UPPER_FIRST && code_point <= CYRILLIC_LAST;
}

void mirror_bytes(const uint8_t* begin, const uint8_t* end, uint8_t* output)
{
    copy(begin, end, output);
    uint8_t* cursor = output;
    uint8_t* output_end = output + (end - begin);

    while (cursor < output_end) {
        unsigned code_point = 0;
        if (decode_cyrillic(cursor, output_end, code_point)) {
            unsigned first = code_point < CYRILLIC_LOWER_FIRST ? CYRILLIC_UPPER_FIRST : CYRILLIC_LOWER_FIRST;
            unsigned mirrored = first + (CYRILLIC_ALPHABET_SIZE - 1) - (code_point - first);
            cursor[0] = static_cast<uint8_t>(0xC0 | (mirrored >> 6));
            cursor[1] = static_cast<uint8_t>(0x80 | (mirrored & 0x3F));
            cursor += 2;
        } else {
            *cursor = mirror_latin(*cursor);
            ++cursor;
        }
    }
}

int transform(ConstBuffer key, ConstBuffer input, MutBuffer* output)
{
    if (output == nullptr || (input.data == nullptr && input.size > 0)) return CRYPTO_ERROR_INVALID_ARGUMENT;
    if (key.size != 0) return CRYPTO_ERROR_INVALID_KEY;
    if (output->size < input.size || (output->data == nullptr && input.size > 0)) return CRYPTO_ERROR_BUFFER_TOO_SMALL;

    if (input.size > 0) mirror_bytes(input.data, input.data + input.size, output->data);
    output->size = input.size;
    return CRYPTO_OK;
}

}  // namespace

CRYPTO_EXPORT const AlgorithmInfo* get_algorithm_info()
{
    return &ATBASH_INFO;
}

CRYPTO_EXPORT size_t get_output_size(size_t input_size, int operation_type)
{
    (void)operation_type;
    return input_size;
}

CRYPTO_EXPORT int encrypt(ConstBuffer key, ConstBuffer input, MutBuffer* output)
{
    return transform(key, input, output);
}

CRYPTO_EXPORT int decrypt(ConstBuffer key, ConstBuffer input, MutBuffer* output)
{
    return transform(key, input, output);
}

CRYPTO_EXPORT int generate_key(MutBuffer* key)
{
    if (key == nullptr) return CRYPTO_ERROR_INVALID_ARGUMENT;
    key->size = 0;
    return CRYPTO_OK;
}
