#pragma once

#include <cstddef>
#include <cstdint>

#include "secure_memory.h"

#if defined(_WIN32)
    #include <bcrypt.h>
#else
    #include <cerrno>
    #include <sys/random.h>
#endif

// Криптостойкий генератор ОС: BCryptGenRandom (Windows) или getrandom (Linux)
inline bool fill_random_bytes(uint8_t* buffer, size_t size)
{
#if defined(_WIN32)
    return BCryptGenRandom(nullptr, buffer, static_cast<ULONG>(size), BCRYPT_USE_SYSTEM_PREFERRED_RNG) == 0;
#else
    size_t filled = 0;
    while (filled < size) {
        ssize_t received = getrandom(buffer + filled, size - filled, 0);
        if (received < 0) {
            if (errno == EINTR) continue;
            return false;
        }
        filled += static_cast<size_t>(received);
    }
    return true;
#endif
}

// Равномерное число из [0, bound) без смещения: значения из неполного «хвоста» байта отбрасываются
inline bool random_index(size_t bound, size_t& result)
{
    if (bound == 0 || bound > 256) return false;
    const unsigned limit = 256u - (256u % static_cast<unsigned>(bound));
    uint8_t value = 0;
    do {
        if (!fill_random_bytes(&value, 1)) return false;
    } while (value >= limit);
    result = value % bound;
    return true;
}
