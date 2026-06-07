// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQMLDEBUGMESSAGECLIENT_P_H
#define QQMLDEBUGMESSAGECLIENT_P_H

#include "qqmldebugclient_p.h"

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

struct QQmlDebugContextInfo
{
    int line;
    QString file;
    QString function;
    QString category;
    qint64 timestamp;
};

class QQmlDebugMessageClient : public QQmlDebugClient
{
    Q_OBJECT

public:
    explicit QQmlDebugMessageClient(QQmlDebugConnection *client);

    virtual void messageReceived(const QByteArray &) override;

Q_SIGNALS:
    void message(QtMsgType, const QString &, const QQmlDebugContextInfo &);
};

QT_END_NAMESPACE

#endif // QQMLDEBUGMESSAGECLIENT_P_H
