#pragma once

#include <array>
#include <optional>

#include <QtCore/QByteArrayView>
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

    /// Frames @a bytes, delivering each completed frame. A delivery that returns false, because it ended the
    /// stream or retired its owner, stops framing at once; feed() then returns false.
    template <typename Deliver>
    bool feed(QByteArrayView bytes, qint64 receivedAtMs, Deliver&& deliver)
    {
        for (const char byte : bytes) {
            for (auto frame = addByte(static_cast<uint8_t>(byte), receivedAtMs); frame; frame = nextFrame()) {
                if (!deliver(*frame)) {
                    return false;
                }
            }
        }
        return true;
    }

    void setWhitelist(const QVector<int>& ids) { _whitelist = QSet<int>(ids.begin(), ids.end()); }

private:
    RTCMDecodedFrame _result() const;

    RTCMFramer _framer;
    QSet<int> _whitelist;
    std::array<qint64, RTCMFramer::MAX_FRAME_SIZE> _receiptTimes{};
    size_t _nextReceiptIndex = 0;
};
