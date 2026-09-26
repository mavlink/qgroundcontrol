#pragma once

#include <chrono>
#include <cstdint>
#include <optional>
#include <string_view>

#include <QtCore/QByteArray>
#include <QtCore/QString>

#include "GPSBaseStationConfig.h"
#include "GPSNMEAStream.h"
#include "GPSProtocolMath.h"
#include "GPSReceiveUpdates.h"
#include "Unicore/UnicorePlan.h"

class GPSDecodeContext;
class GPSStreamDemux;
struct GPSFrame;

namespace Unicore {

/// Decodes UM980/UM982 output: standard NMEA epochs, RTCM3, command acknowledgements and the VERSIONA, MODE and
/// BESTNAVXYZA logs. It resolves the reply of the command the configurator expects, and monitors the base: the base
/// is valid while fresh BESTNAVXYZA epochs report FIXEDPOS at the configured coordinates, at most
/// BASE_STATUS_TIMEOUT apart. Once configured, a reboot, another mode or a lost base ends the session.
class Decoder
{
public:
    explicit Decoder(bool satelliteInfoEnabled);

    GPSReceiveUpdates onFrame(const GPSFrame& frame, GPSDecodeContext& context);

    /// Ends a session whose base status expired or which failed, then flushes the NMEA stream.
    void flush(GPSDecodeContext& context);

    /// Starts a configuration attempt: forgets the receiver and its stream, stops corrections and revokes the base.
    void startSession(GPSStreamDemux& stream, GPSDecodeContext& context, bool averaging);

    /// The coordinates a fixed base must report.
    void setFixedPosition(const GPSProtocolMath::Ecef& position) { _fixedECEF = position; }

    /// Restarts the stream, as before probing another baud rate.
    void resetStream(GPSStreamDemux& stream) { _nmea.reset(stream); }

    /// The next reply completes @a command. Clears rejection().
    void expect(const Plan::Command& command);

    /// Reports base status from BESTNAVXYZA from now on; an averaging base is active until it reports FIXEDPOS.
    void monitorBase(GPSDecodeContext& context);

    /// The configuration succeeded: corrections flow while the base is valid.
    void finishSession();

    /// Decode-only: the state a successful configuration for @a mode leaves, without publishing an initial status.
    void armDecodeOnly(const GPSBaseStationConfig::Mode& mode);

    /// The configuration failed: revokes a monitored base and stops corrections.
    void failSession(GPSDecodeContext& context);

    [[nodiscard]] bool ready() const { return _ready; }

    [[nodiscard]] const QByteArray& model() const { return _model; }

    [[nodiscard]] const QByteArray& firmware() const { return _firmware; }

    /// Model and firmware from VERSIONA.
    [[nodiscard]] QString identity() const;

    /// Why the receiver that answered the identity query is not supported; empty otherwise.
    [[nodiscard]] const QString& rejection() const { return _rejection; }

    [[nodiscard]] const GPSNMEAStream& nmea() const { return _nmea; }

private:
    static constexpr uint64_t BASE_STATUS_TIMEOUT_US = 5000000;

    GPSReceiveUpdates _decodeLine(std::string_view line, GPSDecodeContext& context);
    GPSReceiveUpdates _decodeAcknowledgement(std::string_view line, GPSDecodeContext& context);
    void _handleVersion(std::string_view body, GPSDecodeContext& context);
    void _handleMode(std::string_view body, GPSDecodeContext& context);
    void _handlePosition(std::string_view body, GPSDecodeContext& context);
    void _publishBase(GPSDecodeContext& context, bool valid, bool active);
    void _invalidateBase(GPSDecodeContext& context);
    void _expireBase(GPSDecodeContext& context);
    bool _awaiting(GPSDecodeContext& context, Plan::Reply reply) const;

    GPSNMEAStream _nmea;
    Plan::Command _command;
    /// The role MODE must report: the readback's while configuring, the base mode's once ready.
    Plan::Reply _expectedMode = Plan::Reply::RoverMode;
    QByteArray _model;
    QByteArray _firmware;
    QString _rejection;
    GPSProtocolMath::Ecef _fixedECEF;
    GPSProtocolMath::Ecef _baseECEF;
    uint64_t _lastBaseStatus = 0;
    std::optional<uint64_t> _lastBaseEpoch;
    bool _ready = false;
    bool _monitorBase = false;
    bool _baseValid = false;
    bool _averaging = false;
};

}  // namespace Unicore
