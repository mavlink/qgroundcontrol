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
    if (!_parser.hasPartialFrame() && byte == RTCMParser::kPreamble) {
        _receivedAtMs = receivedAtMs;
    }
    if (!_parser.addByte(byte)) {
        if (!_parser.hasPartialFrame()) {
            _receivedAtMs = 0;
        }
        return std::nullopt;
    }
    Result result{_parser.currentFrame(), _parser.messageId(), _receivedAtMs};
    result.valid = RTCMParser::isValidFrame(result.data);
    result.filtered = result.valid && !_parser.isWhitelisted(result.messageId);
    reset();
    return result;
}

void RTCMFrameDecoder::reset()
{
    _parser.reset();
    _receivedAtMs = 0;
}
