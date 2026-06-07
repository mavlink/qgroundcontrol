// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:critical reason:network-protocol

#ifndef QQMLDEBUGSERVERCONNECTION_P_H
#define QQMLDEBUGSERVERCONNECTION_P_H

#include <private/qtqmlglobal_p.h>
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

class QQmlDebugServer;
class Q_QML_EXPORT QQmlDebugServerConnection : public QObject
{
    Q_OBJECT
public:
    QQmlDebugServerConnection(QObject *parent = nullptr) : QObject(parent) {}
    ~QQmlDebugServerConnection() override;

    virtual void setServer(QQmlDebugServer *server) = 0;
    virtual bool setPortRange(int portFrom, int portTo, bool block, const QString &hostaddress) = 0;
    virtual bool setFileName(const QString &fileName, bool block) = 0;
    virtual bool isConnected() const = 0;
    virtual void disconnect() = 0;
    virtual void waitForConnection() = 0;
    virtual void flush() = 0;
};

class Q_QML_EXPORT QQmlDebugServerConnectionFactory : public QObject
{
    Q_OBJECT
public:
    ~QQmlDebugServerConnectionFactory() override;
    virtual QQmlDebugServerConnection *create(const QString &key) = 0;
};

#define QQmlDebugServerConnectionFactory_iid "org.qt-project.Qt.QQmlDebugServerConnectionFactory"
Q_DECLARE_INTERFACE(QQmlDebugServerConnectionFactory, QQmlDebugServerConnectionFactory_iid)

QT_END_NAMESPACE

#endif // QQMLDEBUGSERVERCONNECTION_H
