#pragma once

#include <chrono>
#include <cstdint>
#include <optional>
#include <string_view>

#include "GPSDecodedReports.h"
#include "GPSFrame.h"
#include "GPSProtocolEvent.h"
#include "GPSStreamDemux.h"
#include "GPSTask.h"
#include "NMEANavigationEpoch.h"
#include "NMEASatelliteEpoch.h"

class GPSCommandChannel;
class GPSDecodeContext;

/// Standard NMEA navigation and satellite epochs plus RTCM3 pass-through, composed by the ASCII receiver families.
/// Binary RTCM payloads never enter the line parser.
///
/// A family routes RTCM3 frames to decodeRTCM(). For each line it calls decodeStandard(), then its own vendor
/// handling, then finishLine() with the combined updates, which publishes the working position when an epoch
/// completed. Vendor code shares the working position and satellites through position() and satellites().
class GPSNMEAStream
{
public:
    /// Who turns sentences into positions: this stream from standard NMEA, or the family's own sentences.
    enum class Navigation
    {
        StandardNMEA,
        ReceiverSpecific,
    };

    /// Framers an ASCII family's descriptor declares.
    static constexpr GPSStreamConfig STREAM{.framers = GPSFrameKind::ASCIILine | GPSFrameKind::RTCM3,
                                            .enabled = GPSFrameKind::ASCIILine | GPSFrameKind::RTCM3};

    explicit GPSNMEAStream(Navigation navigation = Navigation::StandardNMEA, bool satelliteInfoEnabled = true);

    /// Starts a new receiver session: clears the framers, epochs and working reports.
    void reset(GPSStreamDemux& stream);

    void setRTCMEnabled(bool enabled) { _rtcmEnabled = enabled; }

    /// Publishes a receiver RTCM3 frame while RTCM output is enabled.
    GPSReceiveUpdates decodeRTCM(const GPSFrame& frame, GPSDecodeContext& context);

    /// Standard NMEA handling of one complete line, without CR/LF.
    GPSReceiveUpdates decodeStandard(std::string_view line, GPSDecodeContext& context);

    /// Drains completed satellite systems and publishes the working position when @a updates includes a position.
    /// @return @a updates.
    GPSReceiveUpdates finishLine(GPSReceiveUpdates updates, GPSDecodeContext& context);

    /// The whole frame handling for a family without vendor sentences.
    GPSReceiveUpdates onFrame(const GPSFrame& frame, GPSDecodeContext& context);

    /// Expires untimed metadata and due satellite epochs, and drains queued RTCM3 frames and satellite systems.
    void flush(GPSDecodeContext& context);

    /// Shortens a receive @a timeout so a pending satellite epoch is published when it becomes due.
    [[nodiscard]] std::chrono::milliseconds limitReceiveTimeout(std::chrono::milliseconds timeout,
                                                                uint64_t nowUs) const;

    /// An ASCII family's streaming receive: a receive cycle bounded by limitReceiveTimeout(), then the streaming
    /// services.
    GPSTask<GPSReceiveUpdates> receive(GPSCommandChannel& channel, std::chrono::milliseconds timeout) const;

    [[nodiscard]] GPSDecodedPosition& position() { return _position; }

    /// Null when satellite information is disabled.
    [[nodiscard]] GPSDecodedSatellites* satellites() { return _satellites ? &*_satellites : nullptr; }

private:
    void _queueSatellites(const NMEA::SatelliteEpoch& epoch);
    void _drainSatellites(GPSDecodeContext& context);

    static constexpr std::chrono::microseconds METADATA_MAX_AGE{2000000};

    GPSDecodedPosition _position;
    std::optional<GPSDecodedSatellites> _satellites;
    NMEA::SatelliteAssembler _satelliteAssembler;
    NMEA::SatelliteEpoch _pendingSatellites;
    NMEA::NavigationEpochAssembler _navigationAssembler;
    Navigation _navigation = Navigation::StandardNMEA;
    bool _rtcmEnabled = true;
};
