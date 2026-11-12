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

/// The base settings a Plan::READ_BASE reply reports.
struct BaseSettings
{
    unsigned mode = 0;
    unsigned observations = 0;
    double accuracy = 0;
    GPSProtocolMath::Ecef position;
    /// Zero in a reply without the distance field.
    double distance = 0;
};

/// Reads a Plan::READ_BASE reply; nothing unless it carries every field within its range.
std::optional<BaseSettings> baseSettings(const Fields& reply)
{
    BaseSettings base;
    const bool hasDistance = reply.size() == Field::BASE_MAX_FIELDS;
    const bool read = (reply.size() == Field::BASE_MIN_FIELDS || hasDistance) && reply[Field::NAME] == "PQTMCFGSVIN" &&
                      reply[Field::STATUS] == REPLY_OK && number(reply[Field::BASE_MODE], base.mode) &&
                      number(reply[Field::BASE_OBSERVATIONS], base.observations) &&
                      number(reply[Field::BASE_ACCURACY], base.accuracy) &&
                      number(reply[Field::BASE_X], base.position.x) && number(reply[Field::BASE_Y], base.position.y) &&
                      number(reply[Field::BASE_Z], base.position.z) &&
                      (!hasDistance || number(reply[Field::BASE_DISTANCE], base.distance));
    const bool inRange = base.mode <= Plan::BaseMode::MAXIMUM &&
                         std::chrono::seconds(base.observations) <= Plan::MAXIMUM_SURVEY_DURATION &&
                         base.accuracy >= 0 && base.accuracy <= Plan::MAXIMUM_SURVEY_ACCURACY_METERS &&
                         base.distance >= 0 && base.distance <= Plan::MAXIMUM_BASE_DISTANCE;
    return read && inRange ? std::optional(base) : std::nullopt;
}

/// Whether @a saved is the @a requested base, whose coordinates are @a fixedECEF when fixed.
bool isRequested(const BaseSettings& saved, const GPSBaseStationConfig::Mode& requested,
                 const GPSProtocolMath::Ecef& fixedECEF)
{
    if (std::holds_alternative<GPSBaseStationConfig::Fixed>(requested)) {
        return saved.mode == Plan::BaseMode::FIXED && saved.observations == 0 && saved.accuracy == 0 &&
               GPSProtocolMath::distance(saved.position, fixedECEF) <= Plan::FIXED_BASE_READBACK_TOLERANCE_METERS;
    }
    const auto* survey = std::get_if<GPSBaseStationConfig::SurveyIn>(&requested);
    return survey && saved.mode == Plan::BaseMode::SURVEY_IN && saved.observations == survey->duration.count() &&
           std::abs(saved.accuracy - survey->accuracyMeters) <= Plan::SURVEY_ACCURACY_READBACK_TOLERANCE_METERS &&
           saved.distance == 0;
}

/// The Plan::RESTART_SURVEY body that runs the survey-in of @a reply again: precisely the read configuration,
/// including otherwise ignored ECEF fields.
QByteArray surveyRestart(const Fields& reply)
{
    QByteArray body = QByteArrayView(Plan::RESTART_SURVEY).toByteArray();
    for (size_t index = Field::BASE_MODE; index < reply.size(); ++index) {
        body += ',';
        body.append(QByteArrayView(reply[index]));
    }
    return body;
}

/// Reads @a reply to Plan::READ_BASE against the @a requested base, whose coordinates are @a fixedECEF when fixed.
SavedBase savedBase(const Fields& reply, const GPSBaseStationConfig::Mode& requested,
                    const GPSProtocolMath::Ecef& fixedECEF)
{
    SavedBase saved;
    const auto settings = baseSettings(reply);
    if (!settings) {
        return saved;
    }
    saved.valid = true;
    saved.hasDistance = reply.size() == Field::BASE_MAX_FIELDS;
    saved.matches = isRequested(*settings, requested, fixedECEF);
    if (saved.matches && std::holds_alternative<GPSBaseStationConfig::SurveyIn>(requested)) {
        saved.surveyRestart = surveyRestart(reply);
    }
    return saved;
}

/// What a PQTMSVINSTATUS sentence reports.
struct SurveyStatus
{
    unsigned tow = 0;
    /// Plan::SurveyValidity; INVALID also when the field is not a number.
    unsigned validity = Plan::SurveyValidity::INVALID;
    unsigned observations = 0;
    unsigned configuredObservations = 0;
    GPSProtocolMath::Ecef position;
    double accuracy = 0;
    /// Every field after the time of week is well formed and within its range.
    bool wellFormed = false;
};

/// Reads a PQTMSVINSTATUS of the version the plan asks for; nothing unless it has its fields and a time of week.
std::optional<SurveyStatus> surveyStatus(const Fields& reply)
{
    SurveyStatus status;
    unsigned version = 0;
    if (reply.size() != Field::SURVEY_FIELDS || !number(reply[Field::SURVEY_VERSION], version) ||
        version != Plan::SURVEY_STATUS_VERSION || !number(reply[Field::SURVEY_TIME_OF_WEEK], status.tow) ||
        status.tow >= GPSTime::WEEK_MS) {
        return std::nullopt;
    }
    status.wellFormed =
        number(reply[Field::SURVEY_VALIDITY], status.validity) && status.validity <= Plan::SurveyValidity::VALID &&
        reply[Field::SURVEY_RESERVED].empty() && number(reply[Field::SURVEY_OBSERVATIONS], status.observations) &&
        std::chrono::seconds(status.observations) <= Plan::MAXIMUM_SURVEY_DURATION &&
        number(reply[Field::SURVEY_CONFIGURED_OBSERVATIONS], status.configuredObservations) &&
        std::chrono::seconds(status.configuredObservations) <= Plan::MAXIMUM_SURVEY_DURATION &&
        number(reply[Field::SURVEY_X], status.position.x) && number(reply[Field::SURVEY_Y], status.position.y) &&
        number(reply[Field::SURVEY_Z], status.position.z) && number(reply[Field::SURVEY_ACCURACY], status.accuracy) &&
        status.accuracy >= 0 && status.accuracy <= (std::numeric_limits<uint32_t>::max)() / 1000.0;
    return status;
}

/// The base status a well-formed @a status reports for the @a requested base, whose coordinates are @a fixedECEF when
/// fixed, without its receipt time; nothing when it describes another base or its position cannot be the base's.
std::optional<GPSBaseStatus> baseStatus(const SurveyStatus& status, const GPSBaseStationConfig::Mode& requested,
                                        const GPSProtocolMath::Ecef& fixedECEF)
{
    namespace Validity = Plan::SurveyValidity;
    const bool fixed = std::holds_alternative<GPSBaseStationConfig::Fixed>(requested);
    const auto* survey = std::get_if<GPSBaseStationConfig::SurveyIn>(&requested);
    const bool matches = survey ? status.configuredObservations == survey->duration.count()
                                : status.configuredObservations == 0 && status.observations == 0;
    const bool coordinatesKnown = GPSProtocolMath::nearEarthSurface(status.position);
    if (!matches || (!coordinatesKnown && (status.validity == Validity::VALID ||
                                           (status.validity == Validity::IN_PROGRESS && status.observations != 0)))) {
        return std::nullopt;
    }
    if (fixed && status.validity == Validity::VALID &&
        GPSProtocolMath::distance(status.position, fixedECEF) > Plan::FIXED_BASE_STATUS_TOLERANCE_METERS) {
        return std::nullopt;
    }
    // A one-observation survey can finish before any progress notification is sent.
    const bool valid =
        status.validity == Validity::VALID && (fixed || status.observations >= status.configuredObservations);
    GPSBaseStatus report{};
    // Fixed mode's MeanAcc=0 describes supplied coordinates, not measured position uncertainty.
    if (!fixed && status.validity != Validity::INVALID && status.observations != 0 && coordinatesKnown) {
        report.meanAccuracyMeters = GPSProtocolMath::roundedToMillimeters(status.accuracy);
    }
    // Base positioning is fixed at 1 Hz. Gaps do not count as accepted observation seconds.
    report.duration = std::chrono::seconds(status.observations);
    report.state = valid                                      ? GPSBaseState::Valid
                   : status.validity == Validity::IN_PROGRESS ? GPSBaseState::Active
                                                              : GPSBaseState::Inactive;
    if (coordinatesKnown && status.validity != Validity::INVALID) {
        report.position = GPSProtocolMath::fromEcef(status.position);
    }
    return report;
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

    // The stages of configure(), in order: each records why it failed and returns whether it succeeded.

    /// Finds the receiver at one of Plan::BAUD_RATES, or at @a baud when nonzero, and reads its role.
    bool _findReceiver(GPSCommandChannel& channel, unsigned& baud);
    /// Leaves the receiver in the base role from its saved settings. With @a allowPersistentChanges it restarts first,
    /// and writes, saves and restarts into the role when another one is saved.
    bool _establishRole(GPSCommandChannel& channel, bool allowPersistentChanges);
    /// Leaves the receiver running the requested base from its saved settings, verified after a restart. With
    /// @a allowPersistentChanges it writes, saves and restarts into a base that differs; an unchanged survey is run
    /// again.
    bool _establishBase(GPSCommandChannel& channel, bool allowPersistentChanges);
    /// Starts monitoring the survey and sets the base and NMEA output.
    bool _startOutput(GPSCommandChannel& channel);

    // The steps of the stages: each returns whether it succeeded.

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
    /// The time of week of the latest status that reported a survey since the receiver last started, which later
    /// status must follow.
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
    /// The receiver restarted and nothing was written to it since, so it runs the settings it has saved.
    bool _runsSavedSettings = false;
    bool _baseMatches = false;
    bool _baseHasDistance = false;
    bool _saveAcknowledged = false;
    bool _saveUncertain = false;
};

bool Protocol::configure(GPSCommandChannel& channel, GPSConfig config, unsigned& baud)
{
    if (!supportedBase(config.base.mode)) {
        return _fail(
            channel,
            QStringLiteral("Unsupported LG290P configuration: use native 1 Hz observation count (maximum 86400) and "
                           "3D position accuracy threshold (maximum 1000 m), not receiver-managed survey"));
    }
    const auto scope = channel.deadlineScope(Plan::CONFIGURATION_TIMEOUT);
    _setBase(config.base.mode);
    if (!_findReceiver(channel, baud) || !_establishRole(channel, config.allowPersistentChanges) ||
        !_establishBase(channel, config.allowPersistentChanges) || !_startOutput(channel)) {
        return false;
    }
    _finishAttempt(channel.context());
    return true;
}

bool Protocol::_findReceiver(GPSCommandChannel& channel, unsigned& baud)
{
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
    return true;
}

bool Protocol::_establishRole(GPSCommandChannel& channel, bool allowPersistentChanges)
{
    if (allowPersistentChanges) {
        // Work from saved settings, not another client's uncommitted changes. A role
        // readback alone cannot distinguish a pending role from the active one.
        if (!_restart(channel, Match::NotRequired)) {
            return _fail(
                channel,
                QStringLiteral("LG290P saved role query after restart failed; no persistent change was attempted"));
        }
        _runsSavedSettings = true;
    }
    if (_role == Plan::BASE_ROLE) {
        return true;
    }
    if (!allowPersistentChanges) {
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
    _runsSavedSettings = true;
    return true;
}

bool Protocol::_establishBase(GPSCommandChannel& channel, bool allowPersistentChanges)
{
    const bool survey = std::holds_alternative<GPSBaseStationConfig::SurveyIn>(_baseMode);
    if (!_verifyBase(channel, Match::NotRequired)) {
        return _fail(channel, QStringLiteral("LG290P base settings query failed; no base change was attempted"));
    }
    if (!_baseMatches && !allowPersistentChanges) {
        return _fail(
            channel,
            QStringLiteral("LG290P base settings mismatch: provision/save the requested ECEF coordinates or survey "
                           "observation count/accuracy externally, or explicitly allow persistent changes for this "
                           "connection"),
            GPSProtocolError::ConsentRequired);
    }
    bool baseChanged = false;
    if (!_baseMatches) {
        const QByteArray base = Plan::writeBase(_baseMode, _fixedECEF, _baseHasDistance);
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
        _runsSavedSettings = false;
    }
    if (!_verifySaved(channel, _runsSavedSettings, baseChanged)) {
        return _fail(
            channel,
            QStringLiteral("LG290P restart/readback failed; saved role or base settings do not match the request"));
    }
    return true;
}

bool Protocol::_startOutput(GPSCommandChannel& channel)
{
    _monitorSurvey(channel.context());
    const auto baseOutput = Plan::messageRates(Plan::BASE_OUTPUT);
    if (!channel.runSequence(baseOutput).succeeded()) {
        return _fail(channel, QStringLiteral("LG290P base message output configuration/readback failed"));
    }
    const auto nmeaOutput = Plan::messageRates(Plan::NMEA_OUTPUT);
    if (!channel.runSequence(nmeaOutput).succeeded()) {
        return _fail(channel, QStringLiteral("LG290P NMEA output configuration/readback failed"));
    }
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
                // The receiver's time of week starts again with it.
                _lastTow.reset();
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
    const auto status = surveyStatus(Fields(body));
    // TOW is a modular measurement clock, not a receipt timestamp. A late packet from before Sunday's rollover must
    // not look newer than the current week.
    if (!status || (_lastTow && !GPSTime::towAdvances(status->tow, *_lastTow))) {
        return false;
    }
    // Only a status that reports a survey orders the ones after it: without one the receiver may not have its time
    // yet, as after a restart, and that time of week would lock out the real one.
    if (status->validity != Plan::SurveyValidity::INVALID) {
        _lastTow = status->tow;
    }
    if (_phase != SurveyPhase::Verifying && _phase != SurveyPhase::Monitoring) {
        return false;
    }
    // BOOT + identity + saved-role/base readback establish this survey's session.
    auto report = status->wellFormed ? baseStatus(*status, _baseMode, _fixedECEF) : std::nullopt;
    if (!report) {
        _revokeSurvey(context);
        return false;
    }
    report->timestampUs = context.nowUs();
    _survey = *report;
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
