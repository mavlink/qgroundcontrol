// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QPACKETPROTOCOL_P_H
#define QPACKETPROTOCOL_P_H

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

class QIODevice;

class QPacketProtocolPrivate;
class QPacketProtocol : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QPacketProtocol)
public:
    explicit QPacketProtocol(QIODevice *dev, QObject *parent = nullptr);

    void send(const QByteArray &data);
    qint64 packetsAvailable() const;
    QByteArray read();
    bool waitForReadyRead(int msecs = 3000);

Q_SIGNALS:
    void readyRead();
    void error();

private:
    void bytesWritten(qint64 bytes);
    void readyToRead();
};

QT_END_NAMESPACE

#endif // QPACKETPROTOCOL_P_H
