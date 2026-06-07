// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:insignificant

#ifndef QTQMLCOMPILERGLOBAL_H
#define QTQMLCOMPILERGLOBAL_H

#include <QtCore/qglobal.h>

QT_BEGIN_NAMESPACE

#if defined(QT_STATIC)
#  define Q_QML_COMPILER_EXPORT
#else
#  if defined(QT_BUILD_QML_LIB)
#    define Q_QML_COMPILER_EXPORT Q_DECL_EXPORT
#  else
#    define Q_QML_COMPILER_EXPORT Q_DECL_IMPORT
#  endif
#endif

QT_END_NAMESPACE
#endif // QTQMLCOMPILERGLOBAL_H
