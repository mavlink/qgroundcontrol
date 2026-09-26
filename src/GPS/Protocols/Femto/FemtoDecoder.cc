#include "Femto/FemtoDecoder.h"

#include <optional>
#include <string_view>

#include "GPSEventSink.h"
#include "GPSFamilyProtocol.h"
#include "NMEASentence.h"

namespace Femto {

namespace {

/// GGA fix quality of a position the receiver holds fixed: position averaging finished.
constexpr unsigned AVERAGED_POSITION_QUALITY = 7;

}  // namespace

GPSReceiveUpdates Decoder::onFrame(const GPSFrame& frame, GPSDecodeContext& context)
{
    if (frame.kind == GPSFrameKind::RTCM3) {
        context.sink().publishRTCM(frame.bytes);
        return {};
    }
    if (frame.kind != GPSFrameKind::NMEASentence) {
        return {};
    }
    const std::string_view text = frame.text();
    if (text.size() >= 6 && text.substr(3, 3) == "GGA") {
        const auto sentence = NMEA::sentence(text);
        const auto fix = sentence ? NMEA::gga(*sentence) : std::nullopt;
        if (!fix) {
            return {};
        }
        // Only a survey started by this configuration may complete it; earlier GGA output is stale.
        if (!_session.correctionOutputActive && _session.surveyClock.running() &&
            fix->quality == AVERAGED_POSITION_QUALITY) {
            _session.surveyClock.stop(context.nowUs());
            context.sink().publishSurvey(false, true, _session.surveyClock.duration(),
                                         {.latitudeDegrees = fix->latitude,
                                          .longitudeDegrees = fix->longitude,
                                          .altitudeMeters = static_cast<float>(fix->altitude + fix->geoidSeparation)});
            _session.rtcmActivationPending = true;
        }
        if (_satelliteInfo) {
            context.sink().publishSatelliteUsage(fix->satellitesUsed);
        }
    }

    if (_session.surveyClock.update(context.nowUs())) {
        context.sink().publishSurvey(true, false, _session.surveyClock.duration());
    }
    return {};
}

}  // namespace Femto
