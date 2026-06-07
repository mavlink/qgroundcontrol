// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#ifndef Q23UTILITY_H
#define Q23UTILITY_H

#include <QtCore/qtconfigmacros.h>

#include <QtCore/q20utility.h>

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

namespace q23 {
// like std::forward_like
#ifdef __cpp_lib_forward_like
using std::forward_like;
#else

namespace _detail {

// [forward]/6.1 COPY_CONST
template <typename A, typename B>
using copy_const_t = std::conditional_t<
        std::is_const_v<A>, const B,
        /* else */          B
    >;

// [forward]/6.2 OVERRIDE_REF
template <typename A, typename B>
using override_ref_t = std::conditional_t<
        std::is_rvalue_reference_v<A>, std::remove_reference_t<B>&&,
        /* else */                     B&
    >;

// [forward]/6.3 "V"
template <typename T, typename U>
using forward_like_ret_t = override_ref_t<
        T&&,
        copy_const_t<
            std::remove_reference_t<T>,
            std::remove_reference_t<U>
        >
    >;

} // namespace detail

// http://eel.is/c++draft/forward#lib:forward_like
template <class T, class U>
[[nodiscard]] constexpr auto forward_like(U &&x) noexcept
    -> _detail::forward_like_ret_t<T, U>
{
    using V = _detail::forward_like_ret_t<T, U>;
    return static_cast<V>(x);
}
#endif // __cpp_lib_forward_like
} // namespace q23

QT_END_NAMESPACE

#endif /* Q23UTILITY_H */
