#pragma once

#include <QtCore/QByteArray>
#include <QtCore/QList>

#include <cstdint>

struct GpsRtcmPacket
{
    uint8_t flags = 0;
    QByteArray data;
};

/// Pure GPS_RTCM_DATA fragmentation; no vehicle, link, or application dependencies.
class RTCMMavlinkPacket
{
public:
    static constexpr qsizetype kFragmentLen = 180;
    static constexpr qsizetype kMaxFragments = 4;
    static constexpr qsizetype kMaxAssembledLen = kFragmentLen * kMaxFragments;

    struct PackResult
    {
        QList<GpsRtcmPacket> packets;
        uint8_t nextSequenceId = 0;
    };

    static PackResult pack(QByteArrayView data, uint8_t sequenceId);

private:
    static uint8_t _makeFlags(bool fragmented, uint8_t fragmentId, uint8_t sequenceId);
};
