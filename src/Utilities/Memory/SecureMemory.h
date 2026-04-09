#pragma once

#include <QtCore/QByteArray>
#include <array>
#include <cstddef>
#include <cstring>

#if defined(_WIN32)
#include <windows.h>
#elif (defined(__linux__) && !defined(__ANDROID__)) || defined(__FreeBSD__)
#include <strings.h>  // explicit_bzero (Bionic exposes it only via <string.h>, gated on API; use fallback)
#endif

namespace QGC {

/// Wipe `size` bytes of `data`. Uses platform-provided primitives that the spec or vendor docs
/// guarantee are not elidable by the optimizer (explicit_bzero / SecureZeroMemory).
/// Falls back to a volatile-pointer loop on platforms without one (including Apple).
inline void secureZero(void* data, size_t size)
{
    if (data == nullptr || size == 0) {
        return;
    }
#if defined(_WIN32)
    SecureZeroMemory(data, size);
#elif (defined(__linux__) && !defined(__ANDROID__)) || defined(__FreeBSD__)
    explicit_bzero(data, size);
#else
    volatile unsigned char* p = static_cast<volatile unsigned char*>(data);
    for (size_t i = 0; i < size; ++i) {
        p[i] = 0;
    }
    // Memory barrier prevents the optimizer from coalescing writes with later allocator reuse.
    asm volatile("" ::: "memory");
#endif
}

/// Reliable wipe only when this is the sole owner; data() detaches a shared buffer, leaving the original copy intact.
/// Prefer std::array for secrets.
inline void secureZero(QByteArray& data)
{
    if (!data.isEmpty()) {
        secureZero(data.data(), static_cast<size_t>(data.size()));
    }
    data.clear();
}

template <typename T, size_t N>
inline void secureZero(std::array<T, N>& arr)
{
    secureZero(arr.data(), arr.size() * sizeof(T));
}

}  // namespace QGC
