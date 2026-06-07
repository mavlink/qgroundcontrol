// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QTPREPROCESSORSUPPORT_H
#define QTPREPROCESSORSUPPORT_H

#if 0
#pragma qt_class(QtPreprocessorSupport)
#pragma qt_sync_stop_processing
#endif

/* These two macros makes it possible to turn the builtin line expander into a
 * string literal. */
#define QT_STRINGIFY2(x) #x
#define QT_STRINGIFY(x) QT_STRINGIFY2(x)

/*
   Avoid "unused parameter" warnings
*/
#define Q_UNUSED(x) (void)x;

#if !defined(Q_UNIMPLEMENTED)
#  define Q_UNIMPLEMENTED() qWarning("Unimplemented code.")
#endif

#endif // QTPREPROCESSORSUPPORT_H
