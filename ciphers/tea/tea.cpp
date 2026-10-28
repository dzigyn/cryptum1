#include <algorithm>
#include <array>

#include "crypto_api.h"
#include "random_bytes.h"
#include "secure_memory.h"

using namespace std;

namespace {

constexpr size_t KEY_SIZE = 16;
constexpr size_t BLOCK_SIZE = 8;
constexpr size_t IV_SIZE = BLOCK_SIZE;
constexpr uint32_t DELTA = 0x9E3779B9u;  // floor((sqrt(5) - 1) * 2^31), Wheeler & Needham, 1994
constexpr int CYCLES = 32;

constexpr AlgorithmInfo TEA_INFO = {
    "TEA", "Tiny Encryption Algorithm: блок 64 бита, ключ 128 бит, режим CBC", KEY_SIZE
};

using Block = array<uint8_t, BLOCK_SIZE>;
using KeyWords = array<uint32_t, 4>;

uint32_t load_word(const uint8_t* bytes)
{
    return static_cast<uint32_t>(bytes[0]) | (static_cast<uint32_t>(bytes[1]) << 8) |
           (static_cast<uint32_t>(bytes[2]) << 16) | (static_cast<uint32_t>(bytes[3]) << 24);
}

void store_word(uint32_t word, uint8_t* bytes)
{
    for (int i = 0; i < 4; ++i) bytes[i] = static_cast<uint8_t>(word >> (8 * i));
}

KeyWords load_key(const uint8_t* key)
{
    return { load_word(key), load_word(key + 4), load_word(key + 8), load_word(key + 12) };
}

void encrypt_block(Block& block, const KeyWords& key)
{
    uint32_t v0 = load_word(block.data());
    uint32_t v1 = load_word(block.data() + 4);
    uint32_t sum = 0;
    for (int cycle = 0; cycle < CYCLES; ++cycle) {
        sum += DELTA;
        v0 += ((v1 << 4) + key[0]) ^ (v1 + sum) ^ ((v1 >> 5) + key[1]);
        v1 += ((v0 << 4) + key[2]) ^ (v0 + sum) ^ ((v0 >> 5) + key[3]);
    }
    store_word(v0, block.data());
    store_word(v1, block.data() + 4);
}

void decrypt_block(Block& block, const KeyWords& key)
{
    uint32_t v0 = load_word(block.data());
    uint32_t v1 = load_word(block.data() + 4);
    uint32_t sum = DELTA * CYCLES;
    for (int cycle = 0; cycle < CYCLES; ++cycle) {
        v1 -= ((v0 << 4) + key[2]) ^ (v0 + sum) ^ ((v0 >> 5) + key[3]);
        v0 -= ((v1 << 4) + key[0]) ^ (v1 + sum) ^ ((v1 >> 5) + key[1]);
        sum -= DELTA;
    }
    store_word(v0, block.data());
    store_word(v1, block.data() + 4);
}

void xor_into(Block& block, const uint8_t* mask)
{
    transform(block.begin(), block.end(), mask, block.begin(), [](uint8_t a, uint8_t b) { return a ^ b; });
}

size_t encrypted_size(size_t input_size)
{
    return IV_SIZE + (input_size / BLOCK_SIZE + 1) * BLOCK_SIZE;
}

int encrypt_cbc(ConstBuffer key, const uint8_t* iv, ConstBuffer input, MutBuffer* output)
{
    if (output == nullptr || (input.data == nullptr && input.size > 0)) return CRYPTO_ERROR_INVALID_ARGUMENT;
    if (key.data == nullptr || key.size != KEY_SIZE) return CRYPTO_ERROR_INVALID_KEY;
    const size_t result_size = encrypted_size(input.size);
    if (output->data == nullptr || output->size < result_size) return CRYPTO_ERROR_BUFFER_TOO_SMALL;

    KeyWords words = load_key(key.data);
    copy(iv, iv + IV_SIZE, output->data);

    // PKCS#7: добавляется p = 8 - (L mod 8) байт со значением p
    const size_t padding = BLOCK_SIZE - input.size % BLOCK_SIZE;
    const size_t padded_size = input.size + padding;
    const uint8_t* previous = output->data;
    Block block{};

    for (size_t offset = 0; offset < padded_size; offset += BLOCK_SIZE) {
        for (size_t i = 0; i < BLOCK_SIZE; ++i) {
            size_t position = offset + i;
            block[i] = position < input.size ? input.data[position] : static_cast<uint8_t>(padding);
        }
        xor_into(block, previous);
        encrypt_block(block, words);
        uint8_t* destination = output->data + IV_SIZE + offset;
        copy(block.begin(), block.end(), destination);
        previous = destination;
    }

    secure_wipe(block.data(), block.size());
    secure_wipe(words.data(), sizeof(words));
    output->size = result_size;
    return CRYPTO_OK;
}

}  // namespace

CRYPTO_EXPORT const AlgorithmInfo* get_algorithm_info()
{
    return &TEA_INFO;
}

CRYPTO_EXPORT size_t get_output_size(size_t input_size, int operation_type)
{
    if (operation_type == OPERATION_ENCRYPT) return encrypted_size(input_size);
    return input_size > IV_SIZE ? input_size - IV_SIZE : 0;
}

CRYPTO_EXPORT int encrypt(ConstBuffer key, ConstBuffer input, MutBuffer* output)
{
    Block iv{};
    if (!fill_random_bytes(iv.data(), iv.size())) return CRYPTO_ERROR_RANDOM_FAILURE;
    return encrypt_cbc(key, iv.data(), input, output);
}

CRYPTO_EXPORT int encrypt_with_iv(ConstBuffer key, ConstBuffer iv, ConstBuffer input, MutBuffer* output)
{
    if (iv.data == nullptr || iv.size != IV_SIZE) return CRYPTO_ERROR_INVALID_ARGUMENT;
    return encrypt_cbc(key, iv.data, input, output);
}

CRYPTO_EXPORT int decrypt(ConstBuffer key, ConstBuffer input, MutBuffer* output)
{
    if (output == nullptr || input.data == nullptr) return CRYPTO_ERROR_INVALID_ARGUMENT;
    if (key.data == nullptr || key.size != KEY_SIZE) return CRYPTO_ERROR_INVALID_KEY;
    if (input.size < IV_SIZE + BLOCK_SIZE || (input.size - IV_SIZE) % BLOCK_SIZE != 0) return CRYPTO_ERROR_CORRUPTED_DATA;
    const size_t body_size = input.size - IV_SIZE;
    if (output->data == nullptr || output->size < body_size) return CRYPTO_ERROR_BUFFER_TOO_SMALL;

    KeyWords words = load_key(key.data);
    const uint8_t* previous = input.data;
    Block block{};

    for (size_t offset = 0; offset < body_size; offset += BLOCK_SIZE) {
        const uint8_t* source = input.data + IV_SIZE + offset;
        copy(source, source + BLOCK_SIZE, block.begin());
        decrypt_block(block, words);
        xor_into(block, previous);
        copy(block.begin(), block.end(), output->data + offset);
        previous = source;
    }
    secure_wipe(block.data(), block.size());
    secure_wipe(words.data(), sizeof(words));

    const uint8_t padding = output->data[body_size - 1];
    const uint8_t* data_end = output->data + body_size;
    bool valid = padding >= 1 && padding <= BLOCK_SIZE &&
                 all_of(data_end - padding, data_end, [padding](uint8_t byte) { return byte == padding; });
    if (!valid) {
        secure_wipe(output->data, body_size);
        return CRYPTO_ERROR_CORRUPTED_DATA;
    }
    output->size = body_size - padding;
    return CRYPTO_OK;
}

CRYPTO_EXPORT int generate_key(MutBuffer* key)
{
    if (key == nullptr || key->data == nullptr || key->size < KEY_SIZE) return CRYPTO_ERROR_INVALID_ARGUMENT;
    if (!fill_random_bytes(key->data, KEY_SIZE)) return CRYPTO_ERROR_RANDOM_FAILURE;
    key->size = KEY_SIZE;
    return CRYPTO_OK;
}
