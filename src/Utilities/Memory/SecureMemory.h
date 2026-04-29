#pragma once

#include <QtCore/QByteArray>
#include <array>
#include <cstddef>

/// Volatile pointer prevents dead-store elision; use instead of memset (elidable) or QByteArray::fill (COW detach).
namespace QGC {

inline void secureZero(void* data, size_t size)
{
    auto* p = static_cast<volatile unsigned char*>(data);
    while (size--) {
        *p++ = 0;
    }
}

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
