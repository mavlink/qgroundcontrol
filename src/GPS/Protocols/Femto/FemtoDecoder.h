#pragma once

#include "GPSFrame.h"
#include "GPSReceiveUpdates.h"
#include "GPSSurveyClock.h"

class GPSDecodeContext;

namespace Femto {

/// What configuration established about the receiver, and the RTCM activation the decoder leaves for the
/// configurator's streaming services.
struct Session
{
    bool configured = false;
    bool correctionOutputActive = false;
    /// Position averaging finished, so RTCM output is due.
    bool rtcmActivationPending = false;
    GPSSurveyClock surveyClock;
};

/// Decodes the Femtomes stream without I/O: RTCM3 frames, and GGA sentences, which a base station uses only to
/// observe its survey-in.
class Decoder
{
public:
    explicit Decoder(bool satelliteInfoEnabled)
        : _satelliteInfo(satelliteInfoEnabled)
    {}

    GPSReceiveUpdates onFrame(const GPSFrame& frame, GPSDecodeContext& context);

    [[nodiscard]] Session& session() { return _session; }

    [[nodiscard]] const Session& session() const { return _session; }

private:
    Session _session;
    bool _satelliteInfo = true;
};

}  // namespace Femto
