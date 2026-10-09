#pragma once

#include <chrono>
#include <cstdint>
#include <string_view>

#include "GPSProtocolEvent.h"
#include "GPSStreamDemux.h"
#include "NMEANavigationEpoch.h"
#include "NMEASatellites.h"

class GPSCommandChannel;
class GPSDecodeContext;

/// Standard NMEA navigation and satellite epochs plus RTCM3 pass-through, composed by the ASCII receiver families.
/// Binary RTCM payloads never enter the line parser. A family hands each frame, a line or a checked sentence, to
/// onFrame() with its vendor handling, which shares the working position through position().
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

    explicit GPSNMEAStream(Navigation navigation = Navigation::StandardNMEA);

    /// Starts a new receiver session: clears every framer of @a stream, the epochs and the working reports.
    void reset(GPSStreamDemux& stream);

    void setRTCMEnabled(bool enabled) { _rtcmEnabled = enabled; }

    /// The whole frame handling: publishes an RTCM3 frame, or decodes a line or sentence as standard NMEA, then with
    /// @a vendor, which takes the line and returns its updates, and publishes what the line completed.
    template <typename Vendor>
    GPSReceiveUpdates onFrame(const GPSFrame& frame, GPSDecodeContext& context, Vendor&& vendor)
    {
        if (frame.kind == GPSFrameKind::RTCM3) {
            return _decodeRTCM(frame, context);
        }
        if (frame.kind != GPSFrameKind::ASCIILine && frame.kind != GPSFrameKind::NMEASentence) {
            return {};
        }
        GPSReceiveUpdates updates = _decodeStandard(frame.text(), context);
        updates |= vendor(frame.text());
        return _finishLine(updates, context);
    }

    /// The whole frame handling for a family without vendor sentences.
    GPSReceiveUpdates onFrame(const GPSFrame& frame, GPSDecodeContext& context)
    {
        return onFrame(frame, context, [](std::string_view) { return GPSReceiveUpdates{}; });
    }

    /// Expires untimed metadata and publishes satellite epochs that became due.
    void flush(GPSDecodeContext& context);

    /// Shortens a receive @a timeout so a pending satellite epoch is published when it becomes due.
    [[nodiscard]] std::chrono::milliseconds limitReceiveTimeout(std::chrono::milliseconds timeout,
                                                                uint64_t nowUs) const;

    /// An ASCII family's streaming receive: a receive cycle bounded by limitReceiveTimeout(), then the streaming
    /// services.
    GPSReceiveUpdates receive(GPSCommandChannel& channel, std::chrono::milliseconds timeout) const;

    [[nodiscard]] GPSDecodedPosition& position() { return _position; }

private:
    /// Publishes a receiver RTCM3 frame while RTCM output is enabled.
    GPSReceiveUpdates _decodeRTCM(const GPSFrame& frame, GPSDecodeContext& context);

    /// Standard NMEA handling of one complete line, without CR/LF.
    GPSReceiveUpdates _decodeStandard(std::string_view line, GPSDecodeContext& context);

    /// Publishes the satellite systems completed by the line, then the working position when @a updates includes a
    /// position. @return @a updates.
    GPSReceiveUpdates _finishLine(GPSReceiveUpdates updates, GPSDecodeContext& context);

    void _queueSatellites(const NMEA::SatelliteEpoch& epoch);
    void _publishSatellites(GPSDecodeContext& context);

    GPSDecodedPosition _position;
    NMEA::SatelliteAssembler _satelliteAssembler;
    /// Systems a line completed, published by _finishLine() after the line's vendor events.
    NMEA::SatelliteEpoch _pendingSatellites;
    NMEA::NavigationEpochAssembler _navigationAssembler;
    Navigation _navigation = Navigation::StandardNMEA;
    bool _rtcmEnabled = true;
};
