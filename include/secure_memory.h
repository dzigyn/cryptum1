#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#if defined(_WIN32)
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #include <windows.h>
#endif

// Очистка через volatile-указатель (или SecureZeroMemory) не удаляется оптимизатором
inline void secure_wipe(void* data, size_t size)
{
    if (data == nullptr || size == 0) return;
#if defined(_WIN32)
    SecureZeroMemory(data, size);
#else
    volatile uint8_t* cursor = static_cast<volatile uint8_t*>(data);
    while (size-- > 0) *cursor++ = 0;
#endif
}

inline void secure_wipe(std::vector<uint8_t>& bytes)
{
    secure_wipe(bytes.data(), bytes.size());
}

// Затирает буфер при выходе из области видимости, в том числе при исключении
struct ScopedWipe {
    std::vector<uint8_t>& bytes;
    ~ScopedWipe() { secure_wipe(bytes); }
};
