#pragma once

#include <filesystem>
#include <string>

namespace file_browser {

// Рабочие папки создаются рядом с местом запуска программы: files/input, files/output, files/keys
std::filesystem::path input_folder();
std::filesystem::path output_folder();
std::filesystem::path keys_folder();

void ensure_folders();

std::string display_name(const std::filesystem::path& path);

// Нумерованный список файлов папки; пункт 0 — ввод пути вручную
std::filesystem::path choose_file(const std::filesystem::path& folder, const std::string& title);

// Имя без пути сохраняется в указанную папку; пустой ввод — имя по умолчанию
std::filesystem::path ask_output_path(const std::filesystem::path& folder, const std::string& default_name);

}  // namespace file_browser
