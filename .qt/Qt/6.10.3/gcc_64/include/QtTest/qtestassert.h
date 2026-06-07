// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QTESTASSERT_H
#define QTESTASSERT_H

#include <QtCore/qglobal.h>

QT_BEGIN_NAMESPACE

#define QTEST_ASSERT(cond) ((cond) ? static_cast<void>(0) : qt_assert(#cond, __FILE__, __LINE__))

#define QTEST_ASSERT_X(cond, where, what) ((cond) ? static_cast<void>(0) : qt_assert_x(where, what, __FILE__, __LINE__))

QT_END_NAMESPACE

#endif
