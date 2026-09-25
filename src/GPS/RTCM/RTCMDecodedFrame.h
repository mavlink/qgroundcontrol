#pragma once

#include <QtCore/QByteArray>

struct RTCMDecodedFrame
{
    QByteArray data;
    int messageId = 0;
    qint64 receivedAtMs = 0;
    bool valid = false;
    bool filtered = false;
};
