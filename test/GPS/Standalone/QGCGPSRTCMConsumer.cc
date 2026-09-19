#include <algorithm>
#include <array>
#include <cstdint>
#include <optional>
#include <span>

#include <QtCore/QByteArray>

#include "RTCMDecodedFrame.h"
#include "RTCMFrameDecoder.h"
#include "RTCMFramer.h"

#if defined(QT_POSITIONING_LIB) || defined(QT_NETWORK_LIB) || defined(QT_QML_LIB)
#error RTCM must not inherit positioning, transport, or application dependencies.
#endif

int main()
{
    constexpr std::array<uint8_t, 8> frame{0xd3, 0x00, 0x02, 0x3e, 0xd0, 0xa4, 0xe0, 0x00};
    const auto bytes = QByteArrayView(frame).toByteArray();
    if (!RTCMFramer::isValidFrame(bytes) || !RTCMFramer::isValidFrame(std::span<const uint8_t>(frame))) {
        return 1;
    }
    RTCMFramer framer;
    for (const auto byte : frame) {
        framer.addByte(byte);
    }
    if (!framer.valid() || framer.messageId() != 1005 || !std::ranges::equal(framer.frame(), frame)) {
        return 2;
    }
    if (framer.nextFrame() || framer.hasPartialFrame() || !framer.frame().empty()) {
        return 3;
    }
    framer.addByte(RTCMFramer::PREAMBLE);
    if (!framer.hasPartialFrame()) {
        return 4;
    }
    framer.reset();
    if (framer.hasPartialFrame() || framer.bufferedSize() != 0) {
        return 5;
    }

    RTCMFrameDecoder decoder;
    std::optional<RTCMDecodedFrame> decoded;
    int rejected = 0;
    for (const char byte : QByteArray::fromHex("d3") + bytes) {
        for (auto result = decoder.addByte(static_cast<uint8_t>(byte), 1000); result; result = decoder.nextFrame()) {
            if (result->valid) {
                decoded = result;
            } else {
                ++rejected;
            }
        }
    }
    if (rejected != 1 || !decoded || !decoded->valid || decoded->data != bytes || decoded->messageId != 1005 ||
        decoded->receivedAtMs != 1000) {
        return 6;
    }
    return 0;
}
