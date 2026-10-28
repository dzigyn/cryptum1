#include "file_browser.h"

#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <vector>

#include "console_ui.h"
#include "platform.h"

using namespace std;

namespace file_browser {

namespace {

filesystem::path workspace_folder(const char* name)
{
    return (filesystem::path("files") / name).make_preferred();
}

vector<filesystem::path> list_files(const filesystem::path& folder)
{
    vector<filesystem::path> files;
    error_code error;
    for (const auto& entry : filesystem::directory_iterator(folder, error)) {
        if (entry.is_regular_file()) files.push_back(entry.path());
    }
    sort(files.begin(), files.end());
    return files;
}

}  // namespace

filesystem::path input_folder()
{
    return workspace_folder("input");
}

filesystem::path output_folder()
{
    return workspace_folder("output");
}

filesystem::path keys_folder()
{
    return workspace_folder("keys");
}

void ensure_folders()
{
    for (const filesystem::path& folder : { input_folder(), output_folder(), keys_folder() }) {
        error_code error;
        filesystem::create_directories(folder, error);
    }
}

string display_name(const filesystem::path& path)
{
    return platform::to_utf8(path.lexically_normal());
}

filesystem::path choose_file(const filesystem::path& folder, const string& title)
{
    vector<filesystem::path> files = list_files(folder);
    cout << '\n' << title << " из папки " << display_name(folder) << ":\n";
    if (files.empty()) {
        cout << "  (папка пуста)\n";
    } else {
        cout << "Доступные файлы:\n";
        for (size_t i = 0; i < files.size(); ++i) {
            cout << "  " << i + 1 << ". " << platform::to_utf8(files[i].filename()) << '\n';
        }
    }
    cout << "  0. Ввести путь вручную\n";

    int choice = console_ui::ask_choice("Выберите файл: ", 0, static_cast<int>(files.size()));
    if (choice > 0) return files[static_cast<size_t>(choice - 1)];

    string path = console_ui::read_line("Введите путь к файлу: ");
    if (path.empty()) throw runtime_error("путь к файлу не указан");
    return platform::to_path(path);
}

filesystem::path ask_output_path(const filesystem::path& folder, const string& default_name)
{
    string name = console_ui::read_line("Введите имя выходного файла (будет сохранён в " + display_name(folder) +
                                        ") [" + default_name + "]: ");
    if (name.empty()) name = default_name;
    filesystem::path path = platform::to_path(name);
    return path.has_parent_path() ? path : folder / path;
}

}  // namespace file_browser
