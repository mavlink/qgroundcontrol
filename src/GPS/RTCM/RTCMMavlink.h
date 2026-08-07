#pragma once

#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtCore/QObject>
#include <atomic>
#include <cstdint>

#include "DataRateTracker.h"

typedef struct __mavlink_gps_rtcm_data_t mavlink_gps_rtcm_data_t;

/// One GPS_RTCM_DATA payload ready to encode. flags layout matches MAVLink:
/// bit0 = fragmented, bits1-2 = fragment ID, bits3-7 = sequence ID.
struct GpsRtcmPacket
{
    uint8_t flags = 0;
    QByteArray data;  // 0..kFragmentLen bytes
};

class RTCMMavlink : public QObject
{
    Q_OBJECT
    Q_PROPERTY(quint64 totalBytesSent READ totalBytesSent NOTIFY bandwidthChanged)
    Q_PROPERTY(double bandwidthKBps READ bandwidthKBps NOTIFY bandwidthChanged)

public:
    /// MAVLink GPS_RTCM_DATA data[] field length.
    static constexpr qsizetype kFragmentLen = 180;
    /// Fragment ID is 2 bits — at most 4 fragments per reassembled message.
    static constexpr qsizetype kMaxFragments = 4;
    /// Max payload that fits one fragmented sequence (4 * 180).
    static constexpr qsizetype kMaxAssembledLen = kFragmentLen * kMaxFragments;

    RTCMMavlink(QObject* parent = nullptr);
    ~RTCMMavlink();

    quint64 totalBytesSent() const { return _rateTracker.totalBytes(); }

    double bandwidthKBps() const { return _rateTracker.kBps(); }

    /// Pack one RTCM blob into GPS_RTCM_DATA packets per MAVLink rules.
    ///
    /// - size 0: no packets
    /// - size <= 180: one unfragmented packet
    /// - 181..720: fragmented; exact multiples of 180 with fewer than 4 fragments
    ///   get a final zero-length fragment (required by MAVLink / ArduPilot / PX4)
    /// - size > 720: stream as successive unfragmented chunks (protocol cannot
    ///   reassemble more than 720 bytes in one sequence)
    ///
    /// @param sequenceId starting sequence id (0..31); advanced for each logical message
    /// @return packets plus the next sequence id to use
    struct PackResult
    {
        QList<GpsRtcmPacket> packets;
        uint8_t nextSequenceId = 0;
    };

    static PackResult pack(QByteArrayView data, uint8_t sequenceId);

public slots:
    void RTCMDataUpdate(QByteArrayView data);

public:
    /// Stream synthetic RTCM frames to vehicles until requestStop, for the
    /// SIMULATE_RTCM_OUTPUT dev build. Blocks the calling thread.
    void sendSimulatedData(const std::atomic_bool& requestStop);

signals:
    void bandwidthChanged();

private:
    static void _sendMessageOnAllLinks(const mavlink_gps_rtcm_data_t& data);
    static uint8_t _makeFlags(bool fragmented, uint8_t fragmentId, uint8_t sequenceId);

    uint8_t _sequenceId = 0;
    DataRateTracker _rateTracker;
};
