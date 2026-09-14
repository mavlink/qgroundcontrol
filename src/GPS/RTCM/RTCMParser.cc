#include "RTCMParser.h"

#include <QtCore/QByteArrayView>

RTCMParser::RTCMParser() = default;

void RTCMParser::reset()
{
    _framer.reset();
}

bool RTCMParser::addByte(uint8_t byte)
{
    return _framer.addByte(byte);
}

uint32_t RTCMParser::crc24q(const uint8_t* data, size_t len)
{
    return RTCMFramer::crc24q({data, len});
}

QByteArray RTCMParser::currentFrame() const
{
    const auto frame = _framer.frame();
    return QByteArrayView(frame).toByteArray();
}
