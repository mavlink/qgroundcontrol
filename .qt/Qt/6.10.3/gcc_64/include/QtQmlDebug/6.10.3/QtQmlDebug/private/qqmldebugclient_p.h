// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQMLDEBUGCLIENT_P_H
#define QQMLDEBUGCLIENT_P_H

#include <QtCore/qobject.h>
#include <QtCore/private/qglobal_p.h>

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

class QQmlDebugConnection;
class QQmlDebugClientPrivate;
class QQmlDebugClient : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(QQmlDebugClient)
    Q_DECLARE_PRIVATE(QQmlDebugClient)

public:
    enum State { NotConnected, Unavailable, Enabled };

    QQmlDebugClient(const QString &name, QQmlDebugConnection *parent);
    ~QQmlDebugClient();

    QString name() const;
    float serviceVersion() const;
    State state() const;
    void sendMessage(const QByteArray &message);

    QQmlDebugConnection *connection() const;

Q_SIGNALS:
    void stateChanged(State state);

protected:
    QQmlDebugClient(QQmlDebugClientPrivate &dd);

private:
    friend class QQmlDebugConnection;
    virtual void messageReceived(const QByteArray &message);
};

QT_END_NAMESPACE

#endif // QQMLDEBUGCLIENT_P_H
