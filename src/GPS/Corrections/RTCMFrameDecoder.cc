#include "RTCMFrameDecoder.h"

#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(RTCMFrameDecoderLog, "GPS.Corrections.RTCMFrameDecoder")

RTCMFrameDecoder::RTCMFrameDecoder()
{
    qCDebug(RTCMFrameDecoderLog) << this;
}

RTCMFrameDecoder::~RTCMFrameDecoder()
{
    qCDebug(RTCMFrameDecoderLog) << this;
}

std::optional<RTCMFrameDecoder::Result> RTCMFrameDecoder::addByte(uint8_t byte, qint64 receivedAtMs)
{
    if (!_framer.hasPartialFrame() && byte == RTCMFramer::PREAMBLE) {
        _receivedAtMs = receivedAtMs;
    }
    if (!_framer.addByte(byte)) {
        if (!_framer.hasPartialFrame()) {
            _receivedAtMs = 0;
        }
        return std::nullopt;
    }
    Result result{QByteArray(reinterpret_cast<const char*>(_framer.message()), _framer.messageLength()),
                  _framer.messageId(), _receivedAtMs};
    result.valid = _framer.valid();
    result.filtered = result.valid && !_whitelist.isEmpty() && !_whitelist.contains(result.messageId);
    reset();
    return result;
}

void RTCMFrameDecoder::reset()
{
    _framer.reset();
    _receivedAtMs = 0;
}
