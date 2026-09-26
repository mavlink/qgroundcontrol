#include "Unicore/UnicoreConfigurator.h"

#include <utility>
#include <variant>

#include <QtCore/QLatin1StringView>
#include <QtCore/QList>
#include <QtCore/QString>

#include "GPSCommandChannel.h"
#include "GPSProtocolMath.h"
#include "Unicore/UnicoreDecoder.h"
#include "Unicore/UnicorePlan.h"

namespace Unicore {

namespace {

/// One configuration attempt; its failure description ends in the channel's error detail.
class Session
{
public:
    Session(GPSCommandChannel& channel, Decoder& decoder)
        : _channel(channel)
        , _decoder(decoder)
    {}

    GPSTask<bool> run(GPSConfig config, unsigned& baud);

private:
    GPSTask<bool> _identify(unsigned& baud);
    GPSTask<bool> _run(QList<Plan::Command> commands, QLatin1StringView stage);
    GPSTask<bool> _execute(Plan::Command command, QLatin1StringView stage, qsizetype step);
    bool _fail(const QString& reason = {});

    GPSCommandChannel& _channel;
    Decoder& _decoder;
    QString _detail;
};

GPSTask<bool> Session::run(GPSConfig config, unsigned& baud)
{
    const auto* fixed = std::get_if<GPSBaseStationConfig::Fixed>(&config.base.mode);
    _decoder.startSession(_channel.stream(), _channel.context(), !fixed);
    (void) _channel.flush();
    if (!_channel.validateConfiguration(config)) {
        co_return _fail(
            QStringLiteral("Invalid Unicore receiver configuration: check the role, base position and survey settings; "
                           "persistent changes are not supported"));
    }
    const auto* averaging = std::get_if<GPSBaseStationConfig::ReceiverAveraging>(&config.base.mode);
    if (!fixed && !averaging) {
        qCWarning(UnicoreProtocolLog) << "Unicore supports receiver-managed averaging, not accuracy-controlled survey";
        co_return _fail(
            QStringLiteral("Unicore requires receiver-managed averaging with a duration between 1 and 3600 seconds"));
    }
    GPSProtocolMath::Ecef position;
    if (fixed) {
        position = GPSProtocolMath::toEcef(fixed->position);
        _decoder.setFixedPosition(position);
    }
    const auto scope = _channel.deadlineScope(Plan::CONFIGURATION_TIMEOUT);
    if (!co_await _identify(baud)) {
        co_return _fail();
    }
    const auto plan = fixed ? Plan::fixedBase(position) : Plan::averagingBase(averaging->maximumDuration);
    if (!co_await _run(plan.role, QLatin1StringView("role"))) {
        co_return _fail();
    }
    _decoder.monitorBase(_channel.context());
    if (!co_await _run(plan.output, QLatin1StringView("output"))) {
        co_return _fail();
    }
    _decoder.finishSession();
    co_return true;
}

GPSTask<bool> Session::_identify(unsigned& baud)
{
    const auto detection =
        co_await _channel.detectBaud(Plan::BAUD_RATES, baud, [this](unsigned) -> GPSTask<GPSBaudProbe> {
            _decoder.resetStream(_channel.stream());
            if (co_await _execute(Plan::identify(), QLatin1StringView("identity"), 0)) {
                co_return GPSBaudProbe::Found;
            }
            // A receiver that reported its model answered at this rate.
            co_return _decoder.model().isEmpty() ? GPSBaudProbe::TryNext : GPSBaudProbe::Stop;
        });
    if (!detection.found) {
        if (detection.linkFailed) {
            _detail = QStringLiteral("Cannot configure Unicore host serial speed %1").arg(detection.baud);
        }
        co_return false;
    }
    baud = detection.baud;
    qCDebug(UnicoreProtocolLog).noquote() << "Unicore" << _decoder.model() << "firmware" << _decoder.firmware();
    co_return true;
}

GPSTask<bool> Session::_run(QList<Plan::Command> commands, QLatin1StringView stage)
{
    for (qsizetype step = 0; step < commands.size(); ++step) {
        if (!co_await _execute(commands.at(step), stage, step)) {
            co_return false;
        }
    }
    co_return true;
}

GPSTask<bool> Session::_execute(Plan::Command command, QLatin1StringView stage, qsizetype step)
{
    _decoder.expect(command);
    const QByteArray wire = command.text + "\r\n";
    const auto result = co_await _channel.transact({command.name().toStdString(), Plan::COMMAND_TIMEOUT}, wire);
    _detail = _decoder.rejection();
    if (result.evidence.writtenBytes < wire.size()) {
        _detail = QStringLiteral("Unicore command '%1' could not be written").arg(QString::fromUtf8(command.name()));
        co_return false;
    }
    if (result.succeeded()) {
        co_return true;
    }
    const auto outcome = result.evidence.outcome;
    if (outcome != GPSCommandOutcome::Cancelled) {
        if (_detail.isEmpty()) {
            const QString failure = outcome == GPSCommandOutcome::TimedOut ? QStringLiteral("timed out")
                                    : outcome == GPSCommandOutcome::Rejected
                                        ? QStringLiteral("was rejected or its readback did not match")
                                        : QStringLiteral("failed");
            _detail = QStringLiteral("Unicore command '%1' %2").arg(QString::fromUtf8(command.name()), failure);
        }
        qCWarning(UnicoreProtocolLog).noquote()
            << "Unicore" << stage << "command" << step << "failed with outcome" << static_cast<int>(outcome);
    }
    co_return false;
}

bool Session::_fail(const QString& reason)
{
    if (!reason.isEmpty()) {
        _detail = reason;
    }
    _decoder.failSession(_channel.context());
    (void) _channel.flush();
    if (_channel.error() != GPSProtocolError::Cancelled && _channel.errorDetail().isEmpty()) {
        _channel.setErrorDetail(_detail);
    }
    return false;
}

}  // namespace

GPSTask<bool> configureBase(GPSCommandChannel& channel, Decoder& decoder, GPSConfig config, unsigned& baud)
{
    Session session(channel, decoder);
    co_return co_await session.run(std::move(config), baud);
}

}  // namespace Unicore
