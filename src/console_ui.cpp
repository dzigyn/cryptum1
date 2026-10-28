#include "console_ui.h"

#include <algorithm>
#include <cctype>
#include <iomanip>
#include <iostream>
#include <sstream>

#include "platform.h"

using namespace std;

namespace console_ui {

namespace {

constexpr const char* COLOR_RESET = "\033[0m";
constexpr const char* COLOR_TITLE = "\033[1;36m";
constexpr const char* COLOR_SUCCESS = "\033[32m";
constexpr const char* COLOR_WARNING = "\033[33m";
constexpr const char* COLOR_ERROR = "\033[31m";
constexpr const char* COLOR_HIGHLIGHT = "\033[1;37m";
constexpr size_t SEPARATOR_WIDTH = 44;
constexpr size_t BYTES_PER_LINE = 16;

int hex_value(char symbol)
{
    if (symbol >= '0' && symbol <= '9') return symbol - '0';
    symbol = static_cast<char>(toupper(static_cast<unsigned char>(symbol)));
    if (symbol >= 'A' && symbol <= 'F') return symbol - 'A' + 10;
    return -1;
}

void print_colored(const char* color, const string& message, ostream& stream)
{
    stream << color << message << COLOR_RESET << '\n';
}

}  // namespace

void print_separator()
{
    cout << string(SEPARATOR_WIDTH, '=') << '\n';
}

void print_title(const string& title)
{
    print_separator();
    cout << COLOR_TITLE << "  " << title << COLOR_RESET << '\n';
    print_separator();
}

void print_success(const string& message)
{
    print_colored(COLOR_SUCCESS, message, cout);
}

void print_warning(const string& message)
{
    print_colored(COLOR_WARNING, message, cout);
}

void print_error(const string& message)
{
    cout.flush();
    print_colored(COLOR_ERROR, message, cerr);
}

void print_highlight(const string& message)
{
    print_colored(COLOR_HIGHLIGHT, message, cout);
}

string read_line(const string& prompt)
{
    cout << prompt << flush;
    string line;
    if (!platform::read_console_line(line)) throw InputClosed();
    return line;
}

int ask_choice(const string& prompt, int min_value, int max_value)
{
    string current_prompt = prompt;
    while (true) {
        string line = read_line(current_prompt);
        line.erase(remove_if(line.begin(), line.end(), [](unsigned char c) { return isspace(c); }), line.end());
        bool is_number = !line.empty() && line.size() <= 9 &&
                         all_of(line.begin(), line.end(), [](unsigned char c) { return isdigit(c); });
        if (!is_number) {
            current_prompt = "Неверный ввод. Повторите: ";
            continue;
        }
        int value = stoi(line);
        if (value >= min_value && value <= max_value) return value;
        current_prompt = "Неверный выбор. Повторите: ";
    }
}

bool ask_yes_no(const string& prompt)
{
    string current_prompt = prompt + " (да/нет): ";
    while (true) {
        string answer = read_line(current_prompt);
        if (answer == "да" || answer == "Да" || answer == "ДА" || answer == "y" || answer == "Y") return true;
        if (answer == "нет" || answer == "Нет" || answer == "НЕТ" || answer == "n" || answer == "N") return false;
        current_prompt = "Ответьте «да» или «нет»: ";
    }
}

void wait_for_enter()
{
    read_line("\nНажмите Enter, чтобы вернуться в меню...");
}

string to_hex(const vector<uint8_t>& bytes)
{
    ostringstream stream;
    stream << uppercase << hex << setfill('0');
    for (size_t i = 0; i < bytes.size(); ++i) {
        if (i > 0) stream << ' ';
        stream << setw(2) << static_cast<int>(bytes[i]);
    }
    return stream.str();
}

bool parse_hex(const string& text, vector<uint8_t>& bytes)
{
    string digits;
    copy_if(text.begin(), text.end(), back_inserter(digits), [](unsigned char c) { return !isspace(c); });
    if (digits.empty() || digits.size() % 2 != 0) return false;

    vector<uint8_t> result;
    for (size_t i = 0; i < digits.size(); i += 2) {
        int high = hex_value(digits[i]);
        int low = hex_value(digits[i + 1]);
        if (high < 0 || low < 0) return false;
        result.push_back(static_cast<uint8_t>(high * 16 + low));
    }
    bytes.swap(result);
    return true;
}

void print_hex_dump(const vector<uint8_t>& bytes, size_t total_size)
{
    cout << "Смещение  00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F  Текст\n";
    for (size_t offset = 0; offset < bytes.size(); offset += BYTES_PER_LINE) {
        size_t line_end = min(offset + BYTES_PER_LINE, bytes.size());
        cout << uppercase << hex << setfill('0') << setw(8) << offset << "  ";
        for (size_t i = offset; i < offset + BYTES_PER_LINE; ++i) {
            if (i < line_end) cout << setw(2) << static_cast<int>(bytes[i]) << ' ';
            else cout << "   ";
        }
        cout << ' ';
        for (size_t i = offset; i < line_end; ++i) cout << (bytes[i] >= 0x20 && bytes[i] < 0x7F ? static_cast<char>(bytes[i]) : '.');
        cout << dec << '\n';
    }
    if (total_size > bytes.size()) {
        cout << "... показаны первые " << bytes.size() << " байт из " << total_size << '\n';
    }
}

}  // namespace console_ui
