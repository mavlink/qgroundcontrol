#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <limits>
#include <optional>
#include <string_view>
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

namespace Quectel {

namespace {

using QuectelCodec::Fields;
using QuectelCodec::number;
using QuectelCodec::readback;
using QuectelCodec::rejected;
using QuectelCodec::REPLY_OK;
namespace Field = QuectelCodec::Field;

/// Whether the LG290P can run the base @a mode: fixed coordinates, or a survey-in within the plan's observation count
/// and accuracy limits. It has no receiver-managed averaging.
bool supportedBase(const GPSBaseStationConfig::Mode& mode)
{
    if (const auto* survey = std::get_if<GPSBaseStationConfig::SurveyIn>(&mode)) {
        return survey->duration <= Plan::MAXIMUM_SURVEY_DURATION &&
               survey->accuracyMeters <= Plan::MAXIMUM_SURVEY_ACCURACY_METERS;
    }
    return std::holds_alternative<GPSBaseStationConfig::Fixed>(mode);
}

/// What a Plan::READ_BASE reply says about the saved base.
struct SavedBase
{
    /// The reply carries every field within its range.
    bool valid = false;
    /// It is the requested base.
    bool matches = false;
    /// It ends with the distance field, which a base write must then carry too.
    bool hasDistance = false;
    /// The Plan::RESTART_SURVEY body that re-runs a matching survey-in; empty otherwise.
    QByteArray surveyRestart;
};

/// Reads @a reply to Plan::READ_BASE against the @a requested base, whose coordinates are @a fixedECEF when fixed.
SavedBase savedBase(const Fields& reply, const GPSBaseStationConfig::Mode& requested,
                    const GPSProtocolMath::Ecef& fixedECEF)
{
    unsigned mode = 0;
    unsigned count = 0;
    double accuracy = 0;
    double distance = 0;
    GPSProtocolMath::Ecef ecef;
    SavedBase saved;
    saved.valid = (reply.size() == Field::BASE_MIN_FIELDS || reply.size() == Field::BASE_MAX_FIELDS) &&
                  reply[Field::NAME] == "PQTMCFGSVIN" && reply[Field::STATUS] == REPLY_OK &&
                  number(reply[Field::BASE_MODE], mode) && mode <= Plan::BaseMode::MAXIMUM &&
                  number(reply[Field::BASE_OBSERVATIONS], count) && number(reply[Field::BASE_ACCURACY], accuracy) &&
                  number(reply[Field::BASE_X], ecef.x) && number(reply[Field::BASE_Y], ecef.y) &&
                  number(reply[Field::BASE_Z], ecef.z) &&
                  (reply.size() == Field::BASE_MIN_FIELDS || number(reply[Field::BASE_DISTANCE], distance)) &&
                  std::chrono::seconds(count) <= Plan::MAXIMUM_SURVEY_DURATION && accuracy >= 0 &&
                  accuracy <= Plan::MAXIMUM_SURVEY_ACCURACY_METERS && distance >= 0 &&
                  distance <= Plan::MAXIMUM_BASE_DISTANCE;
    if (!saved.valid) {
        return saved;
    }
    saved.hasDistance = reply.size() == Field::BASE_MAX_FIELDS;
    if (std::holds_alternative<GPSBaseStationConfig::Fixed>(requested)) {
        saved.matches = mode == Plan::BaseMode::FIXED && count == 0 && accuracy == 0 &&
                        GPSProtocolMath::distance(ecef, fixedECEF) <= Plan::FIXED_BASE_READBACK_TOLERANCE_METERS;
    } else if (const auto* survey = std::get_if<GPSBaseStationConfig::SurveyIn>(&requested)) {
        saved.matches =
            mode == Plan::BaseMode::SURVEY_IN && count == survey->duration.count() &&
            std::abs(accuracy - survey->accuracyMeters) <= Plan::SURVEY_ACCURACY_READBACK_TOLERANCE_METERS &&
            distance == 0;
        if (saved.matches) {
            // Re-execute precisely the read configuration, including otherwise ignored ECEF fields.
            saved.surveyRestart = QByteArrayView(Plan::RESTART_SURVEY).toByteArray();
            for (size_t index = Field::BASE_MODE; index < reply.size(); ++index) {
                saved.surveyRestart += ',';
                saved.surveyRestart.append(QByteArrayView(reply[index]));
            }
        }
    }
    return saved;
}

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
/// with the requested base, an unexpected boot banner or a failed configuration revoke it.
class Protocol final : public GPSNMEAFamilyProtocol
{
public:
    bool configure(GPSCommandChannel& channel, GPSConfig config, unsigned& baud) override;

    bool probe(GPSCommandChannel& channel) override;

    /// The firmware PQTMVERNO reported.
    QString identity() const override { return QString::fromUtf8(_firmware); }

    bool armDecodeOnly(const GPSConfig& config, GPSDecodeContext& context) override
    {
        if (!supportedBase(config.base.mode)) {
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

protected:
    /// Ends readiness after a recorded failure; otherwise revokes stale survey status.
    void _expire(GPSDecodeContext& context) override;
    GPSReceiveUpdates _decodeLine(std::string_view line, GPSDecodeContext& context) override;

private:
    enum class SurveyPhase
    {
        Off,
        AwaitingBoot,
        Verifying,
        Monitoring,
    };

    /// Whether a saved setting read back must be the requested one. When not, any valid setting passes and the caller
    /// compares what was read.
    enum class Match
    {
        Required,
        NotRequired,
    };

    /// Ends readiness and revokes the survey.
    void _stop(GPSDecodeContext& context);
    /// The base the survey status must describe.
    void _setBase(const GPSBaseStationConfig::Mode& mode);

    // The steps of configure(): each returns whether it succeeded.

    /// Labels the command @a label, or its @a body when empty.
    bool _acknowledge(GPSCommandChannel& channel, QByteArrayView body, std::string_view label = {});
    bool _identify(GPSCommandChannel& channel, std::chrono::milliseconds timeout = Plan::COMMAND_TIMEOUT);
    bool _verifyRole(GPSCommandChannel& channel, Match match = Match::Required);
    bool _verifyBase(GPSCommandChannel& channel, Match match = Match::Required);
    bool _changeRole(GPSCommandChannel& channel);
    bool _changeBase(GPSCommandChannel& channel, QByteArrayView body, std::string_view label);
    bool _save(GPSCommandChannel& channel);
    /// Restarts the receiver and reads its saved role back.
    bool _restart(GPSCommandChannel& channel, Match roleMatch = Match::Required);
    bool _restartWithBase(GPSCommandChannel& channel);
    bool _verifySaved(GPSCommandChannel& channel, bool restarted, bool baseVerified);
    /// Records why configure() failed and returns false. @a failure is ConsentRequired when the request needs a
    /// persistent change without consent.
    bool _fail(GPSCommandChannel& channel, const QString& reason,
               GPSProtocolError failure = GPSProtocolError::Protocol);

    /// Publishes survey status from now on, starting with any status held since the verified restart.
    void _monitorSurvey(GPSDecodeContext& context);
    /// The configuration succeeded: the receiver is ready, and a survey status that went stale meanwhile is revoked.
    void _finishAttempt(GPSDecodeContext& context);
    /// Revokes survey status that is stale or belongs to a failed configuration.
    void _expireSurvey(GPSDecodeContext& context);
    bool _handleSurvey(std::string_view body, GPSDecodeContext& context);
    void _revokeSurvey(GPSDecodeContext& context);
    void _publishSurvey(GPSDecodeContext& context);

    GPSBaseStationConfig::Mode _baseMode = GPSBaseStationConfig::SurveyIn{};
    /// Coordinates of a fixed base, in ECEF metres.
    GPSProtocolMath::Ecef _fixedECEF;
    /// The firmware PQTMVERNO reported; the boot banner must name it.
    QByteArray _firmware;
    std::optional<unsigned> _lastTow;
    std::optional<GPSBaseStatus> _survey;
    SurveyPhase _phase = SurveyPhase::Off;
    /// A boot banner is expected; otherwise one ends readiness.
    bool _expectingBoot = false;
    bool _sawBoot = false;
    bool _restartRejected = false;

    // What configure() read back, and whether a flash save may have changed the receiver. The runtime configures each
    // protocol once, so these start from their defaults.

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
    const auto* survey = std::get_if<GPSBaseStationConfig::SurveyIn>(&config.base.mode);
    if (!supportedBase(config.base.mode)) {
        return _fail(
            channel,
            QStringLiteral("Unsupported LG290P configuration: use native 1 Hz observation count (maximum 86400) and "
                           "3D position accuracy threshold (maximum 1000 m), not receiver-managed survey"));
    }
    const auto scope = channel.deadlineScope(Plan::CONFIGURATION_TIMEOUT);
    _setBase(config.base.mode);
    const auto detection = channel.detectBaud(Plan::BAUD_RATES, baud, [this, &channel]() -> GPSBaudProbe {
        _nmea.reset(channel.stream());
        if (_identify(channel)) {
            return GPSBaudProbe::Found;
        }
        // Firmware that answered but is not an LG290P(03) answers the same at every rate.
        return _firmware.isEmpty() ? GPSBaudProbe::TryNext : GPSBaudProbe::Stop;
    });
    if (!detection.found) {
        if (_firmware.isEmpty()) {
            channel.failNoAnswer(detection.probed);
            return _fail(channel, {});
        }
        return _fail(channel,
                     QStringLiteral("No verified LG290P(03) identity; receiver configuration was not changed"));
    }
    baud = detection.baud;
    qCDebug(QuectelProtocolLog).noquote() << "Quectel receiver firmware:" << _firmware;
    if (!_verifyRole(channel, Match::NotRequired)) {
        return _fail(channel, QStringLiteral("LG290P receiver role query failed; no role change was attempted"));
    }

    bool restarted = false;
    if (config.allowPersistentChanges) {
        // Work from saved settings, not another client's uncommitted changes. A role
        // readback alone cannot distinguish a pending role from the active one.
        if (!_restart(channel, Match::NotRequired)) {
            return _fail(
                channel,
                QStringLiteral("LG290P saved role query after restart failed; no persistent change was attempted"));
        }
        restarted = true;
    }
    if (_role != Plan::BASE_ROLE) {
        if (!config.allowPersistentChanges) {
            return _fail(
                channel,
                QStringLiteral("LG290P role mismatch: save the requested base role externally, reboot and reconnect. "
                               "Alternatively, explicitly allow persistent changes for this connection"),
                GPSProtocolError::ConsentRequired);
        }
        if (!_changeRole(channel)) {
            return _fail(channel, QStringLiteral("LG290P role change was rejected or its readback did not match"));
        }
        if (!_save(channel)) {
            return _fail(channel, QStringLiteral("LG290P role save failed; receiver activation is not verified"));
        }
        if (!_restart(channel)) {
            return _fail(channel, QStringLiteral("LG290P saved role could not be verified after restart"));
        }
        restarted = true;
    }

    bool baseChanged = false;
    if (!_verifyBase(channel, Match::NotRequired)) {
        return _fail(channel, QStringLiteral("LG290P base settings query failed; no base change was attempted"));
    }
    if (!_baseMatches && !config.allowPersistentChanges) {
        return _fail(
            channel,
            QStringLiteral("LG290P base settings mismatch: provision/save the requested ECEF coordinates or survey "
                           "observation count/accuracy externally, or explicitly allow persistent changes for this "
                           "connection"),
            GPSProtocolError::ConsentRequired);
    }
    if (!_baseMatches) {
        const QByteArray base = Plan::writeBase(config.base.mode, _fixedECEF, _baseHasDistance);
        if (!_changeBase(channel, base, survey ? std::string_view{} : Plan::FIXED_BASE_LABEL)) {
            return _fail(channel, QStringLiteral("LG290P base change was rejected or its readback did not match"));
        }
        if (!_save(channel)) {
            return _fail(channel, QStringLiteral("LG290P base save failed; receiver activation is not verified"));
        }
        if (!_restartWithBase(channel)) {
            return _fail(channel, QStringLiteral("LG290P saved base settings could not be verified after restart"));
        }
        baseChanged = true;
    }
    if (survey && !baseChanged) {
        qCDebug(QuectelProtocolLog)
            << "LG290P survey uses accepted 1 Hz observations, not wall time; accuracy filters individual 3D fixes. "
               "The receiver itself stores converged coordinates";
        if (!_acknowledge(channel, _surveyRestart, Plan::RESTART_SURVEY_LABEL)) {
            return _fail(channel, QStringLiteral("LG290P rejected restarting the unchanged, externally saved survey"));
        }
        restarted = false;
    }
    if (!_verifySaved(channel, restarted, baseChanged)) {
        return _fail(
            channel,
            QStringLiteral("LG290P restart/readback failed; saved role or base settings do not match the request"));
    }

    _monitorSurvey(channel.context());
    const auto baseOutput = Plan::messageRates(Plan::BASE_OUTPUT);
    if (!channel.runSequence(baseOutput).succeeded()) {
        return _fail(channel, QStringLiteral("LG290P base message output configuration/readback failed"));
    }
    const auto nmeaOutput = Plan::messageRates(Plan::NMEA_OUTPUT);
    if (!channel.runSequence(nmeaOutput).succeeded()) {
        return _fail(channel, QStringLiteral("LG290P NMEA output configuration/readback failed"));
    }
    _finishAttempt(channel.context());
    return true;
}

bool Protocol::_acknowledge(GPSCommandChannel& channel, QByteArrayView body, std::string_view label)
{
    auto command = Plan::command(body, Plan::acknowledgement(body));
    if (!label.empty()) {
        command.step.command = QByteArrayView(label).toByteArray();
    }
    const auto result = channel.transact(command);
    return result.outcome == GPSCommandOutcome::Acknowledged;
}

bool Protocol::_identify(GPSCommandChannel& channel, std::chrono::milliseconds timeout)
{
    const auto result = channel.transact(Plan::command(
        Plan::IDENTIFY,
        [this](std::string_view body) {
            const Fields reply(body);
            if (rejected(reply, Plan::IDENTIFY)) {
                return GPSCommandOutcome::Rejected;
            }
            if (reply.size() != Field::IDENTITY_FIELDS || reply[Field::NAME] != Plan::IDENTIFY) {
                return GPSCommandOutcome::Pending;
            }
            const std::string_view firmware = reply[Field::IDENTITY_FIRMWARE];
            _firmware = QByteArrayView(firmware).toByteArray();
            return firmware.starts_with(Plan::QUALIFIED_FIRMWARE) && firmware.size() > Plan::QUALIFIED_FIRMWARE.size()
                       ? GPSCommandOutcome::ReadbackVerified
                       : GPSCommandOutcome::Rejected;
        },
        timeout));
    return result.outcome == GPSCommandOutcome::ReadbackVerified;
}

bool Protocol::_verifyRole(GPSCommandChannel& channel, Match match)
{
    const auto result = channel.transact(Plan::command(Plan::READ_ROLE, [this, match](std::string_view body) {
        const Fields reply(body);
        unsigned role = 0;
        const bool valid = reply.size() == Field::ROLE_FIELDS && reply[Field::NAME] == "PQTMCFGRCVRMODE" &&
                           reply[Field::STATUS] == REPLY_OK && number(reply[Field::ROLE], role) &&
                           role <= Plan::MAXIMUM_ROLE;
        if (valid) {
            _role = role;
        }
        return readback(reply, "PQTMCFGRCVRMODE", valid && (match == Match::NotRequired || role == Plan::BASE_ROLE));
    }));
    return result.outcome == GPSCommandOutcome::ReadbackVerified;
}

bool Protocol::_verifyBase(GPSCommandChannel& channel, Match match)
{
    _baseMatches = false;
    const auto rate = channel.transact(Plan::command(Plan::READ_FIX_RATE, [](std::string_view body) {
        const Fields reply(body);
        unsigned interval = 0;
        return readback(reply, "PQTMCFGFIXRATE",
                        reply.size() == Field::FIX_RATE_FIELDS && number(reply[Field::FIX_INTERVAL], interval) &&
                            interval == Plan::FIX_INTERVAL_MS);
    }));
    if (rate.outcome != GPSCommandOutcome::ReadbackVerified) {
        return false;
    }
    // Every line is offered until one completes the command, so the last reading is that of the reply, if any.
    SavedBase saved;
    const auto base = channel.transact(Plan::command(Plan::READ_BASE, [this, match, &saved](std::string_view body) {
        const Fields reply(body);
        saved = savedBase(reply, _baseMode, _fixedECEF);
        return readback(reply, "PQTMCFGSVIN", match == Match::Required ? saved.matches : saved.valid);
    }));
    _baseMatches = saved.matches;
    if (saved.valid) {
        _baseHasDistance = saved.hasDistance;
    }
    if (!saved.surveyRestart.isEmpty()) {
        _surveyRestart = saved.surveyRestart;
    }
    return base.outcome == GPSCommandOutcome::ReadbackVerified;
}

bool Protocol::_changeRole(GPSCommandChannel& channel)
{
    if (!_acknowledge(channel, QByteArrayView(Plan::WRITE_BASE_ROLE))) {
        return false;
    }
    return _verifyRole(channel);
}

bool Protocol::_changeBase(GPSCommandChannel& channel, QByteArrayView body, std::string_view label)
{
    if (!_acknowledge(channel, body, label)) {
        return false;
    }
    return _verifyBase(channel);
}

bool Protocol::_restartWithBase(GPSCommandChannel& channel)
{
    if (!_restart(channel)) {
        return false;
    }
    return _verifyBase(channel);
}

bool Protocol::_verifySaved(GPSCommandChannel& channel, bool restarted, bool baseVerified)
{
    // A role readback can reflect an unsaved, not-yet-active change. Reboot and read back
    // the saved role before claiming that the receiver actually operates in that role.
    if (!restarted) {
        if (!_restart(channel)) {
            return false;
        }
    }
    if (baseVerified) {
        return true;
    }
    return _verifyBase(channel);
}

bool Protocol::_save(GPSCommandChannel& channel)
{
    const auto result =
        channel.transact(Plan::command(Plan::SAVE, Plan::acknowledgement(Plan::SAVE), Plan::SAVE_TIMEOUT));
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

bool Protocol::_restart(GPSCommandChannel& channel, Match roleMatch)
{
    // The survey is revoked until the restart is verified.
    _revokeSurvey(channel.context());
    _phase = SurveyPhase::AwaitingBoot;
    qCDebug(QuectelProtocolLog) << "Restarting LG290P and verifying its saved configuration";
    const auto scope = channel.deadlineScope(Plan::RESTART_TIMEOUT);
    const QByteArray restart = NMEAUtils::frame(Plan::RESTART);
    if (!channel.writeCommand({QByteArrayView(Plan::RESTART).toByteArray(), Plan::RESTART_TIMEOUT}, restart)) {
        return false;
    }
    // PQTMSRR has no documented ACK. Do not inflate successful transport completion to acknowledgment.
    channel.finishCommand(GPSCommandOutcome::Written);
    // Restart the stream and watch for the boot banner or a PQTMSRR rejection.
    _nmea.reset(channel.stream());
    _expectingBoot = true;
    _sawBoot = false;
    _restartRejected = false;
    do {
        channel.wait(Plan::RESTART_POLL);
        if (channel.failed()) {
            break;
        }
        const bool identified = _identify(channel, Plan::RESTART_IDENTIFY_TIMEOUT);
        if (_restartRejected) {
            channel.setErrorDetail(QStringLiteral("LG290P rejected PQTMSRR."));
            break;
        }
        if (identified && _sawBoot) {
            _expectingBoot = false;
            return _verifyRole(channel, roleMatch);
        }
    } while (!channel.failed() && channel.nowUs() < channel.operationDeadline().untilUs);
    // The restart was abandoned: a boot banner from now on ends readiness.
    _expectingBoot = false;
    return false;
}

bool Protocol::_fail(GPSCommandChannel& channel, const QString& reason, GPSProtocolError failure)
{
    _stop(channel.context());
    (void) channel.flush();
    QString detail = reason;
    if (!channel.errorDetail().isEmpty()) {
        detail += (detail.isEmpty() ? QString() : QStringLiteral(" ")) + channel.errorDetail();
    }
    channel.failControl(failure);
    if (_saveAcknowledged || _saveUncertain) {
        const QString persistence =
            _saveAcknowledged ? QStringLiteral("An LG290P flash save was acknowledged. ") : QString();
        const QString uncertain =
            _saveUncertain ? QStringLiteral("The latest flash save may have taken effect. ") : QString();
        channel.setErrorDetail(persistence + uncertain + detail +
                               QStringLiteral(" Receiver settings may have changed; no rollback was attempted."));
    } else if (channel.error() != GPSProtocolError::Cancelled) {
        channel.setErrorDetail(detail);
    }
    return false;
}

void Protocol::_stop(GPSDecodeContext& context)
{
    _ready = false;
    _phase = SurveyPhase::Off;
    _revokeSurvey(context);
}

void Protocol::_setBase(const GPSBaseStationConfig::Mode& mode)
{
    _baseMode = mode;
    if (const auto* fixed = std::get_if<GPSBaseStationConfig::Fixed>(&mode)) {
        _fixedECEF = GPSProtocolMath::toEcef(fixed->position);
    }
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
        if (reply.size() == Field::BOOT_FIELDS && reply[Field::BOOT_VERSION] == Plan::BOOT_BANNER_VERSION &&
            reply[Field::BOOT_TYPE] == Plan::BOOT_BANNER_MODULE && reply[Field::BOOT_FIRMWARE] == firmware) {
            // §2.3.1: this is the first output upon each successful startup.
            if (_expectingBoot && !_sawBoot) {
                _sawBoot = true;
                if (_phase == SurveyPhase::AwaitingBoot) {
                    _phase = SurveyPhase::Verifying;
                }
            } else if (!_expectingBoot && (_ready || _phase != SurveyPhase::Off)) {
                _ready = false;
                _phase = SurveyPhase::Off;
                _survey.reset();
                context.loseBase(QStringLiteral("LG290P restarted; its base configuration is no longer verified"));
            }
            return GPSReceiveUpdate::Activity;
        }
    }
    return {};
}

void Protocol::_revokeSurvey(GPSDecodeContext& context)
{
    if (_survey) {
        _survey.reset();
        context.publishBaseStatus(GPSBaseState::Inactive, {});
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
    }
}

bool Protocol::_handleSurvey(std::string_view body, GPSDecodeContext& context)
{
    namespace Validity = Plan::SurveyValidity;
    const Fields reply(body);
    unsigned version = 0;
    unsigned tow = 0;
    unsigned validity = 0;
    unsigned observations = 0;
    unsigned configuredCount = 0;
    double accuracy = 0;
    GPSProtocolMath::Ecef ecef;
    if (reply.size() != Field::SURVEY_FIELDS || !number(reply[Field::SURVEY_VERSION], version) ||
        version != Plan::SURVEY_STATUS_VERSION || !number(reply[Field::SURVEY_TIME_OF_WEEK], tow) ||
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
    if (!number(reply[Field::SURVEY_VALIDITY], validity) || validity > Validity::VALID ||
        !reply[Field::SURVEY_RESERVED].empty() || !number(reply[Field::SURVEY_OBSERVATIONS], observations) ||
        std::chrono::seconds(observations) > Plan::MAXIMUM_SURVEY_DURATION ||
        !number(reply[Field::SURVEY_CONFIGURED_OBSERVATIONS], configuredCount) ||
        std::chrono::seconds(configuredCount) > Plan::MAXIMUM_SURVEY_DURATION ||
        !number(reply[Field::SURVEY_X], ecef.x) || !number(reply[Field::SURVEY_Y], ecef.y) ||
        !number(reply[Field::SURVEY_Z], ecef.z) || !number(reply[Field::SURVEY_ACCURACY], accuracy) || accuracy < 0 ||
        accuracy > (std::numeric_limits<uint32_t>::max)() / 1000.0) {
        _revokeSurvey(context);
        return false;
    }
    const bool fixed = std::holds_alternative<GPSBaseStationConfig::Fixed>(_baseMode);
    const auto* survey = std::get_if<GPSBaseStationConfig::SurveyIn>(&_baseMode);
    const bool matches =
        survey ? configuredCount == survey->duration.count() : configuredCount == 0 && observations == 0;
    const bool coordinatesKnown = GPSProtocolMath::nearEarthSurface(ecef);
    if (!matches || (!coordinatesKnown &&
                     (validity == Validity::VALID || (validity == Validity::IN_PROGRESS && observations != 0)))) {
        _revokeSurvey(context);
        return false;
    }
    if (fixed && validity == Validity::VALID &&
        GPSProtocolMath::distance(ecef, _fixedECEF) > Plan::FIXED_BASE_STATUS_TOLERANCE_METERS) {
        _revokeSurvey(context);
        return false;
    }
    // BOOT + identity + saved-role/base readback establish this survey's session.
    // A one-observation survey can finish before any progress notification is sent.
    const bool valid = validity == Validity::VALID && (fixed || observations >= configuredCount);
    GPSBaseStatus report{};
    // Fixed mode's MeanAcc=0 describes supplied coordinates, not measured position uncertainty.
    if (!fixed && validity != Validity::INVALID && observations != 0 && coordinatesKnown) {
        report.meanAccuracyMeters = GPSProtocolMath::roundedToMillimeters(accuracy);
    }
    // Base positioning is fixed at 1 Hz. Gaps do not count as accepted observation seconds.
    report.duration = std::chrono::seconds(observations);
    report.state = valid                               ? GPSBaseState::Valid
                   : validity == Validity::IN_PROGRESS ? GPSBaseState::Active
                                                       : GPSBaseState::Inactive;
    if (coordinatesKnown && validity != Validity::INVALID) {
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
        return reply.size() == Field::IDENTITY_FIELDS && reply[Field::NAME] == Plan::IDENTIFY
                   ? GPSCommandOutcome::Acknowledged
                   : GPSCommandOutcome::Pending;
    });
    return channel.transact(query).succeeded();
}

}  // namespace

const GPSReceiverFamily FAMILY{
    .type = GPSType::Quectel,
    .logCategory = &QuectelProtocolLog,
    .stream = GPSNMEAStream::STREAM,
    .baudCandidates = Plan::BAUD_RATES,
    .create = &gpsCreateProtocol<Protocol>,
    .signature = &signature,
};

}  // namespace Quectel
