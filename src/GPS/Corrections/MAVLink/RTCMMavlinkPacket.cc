#include "RTCMMavlinkPacket.h"

#include <algorithm>
#include <utility>

uint8_t RTCMMavlinkPacket::_makeFlags(bool fragmented, uint8_t fragmentId, uint8_t sequenceId)
{
    uint8_t flags = static_cast<uint8_t>((sequenceId & 0x1FU) << 3);
    if (fragmented) {
        flags |= 0x01U;
        flags |= static_cast<uint8_t>((fragmentId & 0x03U) << 1);
    }
    return flags;
}

RTCMMavlinkPacket::PackResult RTCMMavlinkPacket::pack(QByteArrayView data, uint8_t sequenceId)
{
    PackResult result;
    result.nextSequenceId = sequenceId;

    if (data.isEmpty()) {
        return result;
    }

    // Oversized frames require stream reconstruction, not wrapped fragment IDs.
    if (data.size() > kMaxAssembledLen) {
        qsizetype start = 0;
        while (start < data.size()) {
            const qsizetype length = std::min(data.size() - start, kFragmentLen);
            GpsRtcmPacket packet;
            packet.flags = _makeFlags(false, 0, result.nextSequenceId);
            packet.data = data.mid(start, length).toByteArray();
            result.packets.append(std::move(packet));
            ++result.nextSequenceId;
            start += length;
        }
        return result;
    }

    if (data.size() <= kFragmentLen) {
        GpsRtcmPacket packet;
        packet.flags = _makeFlags(false, 0, sequenceId);
        packet.data = data.toByteArray();
        result.packets.append(std::move(packet));
        ++result.nextSequenceId;
        return result;
    }

    uint8_t fragmentId = 0;
    qsizetype start = 0;
    while (start < data.size()) {
        const qsizetype length = std::min(data.size() - start, kFragmentLen);
        GpsRtcmPacket packet;
        packet.flags = _makeFlags(true, fragmentId, sequenceId);
        packet.data = data.mid(start, length).toByteArray();
        result.packets.append(std::move(packet));
        ++fragmentId;
        start += length;
    }

    // Full fragments need a terminator unless all four are present.
    if ((data.size() % kFragmentLen) == 0 && fragmentId < kMaxFragments) {
        GpsRtcmPacket terminator;
        terminator.flags = _makeFlags(true, fragmentId, sequenceId);
        terminator.data.clear();
        result.packets.append(std::move(terminator));
    }

    ++result.nextSequenceId;
    return result;
}
