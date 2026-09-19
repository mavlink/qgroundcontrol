#include <QtCore/QByteArray>

#include "RTCMMavlinkPacket.h"

#if defined(QT_POSITIONING_LIB) || defined(QT_NETWORK_LIB) || defined(QT_QML_LIB)
#error MAVLink packetization must not inherit positioning, transport, or application dependencies.
#endif

int main()
{
    const auto bytes = QByteArray::fromHex("d300023ed0a4e000");
    const auto packed = RTCMMavlinkPacket::pack(bytes, 31);
    if (packed.packets.size() != 1 || packed.packets.first().data != bytes || packed.packets.first().flags != 248 ||
        packed.nextSequenceId != 32) {
        return 1;
    }
    return 0;
}
