#pragma once

#include <array>
#include <optional>

#include <QtCore/QByteArray>
#include <QtCore/QMetaType>
#include <QtCore/QSet>
#include <QtCore/QVector>

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
    /// Drain after addByte() returns a result, before feeding more bytes.
    std::optional<Result> nextFrame();
    void reset();

    void setWhitelist(const QVector<int>& ids) { _whitelist = QSet<int>(ids.begin(), ids.end()); }

private:
    Result _result() const;

    RTCMFramer _framer;
    QSet<int> _whitelist;
    std::array<qint64, RTCMFramer::MAX_FRAME_SIZE> _receiptTimes{};
    size_t _nextReceiptIndex = 0;
};

Q_DECLARE_METATYPE(RTCMFrameDecoder::Result)
