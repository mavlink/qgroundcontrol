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

    // Larger than the 4-fragment reassembly window: stream unfragmented chunks so
    // the vehicle's RTCM framer can rebuild frames from the inject stream. Do not
    // invent fragment IDs beyond 0..3 (would clobber the sequence field).
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

    // Fragmented: 181..720 bytes. Fragment ID is only 2 bits (0..3).
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

    // Exact multiple of 180 with fewer than 4 fragments: MAVLink requires a final
    // zero-length fragment so receivers know the message is complete. (All four
    // full fragments complete by the "all fragments present" rule without this.)
    // See ArduPilot AP_GPS::handle_gps_rtcm_fragment and PX4 GpsRtcmMessageAssembler.
    if ((data.size() % kFragmentLen) == 0 && fragmentId < kMaxFragments) {
        GpsRtcmPacket terminator;
        terminator.flags = _makeFlags(true, fragmentId, sequenceId);
        terminator.data.clear();
        result.packets.append(std::move(terminator));
    }

    ++result.nextSequenceId;
    return result;
}
