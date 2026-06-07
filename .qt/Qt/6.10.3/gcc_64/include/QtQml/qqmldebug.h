// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:critical reason:enables-debug-framework

#ifndef QQMLDEBUG_H
#define QQMLDEBUG_H

#include <QtQml/qtqmlglobal.h>
#include <QtCore/qstring.h>
#include <QtCore/qvariant.h>
#include <QtCore/qhash.h> // QVariantHash

QT_BEGIN_NAMESPACE

#if QT_CONFIG(qml_debug)

struct Q_QML_EXPORT QQmlDebuggingEnabler
{
    enum StartMode {
        DoNotWaitForClient,
        WaitForClient
    };

    static void enableDebugging(bool printWarning);

#if QT_DEPRECATED_SINCE(6, 4)
    QT_DEPRECATED_VERSION_X_6_4("Use QQmlTriviallyDestructibleDebuggingEnabler instead "
                                "or just call QQmlDebuggingEnabler::enableDebugging().")
    QQmlDebuggingEnabler(bool printWarning = true);
#endif

    static QStringList debuggerServices();
    static QStringList inspectorServices();
    static QStringList profilerServices();
    static QStringList nativeDebuggerServices();

    static void setServices(const QStringList &services);

    static bool startTcpDebugServer(int port, StartMode mode = DoNotWaitForClient,
                                    const QString &hostName = QString());
    static bool connectToLocalDebugger(const QString &socketFileName,
                                       StartMode mode = DoNotWaitForClient);
    static bool startDebugConnector(const QString &pluginName,
                                    const QVariantHash &configuration = QVariantHash());
};

// Unnamed namespace to signal the compiler that we
// indeed want each TU to have its own QQmlDebuggingEnabler.
namespace {
struct QQmlTriviallyDestructibleDebuggingEnabler {
    QQmlTriviallyDestructibleDebuggingEnabler(bool printWarning = true)
    {
        static_assert(std::is_trivially_destructible_v<QQmlTriviallyDestructibleDebuggingEnabler>);
        QQmlDebuggingEnabler::enableDebugging(printWarning);
    }
};
// Execute code in constructor before first QQmlEngine is instantiated
#if defined(QT_QML_DEBUG_NO_WARNING)
static QQmlTriviallyDestructibleDebuggingEnabler qQmlEnableDebuggingHelper(false);
#elif defined(QT_QML_DEBUG)
static QQmlTriviallyDestructibleDebuggingEnabler qQmlEnableDebuggingHelper(true);
#endif
} // unnamed namespace

#endif

QT_END_NAMESPACE

#endif // QQMLDEBUG_H
