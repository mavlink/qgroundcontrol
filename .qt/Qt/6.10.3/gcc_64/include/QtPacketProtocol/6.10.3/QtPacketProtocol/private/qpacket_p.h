// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QPACKET_H
#define QPACKET_H

#include <QtCore/qdatastream.h>
#include <QtCore/qbuffer.h>

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

class QPacket : public QDataStream
{
public:
    QPacket(int version);
    explicit QPacket(int version, const QByteArray &ba);
    const QByteArray &data() const;
    QByteArray squeezedData() const;
    void clear();

private:
    void init(QIODevice::OpenMode mode);
    QBuffer buf;
};

QT_END_NAMESPACE

#endif // QPACKET_H
