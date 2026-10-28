#include <algorithm>
#include <iostream>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

#include "library_loader.h"
#include "platform.h"

using namespace std;
using library_loader::CipherLibrary;

namespace {

using Bytes = vector<uint8_t>;

struct TestCase {
    string title;
    Bytes key;
    Bytes iv;        // пустой — алгоритм без вектора инициализации
    Bytes input;
    Bytes expected;  // пустой — проверяется только совпадение после расшифрования
};

Bytes text_bytes(const string& text)
{
    return Bytes(text.begin(), text.end());
}

Bytes hex_bytes(const string& hex)
{
    Bytes result;
    for (size_t i = 0; i + 1 < hex.size(); i += 2) result.push_back(static_cast<uint8_t>(stoi(hex.substr(i, 2), nullptr, 16)));
    return result;
}

Bytes random_bytes(size_t size, unsigned seed)
{
    mt19937 generator(seed);
    uniform_int_distribution<int> byte_value(0, 255);
    Bytes data(size);
    generate(data.begin(), data.end(), [&]() { return static_cast<uint8_t>(byte_value(generator)); });
    return data;
}

Bytes transposition_key(const Bytes& columns, const Bytes& rows)
{
    Bytes key(18, 0);
    key[0] = static_cast<uint8_t>(columns.size());
    key[1] = static_cast<uint8_t>(rows.size());
    copy(columns.begin(), columns.end(), key.begin() + 2);
    copy(rows.begin(), rows.end(), key.begin() + 10);
    return key;
}

bool run_case(const CipherLibrary& library, const TestCase& test, string& details)
{
    Bytes cipher(library.get_output_size(test.input.size(), OPERATION_ENCRYPT));
    MutBuffer cipher_buffer{ cipher.data(), cipher.size() };
    ConstBuffer key{ test.key.data(), test.key.size() };
    ConstBuffer input{ test.input.data(), test.input.size() };

    int code = test.iv.empty()
        ? library.encrypt(key, input, &cipher_buffer)
        : library.encrypt_with_iv(key, ConstBuffer{ test.iv.data(), test.iv.size() }, input, &cipher_buffer);
    if (code != CRYPTO_OK) {
        details = "код ошибки шифрования " + to_string(code);
        return false;
    }
    cipher.resize(cipher_buffer.size);
    if (!test.expected.empty() && cipher != test.expected) {
        details = "шифротекст не совпадает с эталоном";
        return false;
    }

    Bytes plain(library.get_output_size(cipher.size(), OPERATION_DECRYPT));
    MutBuffer plain_buffer{ plain.data(), plain.size() };
    code = library.decrypt(key, ConstBuffer{ cipher.data(), cipher.size() }, &plain_buffer);
    if (code != CRYPTO_OK) {
        details = "код ошибки расшифрования " + to_string(code);
        return false;
    }
    plain.resize(plain_buffer.size);
    if (plain != test.input) {
        details = "расшифрованные данные не совпадают с исходными";
        return false;
    }
    details = to_string(test.input.size()) + " байт -> " + to_string(cipher.size()) + " байт";
    return true;
}

vector<TestCase> cases_for(const string& algorithm)
{
    if (algorithm == "atbash") {
        return {
            { "контрольный пример", {}, {}, text_bytes("Shnitko Nikita ABs-527 Шнитко"),
              text_bytes("Hsmrgpl Mrprgz ZYh-527 Зтчнхс") },
            { "случайные данные 4 КиБ", {}, {}, random_bytes(4096, 101), {} },
            { "случайные данные ~1 МиБ", {}, {}, random_bytes((1u << 20) + 3, 102), {} },
        };
    }
    if (algorithm == "transposition") {
        return {
            { "контрольный пример", transposition_key({ 2, 0, 1 }, { 1, 2, 0 }), {}, text_bytes("SHNITKO"),
              text_bytes("XXOHNSTKI") },
            { "случайные данные 4 КиБ, таблица 4x2", transposition_key({ 3, 0, 2, 1 }, { 1, 0 }), {},
              random_bytes(4096, 201), {} },
            { "случайные данные ~1 МиБ, таблица 8x8",
              transposition_key({ 5, 2, 7, 0, 3, 6, 1, 4 }, { 7, 3, 0, 5, 1, 6, 2, 4 }), {},
              random_bytes((1u << 20) + 3, 202), {} },
        };
    }
    Bytes report_key = text_bytes("nikita");
    report_key.resize(16, 0);
    return {
        { "контрольный пример", report_key, hex_bytes("0102030405060708"), text_bytes("Шнитко АБс-527"),
          hex_bytes("0102030405060708deb9eadf87e214a2317b53b476fbf519687bd7bf8b29ebe0") },
        { "случайные данные 4 КиБ", random_bytes(16, 301), hex_bytes("A1B2C3D4E5F60718"), random_bytes(4096, 302), {} },
        { "случайные данные ~1 МиБ", random_bytes(16, 303), hex_bytes("0F1E2D3C4B5A6978"),
          random_bytes((1u << 20) + 3, 304), {} },
    };
}

}  // namespace

int main()
{
    platform::setup_console();
    int total = 0;
    int failed = 0;

    for (const auto& algorithm : library_loader::supported_algorithms()) {
        CipherLibrary library;
        try {
            library = library_loader::load_cipher_library(algorithm.name);
        } catch (const runtime_error& error) {
            cout << "[FAIL] " << algorithm.name << ": " << error.what() << '\n';
            ++failed;
            continue;
        }

        int number = 0;
        for (const TestCase& test : cases_for(algorithm.name)) {
            ++number;
            ++total;
            string details;
            bool passed = run_case(library, test, details);
            if (!passed) ++failed;
            cout << (passed ? "[ OK ] " : "[FAIL] ") << algorithm.name << ", тест " << number << " (" << test.title
                 << "): " << details << '\n';
        }
        library_loader::unload_cipher_library(library);
    }

    cout << "\nВсего тестов: " << total << ", ошибок: " << failed << '\n';
    return failed == 0 ? 0 : 1;
}
