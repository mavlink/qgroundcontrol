// Copyright (C) 2021 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Marc Mutz <marc.mutz@kdab.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#ifndef Q20FUNCTIONAL_H
#define Q20FUNCTIONAL_H

#include <QtCore/qglobal.h>

#include <functional>

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

#include <functional>

QT_BEGIN_NAMESPACE

namespace q20 {
// like std::identity
#ifdef __cpp_lib_ranges
using std::identity;
#else
struct identity
{
    struct is_transparent {};
    template <typename T>
    constexpr T &&operator()(T&& t) const noexcept { return std::forward<T>(t); }
};
#endif // __cpp_lib_ranges
} // namespace q20

QT_END_NAMESPACE

#endif /* Q20FUNCTIONAL_H */
