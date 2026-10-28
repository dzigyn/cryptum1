#pragma once

#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

namespace console_ui {

// Бросается, когда стандартный ввод закрыт (например, при перенаправлении из файла)
struct InputClosed : std::runtime_error {
    InputClosed() : std::runtime_error("ввод завершён") {}
};

void print_separator();
void print_title(const std::string& title);
void print_success(const std::string& message);
void print_warning(const std::string& message);
void print_error(const std::string& message);
void print_highlight(const std::string& message);

std::string read_line(const std::string& prompt);

// Повторяет запрос, пока не будет введено целое число из диапазона [min_value, max_value]
int ask_choice(const std::string& prompt, int min_value, int max_value);

bool ask_yes_no(const std::string& prompt);

void wait_for_enter();

std::string to_hex(const std::vector<uint8_t>& bytes);

// Принимает пары шестнадцатеричных цифр с пробелами или без: "D0 A8" или "d0a8"
bool parse_hex(const std::string& text, std::vector<uint8_t>& bytes);

// Вывод в виде таблицы: смещение, байты в HEX и их представление в ASCII
void print_hex_dump(const std::vector<uint8_t>& bytes, size_t total_size);

}  // namespace console_ui
