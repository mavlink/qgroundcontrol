// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QV4DEBUGCLIENT_P_P_H
#define QV4DEBUGCLIENT_P_P_H

#include "qv4debugclient_p.h"
#include "qqmldebugclient_p_p.h"

#include <QtCore/qjsonobject.h>

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

class QV4DebugClientPrivate : public QQmlDebugClientPrivate
{
    Q_DECLARE_PUBLIC(QV4DebugClient)

public:
    QV4DebugClientPrivate(QQmlDebugConnection *connection);

    void sendMessage(const QByteArray &command, const QJsonObject &args = QJsonObject());
    void flushSendBuffer();
    QByteArray packMessage(const QByteArray &type, const QJsonObject &object);
    void onStateChanged(QQmlDebugClient::State state);

    int seq = 0;
    QList<QByteArray> sendBuffer;
    QByteArray response;
};

QT_END_NAMESPACE

#endif // QV4DEBUGCLIENT_P_P_H
