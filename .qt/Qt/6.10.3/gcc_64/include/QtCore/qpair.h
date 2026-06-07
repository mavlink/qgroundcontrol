// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QPAIR_H
#define QPAIR_H

#include <QtCore/qcontainerfwd.h>
#include <QtCore/qglobal.h>

QT_BEGIN_NAMESPACE

#if 0
#pragma qt_class(QPair)
#endif

#ifndef QT_NO_QPAIR

template <typename T1, typename T2>
constexpr decltype(auto) qMakePair(T1 &&value1, T2 &&value2)
    noexcept(noexcept(std::make_pair(std::forward<T1>(value1), std::forward<T2>(value2))))
{
    return std::make_pair(std::forward<T1>(value1), std::forward<T2>(value2));
}

#endif // QT_NO_QPAIR

QT_END_NAMESPACE

#endif // QPAIR_H
