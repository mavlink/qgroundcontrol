// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef TASKING_GLOBAL_H
#define TASKING_GLOBAL_H

#include <QtCore/qglobal.h>

QT_BEGIN_NAMESPACE

// #if defined(QT_SHARED) || !defined(QT_STATIC)
// #  if defined(TASKING_LIBRARY)
// #    define TASKING_EXPORT Q_DECL_EXPORT
// #  else
// #    define TASKING_EXPORT Q_DECL_IMPORT
// #  endif
// #else
// #  define TASKING_EXPORT
// #endif

#define TASKING_EXPORT

QT_END_NAMESPACE

#endif // TASKING_GLOBAL_H
