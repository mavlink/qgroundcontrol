// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QVERSIONEDPACKET_P_H
#define QVERSIONEDPACKET_P_H

#include "qpacket_p.h"

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

// QPacket with a fixed data stream version, centrally set by some Connector
template<class Connector>
class QVersionedPacket : public QPacket
{
public:
    QVersionedPacket(const QByteArray &ba) : QPacket(Connector::dataStreamVersion(), ba) {}
    QVersionedPacket() : QPacket(Connector::dataStreamVersion()) {}
};

QT_END_NAMESPACE

#endif // QVERSIONEDPACKET_P_H
