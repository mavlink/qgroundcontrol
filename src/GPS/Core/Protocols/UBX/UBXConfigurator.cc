#include <algorithm>
#include <array>
#include <string_view>
#include <utility>
#include <variant>

#include <QtCore/QByteArrayView>

#include "GPSCommandChannel.h"
#include "GPSDeadline.h"
#include "GPSReceiverFamilies.h"
#include "LittleEndian.h"
#include "NMEASentence.h"
#include "QGCLoggingCategory.h"
#include "UBX/UBXProtocol.h"

QGC_LOGGING_CATEGORY(UBXProtocolLog, "GPS.Protocols.UBX")

namespace Cfg = UBX::Cfg;
namespace Msg = UBX::Msg;
namespace Plan = UBX::Plan;
using Plan::NakPolicy;

namespace UBX {

bool Protocol::_baseStationUnsupported() const
{
    const auto& identity = _identity;
    // A receiver without time mode can neither survey in nor hold a fixed position.
    if (identity.timeModeUnsupported) {
        return true;
    }
    if (identity.board == UBX::Board::u_blox8) {
        return !identity.isM8p && !identity.model.isEmpty();
    }
    const auto profile = UBX::receiverProfile(identity.board);
    return !profile.rtcmOutput && profile.baseCapabilityKnown;
}

bool Protocol::configure(GPSCommandChannel& channel, GPSConfig config, unsigned& baud)
{
    _base = config.base;
    _valsetAckAmbiguous = false;
    _legacyRTCM = {};
    _ready = false;
    _setMode({}, channel.stream());
    _controller = {};
    _requests = {};

    const unsigned requestedBaud = baud;
    const auto detection = channel.detectBaud(Plan::BAUD_RATES, requestedBaud, [this, &channel]() -> GPSBaudProbe {
        channel.stream().reset(GPSFrameKind::UBX);
        (void) channel.receiveCycle(Plan::BAUD_PROBE_DRAIN);
        channel.stream().reset(GPSFrameKind::UBX);
        if (channel.failed()) {
            return GPSBaudProbe::Stop;
        }
        const bool identified = _identify(channel);
        return identified ? GPSBaudProbe::Found : GPSBaudProbe::TryNext;
    });
    const auto& identity = _identity;
    // Discovery only polls identity: silence or an unsupported identity must not change receiver settings.
    if (!detection.found) {
        channel.failNoAnswer(detection.probed);
        return false;
    }
    if (identity.board == UBX::Board::unknown) {
        QString detail = QStringLiteral("Unsupported u-blox receiver: hardware version %1")
                             .arg(QString::fromLatin1(identity.hardware));
        if (!identity.protocolVersion.isEmpty()) {
            detail += QStringLiteral(", protocol version %1").arg(QString::fromLatin1(identity.protocolVersion));
        }
        channel.failControl(detail);
        return false;
    }
    if (_baseStationUnsupported()) {
        const QString model =
            identity.model.isEmpty() ? QStringLiteral("This u-blox receiver") : QString::fromLatin1(identity.model);
        channel.failControl(QStringLiteral("%1 cannot run as an RTK base station").arg(model));
        return false;
    }
    const unsigned desiredBaud = requestedBaud ? requestedBaud : Plan::BASE_BAUD;
    if (!_setUpPort(channel, detection.baud, desiredBaud)) {
        return false;
    }
    baud = desiredBaud;
    _setMode({.corrections = true}, channel.stream());

    bool deviceConfigured = false;
    if (identity.protocol27) {
        deviceConfigured = _configureDevice(channel);
    } else {
        deviceConfigured = _configureLegacyDevice(channel);
    }
    if (!deviceConfigured) {
        return false;
    }
    if (!_restartSurveyIn(channel)) {
        return false;
    }
    _ready = true;
    _mode.navigation = true;
    return true;
}

bool Protocol::_identify(GPSCommandChannel& channel)
{
    const auto scope = channel.deadlineScope(Plan::IDENTITY_TIMEOUT);
    _identity.board = UBX::Board::unknown;
    if (!_send(channel, Msg::MON_VER, {}, {{}, Plan::IDENTITY_TIMEOUT})) {
        return false;
    }
    return _waitForAck(channel, Msg::MON_VER).succeeded();
}

bool Protocol::_setUpPort(GPSCommandChannel& channel, unsigned detectedBaud, unsigned desiredBaud)
{
    const bool modern = _identity.protocol27;
    if (modern) {
        if (!_transactValset(channel, Plan::uart1Protocols()).succeeded()) {
            return false;
        }
    } else {
        const auto ports = Plan::legacyPorts(detectedBaud);
        if (!_sendAcknowledged(channel, Msg::CFG_PRT, ports)) {
            return false;
        }
    }
    if (desiredBaud == detectedBaud) {
        return true;
    }

    const UBX::Board identifiedBoard = _identity.board;
    const UBX::MessageId command = modern ? Msg::CFG_VALSET : Msg::CFG_PRT;
    if (modern) {
        if (!_writeValset(channel, Plan::uart1Baudrate(desiredBaud))) {
            return false;
        }
    } else {
        const auto ports = Plan::legacyPorts(desiredBaud);
        if (!_send(channel, Msg::CFG_PRT, ports, {{}, Plan::CONFIG_TIMEOUT, false})) {
            return false;
        }
    }
    const auto result = _waitForAck(channel, command);
    const bool acknowledged = result.succeeded();
    if (channel.failed() || result.outcome == GPSCommandOutcome::Rejected) {
        return false;
    }
    if (!channel.setBaudrate(desiredBaud)) {
        channel.failControl(QStringLiteral("The link cannot run at %1 baud").arg(desiredBaud));
        return false;
    }
    if (modern && !acknowledged) {
        // The ACK may be lost in the UART handoff; every later VALSET needs a matching readback.
        _controller.requireConfigurationReadback(command.value());
    }
    channel.stream().reset(GPSFrameKind::UBX);
    if (!_identify(channel)) {
        return false;
    }
    if (_identity.board != identifiedBoard || _identity.protocol27 != modern || _controller.lateRejection()) {
        return false;
    }
    if (modern && !acknowledged) {
        return _verifyValset(channel, {"UBX-CFG-VALSET readback", Plan::CONFIG_TIMEOUT}).succeeded();
    }
    return true;
}

bool Protocol::_configureDevice(GPSCommandChannel& channel)
{
    const auto profile = UBX::receiverProfile(_identity.board);
    const auto portsAndNavigation = Plan::portsAndNavigation(profile);
    if (!_runPlan(channel, portsAndNavigation)) {
        return false;
    }
    if (!_configureJammingDetection(channel)) {
        return false;
    }
    _secSigSeen = false;
    const auto messageOutput = Plan::messageOutput(profile);
    return _runPlan(channel, messageOutput);
}

bool Protocol::_configureJammingDetection(GPSCommandChannel& channel)
{
    // Firmware with CFG-SEC-JAMDET has detection always on and no CFG-ITFM, so a rejection asks for the older monitor.
    // An unanswered or refused probe leaves the receiver state unknown.
    const auto probe = _transactValset(channel, Plan::jammingDetection());
    if (probe.succeeded()) {
        return true;
    }
    if (probe.outcome != GPSCommandOutcome::Rejected || _valsetsRefused()) {
        if (!channel.failed()) {
            qCWarning(UBXProtocolLog) << "CFG-SEC-JAMDET probe unanswered";
        }
        return false;
    }
    return _runPlan(channel, {Plan::interferenceMonitor()});
}

bool Protocol::_configureLegacyDevice(GPSCommandChannel& channel)
{
    const auto rate = Wire::encode(Plan::LEGACY_SURVEY_RATE);
    const auto navigation = Wire::encode(Plan::LEGACY_NAVIGATION);
    if (!_sendAcknowledged(channel, Msg::CFG_RATE, rate)) {
        return false;
    }
    if (!_sendAcknowledged(channel, Msg::CFG_NAV5, navigation)) {
        return false;
    }
    for (const auto& output : Plan::legacyMessageOutput()) {
        if (!_setMessageRateAcknowledged(channel, output)) {
            return false;
        }
    }
    return true;
}

bool Protocol::_restartSurveyIn(GPSCommandChannel& channel)
{
    if (!_identity.protocol27) {
        return _restartLegacySurveyIn(channel);
    }
    (void) _transactValset(channel, Plan::disableRTCMOutput());
    if (const auto* fixed = std::get_if<GPSBaseStationConfig::Fixed>(&_base.mode)) {
        if (!_transactValset(channel, Plan::fixedBase(*fixed)).succeeded()) {
            return false;
        }
        return _activateRTCMOutput(channel) == RTCMActivation::Active;
    }
    const auto* survey = std::get_if<GPSBaseStationConfig::SurveyIn>(&_base.mode);
    if (!survey) {
        return false;
    }
    // Reapplying survey-in mode does not restart an existing survey.
    if (!_disableTimeMode(channel)) {
        return false;
    }
    if (!_waitForSurveyStop(channel)) {
        return false;
    }
    return _transactValset(channel, Plan::surveyIn(*survey)).succeeded();
}

bool Protocol::_restartLegacySurveyIn(GPSCommandChannel& channel)
{
    for (const auto& output : Plan::legacyDisableRTCMOutput()) {
        (void) _setMessageRate(channel, output);
    }
    const auto* fixed = std::get_if<GPSBaseStationConfig::Fixed>(&_base.mode);
    if (!_disableTimeMode(channel)) {
        return false;
    }
    if (fixed) {
        const auto timeMode = Wire::encode(Plan::legacyFixedBase(*fixed));
        if (!_sendAcknowledged(channel, Msg::CFG_TMODE3, timeMode)) {
            return false;
        }
        return _activateRTCMOutput(channel) == RTCMActivation::Active;
    }
    const auto* survey = std::get_if<GPSBaseStationConfig::SurveyIn>(&_base.mode);
    if (!survey) {
        return false;
    }
    if (!_waitForSurveyStop(channel)) {
        return false;
    }
    const auto timeMode = Wire::encode(Plan::legacySurveyIn(*survey));
    if (!_sendAcknowledged(channel, Msg::CFG_TMODE3, timeMode)) {
        return false;
    }
    return _setMessageRateAcknowledged(channel, Plan::LEGACY_SURVEY_STATUS);
}

bool Protocol::_disableTimeMode(GPSCommandChannel& channel)
{
    if (_identity.protocol27) {
        if (!_transactValset(channel, Plan::disableTimeMode()).succeeded()) {
            return false;
        }
        return _verifyValset(channel,
                             {"UBX-CFG-VALGET " + QByteArray::number(Cfg::TMODE_MODE.id), Plan::CONFIG_TIMEOUT})
            .succeeded();
    }
    const auto disabled = Wire::encode(UBX::CfgTmode3{});
    if (!_sendAcknowledged(channel, Msg::CFG_TMODE3, disabled)) {
        return false;
    }
    _timeModeReadbackPending = true;
    _timeModeReadback.reset();
    if (!_send(channel, Msg::CFG_TMODE3, {}, {"UBX-CFG-TMODE3 disabled readback", Plan::CONFIG_TIMEOUT})) {
        _timeModeReadbackPending = false;
        return false;
    }
    const auto result = channel.awaitReply([this] {
        if (_controller.lateRejection()) {
            return GPSCommandOutcome::Rejected;
        }
        return !_timeModeReadback        ? GPSCommandOutcome::Pending
               : *_timeModeReadback == 0 ? GPSCommandOutcome::ReadbackVerified
                                         : GPSCommandOutcome::Rejected;
    });
    _timeModeReadbackPending = false;
    return result.outcome == GPSCommandOutcome::ReadbackVerified;
}

bool Protocol::_waitForSurveyStop(GPSCommandChannel& channel)
{
    _surveyStopped = false;
    const uint64_t deadline = GPSDeadline::after(channel.nowUs(), Plan::SURVEY_STOP_TIMEOUT).untilUs;
    while (!_surveyStopped && channel.nowUs() < deadline) {
        if (!_send(channel, Msg::NAV_SVIN, {}, {"UBX-NAV-SVIN stopped", Plan::CONFIG_TIMEOUT})) {
            return false;
        }
        (void) channel.receiveUntil([this] { return _surveyStopped; }, Plan::SURVEY_STOP_POLL);
        if (channel.failed()) {
            return false;
        }
    }
    if (!_surveyStopped) {
        qCWarning(UBXProtocolLog) << "Time mode did not stop";
        channel.finishCommand(GPSCommandOutcome::TimedOut);
        return false;
    }
    channel.finishCommand(GPSCommandOutcome::ReadbackVerified);
    return true;
}

Protocol::RTCMActivation Protocol::_activateRTCMOutput(GPSCommandChannel& channel)
{
    if (!_identity.protocol27) {
        return _activateLegacyRTCMOutput(channel);
    }
    const bool activated = _transactValset(channel, Plan::rtcmOutput(_base.compactObservations)).succeeded();
    return activated ? RTCMActivation::Active : RTCMActivation::Failed;
}

Protocol::RTCMActivation Protocol::_activateLegacyRTCMOutput(GPSCommandChannel& channel)
{
    auto& progress = _legacyRTCM;
    const auto rate = Wire::encode(Plan::LEGACY_BASE_RATE);
    if (!_send(channel, Msg::CFG_RATE, rate, {{}, Plan::LEGACY_REPLY_TIMEOUT})) {
        progress = {};
        return RTCMActivation::Failed;
    }
    // A CFG-MSG acknowledgement does not name the message, so replies are attributed by their order: the base-rate
    // reply names CFG-RATE and follows any late reply to an earlier activation, and each later command waits for its
    // own. A rate left unanswered is polled; once a poll expires with a reply still to come, later replies cannot be
    // attributed, so no more rates are written and activation depends on the outputs confirmed so far. An unanswered
    // base rate leaves activation to a later service; a rejected one leaves RTCM at the survey rate.
    const auto baseRate = _waitForAck(channel, Msg::CFG_RATE).outcome;
    if (baseRate == GPSCommandOutcome::TimedOut) {
        return RTCMActivation::Unanswered;
    }
    if (baseRate != GPSCommandOutcome::Acknowledged && baseRate != GPSCommandOutcome::Rejected) {
        progress = {};
        return RTCMActivation::Failed;
    }
    // The base status rates come first; their rejection is ignored, as that of an optional RTCM message.
    const auto status = Plan::legacyBaseStatus();
    const auto rtcm = Plan::legacyRTCMOutput(_base.compactObservations);
    bool unanswered = false;
    for (; progress.next < status.size() + rtcm.size(); ++progress.next) {
        // Send a rate only with time for its poll; a late reply would be credited to the next command.
        if (channel.operationDeadline().remaining(channel.nowUs()) <
            Plan::CONFIG_TIMEOUT + Plan::LEGACY_REPLY_TIMEOUT) {
            qCDebug(UBXProtocolLog) << "Service budget spent; RTCM activation resumes in the next service";
            return RTCMActivation::Unanswered;
        }
        const auto output = progress.next < status.size()
                                ? Plan::RTCMOutput{status[progress.next], Plan::RTCMContent::Optional}
                                : rtcm[progress.next - status.size()];
        const auto reply = _confirmMessageRate(channel, output.rate);
        if (reply.outcome == GPSCommandOutcome::TimedOut) {
            unanswered = true;
            break;
        }
        if (reply.outcome != GPSCommandOutcome::Acknowledged && reply.outcome != GPSCommandOutcome::ReadbackVerified &&
            reply.outcome != GPSCommandOutcome::Rejected) {
            progress = {};
            return RTCMActivation::Failed;
        }
        if (reply.outcome != GPSCommandOutcome::Rejected) {
            progress.stationPosition = progress.stationPosition || output.content == Plan::RTCMContent::StationPosition;
            progress.observations = progress.observations || output.content == Plan::RTCMContent::Observations;
        }
        if (!reply.settled) {
            ++progress.next;
            unanswered = true;
            break;
        }
    }
    const bool active = progress.stationPosition && progress.observations;
    if (unanswered) {
        qCWarning(UBXProtocolLog) << "Receiver did not answer a CFG-MSG rate or its poll; later RTCM messages are not"
                                     " configured";
        if (!active) {
            return RTCMActivation::Unanswered;
        }
    } else if (!active) {
        qCWarning(UBXProtocolLog) << "Receiver rejected the RTCM station position or every observation message";
    }
    progress = {};
    return active ? RTCMActivation::Active : RTCMActivation::Failed;
}

void Protocol::serviceStreaming(GPSCommandChannel& channel)
{
    auto& requests = _requests;
    if (requests.rtcmActivation) {
        const auto activation = _activateRTCMOutput(channel);
        // A slow receiver keeps its session and finished survey: the next service resumes activation. Survey reports
        // decoded meanwhile do not request another.
        requests.rtcmActivation = activation == RTCMActivation::Unanswered;
        if (activation == RTCMActivation::Failed) {
            channel.failControl(QStringLiteral("u-blox receiver did not start RTCM output after the survey-in"));
        }
    }
    if (const uint16_t message = std::exchange(requests.disableMessage, 0)) {
        _disableUnexpectedMessage(channel, message);
    }
}

void Protocol::_disableUnexpectedMessage(GPSCommandChannel& channel, uint16_t message)
{
    const uint64_t now = channel.nowUs();
    if (now <= _lastDisableUs + static_cast<uint64_t>(Plan::DISABLE_MESSAGE_INTERVAL.count())) {
        return;
    }
    if (!_identity.protocol27) {
        _lastDisableUs = now;
        (void) _setMessageRate(channel,
                               {{static_cast<uint8_t>(message), static_cast<uint8_t>(message >> 8)}, Plan::NO_OUTPUT});
        return;
    }
    const auto output = std::ranges::find(Plan::UNEXPECTED_OUTPUT, message,
                                          [](const Plan::UnexpectedOutput& entry) { return entry.message.value(); });
    if (output != Plan::UNEXPECTED_OUTPUT.end() && _ready) {
        _lastDisableUs = now;
        (void) _writeValset(channel, Plan::disableMessage(output->key));
    }
}

bool Protocol::_send(GPSCommandChannel& channel, UBX::MessageId message, std::span<const uint8_t> payload,
                     GPSConfigurationStep step)
{
    if (step.command.isEmpty()) {
        step.command = UBX::messageName(message);
    }
    channel.beginCommand(std::move(step));
    // Identity replies can take two seconds, but writing the frame keeps the shorter configuration cap.
    const auto scope = channel.deadlineScope(Plan::CONFIG_TIMEOUT);
    std::array<uint8_t, UBX::FRAME_OVERHEAD + Plan::VALSET_CAPACITY> buffer{};
    const auto frame = UBX::encodeFrame(message, payload, buffer);
    if (frame.empty()) {
        channel.finishCommand(GPSCommandOutcome::TransportError);
        return false;
    }
    return channel.write(frame);
}

bool Protocol::_sendAcknowledged(GPSCommandChannel& channel, UBX::MessageId message, std::span<const uint8_t> payload)
{
    if (!_send(channel, message, payload, {{}, Plan::CONFIG_TIMEOUT})) {
        return false;
    }
    return _waitForAck(channel, message).succeeded();
}

GPSConfigurationEvidence Protocol::_waitForAck(GPSCommandChannel& channel, UBX::MessageId message)
{
    auto& controller = _controller;
    const bool valset = message.value() == Msg::CFG_VALSET.value();
    controller.beginAcknowledgement(message.value());
    if (valset && controller.configurationReadbackRequired()) {
        controller.finishAcknowledgement();
        const GPSDeadline commandDeadline = channel.commandDeadline();
        const auto scope = channel.deadlineScope(commandDeadline);
        const auto& command = channel.currentCommand();
        return _verifyValset(channel,
                             {"UBX-CFG-VALSET readback", commandDeadline.remaining(channel.nowUs()), command.required});
    }
    const auto result = channel.awaitReply([&controller] { return controller.acknowledgement(); });
    controller.finishAcknowledgement();
    if (valset && result.outcome == GPSCommandOutcome::TimedOut) {
        _valsetAckAmbiguous = true;
    }
    return result;
}

bool Protocol::_setMessageRate(GPSCommandChannel& channel, UBX::Plan::MessageRate rate)
{
    const auto payload = Wire::encode(UBX::CfgMsg{.msg = rate.message.value(), .rate = rate.rate});
    return _send(channel, Msg::CFG_MSG, payload, {{}, Plan::CONFIG_TIMEOUT});
}

bool Protocol::_setMessageRateAcknowledged(GPSCommandChannel& channel, UBX::Plan::MessageRate rate)
{
    return _setMessageRateReply(channel, rate) == GPSCommandOutcome::Acknowledged;
}

GPSCommandOutcome Protocol::_setMessageRateReply(GPSCommandChannel& channel, UBX::Plan::MessageRate rate)
{
    if (!_setMessageRate(channel, rate)) {
        return GPSCommandOutcome::TransportError;
    }
    return _waitForAck(channel, Msg::CFG_MSG).outcome;
}

Protocol::RateConfirmation Protocol::_confirmMessageRate(GPSCommandChannel& channel, UBX::Plan::MessageRate rate)
{
    const auto outcome = _setMessageRateReply(channel, rate);
    if (outcome != GPSCommandOutcome::TimedOut) {
        return RateConfirmation{outcome, true};
    }
    auto& controller = _controller;
    const std::array<uint8_t, 2> poll{rate.message.cls, rate.message.id};
    controller.beginRatePoll(rate.message.value());
    if (!_send(channel, Msg::CFG_MSG, poll,
               {"UBX-CFG-MSG " + UBX::messageName(rate.message) + " readback", Plan::LEGACY_REPLY_TIMEOUT})) {
        controller.finishRatePoll();
        return RateConfirmation{GPSCommandOutcome::TransportError, false};
    }
    // The 3-byte CFG-MSG sets the rate of the port it arrives on, which is UART1 or USB.
    const auto decide = [&controller, expected = rate.rate] {
        if (const auto rates = controller.polledRates()) {
            return (*rates)[Plan::LEGACY_PORT_UART1] == expected || (*rates)[Plan::LEGACY_PORT_USB] == expected
                       ? GPSCommandOutcome::ReadbackVerified
                       : GPSCommandOutcome::Rejected;
        }
        return controller.ratePollRejected() ? GPSCommandOutcome::Rejected : GPSCommandOutcome::TimedOut;
    };
    const auto result = channel.awaitReply([&channel, &controller, decide] {
        const bool expired = channel.nowUs() >= channel.commandDeadline().untilUs;
        return controller.ratePollSettled() || expired ? decide() : GPSCommandOutcome::Pending;
    });
    const bool settled = controller.ratePollSettled();
    controller.finishRatePoll();
    return RateConfirmation{result.outcome, settled};
}

bool Protocol::_runPlan(GPSCommandChannel& channel, const std::vector<UBX::Plan::ValsetBatch>& batches)
{
    for (const auto& batch : batches) {
        if (!batch.included) {
            continue;
        }
        const bool acknowledged = _transactValset(channel, batch).succeeded();
        if (channel.failed() || (!acknowledged && batch.policy == NakPolicy::Fail)) {
            return false;
        }
        if (!acknowledged && batch.policy == NakPolicy::Warn) {
            qCWarning(UBXProtocolLog) << batch.warning;
        }
    }
    return true;
}

bool Protocol::_valsetsRefused() const
{
    return _valsetAckAmbiguous && !_controller.configurationReadbackRequired();
}

void Protocol::_load(const UBX::Plan::ValsetBatch& batch)
{
    _valset = {};
    for (const auto& item : batch.items) {
        if (!item.included) {
            continue;
        }
        if (item.msgOut) {
            const UBX::MsgOutKey family{{item.key}};
            (void) _valset.append(family.port(UBX::MsgOutPort::UART1).id, item.value);
            (void) _valset.append(family.port(UBX::MsgOutPort::USB).id, item.value);
        } else {
            (void) _valset.append(item.key, item.value);
        }
    }
}

bool Protocol::_writeValset(GPSCommandChannel& channel, const UBX::Plan::ValsetBatch& batch)
{
    _load(batch);
    const auto payload = _valset.payload();
    GPSConfigurationStep step{{}, batch.timeout, batch.required()};
    if (payload.empty() || _valsetsRefused()) {
        step.command = UBX::messageName(Msg::CFG_VALSET);
        channel.beginCommand(std::move(step));
        channel.finishCommand(GPSCommandOutcome::Rejected);
        return false;
    }
    return _send(channel, Msg::CFG_VALSET, payload, std::move(step));
}

GPSConfigurationEvidence Protocol::_transactValset(GPSCommandChannel& channel, const UBX::Plan::ValsetBatch& batch)
{
    if (!_writeValset(channel, batch)) {
        return channel.completeCommand(channel.error() == GPSProtocolError::Cancelled
                                           ? GPSCommandOutcome::Cancelled
                                           : GPSCommandOutcome::TransportError);
    }
    return _awaitValsetAck(channel);
}

GPSConfigurationEvidence Protocol::_awaitValsetAck(GPSCommandChannel& channel)
{
    auto result = _waitForAck(channel, Msg::CFG_VALSET);
    if (result.outcome != GPSCommandOutcome::TimedOut || result.required ||
        _controller.configurationReadbackRequired()) {
        return result;
    }
    // A VALSET acknowledgement does not say which VALSET it answers, so a late one would be taken for the next. The
    // receiver answers in order: the readback reply follows any late reply to this batch, so a late ACK and matching
    // values accept it, a late NAK, other values or a NAK of the readback reject it, and either way nothing of it is
    // left to arrive. Required batches still fail on the timeout.
    result = _verifyValset(channel, {"UBX-CFG-VALSET readback", Plan::CONFIG_TIMEOUT, false});
    if (result.outcome == GPSCommandOutcome::ReadbackVerified || result.outcome == GPSCommandOutcome::Rejected) {
        _valsetAckAmbiguous = false;
    }
    return result;
}

GPSConfigurationEvidence Protocol::_verifyValset(GPSCommandChannel& channel, GPSConfigurationStep step)
{
    auto& controller = _controller;
    const auto scope = channel.deadlineScope(step.timeout);
    UBX::ConfigurationValueCursor cursor(_valset.entries());
    GPSConfigurationEvidence result;
    while (!cursor.empty()) {
        UBX::ConfigurationValues expected;
        std::array<uint32_t, UBX::ConfigurationValues::MAX_KEYS> keys{};
        std::array<uint8_t, 4 + UBX::ConfigurationValues::MAX_KEYS * sizeof(uint32_t)> request{};
        while (!cursor.empty() && expected.count < expected.values.size()) {
            const auto entry = cursor.next();
            if (!entry) {
                channel.beginCommand(step);
                return channel.completeCommand(GPSCommandOutcome::Rejected);
            }
            (void) LittleEndian::write(request, 4 + expected.count * sizeof(uint32_t), entry->key);
            keys[expected.count] = entry->key;
            expected.values[expected.count++] = *entry;
        }
        controller.beginReadback(std::span(keys).first(expected.count));
        if (!_send(channel, Msg::CFG_VALGET, std::span(request).first(4 + expected.count * sizeof(uint32_t)), step)) {
            controller.finishReadback();
            return channel.completeCommand(GPSCommandOutcome::TransportError);
        }
        result = channel.awaitReply([&controller, &expected] {
            if (controller.lateRejection() || controller.readbackRejected()) {
                return GPSCommandOutcome::Rejected;
            }
            if (!controller.readbackReady()) {
                return GPSCommandOutcome::Pending;
            }
            return controller.readback().values == expected.values ? GPSCommandOutcome::ReadbackVerified
                                                                   : GPSCommandOutcome::Rejected;
        });
        controller.finishReadback();
        if (result.outcome != GPSCommandOutcome::ReadbackVerified) {
            return result;
        }
    }
    return result;
}

namespace {

constexpr auto MON_VER_POLL = [] {
    std::array<uint8_t, FRAME_OVERHEAD> frame{};
    (void) encodeFrame(Msg::MON_VER, {}, frame);
    return frame;
}();

QLatin1StringView signature(const GPSFrame& frame)
{
    if (frame.kind == GPSFrameKind::UBX) {
        return QLatin1StringView("UBX frames");
    }
    const std::string_view text = frame.text();
    if (frame.kind != GPSFrameKind::ASCIILine || !text.starts_with("$PUBX,")) {
        return {};
    }
    const auto sentence = NMEA::frame(text);
    return sentence && sentence->hasValidChecksum() ? QLatin1StringView("$PUBX sentences") : QLatin1StringView();
}

}  // namespace

/// The MON-VER poll Protocol::configure() identifies receivers with, under the same label and timeout.
bool Protocol::probe(GPSCommandChannel& channel)
{
    const auto result =
        channel.transact({UBX::messageName(Msg::MON_VER), Plan::IDENTITY_TIMEOUT}, MON_VER_POLL,
                         GPSFrameMatcher([](const GPSFrame& frame) {
                             return frame.kind == GPSFrameKind::UBX && frame.messageId == Msg::MON_VER.value()
                                        ? GPSCommandOutcome::Acknowledged
                                        : GPSCommandOutcome::Pending;
                         }));
    return result.succeeded();
}

const GPSReceiverFamily FAMILY{
    .type = GPSType::ublox,
    .logCategory = &UBXProtocolLog,
    .stream = {.framers = GPSFrameKind::UBX | GPSFrameKind::RTCM3, .enabled = GPSFrameKind::UBX},
    .baudCandidates = Plan::BAUD_RATES,
    .create = &gpsCreateProtocol<Protocol>,
    .signature = &signature,
};

}  // namespace UBX
