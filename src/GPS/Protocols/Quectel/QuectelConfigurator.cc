#include "Quectel/QuectelConfigurator.h"

#include <cmath>
#include <string>
#include <string_view>
#include <utility>
#include <variant>

#include <QtCore/QByteArray>
#include <QtCore/QByteArrayView>
#include <QtCore/QString>

#include "GPSCommandChannel.h"
#include "GPSDeadline.h"
#include "Quectel/QuectelCodec_p.h"
#include "Quectel/QuectelDecoder.h"
#include "Quectel/QuectelPlan.h"

namespace Quectel {

namespace {

using QuectelCodec::Fields;
using QuectelCodec::number;
using QuectelCodec::readback;
using QuectelCodec::rejected;

/// One configuration attempt: what the readbacks reported and whether a flash save may have changed the receiver.
class Session
{
public:
    Session(GPSCommandChannel& channel, Decoder& decoder)
        : _channel(channel)
        , _decoder(decoder)
    {}

    GPSTask<bool> run(GPSConfig config, unsigned& baud);

private:
    GPSTask<GPSCommandResult> _transact(GPSCommandSequence::Command command);
    /// Labels the command @a label, or its @a body when empty.
    GPSTask<bool> _acknowledge(QByteArrayView body, std::string_view label = {});
    GPSTask<bool> _identify(std::chrono::milliseconds timeout = Plan::COMMAND_TIMEOUT);
    GPSTask<bool> _verifyRole(bool requireMatch = true);
    GPSTask<bool> _verifyBase(bool requireMatch = true);
    GPSTask<bool> _changeRole();
    GPSTask<bool> _changeBase(QByteArrayView body, std::string_view label);
    GPSTask<bool> _save();
    GPSTask<bool> _restart(bool requireRoleMatch = true);
    GPSTask<bool> _restartWithBase();
    GPSTask<bool> _verifySaved(bool restarted, bool baseVerified);
    /// @a failure is ConsentRequired when the request needs a persistent change without consent.
    bool _fail(const QString& reason, GPSProtocolError failure = GPSProtocolError::Protocol);

    GPSCommandChannel& _channel;
    Decoder& _decoder;
    /// Re-runs the unchanged survey READ_BASE reported.
    QByteArray _surveyRestart;
    unsigned _role = 0;
    bool _baseMatches = false;
    bool _baseHasDistance = false;
    bool _saveAcknowledged = false;
    bool _saveUncertain = false;
};

GPSTask<bool> Session::run(GPSConfig config, unsigned& baud)
{
    _decoder.endSession(_channel.context());
    (void) _channel.flush();
    _decoder.startSession(_channel.stream());
    const auto* survey = std::get_if<GPSBaseStationConfig::SurveyIn>(&config.base.mode);
    if (!_channel.validateConfiguration(config) ||
        (survey && (survey->duration > Plan::MAXIMUM_SURVEY_DURATION ||
                    survey->accuracyMeters > Plan::MAXIMUM_SURVEY_ACCURACY_METERS))) {
        co_return _fail(
            QStringLiteral("Unsupported LG290P configuration: use native 1 Hz observation count (maximum 86400) and "
                           "3D position accuracy threshold (maximum 1000 m), not receiver-managed survey"));
    }
    const auto scope = _channel.deadlineScope(Plan::CONFIGURATION_TIMEOUT);
    _decoder.setBase(config.base.mode);
    const auto detection =
        co_await _channel.detectBaud(Plan::BAUD_RATES, baud, [this](unsigned) -> GPSTask<GPSBaudProbe> {
            _decoder.resetStream(_channel.stream());
            if (co_await _identify()) {
                co_return GPSBaudProbe::Found;
            }
            // Firmware that answered but is not an LG290P(03) answers the same at every rate.
            co_return _decoder.firmware().isEmpty() ? GPSBaudProbe::TryNext : GPSBaudProbe::Stop;
        });
    if (detection.linkFailed) {
        co_return _fail(QStringLiteral("Cannot configure LG290P host serial speed"));
    }
    if (!detection.found) {
        co_return _fail(QStringLiteral("No verified LG290P(03) identity; receiver configuration was not changed"));
    }
    baud = detection.baud;
    qCDebug(QuectelProtocolLog).noquote() << "Quectel receiver firmware:" << _decoder.firmware();
    if (!co_await _verifyRole(false)) {
        co_return _fail(QStringLiteral("LG290P receiver role query failed; no role change was attempted"));
    }

    bool restarted = false;
    if (config.allowPersistentChanges) {
        // Work from saved settings, not another client's uncommitted changes. A role
        // readback alone cannot distinguish a pending role from the active one.
        if (!co_await _restart(false)) {
            co_return _fail(
                QStringLiteral("LG290P saved role query after restart failed; no persistent change was attempted"));
        }
        restarted = true;
    }
    if (_role != Plan::BASE_ROLE) {
        if (!config.allowPersistentChanges) {
            co_return _fail(
                QStringLiteral("LG290P role mismatch: save the requested base role externally, reboot and reconnect. "
                               "Alternatively, explicitly allow persistent changes for this connection"),
                GPSProtocolError::ConsentRequired);
        }
        if (!co_await _changeRole()) {
            co_return _fail(QStringLiteral("LG290P role change was rejected or its readback did not match"));
        }
        if (!co_await _save()) {
            co_return _fail(QStringLiteral("LG290P role save failed; receiver activation is not verified"));
        }
        if (!co_await _restart()) {
            co_return _fail(QStringLiteral("LG290P saved role could not be verified after restart"));
        }
        restarted = true;
    }

    bool baseChanged = false;
    if (!co_await _verifyBase(false)) {
        co_return _fail(QStringLiteral("LG290P base settings query failed; no base change was attempted"));
    }
    if (!_baseMatches && !config.allowPersistentChanges) {
        co_return _fail(
            QStringLiteral("LG290P base settings mismatch: provision/save the requested ECEF coordinates or survey "
                           "observation count/accuracy externally, or explicitly allow persistent changes for this "
                           "connection"),
            GPSProtocolError::ConsentRequired);
    }
    if (!_baseMatches) {
        const QByteArray base = Plan::writeBase(config.base.mode, _decoder.fixedPosition(), _baseHasDistance);
        if (!co_await _changeBase(base, survey ? std::string_view{} : Plan::FIXED_BASE_LABEL)) {
            co_return _fail(QStringLiteral("LG290P base change was rejected or its readback did not match"));
        }
        if (!co_await _save()) {
            co_return _fail(QStringLiteral("LG290P base save failed; receiver activation is not verified"));
        }
        if (!co_await _restartWithBase()) {
            co_return _fail(QStringLiteral("LG290P saved base settings could not be verified after restart"));
        }
        baseChanged = true;
    }
    if (survey && !baseChanged) {
        qCDebug(QuectelProtocolLog)
            << "LG290P survey uses accepted 1 Hz observations, not wall time; accuracy filters individual 3D fixes. "
               "The receiver itself stores converged coordinates";
        if (!co_await _acknowledge(_surveyRestart, Plan::RESTART_SURVEY_LABEL)) {
            co_return _fail(QStringLiteral("LG290P rejected restarting the unchanged, externally saved survey"));
        }
        restarted = false;
    }
    if (!co_await _verifySaved(restarted, baseChanged)) {
        co_return _fail(
            QStringLiteral("LG290P restart/readback failed; saved role or base settings do not match the request"));
    }

    _decoder.monitorSurvey(_channel.context());
    const auto baseOutput = Plan::messageRates(Plan::BASE_OUTPUT);
    if (!(co_await _channel.runSequence(baseOutput)).succeeded()) {
        co_return _fail(QStringLiteral("LG290P base message output configuration/readback failed"));
    }
    const auto nmeaOutput = Plan::messageRates(Plan::NMEA_OUTPUT);
    if (!(co_await _channel.runSequence(nmeaOutput)).succeeded()) {
        co_return _fail(QStringLiteral("LG290P NMEA output configuration/readback failed"));
    }
    _decoder.finishSession(_channel.context());
    co_return true;
}

GPSTask<GPSCommandResult> Session::_transact(GPSCommandSequence::Command command)
{
    co_return co_await _channel.transact(command.step, command.wire,
                                         std::get<GPSTextMatcher>(std::move(command.reply)));
}

GPSTask<bool> Session::_acknowledge(QByteArrayView body, std::string_view label)
{
    auto command = Plan::command(body, Plan::acknowledgement(body));
    if (!label.empty()) {
        command.step.command = std::string(label);
    }
    const auto result = co_await _transact(std::move(command));
    co_return result.evidence.outcome == GPSCommandOutcome::Acknowledged;
}

GPSTask<bool> Session::_identify(std::chrono::milliseconds timeout)
{
    const auto result = co_await _transact(Plan::command(
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
            _decoder.setFirmware(QByteArray(firmware.data(), static_cast<qsizetype>(firmware.size())));
            return firmware.starts_with(Plan::QUALIFIED_FIRMWARE) && firmware.size() > Plan::QUALIFIED_FIRMWARE.size()
                       ? GPSCommandOutcome::ReadbackVerified
                       : GPSCommandOutcome::Rejected;
        },
        timeout));
    co_return result.evidence.outcome == GPSCommandOutcome::ReadbackVerified;
}

GPSTask<bool> Session::_verifyRole(bool requireMatch)
{
    const auto result = co_await _transact(Plan::command(Plan::READ_ROLE, [this, requireMatch](std::string_view body) {
        const Fields reply(body);
        unsigned role = 0;
        const bool valid = reply.size() == 3 && reply[0] == "PQTMCFGRCVRMODE" && reply[1] == "OK" &&
                           number(reply[2], role) && role <= 2;
        if (valid) {
            _role = role;
        }
        return readback(reply, "PQTMCFGRCVRMODE", valid && (!requireMatch || role == Plan::BASE_ROLE));
    }));
    co_return result.evidence.outcome == GPSCommandOutcome::ReadbackVerified;
}

GPSTask<bool> Session::_verifyBase(bool requireMatch)
{
    _baseMatches = false;
    const auto rate = co_await _transact(Plan::command(Plan::READ_FIX_RATE, [](std::string_view body) {
        const Fields reply(body);
        unsigned interval = 0;
        return readback(reply, "PQTMCFGFIXRATE",
                        reply.size() == 3 && number(reply[2], interval) && interval == Plan::FIX_INTERVAL_MS);
    }));
    if (rate.evidence.outcome != GPSCommandOutcome::ReadbackVerified) {
        co_return false;
    }
    const auto base = co_await _transact(Plan::command(Plan::READ_BASE, [this, requireMatch](std::string_view body) {
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
                           count <= 86400 && accuracy >= 0 && accuracy <= 1000 && distance >= 0 && distance <= 10;
        bool matches = valid;
        if (valid) {
            _baseHasDistance = reply.size() == 9;
        }
        const auto& requested = _decoder.baseMode();
        if (matches && std::holds_alternative<GPSBaseStationConfig::Fixed>(requested)) {
            const auto& fixed = _decoder.fixedPosition();
            matches = mode == 2 && count == 0 && accuracy == 0 && std::abs(ecef.x - fixed.x) <= 0.00011 &&
                      std::abs(ecef.y - fixed.y) <= 0.00011 && std::abs(ecef.z - fixed.z) <= 0.00011;
        } else if (matches) {
            const auto& survey = std::get<GPSBaseStationConfig::SurveyIn>(requested);
            matches = mode == 1 && count == survey.duration.count() &&
                      std::abs(accuracy - survey.accuracyMeters) <= 0.000000001 && distance == 0;
            if (matches) {
                // Re-execute precisely the read configuration, including otherwise ignored ECEF fields.
                _surveyRestart =
                    QByteArray(Plan::RESTART_SURVEY.data(), static_cast<qsizetype>(Plan::RESTART_SURVEY.size()));
                for (size_t index = 2; index < reply.size(); ++index) {
                    _surveyRestart += ',';
                    _surveyRestart.append(reply[index].data(), static_cast<qsizetype>(reply[index].size()));
                }
            }
        }
        _baseMatches = matches;
        return readback(reply, "PQTMCFGSVIN", valid && (!requireMatch || matches));
    }));
    co_return base.evidence.outcome == GPSCommandOutcome::ReadbackVerified;
}

// Each step awaits in its own statement: GCC 16 evaluates a co_await in a short-circuited && or || operand.
GPSTask<bool> Session::_changeRole()
{
    if (!co_await _acknowledge(QByteArrayView(Plan::WRITE_BASE_ROLE))) {
        co_return false;
    }
    co_return co_await _verifyRole();
}

GPSTask<bool> Session::_changeBase(QByteArrayView body, std::string_view label)
{
    if (!co_await _acknowledge(body, label)) {
        co_return false;
    }
    co_return co_await _verifyBase();
}

GPSTask<bool> Session::_restartWithBase()
{
    if (!co_await _restart()) {
        co_return false;
    }
    co_return co_await _verifyBase();
}

GPSTask<bool> Session::_verifySaved(bool restarted, bool baseVerified)
{
    // A role readback can reflect an unsaved, not-yet-active change. Reboot and read back
    // the saved role before claiming that the receiver actually operates in that role.
    if (!restarted) {
        if (!co_await _restart()) {
            co_return false;
        }
    }
    if (baseVerified) {
        co_return true;
    }
    co_return co_await _verifyBase();
}

GPSTask<bool> Session::_save()
{
    const auto result =
        co_await _transact(Plan::command(Plan::SAVE, Plan::acknowledgement(Plan::SAVE), Plan::SAVE_TIMEOUT));
    const bool saved = result.evidence.outcome == GPSCommandOutcome::Acknowledged;
    if (saved) {
        _saveAcknowledged = true;
        _saveUncertain = false;
        qCDebug(QuectelProtocolLog) << "LG290P acknowledged saving configuration to nonvolatile memory";
    } else {
        _saveUncertain = result.evidence.outcome != GPSCommandOutcome::Rejected && result.evidence.acceptedBytes > 0;
    }
    co_return saved;
}

GPSTask<bool> Session::_restart(bool requireRoleMatch)
{
    _decoder.beginRestart(_channel.context());
    qCDebug(QuectelProtocolLog) << "Restarting LG290P and verifying its saved configuration";
    const auto scope = _channel.deadlineScope(Plan::RESTART_TIMEOUT);
    const uint64_t deadline = GPSDeadline::after(_channel.nowUs(), Plan::RESTART_TIMEOUT).untilUs;
    const QByteArray restart = QuectelCodec::frame(Plan::RESTART);
    if (!co_await _channel.writeCommand({std::string(Plan::RESTART), Plan::RESTART_TIMEOUT}, restart)) {
        co_return false;
    }
    // PQTMSRR has no documented ACK. Do not inflate successful transport completion to acknowledgment.
    _channel.finishCommand(GPSCommandOutcome::Written);
    _decoder.watchBoot(_channel.stream());
    do {
        co_await _channel.wait(Plan::RESTART_POLL);
        if (_channel.failed()) {
            break;
        }
        const bool identified = co_await _identify(Plan::RESTART_IDENTIFY_TIMEOUT);
        if (_decoder.restartRejected()) {
            _channel.setErrorDetail(QStringLiteral("LG290P rejected PQTMSRR."));
            break;
        }
        if (identified && _decoder.booted()) {
            _decoder.stopWatchingBoot();
            co_return co_await _verifyRole(requireRoleMatch);
        }
    } while (!_channel.failed() && _channel.nowUs() < deadline);
    _decoder.stopWatchingBoot();
    co_return false;
}

bool Session::_fail(const QString& reason, GPSProtocolError failure)
{
    _decoder.endSession(_channel.context());
    (void) _channel.flush();
    QString detail = reason;
    if (!_channel.errorDetail().isEmpty()) {
        detail += QLatin1Char(' ') + _channel.errorDetail();
    }
    _channel.failControl(failure);
    if (_saveAcknowledged || _saveUncertain) {
        const QString persistence =
            _saveAcknowledged ? QStringLiteral("An LG290P flash save was acknowledged. ") : QString();
        const QString uncertain =
            _saveUncertain ? QStringLiteral("The latest flash save may have taken effect. ") : QString();
        _channel.setErrorDetail(persistence + uncertain + detail +
                                QStringLiteral(" Receiver settings may have changed; no rollback was attempted."));
        qCWarning(QuectelProtocolLog).noquote() << _channel.errorDetail();
    } else if (_channel.error() != GPSProtocolError::Cancelled) {
        _channel.setErrorDetail(detail);
        qCWarning(QuectelProtocolLog).noquote() << reason;
    }
    return false;
}

}  // namespace

GPSTask<bool> configureBase(GPSCommandChannel& channel, Decoder& decoder, GPSConfig config, unsigned& baud)
{
    Session session(channel, decoder);
    co_return co_await session.run(std::move(config), baud);
}

}  // namespace Quectel
