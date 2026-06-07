// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QLOGGING_P_H
#define QLOGGING_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of a number of Qt sources files.  This header file may change from
// version to version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/private/qglobal_p.h>
#include "qlogging.h"
#include <QtCore/qloggingcategory.h>

#if !defined(QT_BOOTSTRAPPED) && QT_CONFIG(regularexpression)
#  if __has_include(<cxxabi.h>) && QT_CONFIG(backtrace)
#    include <optional>
#    include "qvarlengtharray.h"
#    define QLOGGING_USE_EXECINFO_BACKTRACE
#    define QLOGGING_HAVE_BACKTRACE
#  elif QT_CONFIG(cxx23_stacktrace)
#    include <optional>
#    include <stacktrace>
#    define QLOGGING_USE_STD_BACKTRACE
#    define QLOGGING_HAVE_BACKTRACE
#  endif
#endif // QT_BOOTSTRAPPED

QT_BEGIN_NAMESPACE

namespace QtPrivate {

Q_CORE_EXPORT bool shouldLogToStderr();

}

class QInternalMessageLogContext : public QMessageLogContext
{
public:
    static constexpr int DefaultBacktraceDepth = 32;

#if defined(QLOGGING_USE_EXECINFO_BACKTRACE)
    using BacktraceStorage = QVarLengthArray<void *, DefaultBacktraceDepth>;
#elif defined(QLOGGING_USE_STD_BACKTRACE)
    using BacktraceStorage = std::stacktrace;
#else
    using BacktraceStorage = bool; // dummy
#endif

    std::optional<BacktraceStorage> backtrace;

    Q_ALWAYS_INLINE QInternalMessageLogContext(const QMessageLogContext &logContext)
    {
        int backtraceFrames = initFrom(logContext);
        if (backtraceFrames)
            populateBacktrace(backtraceFrames);
    }
    QInternalMessageLogContext(const QMessageLogContext &logContext,
                               const QLoggingCategory &categoryOverride)
        : QInternalMessageLogContext(logContext)
    {
        category = categoryOverride.categoryName();
    }

    int initFrom(const QMessageLogContext &logContext);
    void populateBacktrace(int frameCount);
};

QT_END_NAMESPACE

#endif // QLOGGING_P_H
