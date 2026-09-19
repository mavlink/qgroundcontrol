#pragma once

#include <array>
#include <optional>

#include <QtCore/QSet>
#include <QtCore/QVector>

#include "RTCMDecodedFrame.h"
#include "RTCMFramer.h"

/// Frames RTCM bytes while retaining the first fragment's monotonic receipt time.
class RTCMFrameDecoder
{
public:
    RTCMFrameDecoder();
    ~RTCMFrameDecoder();
    std::optional<RTCMDecodedFrame> addByte(uint8_t byte, qint64 receivedAtMs);
    /// Drain after addByte() returns a result, before feeding more bytes.
    std::optional<RTCMDecodedFrame> nextFrame();
    void reset();

    void setWhitelist(const QVector<int>& ids) { _whitelist = QSet<int>(ids.begin(), ids.end()); }

private:
    RTCMDecodedFrame _result() const;

    RTCMFramer _framer;
    QSet<int> _whitelist;
    std::array<qint64, RTCMFramer::MAX_FRAME_SIZE> _receiptTimes{};
    size_t _nextReceiptIndex = 0;
};
