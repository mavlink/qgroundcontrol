#pragma once

#include <chrono>
#include <string_view>

#include "GPSCommandChannel.h"
#include "GPSFamilyProtocol.h"
#include "GPSNMEAStream.h"

/// A family whose receivers speak NMEA-framed text with RTCM3: GPSNMEAStream decodes the standard sentences and
/// corrections, and the family adds its own lines, its expiry and its configuration. RTCM3 is published only once the
/// family enables it.
class GPSNMEAFamilyProtocol : public GPSFamilyProtocol
{
public:
    GPSReceiveUpdates onFrame(const GPSFrame& frame, GPSDecodeContext& context) final
    {
        return _nmea.onFrame(frame, context,
                             [this, &context](std::string_view line) { return _decodeLine(line, context); });
    }

    /// Expires the family's stale state, then flushes the NMEA stream.
    void flush(GPSDecodeContext& context) final
    {
        _expire(context);
        _nmea.flush(context);
    }

    bool receiverReady() const final { return _ready; }

    /// After a failure, flushes at once so the expiry it causes is published with it.
    GPSReceiveUpdates receive(GPSCommandChannel& channel, std::chrono::milliseconds timeout) final
    {
        const GPSReceiveUpdates updates = _nmea.receive(channel, timeout);
        if (channel.failed()) {
            (void) channel.flush();
        }
        return updates;
    }

protected:
    /// How long a periodic receiver report, such as base status or time metadata, stays current.
    static constexpr std::chrono::seconds STATUS_MAX_AGE{5};
    /// The longest read of a family that expires its status, so expiry runs on a silent link too.
    static constexpr std::chrono::milliseconds STATUS_POLL{1000};

    explicit GPSNMEAFamilyProtocol(GPSNMEAStream::Navigation navigation = GPSNMEAStream::Navigation::StandardNMEA)
        : _nmea(navigation)
    {
        _nmea.setRTCMEnabled(false);
    }

    /// Decodes one line after standard NMEA did. @return its updates.
    virtual GPSReceiveUpdates _decodeLine(std::string_view line, GPSDecodeContext& context) = 0;

    /// Drops state that is stale or that a recorded failure invalidated; runs before each decode.
    virtual void _expire(GPSDecodeContext& context) { Q_UNUSED(context) }

    GPSNMEAStream _nmea;
    /// Whether the receiver runs as the latest configure() set it up.
    bool _ready = false;
};
