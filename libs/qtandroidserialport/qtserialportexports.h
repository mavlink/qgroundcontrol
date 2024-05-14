// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QTSERIALPORTEXPORTS_H
#define QTSERIALPORTEXPORTS_H

#include <QtCore/qcompilerdetection.h>
#include <QtCore/qtconfigmacros.h> // Q_SERIALPORT_EXPORT
#include <QtCore/qtdeprecationmarkers.h> // QT_IF_DEPRECATED_SINCE

#if defined(QT_SHARED) || !defined(QT_STATIC)
#  if defined(QT_BUILD_SERIALPORT_LIB)
#    define Q_SERIALPORT_EXPORT Q_DECL_EXPORT
#  else
#    define Q_SERIALPORT_EXPORT Q_DECL_IMPORT
#  endif
#else
#  define Q_SERIALPORT_EXPORT
#endif

#if !defined(QT_BUILD_SERIALPORT_LIB) && !defined(QT_STATIC)
/* outside library -> inline decl + defi */
/* static builds treat everything as part of the library, so they never inline */
#  define QT_SERIALPORT_INLINE_SINCE(major, minor) inline
#  define QT_SERIALPORT_INLINE_IMPL_SINCE(major, minor) 1
#elif defined(QT_SERIALPORT_BUILD_REMOVED_API)
/* inside library, inside removed_api.cpp:
 * keep deprecated API -> non-inline decl;
 * remove deprecated API -> inline decl;
 * definition is always available */
#  define QT_SERIALPORT_INLINE_SINCE(major, minor) \
    QT_IF_DEPRECATED_SINCE(major, minor, inline, /* not inline */)
#  define QT_SERIALPORT_INLINE_IMPL_SINCE(major, minor) 1
#else
/* inside library, outside removed_api.cpp:
 * keep deprecated API -> non-inline decl, no defi;
 * remove deprecated API -> inline decl, defi */
#  define QT_SERIALPORT_INLINE_SINCE(major, minor) \
    QT_IF_DEPRECATED_SINCE(major, minor, inline, /* not inline */)
#  define QT_SERIALPORT_INLINE_IMPL_SINCE(major, minor) \
    QT_IF_DEPRECATED_SINCE(major, minor, 1, 0)
#endif

#ifdef QT_SERIALPORT_BUILD_REMOVED_API
#  define QT_SERIALPORT_REMOVED_SINCE(major, minor) QT_DEPRECATED_SINCE(major, minor)
#else
#  define QT_SERIALPORT_REMOVED_SINCE(major, minor) 0
#endif

#endif // QTSERIALPORTEXPORTS_H
