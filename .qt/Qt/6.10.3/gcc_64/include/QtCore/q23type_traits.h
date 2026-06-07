// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#ifndef Q23TYPE_TRAITS_H
#define Q23TYPE_TRAITS_H

#include <QtCore/q20type_traits.h>

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
// like std::is_scoped_enum
#ifdef __cpp_lib_is_scoped_enum
using std::is_scoped_enum;
using std::is_scoped_enum_v;
#else

template <typename E, bool isEnum = std::is_enum_v<E>>
struct is_scoped_enum :  std::negation<std::is_convertible<E, std::underlying_type_t<E>>>
{};

template<typename T>
struct is_scoped_enum<T, false> : std::false_type
{};

template <typename E>
inline constexpr bool is_scoped_enum_v = is_scoped_enum<E>::value;
#endif // __cpp_lib_is_scoped_enum
}

QT_END_NAMESPACE

#endif /* Q23TYPE_TRAITS_H */
