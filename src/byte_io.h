#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace byte_io {

// Файлы всегда открываются в двоичном режиме, поэтому поддерживаются данные любого формата
std::ifstream open_input(const std::filesystem::path& path);
std::ofstream open_output(const std::filesystem::path& path);

// Читает до size байт; меньшее значение означает конец файла
size_t read_up_to(std::istream& input, uint8_t* buffer, size_t size);

void write_all(std::ostream& output, const uint8_t* data, size_t size);

std::vector<uint8_t> read_small_file(const std::filesystem::path& path, size_t max_size);
void save_file(const std::filesystem::path& path, const std::vector<uint8_t>& data);

// Удаляет недописанный выходной файл после ошибки, чтобы не оставлять частичный результат
void discard_file(const std::filesystem::path& path);

}  // namespace byte_io
