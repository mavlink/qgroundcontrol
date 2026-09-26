#include "Ashtech/AshtechConfigurator.h"

#include <algorithm>
#include <variant>

#include <QtCore/QByteArray>
#include <QtCore/QString>

#include "Ashtech/AshtechDecoder.h"
#include "Ashtech/AshtechPlan.h"
#include "GPSCommandChannel.h"
#include "GPSEventSink.h"
#include "GPSFamilyProtocol.h"

namespace Ashtech {

GPSTask<bool> Configurator::configure(GPSCommandChannel& channel, GPSConfig config, unsigned& baud)
{
    _decoder.reset(channel.stream());
    if (!channel.validateConfiguration(config)) {
        co_return false;
    }
    Session& session = _decoder.session();
    session.base = config.base;
    // A configured rate is used only when it is one of the rates Ashtech receivers are probed at.
    if (baud > 0 && std::ranges::find(Plan::BAUD_RATES, baud) == std::ranges::end(Plan::BAUD_RATES)) {
        co_return false;
    }
    const auto detection =
        co_await channel.detectBaud(Plan::BAUD_RATES, baud, [this, &channel](unsigned) -> GPSTask<GPSBaudProbe> {
            const bool answered = co_await _queryPort(channel, Plan::PORT_QUERY_ATTEMPTS);
            co_return answered ? GPSBaudProbe::Found : GPSBaudProbe::TryNext;
        });
    if (!detection.found) {
        co_return false;
    }

    baud = detection.baud;
    if (baud != Plan::LINK_BAUD) {
        baud = Plan::LINK_BAUD;
        const QByteArray speed = Plan::line(Plan::LINK_SPEED, _port);
        (void) co_await channel.writeCommand({speed.toStdString(), Plan::RESPONSE_TIMEOUT}, speed);
        _decoder.nmea().reset(channel.stream());
        co_await channel.receiveFor(Plan::LINK_SPEED_SETTLE);
        _decoder.nmea().reset(channel.stream());
        (void) co_await channel.setBaudrate(baud);
        if (!co_await _queryPort(channel, Plan::PORT_QUERY_ATTEMPTS_AT_LINK_BAUD)) {
            co_return false;
        }
    }

    const GPSCommandSequence board{
        {Plan::command(Plan::line(Plan::BOARD_QUERY), [this](std::string_view reply) { return _boardReply(reply); })}};
    if (!(co_await channel.runSequence(board)).succeeded()) {
        co_return false;
    }
    (void) co_await channel.runSequence(Plan::sequence(Plan::OUTPUTS, _port));

    _decoder.nmea().setRTCMEnabled(true);
    if (session.board == Board::MBTwo) {
        channel.context().sink().publishSurvey(true, false, {});
    }
    session.configured = true;
    co_return !channel.failed();
}

GPSTask<void> Configurator::serviceStreaming(GPSCommandChannel& channel)
{
    Session& session = _decoder.session();
    if (session.correctionSetupPending) {
        session.correctionSetupPending = false;
        co_await _activateCorrectionOutput(channel);
    }
    if (session.rtcmActivationPending) {
        session.rtcmActivationPending = false;
        co_await _activateRTCMOutput(channel);
    }
}

GPSTask<bool> Configurator::_queryPort(GPSCommandChannel& channel, unsigned attempts)
{
    auto query =
        Plan::command(Plan::line(Plan::PORT_QUERY), [this](std::string_view reply) { return _portReply(reply); });
    query.attempts = attempts;
    const GPSCommandSequence sequence{{std::move(query)}};
    co_return (co_await channel.runSequence(sequence)).succeeded();
}

GPSTask<void> Configurator::_activateCorrectionOutput(GPSCommandChannel& channel)
{
    Session& session = _decoder.session();
    if (session.correctionOutputActive) {
        co_return;
    }
    GPSEventSink& sink = channel.context().sink();
    if (const auto* fixed = std::get_if<GPSBaseStationConfig::Fixed>(&session.base.mode)) {
        const GPSCommandSequence position{{Plan::fixedPositionCommand(fixed->position)}};
        if (!(co_await channel.runSequence(position)).succeeded()) {
            channel.failControl();
            co_return;
        }
        co_await _activateRTCMOutput(channel);
        if (channel.failed()) {
            co_return;
        }
        sink.publishSurvey(false, true, {}, fixed->position);
    } else if (const auto* survey = std::get_if<GPSBaseStationConfig::SurveyIn>(&session.base.mode)) {
        // The decoder acknowledges the command from its start receipt; the matcher only sees a NAK.
        const GPSCommandSequence start{{Plan::command(Plan::surveyStart(survey->duration), &Plan::rejection)}};
        _decoder.requestSurveyReceipts(true);
        session.awaitingSurveyReceipt = true;
        const bool started = (co_await channel.runSequence(start)).succeeded();
        session.awaitingSurveyReceipt = false;
        if (!started) {
            _decoder.requestSurveyReceipts(false);
            channel.failControl();
            if (channel.error() != GPSProtocolError::Cancelled && channel.errorDetail().isEmpty()) {
                channel.setErrorDetail(QStringLiteral("No matching Ashtech survey-start receipt"));
            }
            co_return;
        }
        if (!(co_await channel.runSequence(Plan::sequence(Plan::STATION, _port))).succeeded()) {
            channel.failControl();
            co_return;
        }
        // A finish receipt may already have arrived with the station commands' replies.
        if (!session.rtcmActivationPending) {
            session.surveyClock.start(channel.nowUs());
            sink.publishSurvey(true, false, session.surveyClock.duration());
        }
    }
    session.correctionOutputActive = true;
}

GPSTask<void> Configurator::_activateRTCMOutput(GPSCommandChannel& channel)
{
    if (!(co_await channel.runSequence(Plan::sequence(Plan::RTCM_OUTPUTS, _port))).succeeded()) {
        channel.failControl();
    }
}

GPSCommandOutcome Configurator::_portReply(std::string_view reply)
{
    if (!reply.starts_with("$PASHR,PRT,") || std::count(reply.begin(), reply.end(), ',') != 3) {
        return Plan::rejection(reply);
    }
    _port = reply[11];
    return GPSCommandOutcome::Acknowledged;
}

GPSCommandOutcome Configurator::_boardReply(std::string_view reply)
{
    if (!reply.starts_with("$PASHR,RID,")) {
        return Plan::rejection(reply);
    }
    _decoder.session().board = reply.substr(11).starts_with("MB2") ? Board::MBTwo : Board::Other;
    return GPSCommandOutcome::Acknowledged;
}

}  // namespace Ashtech
