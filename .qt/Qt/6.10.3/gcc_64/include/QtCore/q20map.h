// Copyright (C) 2023 Ahmad Samir <a.samirh78@gmail.com>
// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef Q20MAP_H
#define Q20MAP_H

#include <QtCore/qtconfigmacros.h>

#include <map>
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
// like std::erase/std::erase_if for std::map
#if defined(__cpp_lib_erase_if) && __cpp_lib_erase_if >= 202002L // the one returning size_type
using std::erase_if;
#else

// Make it more specialized than the compiler's, so that our implementation is preferred over
// the compiler's (which may be present, but return void instead of the number of erased elements).

#define MAKE_OVERLOAD(map, allocator) \
    template <typename Key, typename T, typename Compare, typename Pred> \
    constexpr typename std::map<Key, T, Compare, std::allocator<std::pair<const Key, T>>>::size_type \
    erase_if(std::map<Key, T, Compare, std::allocator<std::pair<const Key, T>>> &c, Pred p) \
    { \
        const auto origSize = c.size(); \
        for (auto it = c.begin(), end = c.end(); it != end; /* erasing */) { \
            if (p(*it)) \
                it = c.erase(it); \
            else \
                ++it; \
        } \
        return origSize - c.size(); \
    } \
    /* end */

MAKE_OVERLOAD(map, allocator)
MAKE_OVERLOAD(multimap, allocator)
#ifdef __cpp_lib_polymorphic_allocator
MAKE_OVERLOAD(map, pmr::polymorphic_allocator)
MAKE_OVERLOAD(multimap, pmr::polymorphic_allocator)
#endif // __cpp_lib_polymorphic_allocator

#undef MAKE_OVERLOAD

#endif // __cpp_lib_erase_if
} // namespace q20

QT_END_NAMESPACE

#endif /* Q20MAP_H */
