#include "menu.h"

#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <string>

#include "byte_io.h"
#include "console_ui.h"
#include "crypto_operations.h"
#include "file_browser.h"
#include "platform.h"
#include "secure_memory.h"

using namespace std;
using library_loader::CipherLibrary;

namespace menu {

namespace {

constexpr size_t MAX_KEY_FILE_SIZE = 4096;
constexpr size_t VIEW_LIMIT = 512;
constexpr const char* ENCRYPTED_EXTENSION = ".enc";

enum class MainMenuItem {
    EXIT = 0,
    ENCRYPT_TEXT = 1,
    DECRYPT_TEXT = 2,
    ENCRYPT_FILE = 3,
    DECRYPT_FILE = 4,
    GENERATE_KEY = 5,
    VIEW_FILE = 6
};

enum class KeySource {
    BACK = 0,
    HEX = 1,
    TEXT = 2,
    FILE = 3,
    GENERATE = 4
};

MainMenuItem show_main_menu()
{
    cout << '\n';
    console_ui::print_title("CRYPTUM — СИСТЕМА ШИФРОВАНИЯ");
    cout << "  1. Шифрование текста\n"
            "  2. Дешифрование текста\n"
            "  3. Шифрование файла\n"
            "  4. Дешифрование файла\n"
            "  5. Генератор ключей\n"
            "  6. Просмотр файла\n"
            "  0. Выход\n";
    console_ui::print_separator();
    return static_cast<MainMenuItem>(console_ui::ask_choice("Выберите пункт: ", 0, 6));
}

const CipherLibrary* choose_algorithm(const vector<CipherLibrary>& libraries)
{
    cout << "\nДоступные алгоритмы:\n";
    for (size_t i = 0; i < libraries.size(); ++i) {
        cout << "  " << i + 1 << ". " << libraries[i].info->algorithm_name << " — " << libraries[i].info->description
             << '\n';
    }
    cout << "  0. Назад\n";
    int choice = console_ui::ask_choice("Выберите алгоритм: ", 0, static_cast<int>(libraries.size()));
    return choice == 0 ? nullptr : &libraries[static_cast<size_t>(choice - 1)];
}

void check_key_size(const CipherLibrary& library, vector<uint8_t>& key)
{
    if (key.size() != library.info->key_size) {
        size_t actual = key.size();
        secure_wipe(key);
        throw runtime_error("длина ключа " + to_string(actual) + " байт, а алгоритм требует " +
                            to_string(library.info->key_size) + " байт");
    }
}

void offer_to_save_key(const vector<uint8_t>& key)
{
    if (!console_ui::ask_yes_no("Сохранить ключ в файл?")) return;
    filesystem::path path = file_browser::ask_output_path(file_browser::keys_folder(), "new.key");
    byte_io::save_file(path, key);
    console_ui::print_success("Ключ сохранён в файл " + file_browser::display_name(path));
}

// false — пользователь вернулся назад
bool obtain_key(const CipherLibrary& library, bool allow_generate, vector<uint8_t>& key)
{
    key.clear();
    const size_t key_size = library.info->key_size;
    if (key_size == 0) {
        cout << "Алгоритм «" << library.info->algorithm_name << "» не использует ключ.\n";
        return true;
    }

    cout << "\nВвод ключа (длина " << key_size << " байт):\n"
            "  1. Ввести вручную (hex)\n"
            "  2. Ввести вручную (текст)\n"
            "  3. Загрузить из файла\n";
    if (allow_generate) cout << "  4. Сгенерировать случайный\n";
    cout << "  0. Назад\n";
    auto source = static_cast<KeySource>(console_ui::ask_choice("Выберите: ", 0, allow_generate ? 4 : 3));

    switch (source) {
        case KeySource::BACK:
            return false;
        case KeySource::HEX: {
            string text = console_ui::read_line("Введите ключ (hex): ");
            if (!console_ui::parse_hex(text, key)) throw runtime_error("не удалось разобрать hex-строку ключа");
            check_key_size(library, key);
            return true;
        }
        case KeySource::TEXT: {
            string text = console_ui::read_line("Введите ключ (текст): ");
            if (text.empty() || text.size() > key_size) {
                throw runtime_error("текстовый ключ должен содержать от 1 до " + to_string(key_size) + " байт");
            }
            key.assign(text.begin(), text.end());
            secure_wipe(text.data(), text.size());
            if (key.size() < key_size) {
                cout << "Ключ дополнен нулевыми байтами до " << key_size << " байт.\n";
                key.resize(key_size, 0);
            }
            return true;
        }
        case KeySource::FILE: {
            filesystem::path path = file_browser::choose_file(file_browser::keys_folder(), "Выберите файл ключа");
            key = byte_io::read_small_file(path, MAX_KEY_FILE_SIZE);
            check_key_size(library, key);
            cout << "Ключ загружен из файла " << file_browser::display_name(path) << ".\n";
            return true;
        }
        case KeySource::GENERATE:
            key = crypto_operations::generate_key(library);
            cout << "Сгенерированный ключ (hex): " << console_ui::to_hex(key) << '\n';
            console_ui::print_warning("Сохраните ключ — без него данные не расшифровать!");
            offer_to_save_key(key);
            return true;
    }
    return false;
}

void encrypt_text(const vector<CipherLibrary>& libraries)
{
    console_ui::print_title("ШИФРОВАНИЕ ТЕКСТА");
    const CipherLibrary* library = choose_algorithm(libraries);
    vector<uint8_t> key;
    ScopedWipe key_guard{ key };
    if (library == nullptr || !obtain_key(*library, true, key)) return;

    string text = console_ui::read_line("Введите текст для шифрования:\n");
    if (text.empty()) throw runtime_error("текст не может быть пустым");
    vector<uint8_t> plain(text.begin(), text.end());
    ScopedWipe plain_guard{ plain };
    secure_wipe(text.data(), text.size());

    vector<uint8_t> cipher = crypto_operations::encrypt_bytes(*library, key, plain);
    cout << "\nРезультат (hex):\n" << console_ui::to_hex(cipher) << '\n';
}

void decrypt_text(const vector<CipherLibrary>& libraries)
{
    console_ui::print_title("ДЕШИФРОВАНИЕ ТЕКСТА");
    const CipherLibrary* library = choose_algorithm(libraries);
    vector<uint8_t> key;
    ScopedWipe key_guard{ key };
    if (library == nullptr || !obtain_key(*library, false, key)) return;

    vector<uint8_t> cipher;
    if (!console_ui::parse_hex(console_ui::read_line("Введите hex-строку для дешифрования:\n"), cipher)) {
        throw runtime_error("не удалось разобрать hex-строку");
    }
    vector<uint8_t> plain = crypto_operations::decrypt_bytes(*library, key, cipher);
    ScopedWipe plain_guard{ plain };
    cout << "\nРезультат:\n" << string(plain.begin(), plain.end()) << '\n';
}

void process_file(const CipherLibrary& library, const vector<uint8_t>& key, const filesystem::path& input_path,
                  const filesystem::path& output_path, bool encrypt)
{
    ifstream input = byte_io::open_input(input_path);
    ofstream output = byte_io::open_output(output_path);
    try {
        crypto_operations::OperationResult result =
            encrypt ? crypto_operations::encrypt_stream(library, key, input, output)
                    : crypto_operations::decrypt_stream(library, key, input, output);
        output.close();
        console_ui::print_success(string(encrypt ? "Файл успешно зашифрован: " : "Файл успешно дешифрован: ") +
                                  file_browser::display_name(output_path));
        cout << "Прочитано " << result.bytes_read << " байт, записано " << result.bytes_written << " байт.\n";
    } catch (...) {
        output.close();
        byte_io::discard_file(output_path);
        throw;
    }
}

void encrypt_file(const vector<CipherLibrary>& libraries)
{
    console_ui::print_title("ШИФРОВАНИЕ ФАЙЛА");
    const CipherLibrary* library = choose_algorithm(libraries);
    vector<uint8_t> key;
    ScopedWipe key_guard{ key };
    if (library == nullptr || !obtain_key(*library, true, key)) return;

    filesystem::path input = file_browser::choose_file(file_browser::input_folder(), "Выберите файл для шифрования");
    string default_name = platform::to_utf8(input.filename()) + ENCRYPTED_EXTENSION;
    filesystem::path output = file_browser::ask_output_path(file_browser::output_folder(), default_name);
    process_file(*library, key, input, output, true);
}

void decrypt_file(const vector<CipherLibrary>& libraries)
{
    console_ui::print_title("ДЕШИФРОВАНИЕ ФАЙЛА");
    const CipherLibrary* library = choose_algorithm(libraries);
    vector<uint8_t> key;
    ScopedWipe key_guard{ key };
    if (library == nullptr || !obtain_key(*library, false, key)) return;

    filesystem::path input = file_browser::choose_file(file_browser::output_folder(), "Выберите файл для дешифрования");
    string name = platform::to_utf8(input.filename());
    const string extension = ENCRYPTED_EXTENSION;
    if (name.size() > extension.size() && name.compare(name.size() - extension.size(), extension.size(), extension) == 0) {
        name.erase(name.size() - extension.size());
    }
    filesystem::path output = file_browser::ask_output_path(file_browser::output_folder(), "decrypted_" + name);
    process_file(*library, key, input, output, false);
}

void generate_key(const vector<CipherLibrary>& libraries)
{
    console_ui::print_title("ГЕНЕРАТОР КЛЮЧЕЙ");
    const CipherLibrary* library = choose_algorithm(libraries);
    if (library == nullptr) return;
    if (library->info->key_size == 0) {
        console_ui::print_warning("Алгоритм «" + string(library->info->algorithm_name) + "» не использует ключ.");
        return;
    }
    vector<uint8_t> key = crypto_operations::generate_key(*library);
    ScopedWipe key_guard{ key };
    cout << "Длина ключа: " << key.size() << " байт\n"
         << "Ключ (hex): " << console_ui::to_hex(key) << '\n';
    offer_to_save_key(key);
}

bool looks_like_text(const vector<uint8_t>& bytes)
{
    return all_of(bytes.begin(), bytes.end(), [](uint8_t byte) { return byte >= 0x20 || byte == '\n' || byte == '\r' || byte == '\t'; });
}

void view_file()
{
    console_ui::print_title("ПРОСМОТР ФАЙЛА");
    const filesystem::path folders[] = { file_browser::input_folder(), file_browser::output_folder(),
                                         file_browser::keys_folder() };
    cout << "\nПапка:\n";
    for (size_t i = 0; i < 3; ++i) cout << "  " << i + 1 << ". " << file_browser::display_name(folders[i]) << '\n';
    cout << "  0. Назад\n";
    int choice = console_ui::ask_choice("Выберите: ", 0, 3);
    if (choice == 0) return;

    filesystem::path path = file_browser::choose_file(folders[choice - 1], "Выберите файл для просмотра");
    ifstream file = byte_io::open_input(path);
    vector<uint8_t> bytes(VIEW_LIMIT);
    bytes.resize(byte_io::read_up_to(file, bytes.data(), bytes.size()));
    error_code error;
    uintmax_t total = filesystem::file_size(path, error);

    cout << "\nФайл: " << file_browser::display_name(path) << ", размер " << (error ? bytes.size() : total) << " байт\n";
    console_ui::print_hex_dump(bytes, error ? bytes.size() : static_cast<size_t>(total));
    if (!bytes.empty() && looks_like_text(bytes)) {
        cout << "\nСодержимое как текст:\n" << string(bytes.begin(), bytes.end()) << '\n';
    }
}

}  // namespace

void run(const vector<CipherLibrary>& libraries)
{
    while (true) {
        MainMenuItem item = show_main_menu();
        if (item == MainMenuItem::EXIT) {
            cout << "Выход из программы.\n";
            return;
        }
        try {
            switch (item) {
                case MainMenuItem::ENCRYPT_TEXT: encrypt_text(libraries); break;
                case MainMenuItem::DECRYPT_TEXT: decrypt_text(libraries); break;
                case MainMenuItem::ENCRYPT_FILE: encrypt_file(libraries); break;
                case MainMenuItem::DECRYPT_FILE: decrypt_file(libraries); break;
                case MainMenuItem::GENERATE_KEY: generate_key(libraries); break;
                case MainMenuItem::VIEW_FILE: view_file(); break;
                case MainMenuItem::EXIT: break;
            }
        } catch (const console_ui::InputClosed&) {
            throw;
        } catch (const bad_alloc&) {
            console_ui::print_error("[Ошибка] Недостаточно оперативной памяти");
        } catch (const exception& error) {
            console_ui::print_error(string("[Ошибка] ") + error.what());
        }
        console_ui::wait_for_enter();
    }
}

}  // namespace menu
