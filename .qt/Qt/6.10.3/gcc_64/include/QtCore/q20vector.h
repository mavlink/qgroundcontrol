// Copyright (C) 2023 Ahmad Samir <a.samirh78@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef Q20VECTOR_H
#define Q20VECTOR_H

#include <QtCore/qtconfigmacros.h>

#include <algorithm>
#include <vector>
#if __has_include(<memory_resource>)
#  include <memory_resource>
#endif

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. Types and functions defined in this
// file can reliably be replaced by their std counterparts, once available.
// You may use these definitions in your own code, but be aware that we
// will remove them once Qt depends on the C++ version that supports
// them in namespace std. There will be NO deprecation warning, the
// definitions will JUST go away.
//
// If you can't agree to these terms, don't use these definitions!
//
// We mean it.
//

QT_BEGIN_NAMESPACE

namespace q20 {
// like std::erase/std::erase_if for std::vector
#if defined(__cpp_lib_erase_if) && __cpp_lib_erase_if >= 202002L // the one returning size_type
using std::erase;
using std::erase_if;
#else

// Make it more specialized than the compiler's, so that our implementation is preferred over
// the compiler's (which may be present, but return void instead of the number of erased elements).

template <typename T, typename U>
constexpr typename std::vector<T, std::allocator<T>>::size_type
erase(std::vector<T, std::allocator<T>> &c, const U &value)
{
    const auto origSize = c.size();
    auto it = std::remove(c.begin(), c.end(), value);
    c.erase(it, c.end());
    return origSize - c.size();
}

template <typename T, typename Pred>
constexpr typename std::vector<T, std::allocator<T>>::size_type
erase_if(std::vector<T, std::allocator<T>> &c, Pred pred)
{
    const auto origSize = c.size();
    auto it = std::remove_if(c.begin(), c.end(), pred);
    c.erase(it, c.end());
    return origSize - c.size();
}

#ifdef __cpp_lib_polymorphic_allocator
template <typename T, typename U>
constexpr typename std::vector<T, std::pmr::polymorphic_allocator<T>>::size_type
erase(std::vector<T, std::pmr::polymorphic_allocator<T>> &c, const U &value)
{
    const auto origSize = c.size();
    auto it = std::remove(c.begin(), c.end(), value);
    c.erase(it, c.end());
    return origSize - c.size();
}

template <typename T, typename Pred>
constexpr typename std::vector<T, std::pmr::polymorphic_allocator<T>>::size_type
erase_if(std::vector<T, std::pmr::polymorphic_allocator<T>> &c, Pred pred)
{
    const auto origSize = c.size();
    auto it = std::remove_if(c.begin(), c.end(), pred);
    c.erase(it, c.end());
    return origSize - c.size();
}
#endif // __cpp_lib_polymorphic_allocator

#endif // __cpp_lib_erase_if
} // namespace q20

QT_END_NAMESPACE

#endif /* Q20VECTOR_H */
