#pragma once

#include <QtCore/QByteArray>
#include <QtCore/QMetaType>

struct RTCMDecodedFrame
{
    QByteArray data;
    int messageId = 0;
    qint64 receivedAtMs = 0;
    bool valid = false;
    bool filtered = false;
};

Q_DECLARE_METATYPE(RTCMDecodedFrame)
