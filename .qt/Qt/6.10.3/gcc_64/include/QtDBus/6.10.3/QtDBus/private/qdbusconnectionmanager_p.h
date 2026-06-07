// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (C) 2016 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

//
//  W A R N I N G
//  -------------
//
// This file is not part of the public API.  This header file may
// change from version to version without notice, or even be
// removed.
//
// We mean it.
//
//

#ifndef QDBUSCONNECTIONMANAGER_P_H
#define QDBUSCONNECTIONMANAGER_P_H

#include <QtDBus/private/qtdbusglobal_p.h>
#include "qdbusconnection_p.h"
#include "private/qthread_p.h"

#ifndef QT_NO_DBUS

QT_BEGIN_NAMESPACE

class QDBusServer;

class QDBusConnectionManager : public QDaemonThread
{
    Q_OBJECT
public:
    QDBusConnectionManager();
    ~QDBusConnectionManager();
    static QDBusConnectionManager* instance();

    QDBusConnectionPrivate *busConnection(QDBusConnection::BusType type);
    QDBusConnectionPrivate *existingConnection(const QString &name) const;

    void removeConnections(const QStringList &names);
    void disconnectFrom(const QString &name, QDBusConnectionPrivate::ConnectionMode mode);
    void addConnection(const QString &name, QDBusConnectionPrivate *c);

    QDBusConnectionPrivate *connectToBus(QDBusConnection::BusType type, const QString &name, bool suspendedDelivery);
    QDBusConnectionPrivate *connectToBus(const QString &address, const QString &name);
    QDBusConnectionPrivate *connectToPeer(const QString &address, const QString &name);

    void createServer(const QString &address, QDBusServer *server);

protected:
    void run() override;

private:
    QDBusConnectionPrivate *doConnectToStandardBus(QDBusConnection::BusType type,
                                                   const QString &name, bool suspendedDelivery);
    QDBusConnectionPrivate *doConnectToBus(const QString &address, const QString &name);
    QDBusConnectionPrivate *doConnectToPeer(const QString &address, const QString &name);

    mutable QMutex mutex;
    QHash<QString, QDBusConnectionPrivate *> connectionHash;
    QDBusConnectionPrivate *connection(const QString &name) const;
    void removeConnection(const QString &name);
    void setConnection(const QString &name, QDBusConnectionPrivate *c);

    QMutex defaultBusMutex;
    QDBusConnectionPrivate *defaultBuses[2];
};

QT_END_NAMESPACE

#endif // QT_NO_DBUS
#endif
