#include "RTCMFrameDecoder.h"

#include <QtCore/QByteArrayView>

#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(RTCMFrameDecoderLog, "GPS.RTCM.RTCMFrameDecoder")

RTCMFrameDecoder::RTCMFrameDecoder()
{
    qCDebug(RTCMFrameDecoderLog) << this;
}

RTCMFrameDecoder::~RTCMFrameDecoder()
{
    qCDebug(RTCMFrameDecoderLog) << this;
}

std::optional<RTCMDecodedFrame> RTCMFrameDecoder::addByte(uint8_t byte, qint64 receivedAtMs)
{
    _receiptTimes[_nextReceiptIndex] = receivedAtMs;
    _nextReceiptIndex = (_nextReceiptIndex + 1) % _receiptTimes.size();
    if (!_framer.addByte(byte)) {
        return std::nullopt;
    }
    return _result();
}

std::optional<RTCMDecodedFrame> RTCMFrameDecoder::nextFrame()
{
    if (!_framer.nextFrame()) {
        return std::nullopt;
    }
    return _result();
}

RTCMDecodedFrame RTCMFrameDecoder::_result() const
{
    const auto frame = _framer.frame();
    const size_t firstReceiptIndex =
        (_nextReceiptIndex + _receiptTimes.size() - _framer.bufferedSize()) % _receiptTimes.size();
    RTCMDecodedFrame result{QByteArrayView(frame).toByteArray(), _framer.messageId(), _receiptTimes[firstReceiptIndex]};
    result.valid = _framer.valid();
    result.filtered = result.valid && !_whitelist.isEmpty() && !_whitelist.contains(result.messageId);
    return result;
}

void RTCMFrameDecoder::reset()
{
    _framer.reset();
    _nextReceiptIndex = 0;
}
