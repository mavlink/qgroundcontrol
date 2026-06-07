// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:insignificant

#ifndef QMLDOM_GLOBAL_H
#define QMLDOM_GLOBAL_H

#include <QtCore/qglobal.h>

#if defined(QMLDOM_DYNAMIC)
#if defined(QMLDOM_LIBRARY)
#  define QMLDOM_EXPORT Q_DECL_EXPORT
#else
#  define QMLDOM_EXPORT Q_DECL_IMPORT
#endif
#else
#  define QMLDOM_EXPORT
#endif

QT_BEGIN_NAMESPACE
// avoid annoying warning about missing QT_BEGIN_NAMESPACE...
QT_END_NAMESPACE

#endif // QMLDOM_GLOBAL_H
