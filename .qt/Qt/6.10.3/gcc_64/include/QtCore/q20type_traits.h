// Copyright (C) 2021 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Marc Mutz <marc.mutz@kdab.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#ifndef Q20TYPE_TRAITS_H
#define Q20TYPE_TRAITS_H

#include <QtCore/qcompilerdetection.h>
#include <QtCore/qsystemdetection.h>
#include <QtCore/qtconfigmacros.h>

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

#include <type_traits>

QT_BEGIN_NAMESPACE

namespace q20 {
// like std::is_(un)bounded_array
#ifdef __cpp_lib_bounded_array_traits
using std::is_bounded_array;
using std::is_bounded_array_v;
using std::is_unbounded_array;
using std::is_unbounded_array_v;
#else
template <typename T> struct is_bounded_array : std::false_type {};
template <typename T, std::size_t N> struct is_bounded_array<T[N]> : std::true_type {};
template <typename T> struct is_unbounded_array : std::false_type {};
template <typename T> struct is_unbounded_array<T[]> : std::true_type {};
template <typename T> constexpr inline bool is_bounded_array_v = q20::is_bounded_array<T>::value;
template <typename T> constexpr inline bool is_unbounded_array_v = q20::is_unbounded_array<T>::value;
#endif
}

namespace q20 {
// like std::is_constant_evaluated
#ifdef __cpp_lib_is_constant_evaluated
using std::is_constant_evaluated;
#define QT_SUPPORTS_IS_CONSTANT_EVALUATED
#else
constexpr bool is_constant_evaluated() noexcept
{
#ifdef Q_OS_INTEGRITY
    // Integrity complains "calling __has_builtin() from a constant expression".
    // Avoid the __has_builtin check until we know what's going on.
    return false;
#elif __has_builtin(__builtin_is_constant_evaluated) || \
    (defined(Q_CC_MSVC_ONLY) /* >= 1925, but we require 1927 in qglobal.h */)
#  define QT_SUPPORTS_IS_CONSTANT_EVALUATED
    return __builtin_is_constant_evaluated();
#else
    return false;
#endif
}
#endif // __cpp_lib_is_constant_evaluated
}

namespace q20 {
// like std::remove_cvref(_t)
#ifdef __cpp_lib_remove_cvref
using std::remove_cvref;
using std::remove_cvref_t;
#else
template <typename T>
using remove_cvref = std::remove_cv<std::remove_reference_t<T>>;
template <typename T>
using remove_cvref_t = std::remove_cv_t<std::remove_reference_t<T>>;
#endif // __cpp_lib_remove_cvref
}

namespace q20 {
// like std::type_identity(_t)
#ifdef __cpp_lib_type_identity
using std::type_identity;
using std::type_identity_t;
#else
template <typename T>
struct type_identity { using type = T; };
template <typename T>
using type_identity_t = typename type_identity<T>::type;
#endif // __cpp_lib_type_identity
}

QT_END_NAMESPACE

#endif /* Q20TYPE_TRAITS_H */
