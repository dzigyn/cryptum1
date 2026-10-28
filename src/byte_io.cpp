#include "byte_io.h"

#include <stdexcept>

#include "platform.h"
#include "secure_memory.h"

using namespace std;

namespace byte_io {

ifstream open_input(const filesystem::path& path)
{
    ifstream file(path, ios::in | ios::binary);
    if (!file) throw runtime_error("не удалось открыть файл «" + platform::to_utf8(path) + "»");
    return file;
}

ofstream open_output(const filesystem::path& path)
{
    ofstream file(path, ios::out | ios::binary | ios::trunc);
    if (!file) throw runtime_error("не удалось создать файл «" + platform::to_utf8(path) + "»");
    return file;
}

size_t read_up_to(istream& input, uint8_t* buffer, size_t size)
{
    size_t total = 0;
    while (total < size && input) {
        input.read(reinterpret_cast<char*>(buffer + total), static_cast<streamsize>(size - total));
        total += static_cast<size_t>(input.gcount());
    }
    if (input.bad()) throw runtime_error("ошибка чтения файла");
    return total;
}

void write_all(ostream& output, const uint8_t* data, size_t size)
{
    if (size == 0) return;
    output.write(reinterpret_cast<const char*>(data), static_cast<streamsize>(size));
    if (!output) throw runtime_error("ошибка записи файла");
}

vector<uint8_t> read_small_file(const filesystem::path& path, size_t max_size)
{
    ifstream file = open_input(path);
    vector<uint8_t> data(max_size + 1);
    size_t size = read_up_to(file, data.data(), data.size());
    if (size > max_size) {
        secure_wipe(data);
        throw runtime_error("файл «" + platform::to_utf8(path) + "» слишком большой");
    }
    data.resize(size);
    return data;
}

void save_file(const filesystem::path& path, const vector<uint8_t>& data)
{
    ofstream file = open_output(path);
    write_all(file, data.data(), data.size());
}

void discard_file(const filesystem::path& path)
{
    error_code ignored;
    filesystem::remove(path, ignored);
}

}  // namespace byte_io
