#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#include "console_ui.h"
#include "file_browser.h"
#include "library_loader.h"
#include "menu.h"
#include "platform.h"

using namespace std;

namespace {

constexpr int EXIT_OK = 0;
constexpr int EXIT_FAILURE_RUNTIME = 1;
constexpr int EXIT_FAILURE_USAGE = 2;

void print_help()
{
    cout << "Multi-Algo Cryptotool (cryptum) — шифрование и расшифрование текста и файлов\n\n"
            "Синтаксис:\n"
            "  cryptum              запуск интерактивного меню\n"
            "  cryptum -h, --help   вывод этой справки\n\n"
            "Пункты меню: 1 — шифрование текста, 2 — дешифрование текста, 3 — шифрование файла,\n"
            "4 — дешифрование файла, 5 — генератор ключей, 6 — просмотр файла, 0 — выход.\n\n"
            "Алгоритмы (библиотеки загружаются из папки программы):\n";
    for (const auto& algorithm : library_loader::supported_algorithms()) {
        cout << "  " << left << setw(15) << algorithm.name << "(" << library_loader::library_file_name(algorithm.library_base_name) << ")\n";
    }
    cout << "\nРабочие папки создаются в текущей папке:\n"
            "  files/input   — исходные файлы\n"
            "  files/output  — зашифрованные и расшифрованные файлы\n"
            "  files/keys    — файлы ключей\n";
}

}  // namespace

int main(int argc, char** argv)
{
    platform::setup_console();

    if (argc > 1) {
        string argument = argv[1];
        if (argument == "-h" || argument == "--help") {
            print_help();
            return EXIT_OK;
        }
        console_ui::print_error("Неизвестный аргумент «" + argument + "». Справка: cryptum -h");
        return EXIT_FAILURE_USAGE;
    }

    cout << "Загрузка библиотек шифрования...\n";
    vector<string> errors;
    vector<library_loader::CipherLibrary> libraries = library_loader::load_all_libraries(errors);
    for (const string& error : errors) console_ui::print_error("[Ошибка] " + error);
    if (libraries.empty()) {
        console_ui::print_error("Не найдено ни одной библиотеки шифрования. Работа программы завершена.");
        return EXIT_FAILURE_RUNTIME;
    }

    cout << "Загружено библиотек: " << libraries.size() << '\n';
    for (const auto& library : libraries) {
        cout << "  + " << library.info->algorithm_name << " (" << library.file_name << ")\n";
    }
    file_browser::ensure_folders();

    int exit_code = EXIT_OK;
    try {
        menu::run(libraries);
    } catch (const console_ui::InputClosed&) {
        cout << "\nВвод завершён. Выход из программы.\n";
    } catch (const exception& error) {
        console_ui::print_error(string("[Ошибка] ") + error.what());
        exit_code = EXIT_FAILURE_RUNTIME;
    }
    library_loader::unload_all_libraries(libraries);
    return exit_code;
}
