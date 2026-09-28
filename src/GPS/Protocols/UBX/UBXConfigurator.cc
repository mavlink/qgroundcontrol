#include "UBX/UBXConfigurator.h"

#include <algorithm>
#include <array>
#include <string>
#include <utility>
#include <variant>

#include "Checksums.h"
#include "GPSCommandChannel.h"
#include "LittleEndian.h"
#include "UBX/UBXDecoder.h"
#include "UBX/UBXFamily.h"

namespace Cfg = UBX::Cfg;
namespace Msg = UBX::Msg;
namespace Plan = UBX::Plan;
using Plan::NakPolicy;

namespace {

constexpr uint8_t SYNC1 = 0xb5;
constexpr uint8_t SYNC2 = 0x62;

template <size_t N>
QByteArrayView bytes(const std::array<uint8_t, N>& data)
{
    return {data.data(), static_cast<qsizetype>(N)};
}

/// CFG-PRT of UART1 and USB, which older receivers take as one message.
std::array<uint8_t, 2 * UBX::WIRE_SIZE<UBX::CfgPrt>> legacyPortPayload(uint32_t baudrate)
{
    std::array<uint8_t, 2 * UBX::WIRE_SIZE<UBX::CfgPrt>> payload{};
    auto next = payload.begin();
    for (const auto& port : Plan::legacyPorts(baudrate)) {
        const auto encoded = Wire::encode(port);
        next = std::copy(encoded.begin(), encoded.end(), next);
    }
    return payload;
}

}  // namespace

UBXConfigurator::UBXConfigurator(UBXDecoder& decoder)
    : _decoder(decoder)
{}

UBX::Plan::Target UBXConfigurator::_target() const
{
    return {UBX::receiverProfile(_decoder.state().identity.board), _decoder.satelliteInfoEnabled()};
}

bool UBXConfigurator::_baseStationUnsupported() const
{
    const auto& identity = _decoder.state().identity;
    if (identity.board == UBX::Board::u_blox8) {
        return !identity.isM8p && !identity.model.isEmpty();
    }
    const auto profile = UBX::receiverProfile(identity.board);
    return !profile.rtcmOutput && profile.baseCapabilityKnown;
}

GPSTask<bool> UBXConfigurator::configure(GPSCommandChannel& channel, GPSConfig config, unsigned& baud)
{
    _channel = &channel;
    auto& state = _decoder.state();
    _base = config.base;
    state.identity.timeModeUnsupported = false;
    _valsetAckAmbiguous = false;
    _legacyRTCM = {};
    state.configured = false;
    _decoder.setMode({}, channel.stream());
    state.controller = {};
    state.requests = {};
    if (!channel.validateConfiguration(config)) {
        co_return false;
    }

    const unsigned requestedBaud = baud;
    const auto detection =
        co_await channel.detectBaud(Plan::BAUD_RATES, requestedBaud, [this](unsigned) -> GPSTask<GPSBaudProbe> {
            _channel->stream().reset(GPSFrameKind::UBX);
            (void) co_await _channel->receiveCycle(Plan::BAUD_PROBE_DRAIN);
            _channel->stream().reset(GPSFrameKind::UBX);
            if (_channel->failed()) {
                co_return GPSBaudProbe::Stop;
            }
            const bool identified = co_await _identify();
            co_return identified ? GPSBaudProbe::Found : GPSBaudProbe::TryNext;
        });
    const auto& identity = state.identity;
    // Discovery only polls identity: silence or an unsupported identity must not change receiver settings.
    if (!detection.found || identity.board == UBX::Board::unknown || _baseStationUnsupported()) {
        co_return false;
    }
    const unsigned desiredBaud = requestedBaud ? requestedBaud : Plan::BASE_BAUD;
    if (!co_await _setUpPort(detection.baud, desiredBaud)) {
        co_return false;
    }
    baud = desiredBaud;
    channel.stream().setEnabled(GPSFrameKind::RTCM3, true);

    bool deviceConfigured = false;
    if (identity.protocol27) {
        deviceConfigured = co_await _configureDevice();
    } else {
        deviceConfigured = co_await _configureLegacyDevice();
    }
    if (!deviceConfigured) {
        co_return false;
    }
    if (!co_await _restartSurveyIn()) {
        co_return false;
    }
    state.configured = true;
    state.mode.navigation = true;
    state.mode.assembleEpochs = true;
    co_return true;
}

GPSTask<bool> UBXConfigurator::_identify()
{
    const auto scope = _channel->deadlineScope(Plan::IDENTITY_TIMEOUT);
    _decoder.state().identity.board = UBX::Board::unknown;
    if (!co_await _send(Msg::MON_VER, {}, {{}, Plan::IDENTITY_TIMEOUT})) {
        co_return false;
    }
    co_return (co_await _waitForAck(Msg::MON_VER)).succeeded();
}

GPSTask<bool> UBXConfigurator::_setUpPort(unsigned detectedBaud, unsigned desiredBaud)
{
    auto& channel = *_channel;
    auto& state = _decoder.state();
    const bool modern = state.identity.protocol27;
    if (modern) {
        if (!(co_await _transactValset(Plan::uart1Protocols())).succeeded()) {
            co_return false;
        }
    } else {
        const auto ports = legacyPortPayload(detectedBaud);
        if (!co_await _sendAcknowledged(Msg::CFG_PRT, bytes(ports))) {
            co_return false;
        }
    }
    if (desiredBaud == detectedBaud) {
        co_return true;
    }

    const UBX::Board identifiedBoard = state.identity.board;
    const UBX::MessageId command = modern ? Msg::CFG_VALSET : Msg::CFG_PRT;
    if (modern) {
        if (!co_await _writeValset(Plan::uart1Baudrate(desiredBaud))) {
            co_return false;
        }
    } else {
        const auto ports = legacyPortPayload(desiredBaud);
        if (!co_await _send(Msg::CFG_PRT, bytes(ports), {{}, Plan::CONFIG_TIMEOUT, {}, false})) {
            co_return false;
        }
    }
    const auto result = co_await _waitForAck(command);
    const bool acknowledged = result.succeeded();
    if (channel.failed() || result.evidence.outcome == GPSCommandOutcome::Rejected) {
        co_return false;
    }
    if (!co_await channel.setBaudrate(desiredBaud)) {
        co_return false;
    }
    if (modern && !acknowledged) {
        // The ACK may be lost in the UART handoff; every later VALSET needs a matching readback.
        state.controller.requireConfigurationReadback(command.value());
    }
    channel.stream().reset(GPSFrameKind::UBX);
    if (!co_await _identify()) {
        co_return false;
    }
    if (state.identity.board != identifiedBoard || state.identity.protocol27 != modern ||
        state.controller.lateRejection()) {
        co_return false;
    }
    if (modern && !acknowledged) {
        co_return (co_await _verifyValset({"UBX-CFG-VALSET readback", Plan::CONFIG_TIMEOUT, _valset.settings}))
            .succeeded();
    }
    co_return true;
}

GPSTask<bool> UBXConfigurator::_configureDevice()
{
    const auto target = _target();
    const auto portsAndNavigation = Plan::portsAndNavigation(target);
    if (!co_await _runPlan(portsAndNavigation)) {
        co_return false;
    }
    if (!co_await _configureJammingDetection()) {
        co_return false;
    }
    auto& state = _decoder.state();
    state.mode.useNavPvt = true;
    state.secSigSeen = false;
    const auto messageOutput = Plan::messageOutput(target);
    co_return co_await _runPlan(messageOutput);
}

GPSTask<bool> UBXConfigurator::_configureJammingDetection()
{
    // Firmware with CFG-SEC-JAMDET has detection always on and no CFG-ITFM, so a rejection asks for the older monitor.
    // An unanswered or refused probe leaves the receiver state unknown.
    const auto outcome = (co_await _transactValset(Plan::jammingDetection())).evidence.outcome;
    if (outcome == GPSCommandOutcome::Acknowledged || outcome == GPSCommandOutcome::ReadbackVerified) {
        co_return true;
    }
    if (outcome != GPSCommandOutcome::Rejected || _valsetsRefused()) {
        if (!_channel->failed()) {
            qCWarning(UBXProtocolLog) << "CFG-SEC-JAMDET_SENSITIVITY_HI not supported";
        }
        co_return false;
    }
    const std::vector interferenceMonitor{Plan::interferenceMonitor()};
    co_return co_await _runPlan(interferenceMonitor);
}

GPSTask<bool> UBXConfigurator::_configureLegacyDevice()
{
    const auto rate = Wire::encode(Plan::LEGACY_ROVER_RATE);
    const auto navigation = Wire::encode(Plan::LEGACY_NAVIGATION);
    if (!co_await _sendAcknowledged(Msg::CFG_RATE, bytes(rate))) {
        co_return false;
    }
    if (!co_await _sendAcknowledged(Msg::CFG_NAV5, bytes(navigation))) {
        co_return false;
    }
    if (!co_await _setMessageRate(Plan::LEGACY_NAV_PVT)) {
        co_return false;
    }
    auto& mode = _decoder.state().mode;
    mode.useNavPvt = (co_await _waitForAck(Msg::CFG_MSG)).succeeded();
    if (!mode.useNavPvt) {
        for (const auto& output : Plan::LEGACY_POSITION_WITHOUT_PVT) {
            if (!co_await _setMessageRateAcknowledged(output)) {
                co_return false;
            }
        }
    }
    for (const auto& output : Plan::legacyStatus(_target())) {
        if (!co_await _setMessageRateAcknowledged(output)) {
            co_return false;
        }
    }
    co_return true;
}

GPSTask<bool> UBXConfigurator::_restartSurveyIn()
{
    if (!_decoder.state().identity.protocol27) {
        co_return co_await _restartLegacySurveyIn();
    }
    (void) co_await _transactValset(Plan::disableRTCMOutput());
    if (const auto* fixed = std::get_if<GPSBaseStationConfig::Fixed>(&_base.mode)) {
        if (!(co_await _transactValset(Plan::fixedBase(*fixed))).succeeded()) {
            co_return false;
        }
        co_return (co_await _activateRTCMOutput()) == RTCMActivation::Active;
    }
    // Reapplying survey-in mode does not restart an existing survey.
    if (!co_await _disableTimeMode()) {
        co_return false;
    }
    if (!co_await _waitForSurveyStop()) {
        co_return false;
    }
    const auto* survey = std::get_if<GPSBaseStationConfig::SurveyIn>(&_base.mode);
    if (!survey) {
        co_return false;
    }
    co_return (co_await _transactValset(Plan::surveyIn(*survey))).succeeded();
}

GPSTask<bool> UBXConfigurator::_restartLegacySurveyIn()
{
    for (const auto& output : Plan::legacyDisableRTCMOutput()) {
        (void) co_await _setMessageRate(output);
    }
    const auto* fixed = std::get_if<GPSBaseStationConfig::Fixed>(&_base.mode);
    if (!co_await _disableTimeMode()) {
        co_return false;
    }
    if (fixed) {
        const auto timeMode = Wire::encode(Plan::legacyFixedBase(*fixed));
        if (!co_await _sendAcknowledged(Msg::CFG_TMODE3, bytes(timeMode))) {
            co_return false;
        }
        co_return (co_await _activateRTCMOutput()) == RTCMActivation::Active;
    }
    const auto* survey = std::get_if<GPSBaseStationConfig::SurveyIn>(&_base.mode);
    if (!survey) {
        co_return false;
    }
    if (!co_await _waitForSurveyStop()) {
        co_return false;
    }
    const auto timeMode = Wire::encode(Plan::legacySurveyIn(*survey));
    if (!co_await _sendAcknowledged(Msg::CFG_TMODE3, bytes(timeMode))) {
        co_return false;
    }
    co_return co_await _setMessageRateAcknowledged(Plan::LEGACY_SURVEY_STATUS);
}

GPSTask<bool> UBXConfigurator::_disableTimeMode()
{
    auto& channel = *_channel;
    auto& state = _decoder.state();
    if (state.identity.timeModeUnsupported) {
        co_return true;
    }
    if (state.identity.protocol27) {
        if (!(co_await _transactValset(Plan::disableTimeMode())).succeeded()) {
            co_return false;
        }
        co_return co_await _verifyValue(Cfg::TMODE_MODE, 0);
    }
    const auto disabled = Wire::encode(UBX::CfgTmode3{});
    if (!co_await _sendAcknowledged(Msg::CFG_TMODE3, bytes(disabled))) {
        co_return false;
    }
    state.timeModeReadbackPending = true;
    state.timeModeReadback.reset();
    if (!co_await _send(Msg::CFG_TMODE3, {}, {"UBX-CFG-TMODE3 disabled readback", Plan::CONFIG_TIMEOUT})) {
        state.timeModeReadbackPending = false;
        co_return false;
    }
    const auto result = co_await channel.awaitReply([&state] {
        if (state.controller.lateRejection()) {
            return GPSCommandOutcome::Rejected;
        }
        return !state.timeModeReadback        ? GPSCommandOutcome::Pending
               : *state.timeModeReadback == 0 ? GPSCommandOutcome::ReadbackVerified
                                              : GPSCommandOutcome::Rejected;
    });
    state.timeModeReadbackPending = false;
    co_return result.evidence.outcome == GPSCommandOutcome::ReadbackVerified;
}

GPSTask<bool> UBXConfigurator::_waitForSurveyStop()
{
    auto& channel = *_channel;
    auto& state = _decoder.state();
    state.surveyStopped = false;
    const uint64_t deadline = GPSDeadline::after(channel.nowUs(), Plan::SURVEY_STOP_TIMEOUT).untilUs;
    while (!state.surveyStopped && channel.nowUs() < deadline) {
        if (!co_await _send(Msg::NAV_SVIN, {}, {"UBX-NAV-SVIN stopped", Plan::CONFIG_TIMEOUT})) {
            co_return false;
        }
        (void) co_await channel.receiveUntil([&state] { return state.surveyStopped; }, Plan::SURVEY_STOP_POLL);
        if (channel.failed()) {
            co_return false;
        }
    }
    if (!state.surveyStopped) {
        qCWarning(UBXProtocolLog) << "Time mode did not stop";
        channel.finishCommand(GPSCommandOutcome::TimedOut);
        co_return false;
    }
    channel.finishCommand(GPSCommandOutcome::ReadbackVerified);
    co_return true;
}

GPSTask<UBXConfigurator::RTCMActivation> UBXConfigurator::_activateRTCMOutput()
{
    if (!_decoder.state().identity.protocol27) {
        co_return co_await _activateLegacyRTCMOutput();
    }
    const bool activated =
        (co_await _transactValset(Plan::rtcmOutput(_target(), _base.compactObservations))).succeeded();
    co_return activated ? RTCMActivation::Active : RTCMActivation::Failed;
}

GPSTask<UBXConfigurator::RTCMActivation> UBXConfigurator::_activateLegacyRTCMOutput()
{
    auto& channel = *_channel;
    auto& progress = _legacyRTCM;
    const auto rate = Wire::encode(Plan::LEGACY_BASE_RATE);
    if (!co_await _send(Msg::CFG_RATE, bytes(rate), {{}, Plan::LEGACY_REPLY_TIMEOUT})) {
        progress = {};
        co_return RTCMActivation::Failed;
    }
    // A CFG-MSG acknowledgement does not name the message, so replies are attributed by their order: the base-rate
    // reply names CFG-RATE and follows any late reply to an earlier activation, and each later command waits for its
    // own. A rate left unanswered is polled; once a poll expires with a reply still to come, later replies cannot be
    // attributed, so no more rates are written and activation depends on the outputs confirmed so far. An unanswered
    // base rate leaves activation to a later service; a rejected one leaves RTCM at the rover rate.
    const auto baseRate = (co_await _waitForAck(Msg::CFG_RATE)).evidence.outcome;
    if (baseRate == GPSCommandOutcome::TimedOut) {
        co_return RTCMActivation::Unanswered;
    }
    if (baseRate != GPSCommandOutcome::Acknowledged && baseRate != GPSCommandOutcome::Rejected) {
        progress = {};
        co_return RTCMActivation::Failed;
    }
    // The base status rates come first; their rejection is ignored, as that of an optional RTCM message.
    const auto status = Plan::legacyBaseStatus(_target());
    const auto rtcm = Plan::legacyRTCMOutput(_base.compactObservations);
    bool unanswered = false;
    for (; progress.next < status.size() + rtcm.size(); ++progress.next) {
        // A write past the service budget fails the transport, so a rate is sent only with time for its poll.
        if (channel.remainingUntil(channel.operationDeadline().untilUs) <
            Plan::CONFIG_TIMEOUT + Plan::LEGACY_REPLY_TIMEOUT) {
            qCDebug(UBXProtocolLog) << "Service budget spent; RTCM activation resumes in the next service";
            co_return RTCMActivation::Unanswered;
        }
        const auto output = progress.next < status.size()
                                ? Plan::RTCMOutput{status[progress.next], Plan::RTCMContent::Optional}
                                : rtcm[progress.next - status.size()];
        const auto reply = co_await _confirmMessageRate(output.rate);
        if (reply.outcome == GPSCommandOutcome::TimedOut) {
            unanswered = true;
            break;
        }
        if (reply.outcome != GPSCommandOutcome::Acknowledged && reply.outcome != GPSCommandOutcome::ReadbackVerified &&
            reply.outcome != GPSCommandOutcome::Rejected) {
            progress = {};
            co_return RTCMActivation::Failed;
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
            co_return RTCMActivation::Unanswered;
        }
    } else if (!active) {
        qCWarning(UBXProtocolLog) << "Receiver rejected the RTCM station position or every observation message";
    }
    progress = {};
    co_return active ? RTCMActivation::Active : RTCMActivation::Failed;
}

GPSTask<void> UBXConfigurator::serviceStreaming(GPSCommandChannel& channel)
{
    _channel = &channel;
    auto& requests = _decoder.state().requests;
    if (std::exchange(requests.commsDiagnostics, false)) {
        co_await _requestCommsDiagnostics();
    }
    if (std::exchange(requests.rtcmActivation, false)) {
        const auto activation = co_await _activateRTCMOutput();
        if (activation == RTCMActivation::Unanswered) {
            // A slow receiver keeps its session and finished survey: the next service resumes activation.
            requests.rtcmActivation = true;
        } else if (activation == RTCMActivation::Failed) {
            channel.failControl();
        }
    }
    if (const uint16_t message = std::exchange(requests.disableMessage, 0)) {
        co_await _disableUnexpectedMessage(message);
    }
}

GPSTask<void> UBXConfigurator::_requestCommsDiagnostics()
{
    const uint64_t now = _channel->nowUs();
    if (now < _nextCommsPollUs) {
        co_return;
    }
    _nextCommsPollUs = now + static_cast<uint64_t>(Plan::COMMS_POLL_INTERVAL.count());
    const bool sent = co_await _send(Msg::MON_COMMS, {}, {{}, Plan::CONFIG_TIMEOUT});
    _decoder.state().commsReplyDeadlineUs = sent ? now + static_cast<uint64_t>(Plan::COMMS_REPLY_WINDOW.count()) : 0;
}

GPSTask<void> UBXConfigurator::_disableUnexpectedMessage(uint16_t message)
{
    const auto& state = _decoder.state();
    const auto interval = static_cast<uint64_t>(Plan::DISABLE_MESSAGE_INTERVAL.count());
    if (state.identity.protocol27) {
        for (const auto& output : Plan::UNEXPECTED_OUTPUT) {
            if (output.message.value() != message) {
                continue;
            }
            const uint64_t now = _channel->nowUs();
            if (now > _lastDisableUs + interval && state.configured) {
                _lastDisableUs = now;
                (void) co_await _writeValset(Plan::disableMessage(output.key));
            }
        }
        co_return;
    }
    const uint64_t now = _channel->nowUs();
    if (now > _lastDisableUs + interval) {
        _lastDisableUs = now;
        (void) co_await _setMessageRate(
            {{static_cast<uint8_t>(message), static_cast<uint8_t>(message >> 8)}, Plan::NO_OUTPUT});
    }
}

GPSTask<bool> UBXConfigurator::_send(UBX::MessageId message, QByteArrayView payload, GPSConfigurationStep step)
{
    auto& channel = *_channel;
    if (message.value() == Msg::CFG_RATE.value()) {
        step.affectedSettings.add(GPSReceiverSetting::OutputRateHz);
    }
    if (step.command.empty()) {
        step.command = std::to_string(message.value());
    }
    channel.beginCommand(std::move(step));
    // Identity replies can take two seconds, but the writes of one frame share the shorter configuration cap.
    auto scope = channel.deadlineScope(Plan::CONFIG_TIMEOUT);
    scope.limitUntil(GPSDeadline::after(channel.currentCommand().evidence.startedAtUs, Plan::CONFIG_TIMEOUT).untilUs);
    const auto length = static_cast<uint16_t>(payload.size());
    const std::array<uint8_t, 6> header{
        SYNC1, SYNC2, message.cls, message.id, static_cast<uint8_t>(length), static_cast<uint8_t>(length >> 8)};
    const auto payloadBytes =
        std::span(reinterpret_cast<const uint8_t*>(payload.data()), static_cast<size_t>(payload.size()));
    const auto checksum = QGC::fletcher8(payloadBytes, QGC::fletcher8(std::span(header).subspan(2)));
    const std::array<uint8_t, 2> trailer{checksum.a, checksum.b};
    if (!co_await channel.write(bytes(header))) {
        co_return false;
    }
    if (!payload.isEmpty()) {
        if (!co_await channel.write(payload)) {
            co_return false;
        }
    }
    co_return co_await channel.write(bytes(trailer));
}

GPSTask<bool> UBXConfigurator::_sendAcknowledged(UBX::MessageId message, QByteArrayView payload)
{
    if (!co_await _send(message, payload, {{}, Plan::CONFIG_TIMEOUT})) {
        co_return false;
    }
    co_return (co_await _waitForAck(message)).succeeded();
}

GPSTask<GPSCommandResult> UBXConfigurator::_waitForAck(UBX::MessageId message)
{
    auto& channel = *_channel;
    auto& controller = _decoder.state().controller;
    const uint64_t commandDeadline = channel.commandDeadline().untilUs;
    auto scope = channel.deadlineScope(channel.remainingUntil(commandDeadline));
    scope.limitUntil(commandDeadline);
    const bool valset = message.value() == Msg::CFG_VALSET.value();
    controller.beginAcknowledgement(message.value());
    if (valset && controller.configurationReadbackRequired()) {
        controller.finishAcknowledgement();
        const auto& command = channel.currentCommand();
        co_return co_await _verifyValset({"UBX-CFG-VALSET readback", channel.remainingUntil(commandDeadline),
                                          command.affectedSettings, command.evidence.required});
    }
    const auto result = co_await channel.awaitReply([&controller] { return controller.acknowledgement(); });
    controller.finishAcknowledgement();
    if (valset && result.evidence.outcome == GPSCommandOutcome::TimedOut) {
        _valsetAckAmbiguous = true;
    }
    co_return result;
}

GPSTask<bool> UBXConfigurator::_setMessageRate(UBX::Plan::MessageRate rate)
{
    const auto payload = Wire::encode(UBX::CfgMsg{.msg = rate.message.value(), .rate = rate.rate});
    co_return co_await _send(Msg::CFG_MSG, bytes(payload), {{}, Plan::CONFIG_TIMEOUT});
}

GPSTask<bool> UBXConfigurator::_setMessageRateAcknowledged(UBX::Plan::MessageRate rate)
{
    co_return (co_await _setMessageRateReply(rate)) == GPSCommandOutcome::Acknowledged;
}

GPSTask<GPSCommandOutcome> UBXConfigurator::_setMessageRateReply(UBX::Plan::MessageRate rate)
{
    if (!co_await _setMessageRate(rate)) {
        co_return GPSCommandOutcome::TransportError;
    }
    co_return (co_await _waitForAck(Msg::CFG_MSG)).evidence.outcome;
}

GPSTask<UBXConfigurator::RateConfirmation> UBXConfigurator::_confirmMessageRate(UBX::Plan::MessageRate rate)
{
    const auto outcome = co_await _setMessageRateReply(rate);
    if (outcome != GPSCommandOutcome::TimedOut) {
        co_return RateConfirmation{outcome, true};
    }
    auto& channel = *_channel;
    auto& controller = _decoder.state().controller;
    const std::array<uint8_t, 2> poll{rate.message.cls, rate.message.id};
    controller.beginRatePoll(rate.message.value());
    if (!co_await _send(
            Msg::CFG_MSG, bytes(poll),
            {"UBX-CFG-MSG " + std::to_string(rate.message.value()) + " readback", Plan::LEGACY_REPLY_TIMEOUT})) {
        controller.finishRatePoll();
        co_return RateConfirmation{GPSCommandOutcome::TransportError, false};
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
    const auto result = co_await channel.awaitReply([&channel, &controller, decide] {
        const bool expired = channel.nowUs() >= channel.commandDeadline().untilUs;
        return controller.ratePollSettled() || expired ? decide() : GPSCommandOutcome::Pending;
    });
    const bool settled = controller.ratePollSettled();
    controller.finishRatePoll();
    co_return RateConfirmation{result.evidence.outcome, settled};
}

GPSTask<bool> UBXConfigurator::_runPlan(const std::vector<UBX::Plan::ValsetBatch>& batches)
{
    bool previousAcknowledged = true;
    for (const auto& batch : batches) {
        if (!batch.included || (batch.fallback && previousAcknowledged)) {
            continue;
        }
        bool acknowledged = false;
        if (batch.policy == NakPolicy::IgnoreReply) {
            if (!co_await _writeValset(batch)) {
                co_return false;
            }
            acknowledged = (co_await _awaitValsetAck()).succeeded();
        } else {
            acknowledged = (co_await _transactValset(batch)).succeeded();
        }
        if (_channel->failed() || (!acknowledged && batch.policy == NakPolicy::Fail)) {
            co_return false;
        }
        if (!acknowledged && batch.policy == NakPolicy::Warn) {
            qCWarning(UBXProtocolLog) << batch.warning;
        }
        previousAcknowledged = acknowledged;
    }
    co_return true;
}

bool UBXConfigurator::_valsetsRefused() const
{
    return _valsetAckAmbiguous && !_decoder.state().controller.configurationReadbackRequired();
}

void UBXConfigurator::_load(const UBX::Plan::ValsetBatch& batch)
{
    _valset = {};
    const bool usb = _target().profile.usb;
    for (const auto& item : batch.items) {
        if (!item.included) {
            continue;
        }
        if (item.msgOut) {
            const UBX::MsgOutKey family{{item.key}};
            (void) _valset.append(family.port(UBX::MsgOutPort::UART1).id, item.value);
            if (usb) {
                (void) _valset.append(family.port(UBX::MsgOutPort::USB).id, item.value);
            }
        } else if (_valset.append(item.key, item.value) &&
                   (item.key == Cfg::RATE_MEAS.id || item.key == Cfg::RATE_NAV.id)) {
            _valset.settings.add(GPSReceiverSetting::OutputRateHz);
        }
    }
}

GPSTask<bool> UBXConfigurator::_writeValset(const UBX::Plan::ValsetBatch& batch)
{
    auto& channel = *_channel;
    _load(batch);
    const auto payload = _valset.payload();
    GPSConfigurationStep step{{}, batch.timeout, _valset.settings, batch.required()};
    if (payload.empty() || _valsetsRefused()) {
        step.command = std::to_string(Msg::CFG_VALSET.value());
        channel.beginCommand(std::move(step));
        channel.finishCommand(GPSCommandOutcome::Rejected);
        co_return false;
    }
    co_return co_await _send(Msg::CFG_VALSET, {payload.data(), static_cast<qsizetype>(payload.size())},
                             std::move(step));
}

GPSTask<GPSCommandResult> UBXConfigurator::_transactValset(const UBX::Plan::ValsetBatch& batch)
{
    auto& channel = *_channel;
    if (!co_await _writeValset(batch)) {
        co_return channel.completeCommand(channel.error() == GPSProtocolError::Cancelled
                                              ? GPSCommandOutcome::Cancelled
                                              : GPSCommandOutcome::TransportError);
    }
    co_return co_await _awaitValsetAck();
}

GPSTask<GPSCommandResult> UBXConfigurator::_awaitValsetAck()
{
    auto result = co_await _waitForAck(Msg::CFG_VALSET);
    if (result.evidence.outcome != GPSCommandOutcome::TimedOut || result.evidence.required ||
        _decoder.state().controller.configurationReadbackRequired()) {
        co_return result;
    }
    // A VALSET acknowledgement does not say which VALSET it answers, so a late one would be taken for the next. The
    // receiver answers in order: the readback reply follows any late reply to this batch, so a late ACK and matching
    // values accept it, a late NAK, other values or a NAK of the readback reject it, and either way nothing of it is
    // left to arrive. Required batches still fail on the timeout.
    result = co_await _verifyValset({"UBX-CFG-VALSET readback", Plan::CONFIG_TIMEOUT, _valset.settings, false});
    if (result.evidence.outcome == GPSCommandOutcome::ReadbackVerified ||
        result.evidence.outcome == GPSCommandOutcome::Rejected) {
        _valsetAckAmbiguous = false;
    }
    co_return result;
}

GPSTask<GPSCommandResult> UBXConfigurator::_verifyValset(GPSConfigurationStep step)
{
    auto& channel = *_channel;
    auto& controller = _decoder.state().controller;
    const auto scope = channel.deadlineScope(step.timeout);
    UBX::ConfigurationValueCursor cursor(_valset.entries());
    GPSCommandResult result;
    while (!cursor.empty()) {
        UBX::ConfigurationValues expected;
        std::array<uint32_t, 9> keys{};
        std::array<uint8_t, 4 + 9 * sizeof(uint32_t)> request{};
        while (!cursor.empty() && expected.count < expected.values.size()) {
            const auto entry = cursor.next();
            if (!entry) {
                channel.beginCommand(step);
                co_return channel.completeCommand(GPSCommandOutcome::Rejected);
            }
            (void) LittleEndian::write(request, 4 + expected.count * sizeof(uint32_t), entry->key);
            keys[expected.count] = entry->key;
            expected.values[expected.count++] = *entry;
        }
        controller.beginReadback(std::span(keys).first(expected.count));
        const auto length = static_cast<qsizetype>(4 + expected.count * sizeof(uint32_t));
        if (!co_await _send(Msg::CFG_VALGET, QByteArrayView(request.data(), length), step)) {
            controller.finishReadback();
            co_return channel.completeCommand(GPSCommandOutcome::TransportError);
        }
        result = co_await channel.awaitReply([&controller, &expected] {
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
        if (result.evidence.outcome != GPSCommandOutcome::ReadbackVerified) {
            co_return result;
        }
    }
    co_return result;
}

GPSTask<bool> UBXConfigurator::_verifyValue(UBX::CfgKey<uint8_t> key, uint8_t value)
{
    auto& channel = *_channel;
    auto& controller = _decoder.state().controller;
    const std::array keys{key.id};
    controller.beginReadback(keys);
    std::array<uint8_t, 8> request{};
    (void) LittleEndian::write(request, 4, key.id);
    if (!co_await _send(Msg::CFG_VALGET, bytes(request),
                        {"UBX-CFG-VALGET " + std::to_string(key.id), Plan::CONFIG_TIMEOUT})) {
        controller.finishReadback();
        co_return false;
    }
    const auto result = co_await channel.awaitReply([&controller, value] {
        if (controller.lateRejection() || controller.readbackRejected()) {
            return GPSCommandOutcome::Rejected;
        }
        return !controller.readbackReady()                      ? GPSCommandOutcome::Pending
               : controller.readback().values[0].value == value ? GPSCommandOutcome::ReadbackVerified
                                                                : GPSCommandOutcome::Rejected;
    });
    controller.finishReadback();
    co_return result.evidence.outcome == GPSCommandOutcome::ReadbackVerified;
}
