#pragma once

#include <filesystem>
#include <string>

namespace platform {

// Windows: кодовая страница UTF-8 и поддержка цветовых escape-последовательностей
void setup_console();

// Читает строку с клавиатуры в UTF-8; false — ввод закрыт
bool read_console_line(std::string& line);

// Путь из строки UTF-8 с учётом того, что Windows хранит пути в UTF-16
std::filesystem::path to_path(const std::string& utf8_path);

std::string to_utf8(const std::filesystem::path& path);

std::filesystem::path executable_directory();

}  // namespace platform
