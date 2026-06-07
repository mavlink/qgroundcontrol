// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QEXCEPTIONHANDLING_H
#define QEXCEPTIONHANDLING_H

#include <QtCore/qtconfigmacros.h>
#include <QtCore/qcompilerdetection.h>
#include <QtCore/qtcoreexports.h>

#if 0
#pragma qt_class(QtExceptionHandling)
#pragma qt_sync_stop_processing
#endif

QT_BEGIN_NAMESPACE

/* These wrap try/catch so we can switch off exceptions later.

   Beware - do not use more than one QT_CATCH per QT_TRY, and do not use
   the exception instance in the catch block.
   If you can't live with those constraints, don't use these macros.
   Use the QT_NO_EXCEPTIONS macro to protect your code instead.
*/
#ifdef QT_NO_EXCEPTIONS
#  define QT_TRY if (true)
#  define QT_CATCH(A) else
#  define QT_THROW(A) qt_noop()
#  define QT_RETHROW qt_noop()
#else
#  define QT_TRY try
#  define QT_CATCH(A) catch (A)
#  define QT_THROW(A) throw A
#  define QT_RETHROW throw
#endif

#if QT_CORE_REMOVED_SINCE(6, 9)
Q_NORETURN Q_DECL_COLD_FUNCTION Q_CORE_EXPORT void qTerminate() noexcept;
#endif

QT_END_NAMESPACE

#endif // QEXCEPTIONHANDLING_H
