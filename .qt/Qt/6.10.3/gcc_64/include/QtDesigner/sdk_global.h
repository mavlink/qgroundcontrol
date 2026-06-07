// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef SDK_GLOBAL_H
#define SDK_GLOBAL_H

#include <QtCore/qglobal.h>

QT_BEGIN_NAMESPACE

#define QDESIGNER_SDK_EXTERN Q_DECL_EXPORT
#define QDESIGNER_SDK_IMPORT Q_DECL_IMPORT

#ifdef QT_DESIGNER_STATIC
#  define QDESIGNER_SDK_EXPORT
#elif defined(QDESIGNER_SDK_LIBRARY)
#  define QDESIGNER_SDK_EXPORT QDESIGNER_SDK_EXTERN
#else
#  define QDESIGNER_SDK_EXPORT QDESIGNER_SDK_IMPORT
#endif

QT_END_NAMESPACE

#endif // SDK_GLOBAL_H
