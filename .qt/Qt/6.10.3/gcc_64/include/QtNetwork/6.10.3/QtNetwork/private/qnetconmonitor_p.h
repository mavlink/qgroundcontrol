// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QNETCONMONITOR_P_H
#define QNETCONMONITOR_P_H

#include <private/qtnetworkglobal_p.h>

#include <QtCore/qloggingcategory.h>
#include <QtNetwork/qhostaddress.h>
#include <QtCore/qobject.h>

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

QT_BEGIN_NAMESPACE

class QNetworkConnectionMonitorPrivate;
class Q_NETWORK_EXPORT QNetworkConnectionMonitor : public QObject
{
    Q_OBJECT

public:
#ifdef Q_OS_APPLE
    enum class InterfaceType {
        Unknown,
        Ethernet,
        Cellular,
        WiFi,
    };
    Q_ENUM(InterfaceType)
#endif

    QNetworkConnectionMonitor();
    QNetworkConnectionMonitor(const QHostAddress &local, const QHostAddress &remote = {});
    ~QNetworkConnectionMonitor();

    bool setTargets(const QHostAddress &local, const QHostAddress &remote);
    bool isReachable();
#ifdef Q_OS_APPLE
    InterfaceType getInterfaceType() const;
#endif

    // Important: on Darwin you should not call isReachable/isWwan() after
    // startMonitoring(), you have to listen to reachabilityChanged()
    // signal instead.
    bool startMonitoring();
    bool isMonitoring() const;
    void stopMonitoring();

    static bool isEnabled();

Q_SIGNALS:
    // Important: connect to this using QueuedConnection. On Darwin
    // callback is coming on a special dispatch queue.
    void reachabilityChanged(bool isOnline);
#ifdef Q_OS_APPLE
    void interfaceTypeChanged(InterfaceType type);
#endif

private:
    Q_DECLARE_PRIVATE(QNetworkConnectionMonitor)
    Q_DISABLE_COPY_MOVE(QNetworkConnectionMonitor)
};

Q_DECLARE_LOGGING_CATEGORY(lcNetMon)

QT_END_NAMESPACE

#endif // QNETCONMONITOR_P_H
