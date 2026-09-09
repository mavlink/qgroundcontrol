#pragma once

#include <optional>

#include "RTCMParser.h"

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

    void setWhitelist(const QVector<int>& ids) { _parser.setWhitelist(ids); }

private:
    RTCMParser _parser;
    qint64 _receivedAtMs = 0;
};
