#include "RTCMParser.h"

#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(RTCMParserLog, "GPS.Corrections.RTCMParser")

RTCMParser::RTCMParser()
{
    qCDebug(RTCMParserLog) << this;
    reset();
}

RTCMParser::~RTCMParser()
{
    qCDebug(RTCMParserLog) << this;
}

void RTCMParser::reset()
{
    _framer.reset();
}

bool RTCMParser::addByte(uint8_t byte)
{
    return _framer.addByte(byte);
}

uint16_t RTCMParser::messageId() const
{
    return _framer.messageId();
}

uint32_t RTCMParser::crc24q(const uint8_t* data, size_t len)
{
    return RTCMFramer::crc24q({data, len});
}

bool RTCMParser::validateCrc() const
{
    return _framer.valid();
}

QByteArray RTCMParser::currentFrame() const
{
    return QByteArray(reinterpret_cast<const char*>(_framer.message()), _framer.messageLength());
}

bool RTCMParser::isValidFrame(const QByteArray& frame)
{
    return RTCMFramer::isValidFrame(
        {reinterpret_cast<const uint8_t*>(frame.constData()), static_cast<size_t>(frame.size())});
}
