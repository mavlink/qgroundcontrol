// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QTCORE_QSWAP_H
#define QTCORE_QSWAP_H

#include <QtCore/qtconfigmacros.h>

#include <type_traits>
#include <utility>

#if 0
#pragma qt_class(QtSwap)
#pragma qt_sync_stop_processing
#endif

QT_BEGIN_NAMESPACE

template <typename T>
constexpr void qSwap(T &value1, T &value2)
    noexcept(std::is_nothrow_swappable_v<T>)
{
    using std::swap;
    swap(value1, value2);
}

// pure compile-time micro-optimization for our own headers, so not documented:
template <typename T>
constexpr inline void qt_ptr_swap(T* &lhs, T* &rhs) noexcept
{
    T *tmp = lhs;
    lhs = rhs;
    rhs = tmp;
}

QT_END_NAMESPACE

#endif // QTCORE_QSWAP_H
