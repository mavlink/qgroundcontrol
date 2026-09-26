#pragma once

#include <chrono>
#include <cstdint>
#include <optional>
#include <string_view>
#include <utility>

#include <QtCore/QByteArray>
#include <QtCore/QString>

#include "GPSBaseStationConfig.h"
#include "GPSDecodedReports.h"
#include "GPSNMEAStream.h"
#include "GPSProtocolMath.h"
#include "GPSReceiveUpdates.h"

class GPSDecodeContext;
class GPSStreamDemux;
struct GPSFrame;

namespace Quectel {

/// Decodes LG290P output: standard NMEA epochs, RTCM3, PQTM replies for the pending command, the PQTMVER boot banner
/// and PQTMSVINSTATUS survey status. Survey status counts only once a restart has been verified: from the boot banner
/// it is held, and it is published once the configurator starts monitoring. Status older than STATUS_MAX_AGE, a
/// mismatch with the requested base, an unexpected boot banner or a failed session revoke it. Corrections flow only
/// while a configured base reports a valid survey.
class Decoder
{
public:
    explicit Decoder(bool satelliteInfoEnabled);

    GPSReceiveUpdates onFrame(const GPSFrame& frame, GPSDecodeContext& context);

    /// Revokes stale survey status, then flushes the NMEA stream.
    void flush(GPSDecodeContext& context);

    /// Ends the session: not configured, no survey, no corrections.
    void endSession(GPSDecodeContext& context);

    /// Starts a configuration attempt after endSession(): forgets the firmware and survey history, restarts the stream.
    void startSession(GPSStreamDemux& stream);

    /// The base the survey status must describe.
    void setBase(const GPSBaseStationConfig::Mode& mode);

    [[nodiscard]] const GPSBaseStationConfig::Mode& baseMode() const { return _baseMode; }

    /// Coordinates of a fixed base, in ECEF metres.
    [[nodiscard]] const GPSProtocolMath::Ecef& fixedPosition() const { return _fixedECEF; }

    /// Restarts the stream, as before probing another baud rate.
    void resetStream(GPSStreamDemux& stream) { _nmea.reset(stream); }

    /// The firmware PQTMVERNO reported; the boot banner must name it.
    void setFirmware(QByteArray firmware) { _firmware = std::move(firmware); }

    [[nodiscard]] const QByteArray& firmware() const { return _firmware; }

    [[nodiscard]] QString identity() const { return QString::fromUtf8(_firmware); }

    /// A restart is about to be requested: the survey is revoked until the restart is verified.
    void beginRestart(GPSDecodeContext& context);

    /// The restart was requested: restarts the stream and watches for the boot banner or a PQTMSRR rejection.
    void watchBoot(GPSStreamDemux& stream);

    /// Ends watchBoot(); a boot banner is unexpected from now on.
    void stopWatchingBoot() { _expectingBoot = false; }

    [[nodiscard]] bool booted() const { return _sawBoot; }

    [[nodiscard]] bool restartRejected() const { return _restartRejected; }

    /// Publishes survey status from now on, starting with any status held since the verified restart.
    void monitorSurvey(GPSDecodeContext& context);

    /// The configuration succeeded: corrections flow while the survey is valid.
    void finishSession(GPSDecodeContext& context);

    [[nodiscard]] bool configured() const { return _configured; }

    /// Revokes survey status that is stale or belongs to a failed session.
    void expireSurvey(GPSDecodeContext& context);

    [[nodiscard]] std::chrono::milliseconds limitReceiveTimeout(std::chrono::milliseconds timeout, uint64_t nowUs) const
    {
        return _nmea.limitReceiveTimeout(timeout, nowUs);
    }

private:
    enum class SurveyPhase
    {
        Off,
        AwaitingBoot,
        Verifying,
        Monitoring,
    };

    static constexpr uint64_t STATUS_MAX_AGE_US = 5000000;

    GPSReceiveUpdates _decodeLine(std::string_view line, GPSDecodeContext& context);
    bool _handleSurvey(std::string_view body, GPSDecodeContext& context);
    void _revokeSurvey(GPSDecodeContext& context);
    void _publishSurvey(GPSDecodeContext& context);

    GPSNMEAStream _nmea;
    GPSBaseStationConfig::Mode _baseMode = GPSBaseStationConfig::SurveyIn{};
    GPSProtocolMath::Ecef _fixedECEF;
    QByteArray _firmware;
    std::optional<unsigned> _lastTow;
    std::optional<GPSDecodedSurvey> _survey;
    SurveyPhase _phase = SurveyPhase::Off;
    bool _configured = false;
    bool _expectingBoot = false;
    bool _sawBoot = false;
    bool _restartRejected = false;
};

}  // namespace Quectel
