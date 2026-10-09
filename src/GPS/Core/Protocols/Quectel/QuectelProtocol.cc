#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <limits>
#include <optional>
#include <string_view>
#include <utility>
#include <variant>

#include <QtCore/QByteArray>
#include <QtCore/QByteArrayView>
#include <QtCore/QString>

#include "GPSCommandChannel.h"
#include "GPSNMEAFamilyProtocol.h"
#include "GPSProtocolEvent.h"
#include "GPSProtocolMath.h"
#include "GPSReceiverConfig.h"
#include "GPSReceiverFamilies.h"
#include "GPSStreamDemux.h"
#include "GPSTime.h"
#include "MonotonicClock.h"
#include "NMEASentence.h"
#include "QGCLoggingCategory.h"
#include "Quectel/QuectelCodec.h"
#include "Quectel/QuectelPlan.h"

QGC_LOGGING_CATEGORY(QuectelProtocolLog, "GPS.Protocols.Quectel")

namespace {

namespace Plan = Quectel::Plan;
using QuectelCodec::Fields;
using QuectelCodec::number;
using QuectelCodec::readback;
using QuectelCodec::rejected;

/// Configures an LG290P(03) as an RTK base: finds it at one of Plan::BAUD_RATES, or at the rate configure() gets when
/// nonzero, which then reports the rate found. The receiver must run the requested role and base from its saved
/// settings, verified after a restart; with consent to persistent changes, what differs is written, saved and
/// restarted. Then the base and NMEA output of Plan::BASE_OUTPUT and Plan::NMEA_OUTPUT is set and read back. Decoding
/// watches the restarts and the survey. A failure leaves a description in the channel's error detail, including
/// whether a flash save may have changed the receiver.
///
/// Decodes LG290P output: standard NMEA epochs, RTCM3, PQTM replies for the pending command, the PQTMVER boot banner
/// and PQTMSVINSTATUS survey status. Survey status counts only once a restart has been verified: from the boot banner
/// it is held, and it is published once configuration starts monitoring. Status older than STATUS_MAX_AGE, a mismatch
/// with the requested base, an unexpected boot banner or a failed configuration revoke it. Corrections flow only while
/// a configured base reports a valid survey.
class Protocol final : public GPSNMEAFamilyProtocol
{
public:
    bool configure(GPSCommandChannel& channel, GPSConfig config, unsigned& baud) override;

    bool probe(GPSCommandChannel& channel) override;

    /// The firmware PQTMVERNO reported.
    QString identity() const override { return QString::fromUtf8(_firmware); }

    bool armDecodeOnly(const GPSConfig& config, GPSDecodeContext& context) override
    {
        if (std::holds_alternative<GPSBaseStationConfig::ReceiverAveraging>(config.base.mode)) {
            return false;
        }
        _setBase(config.base.mode);
        _monitorSurvey(context);
        _finishAttempt(context);
        return true;
    }

    std::chrono::milliseconds nextReadSlice(std::chrono::milliseconds timeout) const override
    {
        return (std::min) (timeout, STATUS_POLL);
    }

private:
    class Attempt;

    enum class SurveyPhase
    {
        Off,
        AwaitingBoot,
        Verifying,
        Monitoring,
    };

    /// Ends readiness: no survey, no corrections.
    void _stop(GPSDecodeContext& context);
    /// Starts a configuration attempt after _stop(): forgets the firmware and survey history, restarts the stream.
    void _startAttempt(GPSStreamDemux& stream);
    /// The base the survey status must describe.
    void _setBase(const GPSBaseStationConfig::Mode& mode);
    /// A restart is about to be requested: the survey is revoked until the restart is verified.
    void _beginRestart(GPSDecodeContext& context);
    /// The restart was requested: restarts the stream and watches for the boot banner or a PQTMSRR rejection.
    void _watchBoot(GPSStreamDemux& stream);

    /// The restart was verified or abandoned: a later boot banner ends readiness.
    void _endBootWatch() { _expectingBoot = false; }

    /// Restarts the stream for an identity query at another rate.
    void _restartStream(GPSStreamDemux& stream) { _nmea.reset(stream); }

    /// Records the firmware PQTMVERNO reported.
    void _setFirmware(std::string_view firmware) { _firmware = QByteArrayView(firmware).toByteArray(); }

    /// Publishes survey status from now on, starting with any status held since the verified restart.
    void _monitorSurvey(GPSDecodeContext& context);
    /// The configuration succeeded: corrections flow while the survey is valid.
    void _finishAttempt(GPSDecodeContext& context);
    /// Revokes survey status that is stale or belongs to a failed configuration.
    void _expireSurvey(GPSDecodeContext& context);
    /// Ends readiness after a recorded failure; otherwise revokes stale survey status.
    void _expire(GPSDecodeContext& context) override;
    GPSReceiveUpdates _decodeLine(std::string_view line, GPSDecodeContext& context) override;
    bool _handleSurvey(std::string_view body, GPSDecodeContext& context);
    void _revokeSurvey(GPSDecodeContext& context);
    void _publishSurvey(GPSDecodeContext& context);

    GPSBaseStationConfig::Mode _baseMode = GPSBaseStationConfig::SurveyIn{};
    /// Coordinates of a fixed base, in ECEF metres.
    GPSProtocolMath::Ecef _fixedECEF;
    /// The firmware PQTMVERNO reported; the boot banner must name it.
    QByteArray _firmware;
    std::optional<unsigned> _lastTow;
    std::optional<GPSSurveyReport> _survey;
    SurveyPhase _phase = SurveyPhase::Off;
    /// A boot banner is expected; otherwise one ends readiness.
    bool _expectingBoot = false;
    bool _sawBoot = false;
    bool _restartRejected = false;
};

/// One configuration attempt: what the readbacks reported and whether a flash save may have changed the receiver.
class Protocol::Attempt
{
public:
    Attempt(GPSCommandChannel& channel, Protocol& protocol)
        : _channel(channel)
        , _protocol(protocol)
    {}

    bool run(GPSConfig config, unsigned& baud);

private:
    /// Labels the command @a label, or its @a body when empty.
    bool _acknowledge(QByteArrayView body, std::string_view label = {});
    bool _identify(std::chrono::milliseconds timeout = Plan::COMMAND_TIMEOUT);
    bool _verifyRole(bool requireMatch = true);
    bool _verifyBase(bool requireMatch = true);
    bool _changeRole();
    bool _changeBase(QByteArrayView body, std::string_view label);
    bool _save();
    bool _restart(bool requireRoleMatch = true);
    bool _restartWithBase();
    bool _verifySaved(bool restarted, bool baseVerified);
    /// @a failure is ConsentRequired when the request needs a persistent change without consent.
    bool _fail(const QString& reason, GPSProtocolError failure = GPSProtocolError::Protocol);

    GPSCommandChannel& _channel;
    Protocol& _protocol;
    /// Re-runs the unchanged survey READ_BASE reported.
    QByteArray _surveyRestart;
    unsigned _role = 0;
    bool _baseMatches = false;
    bool _baseHasDistance = false;
    bool _saveAcknowledged = false;
    bool _saveUncertain = false;
};

bool Protocol::configure(GPSCommandChannel& channel, GPSConfig config, unsigned& baud)
{
    Attempt attempt(channel, *this);
    return attempt.run(std::move(config), baud);
}

bool Protocol::Attempt::run(GPSConfig config, unsigned& baud)
{
    _protocol._stop(_channel.context());
    (void) _channel.flush();
    _protocol._startAttempt(_channel.stream());
    const auto* survey = std::get_if<GPSBaseStationConfig::SurveyIn>(&config.base.mode);
    if (std::holds_alternative<GPSBaseStationConfig::ReceiverAveraging>(config.base.mode) ||
        (survey && (survey->duration > Plan::MAXIMUM_SURVEY_DURATION ||
                    survey->accuracyMeters > Plan::MAXIMUM_SURVEY_ACCURACY_METERS))) {
        return _fail(
            QStringLiteral("Unsupported LG290P configuration: use native 1 Hz observation count (maximum 86400) and "
                           "3D position accuracy threshold (maximum 1000 m), not receiver-managed survey"));
    }
    const auto scope = _channel.deadlineScope(Plan::CONFIGURATION_TIMEOUT);
    _protocol._setBase(config.base.mode);
    const auto detection = _channel.detectBaud(Plan::BAUD_RATES, baud, [this]() -> GPSBaudProbe {
        _protocol._restartStream(_channel.stream());
        if (_identify()) {
            return GPSBaudProbe::Found;
        }
        // Firmware that answered but is not an LG290P(03) answers the same at every rate.
        return _protocol._firmware.isEmpty() ? GPSBaudProbe::TryNext : GPSBaudProbe::Stop;
    });
    if (!detection.found) {
        if (_protocol._firmware.isEmpty()) {
            _channel.failNoAnswer(detection.probed);
            return _fail({});
        }
        return _fail(QStringLiteral("No verified LG290P(03) identity; receiver configuration was not changed"));
    }
    baud = detection.baud;
    qCDebug(QuectelProtocolLog).noquote() << "Quectel receiver firmware:" << _protocol._firmware;
    if (!_verifyRole(false)) {
        return _fail(QStringLiteral("LG290P receiver role query failed; no role change was attempted"));
    }

    bool restarted = false;
    if (config.allowPersistentChanges) {
        // Work from saved settings, not another client's uncommitted changes. A role
        // readback alone cannot distinguish a pending role from the active one.
        if (!_restart(false)) {
            return _fail(
                QStringLiteral("LG290P saved role query after restart failed; no persistent change was attempted"));
        }
        restarted = true;
    }
    if (_role != Plan::BASE_ROLE) {
        if (!config.allowPersistentChanges) {
            return _fail(
                QStringLiteral("LG290P role mismatch: save the requested base role externally, reboot and reconnect. "
                               "Alternatively, explicitly allow persistent changes for this connection"),
                GPSProtocolError::ConsentRequired);
        }
        if (!_changeRole()) {
            return _fail(QStringLiteral("LG290P role change was rejected or its readback did not match"));
        }
        if (!_save()) {
            return _fail(QStringLiteral("LG290P role save failed; receiver activation is not verified"));
        }
        if (!_restart()) {
            return _fail(QStringLiteral("LG290P saved role could not be verified after restart"));
        }
        restarted = true;
    }

    bool baseChanged = false;
    if (!_verifyBase(false)) {
        return _fail(QStringLiteral("LG290P base settings query failed; no base change was attempted"));
    }
    if (!_baseMatches && !config.allowPersistentChanges) {
        return _fail(
            QStringLiteral("LG290P base settings mismatch: provision/save the requested ECEF coordinates or survey "
                           "observation count/accuracy externally, or explicitly allow persistent changes for this "
                           "connection"),
            GPSProtocolError::ConsentRequired);
    }
    if (!_baseMatches) {
        const QByteArray base = Plan::writeBase(config.base.mode, _protocol._fixedECEF, _baseHasDistance);
        if (!_changeBase(base, survey ? std::string_view{} : Plan::FIXED_BASE_LABEL)) {
            return _fail(QStringLiteral("LG290P base change was rejected or its readback did not match"));
        }
        if (!_save()) {
            return _fail(QStringLiteral("LG290P base save failed; receiver activation is not verified"));
        }
        if (!_restartWithBase()) {
            return _fail(QStringLiteral("LG290P saved base settings could not be verified after restart"));
        }
        baseChanged = true;
    }
    if (survey && !baseChanged) {
        qCDebug(QuectelProtocolLog)
            << "LG290P survey uses accepted 1 Hz observations, not wall time; accuracy filters individual 3D fixes. "
               "The receiver itself stores converged coordinates";
        if (!_acknowledge(_surveyRestart, Plan::RESTART_SURVEY_LABEL)) {
            return _fail(QStringLiteral("LG290P rejected restarting the unchanged, externally saved survey"));
        }
        restarted = false;
    }
    if (!_verifySaved(restarted, baseChanged)) {
        return _fail(
            QStringLiteral("LG290P restart/readback failed; saved role or base settings do not match the request"));
    }

    _protocol._monitorSurvey(_channel.context());
    const auto baseOutput = Plan::messageRates(Plan::BASE_OUTPUT);
    if (!_channel.runSequence(baseOutput).succeeded()) {
        return _fail(QStringLiteral("LG290P base message output configuration/readback failed"));
    }
    const auto nmeaOutput = Plan::messageRates(Plan::NMEA_OUTPUT);
    if (!_channel.runSequence(nmeaOutput).succeeded()) {
        return _fail(QStringLiteral("LG290P NMEA output configuration/readback failed"));
    }
    _protocol._finishAttempt(_channel.context());
    return true;
}

bool Protocol::Attempt::_acknowledge(QByteArrayView body, std::string_view label)
{
    auto command = Plan::command(body, Plan::acknowledgement(body));
    if (!label.empty()) {
        command.step.command = QByteArrayView(label).toByteArray();
    }
    const auto result = _channel.transact(command);
    return result.outcome == GPSCommandOutcome::Acknowledged;
}

bool Protocol::Attempt::_identify(std::chrono::milliseconds timeout)
{
    const auto result = _channel.transact(Plan::command(
        Plan::IDENTIFY,
        [this](std::string_view body) {
            const Fields reply(body);
            if (rejected(reply, Plan::IDENTIFY)) {
                return GPSCommandOutcome::Rejected;
            }
            if (reply.size() != 4 || reply[0] != Plan::IDENTIFY) {
                return GPSCommandOutcome::Pending;
            }
            const std::string_view firmware = reply[1];
            _protocol._setFirmware(firmware);
            return firmware.starts_with(Plan::QUALIFIED_FIRMWARE) && firmware.size() > Plan::QUALIFIED_FIRMWARE.size()
                       ? GPSCommandOutcome::ReadbackVerified
                       : GPSCommandOutcome::Rejected;
        },
        timeout));
    return result.outcome == GPSCommandOutcome::ReadbackVerified;
}

bool Protocol::Attempt::_verifyRole(bool requireMatch)
{
    const auto result = _channel.transact(Plan::command(Plan::READ_ROLE, [this, requireMatch](std::string_view body) {
        const Fields reply(body);
        unsigned role = 0;
        const bool valid = reply.size() == 3 && reply[0] == "PQTMCFGRCVRMODE" && reply[1] == "OK" &&
                           number(reply[2], role) && role <= 2;
        if (valid) {
            _role = role;
        }
        return readback(reply, "PQTMCFGRCVRMODE", valid && (!requireMatch || role == Plan::BASE_ROLE));
    }));
    return result.outcome == GPSCommandOutcome::ReadbackVerified;
}

bool Protocol::Attempt::_verifyBase(bool requireMatch)
{
    _baseMatches = false;
    const auto rate = _channel.transact(Plan::command(Plan::READ_FIX_RATE, [](std::string_view body) {
        const Fields reply(body);
        unsigned interval = 0;
        return readback(reply, "PQTMCFGFIXRATE",
                        reply.size() == 3 && number(reply[2], interval) && interval == Plan::FIX_INTERVAL_MS);
    }));
    if (rate.outcome != GPSCommandOutcome::ReadbackVerified) {
        return false;
    }
    const auto base = _channel.transact(Plan::command(Plan::READ_BASE, [this, requireMatch](std::string_view body) {
        const Fields reply(body);
        unsigned mode = 0;
        unsigned count = 0;
        double accuracy = 0;
        double distance = 0;
        GPSProtocolMath::Ecef ecef;
        const bool valid = (reply.size() == 8 || reply.size() == 9) && reply[0] == "PQTMCFGSVIN" && reply[1] == "OK" &&
                           number(reply[2], mode) && mode <= 2 && number(reply[3], count) &&
                           number(reply[4], accuracy) && number(reply[5], ecef.x) && number(reply[6], ecef.y) &&
                           number(reply[7], ecef.z) && (reply.size() == 8 || number(reply[8], distance)) &&
                           std::chrono::seconds(count) <= Plan::MAXIMUM_SURVEY_DURATION && accuracy >= 0 &&
                           accuracy <= Plan::MAXIMUM_SURVEY_ACCURACY_METERS && distance >= 0 && distance <= 10;
        bool matches = valid;
        if (valid) {
            _baseHasDistance = reply.size() == 9;
        }
        const auto& requested = _protocol._baseMode;
        if (matches && std::holds_alternative<GPSBaseStationConfig::Fixed>(requested)) {
            const auto& fixed = _protocol._fixedECEF;
            matches = mode == 2 && count == 0 && accuracy == 0 && GPSProtocolMath::distance(ecef, fixed) <= 0.00011;
        } else if (matches) {
            const auto& survey = std::get<GPSBaseStationConfig::SurveyIn>(requested);
            matches = mode == 1 && count == survey.duration.count() &&
                      std::abs(accuracy - survey.accuracyMeters) <= 0.000000001 && distance == 0;
            if (matches) {
                // Re-execute precisely the read configuration, including otherwise ignored ECEF fields.
                _surveyRestart = QByteArrayView(Plan::RESTART_SURVEY).toByteArray();
                for (size_t index = 2; index < reply.size(); ++index) {
                    _surveyRestart += ',';
                    _surveyRestart.append(QByteArrayView(reply[index]));
                }
            }
        }
        _baseMatches = matches;
        return readback(reply, "PQTMCFGSVIN", valid && (!requireMatch || matches));
    }));
    return base.outcome == GPSCommandOutcome::ReadbackVerified;
}

bool Protocol::Attempt::_changeRole()
{
    if (!_acknowledge(QByteArrayView(Plan::WRITE_BASE_ROLE))) {
        return false;
    }
    return _verifyRole();
}

bool Protocol::Attempt::_changeBase(QByteArrayView body, std::string_view label)
{
    if (!_acknowledge(body, label)) {
        return false;
    }
    return _verifyBase();
}

bool Protocol::Attempt::_restartWithBase()
{
    if (!_restart()) {
        return false;
    }
    return _verifyBase();
}

bool Protocol::Attempt::_verifySaved(bool restarted, bool baseVerified)
{
    // A role readback can reflect an unsaved, not-yet-active change. Reboot and read back
    // the saved role before claiming that the receiver actually operates in that role.
    if (!restarted) {
        if (!_restart()) {
            return false;
        }
    }
    if (baseVerified) {
        return true;
    }
    return _verifyBase();
}

bool Protocol::Attempt::_save()
{
    const auto result =
        _channel.transact(Plan::command(Plan::SAVE, Plan::acknowledgement(Plan::SAVE), Plan::SAVE_TIMEOUT));
    const bool saved = result.outcome == GPSCommandOutcome::Acknowledged;
    if (saved) {
        _saveAcknowledged = true;
        _saveUncertain = false;
        qCDebug(QuectelProtocolLog) << "LG290P acknowledged saving configuration to nonvolatile memory";
    } else {
        _saveUncertain = result.outcome != GPSCommandOutcome::Rejected && result.acceptedBytes > 0;
    }
    return saved;
}

bool Protocol::Attempt::_restart(bool requireRoleMatch)
{
    _protocol._beginRestart(_channel.context());
    qCDebug(QuectelProtocolLog) << "Restarting LG290P and verifying its saved configuration";
    const auto scope = _channel.deadlineScope(Plan::RESTART_TIMEOUT);
    const QByteArray restart = NMEAUtils::frame(Plan::RESTART);
    if (!_channel.writeCommand({QByteArrayView(Plan::RESTART).toByteArray(), Plan::RESTART_TIMEOUT}, restart)) {
        return false;
    }
    // PQTMSRR has no documented ACK. Do not inflate successful transport completion to acknowledgment.
    _channel.finishCommand(GPSCommandOutcome::Written);
    _protocol._watchBoot(_channel.stream());
    do {
        _channel.wait(Plan::RESTART_POLL);
        if (_channel.failed()) {
            break;
        }
        const bool identified = _identify(Plan::RESTART_IDENTIFY_TIMEOUT);
        if (_protocol._restartRejected) {
            _channel.setErrorDetail(QStringLiteral("LG290P rejected PQTMSRR."));
            break;
        }
        if (identified && _protocol._sawBoot) {
            _protocol._endBootWatch();
            return _verifyRole(requireRoleMatch);
        }
    } while (!_channel.failed() && _channel.nowUs() < _channel.operationDeadline().untilUs);
    _protocol._endBootWatch();
    return false;
}

bool Protocol::Attempt::_fail(const QString& reason, GPSProtocolError failure)
{
    _protocol._stop(_channel.context());
    (void) _channel.flush();
    QString detail = reason;
    if (!_channel.errorDetail().isEmpty()) {
        detail += (detail.isEmpty() ? QString() : QStringLiteral(" ")) + _channel.errorDetail();
    }
    _channel.failControl(failure);
    if (_saveAcknowledged || _saveUncertain) {
        const QString persistence =
            _saveAcknowledged ? QStringLiteral("An LG290P flash save was acknowledged. ") : QString();
        const QString uncertain =
            _saveUncertain ? QStringLiteral("The latest flash save may have taken effect. ") : QString();
        _channel.setErrorDetail(persistence + uncertain + detail +
                                QStringLiteral(" Receiver settings may have changed; no rollback was attempted."));
    } else if (_channel.error() != GPSProtocolError::Cancelled) {
        _channel.setErrorDetail(detail);
    }
    return false;
}

void Protocol::_stop(GPSDecodeContext& context)
{
    _ready = false;
    _phase = SurveyPhase::Off;
    _revokeSurvey(context);
}

void Protocol::_startAttempt(GPSStreamDemux& stream)
{
    _lastTow.reset();
    _survey.reset();
    _phase = SurveyPhase::Off;
    _firmware.clear();
    _nmea.reset(stream);
    _nmea.setRTCMEnabled(false);
}

void Protocol::_setBase(const GPSBaseStationConfig::Mode& mode)
{
    _baseMode = mode;
    if (const auto* fixed = std::get_if<GPSBaseStationConfig::Fixed>(&mode)) {
        _fixedECEF = GPSProtocolMath::toEcef(fixed->position);
    }
}

void Protocol::_beginRestart(GPSDecodeContext& context)
{
    _revokeSurvey(context);
    _phase = SurveyPhase::AwaitingBoot;
}

void Protocol::_watchBoot(GPSStreamDemux& stream)
{
    _nmea.reset(stream);
    _expectingBoot = true;
    _sawBoot = false;
    _restartRejected = false;
}

void Protocol::_monitorSurvey(GPSDecodeContext& context)
{
    _phase = SurveyPhase::Monitoring;
    _publishSurvey(context);
}

void Protocol::_finishAttempt(GPSDecodeContext& context)
{
    _ready = true;
    _expireSurvey(context);
    _nmea.setRTCMEnabled(_survey && _survey->valid);
}

void Protocol::_expire(GPSDecodeContext& context)
{
    if (context.failed()) {
        _stop(context);
    } else {
        _expireSurvey(context);
    }
}

GPSReceiveUpdates Protocol::_decodeLine(std::string_view line, GPSDecodeContext& context)
{
    const auto body = QuectelCodec::checkedBody(line);
    if (body.empty()) {
        return {};
    }
    context.offerReply(body);
    if (_expectingBoot && body.starts_with("PQTMSRR,") && QuectelCodec::rejected(Fields(body), "PQTMSRR")) {
        _restartRejected = true;
    }
    if (body.starts_with("PQTMSVINSTATUS,")) {
        return _handleSurvey(body, context) ? GPSReceiveUpdates(GPSReceiveUpdate::Activity) : GPSReceiveUpdates{};
    } else if (body.starts_with("PQTMVER,")) {
        const Fields reply(body);
        const std::string_view firmware(_firmware.constData(), static_cast<size_t>(_firmware.size()));
        if (reply.size() == 6 && reply[1] == "1" && reply[2] == "MODULE" && reply[3] == firmware) {
            // §2.3.1: this is the first output upon each successful startup.
            if (_expectingBoot && !_sawBoot) {
                _sawBoot = true;
                if (_phase == SurveyPhase::AwaitingBoot) {
                    _phase = SurveyPhase::Verifying;
                }
            } else if (!_expectingBoot && (_ready || _phase != SurveyPhase::Off)) {
                _ready = false;
                _phase = SurveyPhase::Off;
                _revokeSurvey(context);
                context.failControl(QStringLiteral("LG290P restarted; its base configuration is no longer verified"));
            }
            return GPSReceiveUpdate::Activity;
        }
    }
    return {};
}

void Protocol::_revokeSurvey(GPSDecodeContext& context)
{
    _nmea.setRTCMEnabled(false);
    if (_survey) {
        _survey.reset();
        context.publishSurvey(false, false, {});
    }
}

void Protocol::_expireSurvey(GPSDecodeContext& context)
{
    if (_survey &&
        (context.failed() || !MonotonicClock::withinAge(_survey->timestampUs, context.nowUs(), STATUS_MAX_AGE))) {
        _revokeSurvey(context);
    }
}

void Protocol::_publishSurvey(GPSDecodeContext& context)
{
    _expireSurvey(context);
    if (_phase == SurveyPhase::Monitoring && _survey) {
        // A status buffered during boot verification keeps its original receipt time.
        context.publishAsReceived(*_survey);
        _nmea.setRTCMEnabled(_ready && _survey->valid);
    }
}

bool Protocol::_handleSurvey(std::string_view body, GPSDecodeContext& context)
{
    const Fields reply(body);
    unsigned version = 0;
    unsigned tow = 0;
    unsigned validity = 0;
    unsigned observations = 0;
    unsigned configuredCount = 0;
    double accuracy = 0;
    GPSProtocolMath::Ecef ecef;
    if (reply.size() != 12 || !number(reply[1], version) || version != 1 || !number(reply[2], tow) ||
        tow >= GPSTime::WEEK_MS) {
        return false;
    }
    // TOW is a modular measurement clock, not a receipt timestamp. A late packet from before Sunday's rollover must
    // not look newer than the current week.
    if (_lastTow && !GPSTime::towAdvances(tow, *_lastTow)) {
        return false;
    }
    _lastTow = tow;
    if (_phase != SurveyPhase::Verifying && _phase != SurveyPhase::Monitoring) {
        return false;
    }
    if (!number(reply[3], validity) || validity > 2 || !reply[4].empty() || !number(reply[6], observations) ||
        std::chrono::seconds(observations) > Plan::MAXIMUM_SURVEY_DURATION || !number(reply[7], configuredCount) ||
        std::chrono::seconds(configuredCount) > Plan::MAXIMUM_SURVEY_DURATION || !number(reply[8], ecef.x) ||
        !number(reply[9], ecef.y) || !number(reply[10], ecef.z) || !number(reply[11], accuracy) || accuracy < 0 ||
        accuracy > std::numeric_limits<uint32_t>::max() / 1000.0) {
        _revokeSurvey(context);
        return false;
    }
    const bool fixed = std::holds_alternative<GPSBaseStationConfig::Fixed>(_baseMode);
    const bool matches = fixed
                             ? configuredCount == 0 && observations == 0
                             : configuredCount == std::get<GPSBaseStationConfig::SurveyIn>(_baseMode).duration.count();
    const bool coordinatesKnown = GPSProtocolMath::nearEarthSurface(ecef);
    if (!matches || (!coordinatesKnown && (validity == 2 || (validity == 1 && observations != 0)))) {
        _revokeSurvey(context);
        return false;
    }
    if (fixed && validity == 2 && GPSProtocolMath::distance(ecef, _fixedECEF) > 0.001) {
        _revokeSurvey(context);
        return false;
    }
    // BOOT + identity + saved-role/base readback establish this survey's session.
    // A one-observation survey can finish before any progress notification is sent.
    const bool valid = validity == 2 && (fixed || observations >= configuredCount);
    GPSSurveyReport report{};
    // Fixed mode's MeanAcc=0 describes supplied coordinates, not measured position uncertainty.
    if (!fixed && validity != 0 && observations != 0 && coordinatesKnown) {
        report.meanAccuracyMeters = GPSProtocolMath::roundedToMillimeters(accuracy);
    }
    // Base positioning is fixed at 1 Hz. Gaps do not count as accepted observation seconds.
    report.duration = std::chrono::seconds(observations);
    report.valid = valid;
    report.active = validity == 1 && !valid;
    if (coordinatesKnown && validity != 0) {
        report.position = GPSProtocolMath::fromEcef(ecef);
    }
    report.timestampUs = context.nowUs();
    _survey = report;
    _publishSurvey(context);
    return true;
}

QLatin1StringView signature(const GPSFrame& frame)
{
    const std::string_view text = frame.text();
    const bool checked = text.starts_with("$PQTM") && !QuectelCodec::checkedBody(text).empty();
    return frame.kind == GPSFrameKind::ASCIILine && checked ? QLatin1StringView("$PQTM sentences")
                                                            : QLatin1StringView();
}

/// The firmware query configure() probes each rate with. Any firmware answers, qualified or not.
bool Protocol::probe(GPSCommandChannel& channel)
{
    const auto query = Plan::command(Plan::IDENTIFY, [](std::string_view body) {
        const Fields reply(body);
        return reply.size() == 4 && reply[0] == Plan::IDENTIFY ? GPSCommandOutcome::Acknowledged
                                                               : GPSCommandOutcome::Pending;
    });
    return channel.transact(query).succeeded();
}

}  // namespace

namespace Quectel {

const GPSReceiverFamily FAMILY{
    .type = GPSType::quectel,
    .logCategory = &QuectelProtocolLog,
    .stream = GPSNMEAStream::STREAM,
    .baudCandidates = Plan::BAUD_RATES,
    .create = &gpsCreateProtocol<Protocol>,
    .signature = &signature,
};

}  // namespace Quectel
