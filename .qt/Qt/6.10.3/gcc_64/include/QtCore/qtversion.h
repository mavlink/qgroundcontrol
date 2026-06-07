// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QTVERSION_H
#define QTVERSION_H

#if 0
#pragma qt_class(QtVersion)
#pragma qt_sync_stop_processing
#endif

#ifndef __ASSEMBLER__

#include <QtCore/qcompilerdetection.h>
#include <QtCore/qtconfigmacros.h>
#include <QtCore/qtcoreexports.h>

QT_BEGIN_NAMESPACE

/*
 * If we're compiling C++ code:
 *  - and this is a non-namespace build, declare qVersion as extern "C"
 *  - and this is a namespace build, declare it as a regular function
 *    (we're already inside QT_BEGIN_NAMESPACE / QT_END_NAMESPACE)
 * If we're compiling C code, simply declare the function. If Qt was compiled
 * in a namespace, qVersion isn't callable anyway.
 */
#if !defined(QT_NAMESPACE) && defined(__cplusplus) && !defined(Q_QDOC)
extern "C"
#endif
/* defined in qlibraryinfo.cpp */
Q_CORE_EXPORT Q_DECL_CONST_FUNCTION const char *qVersion(void) Q_DECL_NOEXCEPT;

QT_END_NAMESPACE

#endif // __ASSEMBLER__

#endif // QTVERSION_H
