// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef Q20UTILITY_H
#define Q20UTILITY_H

#include <QtCore/qttypetraits.h>

#include <limits>
#include <utility>

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
#ifdef __cpp_lib_integer_comparison_functions
using std::cmp_equal;
using std::cmp_not_equal;
using std::cmp_less;
using std::cmp_greater;
using std::cmp_less_equal;
using std::cmp_greater_equal;
using std::in_range;
#else
namespace detail {
template <class T, class U>
constexpr void checkTypeCompatibility() noexcept
{
    // Both T and U are standard integer types or extended integer types,
    // see https://eel.is/c++draft/utility.intcmp#1
    static_assert(QtPrivate::is_standard_or_extended_integer_type_v<T>,
                  "Integer types should be used for left T class.");
    static_assert(QtPrivate::is_standard_or_extended_integer_type_v<U>,
                  "Integer types should be used for right U class.");
}
}

template <class T, class U>
constexpr bool cmp_equal(T t, U u) noexcept
{
    // Both T and U are standard integer types or extended integer types
    // https://eel.is/c++draft/utility.intcmp#1
    detail::checkTypeCompatibility<T, U>();
    // See https://eel.is/c++draft/utility.intcmp#2
    using UT = std::make_unsigned_t<T>;
    using UU = std::make_unsigned_t<U>;
    if constexpr (std::is_signed_v<T> == std::is_signed_v<U>)
        return t == u;
    else if constexpr (std::is_signed_v<T>)
        return t < 0 ? false : UT(t) == u;
    else
        return u < 0 ? false : t == UU(u);
}

template <class T, class U>
constexpr bool cmp_not_equal(T t, U u) noexcept
{
    return !cmp_equal(t, u);
}

template <class T, class U>
constexpr bool cmp_less(T t, U u) noexcept
{
    // Both T and U are standard integer types or extended integer types
    // https://eel.is/c++draft/utility.intcmp#4
    detail::checkTypeCompatibility<T, U>();
    // See https://eel.is/c++draft/utility.intcmp#5
    using UT = std::make_unsigned_t<T>;
    using UU = std::make_unsigned_t<U>;
    if constexpr (std::is_signed_v<T> == std::is_signed_v<U>)
        return t < u;
    else if constexpr (std::is_signed_v<T>)
        return t < 0 ? true : UT(t) < u;
    else
        return u < 0 ? false : t < UU(u);
}

template <class T, class U>
constexpr bool cmp_greater(T t, U u) noexcept
{
    return cmp_less(u, t);
}

template <class T, class U>
constexpr bool cmp_less_equal(T t, U u) noexcept
{
    return !cmp_greater(t, u);
}

template <class T, class U>
constexpr bool cmp_greater_equal(T t, U u) noexcept
{
    return !cmp_less(t, u);
}

template <class R, class T>
constexpr bool in_range(T t) noexcept
{
    return cmp_less_equal(t, (std::numeric_limits<R>::max)())
            && cmp_greater_equal(t, (std::numeric_limits<R>::min)());
}

#endif // __cpp_lib_integer_comparison_functions
} // namespace q20

// like C++20 std::exchange (ie. constexpr, not yet noexcept)
namespace q20 {
#ifdef __cpp_lib_constexpr_algorithms
using std::exchange;
#else
template <typename T, typename U = T>
constexpr T exchange(T& obj, U&& newValue)
{
    T old = std::move(obj);
    obj = std::forward<U>(newValue);
    return old;
}
#endif
}

QT_END_NAMESPACE

#endif /* Q20UTILITY_H */
