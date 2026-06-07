// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#ifndef QTESTCRASHHANDLER_H
#define QTESTCRASHHANDLER_H

#include <QtCore/qnamespace.h>
#include <QtTest/qttestglobal.h>

#include <QtCore/private/qtools_p.h>

#ifdef Q_OS_UNIX
#include <sys/mman.h>   // for MAP_FAILED
#endif

QT_BEGIN_NAMESPACE
namespace QTest {
namespace CrashHandler {
    enum DebuggerProgram {
        None,
        Gdb,
        Lldb,
#ifdef Q_OS_WIN
        Cdb,
#endif
    };

    bool alreadyDebugging();
    void blockUnixSignals();

#if !defined(Q_OS_WASM) || QT_CONFIG(thread)
    void printTestRunTime();
    void generateStackTrace(quintptr ip = 0);
#endif

    void maybeDisableCoreDump();
    Q_TESTLIB_EXPORT void prepareStackTrace();

#if defined(Q_OS_WIN)
    class Q_TESTLIB_EXPORT WindowsFaultHandler
    {
    public:
        WindowsFaultHandler();
    };
    using FatalSignalHandler = WindowsFaultHandler;
#elif defined(Q_OS_UNIX) && !defined(Q_OS_WASM)
    class Q_TESTLIB_EXPORT FatalSignalHandler
    {
    public:
        FatalSignalHandler();
        ~FatalSignalHandler();

    private:
        Q_DISABLE_COPY_MOVE(FatalSignalHandler)

        int setupAlternateStack();
        void freeAlternateStack();
        void *alternateStackBase = MAP_FAILED;
    };
#else // Q_OS_WASM or weird systems
class Q_TESTLIB_EXPORT FatalSignalHandler {};
inline void blockUnixSignals() {}
#endif // Q_OS_* choice
} // namespace CrashHandler
} // namespace QTest
QT_END_NAMESPACE

#endif // QTESTCRASHHANDLER_H
