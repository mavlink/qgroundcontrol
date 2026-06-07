// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QMALLOC_H
#define QMALLOC_H

#include <QtCore/qcompilerdetection.h>
#include <QtCore/qtconfigmacros.h>
#include <QtCore/qtcoreexports.h>

#include <cstddef> // size_t

#if 0
#pragma qt_class(QtMalloc)
#pragma qt_sync_stop_processing
#endif

QT_BEGIN_NAMESPACE

Q_CORE_EXPORT void *qMallocAligned(size_t size, size_t alignment) Q_ALLOC_SIZE(1);
Q_CORE_EXPORT void *qReallocAligned(void *ptr, size_t size, size_t oldsize, size_t alignment) Q_ALLOC_SIZE(2);
Q_CORE_EXPORT void qFreeAligned(void *ptr);

QT_END_NAMESPACE

#endif // QMALLOC_H
