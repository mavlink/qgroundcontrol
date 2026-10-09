#pragma once

#include <QtCore/QObject>

/// Frames received with one RTCM message ID; ID 0 counts unidentified frames.
struct RTCMMessageCount
{
    Q_GADGET
    Q_PROPERTY(int messageId MEMBER messageId FINAL)
    Q_PROPERTY(quint64 count MEMBER count FINAL)

public:
    int messageId = 0;
    quint64 count = 0;

    bool operator==(const RTCMMessageCount&) const = default;
};
