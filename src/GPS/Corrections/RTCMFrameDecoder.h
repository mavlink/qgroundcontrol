#pragma once

#include <QtCore/QByteArray>
#include <QtCore/QSet>
#include <QtCore/QVector>

#include <optional>

#include "RTCMFramer.h"

/// Frames RTCM bytes while retaining the first fragment's monotonic receipt time.
class RTCMFrameDecoder
{
public:
    struct Result
    {
        QByteArray data;
        int messageId = 0;
        qint64 receivedAtMs = 0;
        bool valid = false;
        bool filtered = false;
    };

    RTCMFrameDecoder();
    ~RTCMFrameDecoder();
    std::optional<Result> addByte(uint8_t byte, qint64 receivedAtMs);
    void reset();

    void setWhitelist(const QVector<int>& ids) { _whitelist = QSet<int>(ids.begin(), ids.end()); }

private:
    RTCMFramer _framer;
    QSet<int> _whitelist;
    qint64 _receivedAtMs = 0;
};
