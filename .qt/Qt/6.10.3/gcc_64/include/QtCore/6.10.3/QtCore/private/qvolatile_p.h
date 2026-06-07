// Copyright (C) 2021 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Marc Mutz <marc.mutz@kdab.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QVOLATILE_P_H
#define QVOLATILE_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of a number of Qt sources files.  This header file may change from
// version to version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/private/qglobal_p.h>

QT_BEGIN_NAMESPACE

namespace QtPrivate {
template <typename T>
using if_volatile = std::enable_if_t<std::is_volatile_v<T>, bool>;

//
// C++20-deprecated volatile compound operations, rewritten as separated operations
//

// these functions return `auto`, not `T`, to strip cv-qualifiers without having
// to mention the `volatile` keyword

template <typename T, QtPrivate::if_volatile<T> = true>
auto volatilePreIncrement(T &x) {
    auto y = x;
    ++y;
    x = y;
    return y;
}

template <typename T, QtPrivate::if_volatile<T> = true>
auto volatilePreDecrement(T &x)
{
    auto y = x;
    --y;
    x = y;
    return y;
}
} // namespace QtPrivate

QT_END_NAMESPACE

#endif // QVOLATILE_H
