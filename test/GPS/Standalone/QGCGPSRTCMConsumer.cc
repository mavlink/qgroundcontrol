#include <QtCore/QByteArray>

#include "RTCMFrame.h"
#include "RTCMFrameDecoder.h"
#include "RTCMMavlinkPacket.h"

#if defined(QT_POSITIONING_LIB) || defined(QT_NETWORK_LIB) || defined(QT_QML_LIB)
#error RTCM must not inherit positioning, transport, or application dependencies.
#endif

int main()
{
    const auto bytes = QByteArray::fromHex("d300023ed0a4e000");
    if (!RTCM::isValidFrame(bytes)) {
        return 1;
    }
    RTCMFrameDecoder decoder;
    std::optional<RTCMFrameDecoder::Result> decoded;
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
        return 2;
    }
    const auto packed = RTCMMavlinkPacket::pack(decoded->data, 31);
    if (packed.packets.size() != 1 || packed.packets.first().data != bytes || packed.packets.first().flags != 248 ||
        packed.nextSequenceId != 32) {
        return 3;
    }
    return 0;
}
