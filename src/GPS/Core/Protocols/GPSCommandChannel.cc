#include "GPSCommandChannel.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <utility>
#include <vector>

#include <QtCore/QScopeGuard>
#include <QtCore/QScopedValueRollback>
#include <QtCore/QStringList>

#include "GPSFamilyProtocol.h"
#include "GPSProtocolRuntime.h"
#include "GPSReceiverDescriptor.h"
#include "QGCLoggingCategory.h"

QString gpsRateList(std::span<const unsigned> rates)
{
    QStringList names;
    for (const unsigned rate : rates) {
        names.append(QString::number(rate));
    }
    return names.join(QStringLiteral(", "));
}

GPSDeadlineScope::GPSDeadlineScope(GPSCommandChannel& channel, std::chrono::milliseconds timeout)
    : GPSDeadlineScope(channel, GPSDeadline::after(channel.nowUs(), timeout))
{}

GPSDeadlineScope::GPSDeadlineScope(GPSCommandChannel& channel, GPSDeadline deadline)
    : _channel(channel)
    , _previous(channel._operationDeadline)
{
    _limitUntil(deadline.untilUs);
}

GPSDeadlineScope::~GPSDeadlineScope()
{
    _channel._operationDeadline = _previous;
}

void GPSDeadlineScope::_limitUntil(uint64_t untilUs)
{
    _channel._operationDeadline.untilUs = std::min(_channel._operationDeadline.untilUs, untilUs);
}

GPSCommandChannel::GPSCommandChannel(GPSProtocolRuntime& runtime)
    : _runtime(runtime)
{}

uint64_t GPSCommandChannel::nowUs() const
{
    return _runtime.nowUs();
}

void GPSCommandChannel::failControl(GPSProtocolError failure)
{
    if (!failed()) {
        _error = failure;
        _errorDetail.clear();
    }
}

void GPSCommandChannel::failControl(const QString& detail)
{
    if (!failed()) {
        _error = GPSProtocolError::Protocol;
        _errorDetail = detail;
    }
}

QString GPSCommandChannel::_commandFailure(const QByteArray& label, GPSCommandOutcome outcome) const
{
    const QString verb =
        outcome == GPSCommandOutcome::Rejected ? QStringLiteral("rejected") : QStringLiteral("did not acknowledge");
    return QStringLiteral("%1 receiver %2 '%3'")
        .arg(gpsReceiverName(_runtime.family().type), verb, QString::fromUtf8(label));
}

void GPSCommandChannel::failSequence(const GPSCommandSequence::Result& result)
{
    failControl(_commandFailure(result.failedLabel, result.outcome));
}

void GPSCommandChannel::_describeFailure()
{
    if (!_errorDetail.isEmpty() || _error == GPSProtocolError::Cancelled || _command.command.isEmpty() ||
        _command.succeeded()) {
        return;
    }
    _errorDetail = _commandFailure(_command.command, _command.outcome);
}

void GPSCommandChannel::failNoAnswer(std::span<const unsigned> rates)
{
    const QString receiver = gpsReceiverName(_runtime.family().type);
    failControl(rates.empty()
                    ? QStringLiteral("The %1 receiver link cannot run at the requested baud rate").arg(receiver)
                    : QStringLiteral("No %1 receiver answered at %2 baud").arg(receiver, gpsRateList(rates)));
}

GPSCommandOutcome GPSCommandChannel::failureOutcome() const
{
    switch (_error) {
        case GPSProtocolError::None:
            return GPSCommandOutcome::Pending;
        case GPSProtocolError::Cancelled:
            return GPSCommandOutcome::Cancelled;
        case GPSProtocolError::Transport:
        case GPSProtocolError::Protocol:
        case GPSProtocolError::InvalidArgument:
        case GPSProtocolError::ConsentRequired:
            return GPSCommandOutcome::TransportError;
    }
    return GPSCommandOutcome::TransportError;
}

void GPSCommandChannel::_resetError()
{
    _error = GPSProtocolError::None;
    _errorDetail.clear();
}

GPSConfigurationEvidence GPSCommandChannel::_failedAttempt(GPSConfigurationStep step) const
{
    GPSConfigurationEvidence failure;
    failure.command = std::move(step.command);
    failure.required = step.required;
    failure.outcome = failureOutcome();
    return failure;
}

void GPSCommandChannel::beginCommand(GPSConfigurationStep step)
{
    if (failed()) {
        // A pending attempt keeps its evidence for whatever ends it.
        if (_commandCompleted) {
            _command = _failedAttempt(std::move(step));
        }
        return;
    }
    finishCommand(GPSCommandOutcome::Written);
    _command = {};
    _command.startedAtUs = nowUs();
    _command.command = std::move(step.command);
    _command.required = step.required;
    _commandDeadline.untilUs =
        std::min(_operationDeadline.untilUs, GPSDeadline::after(_command.startedAtUs, step.timeout).untilUs);
    _commandCompleted = false;
}

GPSConfigurationEvidence GPSCommandChannel::completeCommand(GPSCommandOutcome outcome)
{
    if (_commandCompleted) {
        return _command;
    }
    _command.outcome = outcome;
    _commandCompleted = true;
    // Completion observers may finish evidence again or begin another attempt.
    const auto result = _command;
    _runtime._commandFinished(result);
    return result;
}

int GPSCommandChannel::read(std::span<uint8_t> buffer, std::chrono::milliseconds timeout)
{
    if (failed() || !buffer.data() || buffer.empty()) {
        return failed() ? -1 : 0;
    }
    const auto result =
        _runtime._io.read(buffer, {std::min(_operationDeadline.untilUs, GPSDeadline::after(nowUs(), timeout).untilUs)});
    _errorDetail = result.detail;
    if (result.status == GPSReadStatus::Data && result.bytesRead >= 0 &&
        static_cast<size_t>(result.bytesRead) <= buffer.size()) {
        return result.bytesRead;
    }
    if (result.status == GPSReadStatus::TimedOut && result.bytesRead == 0) {
        return 0;
    }
    _error = result.status == GPSReadStatus::Cancelled ? GPSProtocolError::Cancelled : GPSProtocolError::Transport;
    if (_error != GPSProtocolError::Cancelled) {
        qCWarning(logCategory()).noquote() << QStringLiteral("Receiver read failed (status %1): %2")
                                                  .arg(static_cast<int>(result.status))
                                                  .arg(_errorDetail);
    }
    return -1;
}

GPSReceiveUpdates GPSCommandChannel::_readAndDecode(std::chrono::milliseconds timeout)
{
    std::array<uint8_t, READ_CHUNK_SIZE> buffer{};
    const int count = read(buffer, timeout);
    if (count < 0) {
        return {};
    }
    return _runtime.consume(std::span<const uint8_t>(buffer).first(static_cast<size_t>(count)));
}

bool GPSCommandChannel::write(QByteArrayView bytes)
{
    if (failed()) {
        return false;
    }
    if (!bytes.data()) {
        _errorDetail.clear();
        _error = GPSProtocolError::InvalidArgument;
        return false;
    }
    const uint64_t deadline =
        std::min(_operationDeadline.untilUs, _commandCompleted ? UINT64_MAX : _commandDeadline.untilUs);
    // A late command that wrote nothing just times out. A late later part goes to the transport, whose sticky failure
    // stops the session rather than leave a torn frame on the link.
    if (nowUs() >= deadline && (_commandCompleted || _command.acceptedBytes == 0)) {
        finishCommand(GPSCommandOutcome::TimedOut);
        return false;
    }
    const auto result =
        _runtime._io.cancelToken.isCancelled()
            ? GPSWriteResult{GPSWriteStatus::Cancelled}
            : _runtime._io.write({reinterpret_cast<const uint8_t*>(bytes.data()), static_cast<size_t>(bytes.size())},
                                 {deadline});
    const auto length = bytes.size();
    _errorDetail = result.detail;
    // A write outside an attempt belongs to no reported evidence.
    if (!_commandCompleted) {
        _command.acceptedBytes += result.acceptedBytes;
        _command.writtenBytes += result.writtenBytes;
    }
    if (result.status == GPSWriteStatus::Completed && result.acceptedBytes == length && result.writtenBytes == length &&
        result.uncertainBytes() == 0) {
        return true;
    }
    if (result.status == GPSWriteStatus::Unsupported && result.acceptedBytes == 0 && result.writtenBytes == 0) {
        finishCommand(GPSCommandOutcome::TransportError);
        return false;
    }
    // The transport found the command late before taking any of its bytes, as the check above can just miss.
    if (result.status == GPSWriteStatus::TimedOut && result.acceptedBytes == 0 && result.writtenBytes == 0 &&
        (_commandCompleted || _command.acceptedBytes == 0)) {
        finishCommand(GPSCommandOutcome::TimedOut);
        return false;
    }
    _error = result.status == GPSWriteStatus::Cancelled ? GPSProtocolError::Cancelled : GPSProtocolError::Transport;
    if (_error == GPSProtocolError::Transport && _errorDetail.isEmpty()) {
        const QString receiver = gpsReceiverName(_runtime.family().type);
        const QString command = _commandCompleted ? QString() : QString::fromUtf8(_command.command);
        if (command.isEmpty()) {
            _errorDetail = QStringLiteral("Could not write to the %1 receiver").arg(receiver);
        } else if (result.acceptedBytes > 0) {
            _errorDetail = QStringLiteral("%1 receiver did not accept the full '%2' command").arg(receiver, command);
        } else {
            _errorDetail = QStringLiteral("Could not write '%2' to the %1 receiver").arg(receiver, command);
        }
    }
    finishCommand(result.status == GPSWriteStatus::Cancelled ? GPSCommandOutcome::Cancelled
                                                             : GPSCommandOutcome::TransportError);
    return false;
}

bool GPSCommandChannel::setBaudrate(unsigned baudrate)
{
    return _setBaudrate(baudrate) == GPSBaudStatus::Configured;
}

GPSBaudStatus GPSCommandChannel::_setBaudrate(unsigned baudrate)
{
    if (failed()) {
        return GPSBaudStatus::Error;
    }
    _errorDetail.clear();
    const auto status =
        _runtime._io.cancelToken.isCancelled() ? GPSBaudStatus::Cancelled : _runtime._io.setBaudrate(baudrate);
    if (status == GPSBaudStatus::Cancelled) {
        _error = GPSProtocolError::Cancelled;
    } else if (status == GPSBaudStatus::Error) {
        _error = GPSProtocolError::Transport;
        _errorDetail = QStringLiteral("Cannot set the receiver link to %1 baud").arg(baudrate);
    }
    return status;
}

void GPSCommandChannel::wait(std::chrono::microseconds duration)
{
    if (failed()) {
        return;
    }
    const auto now = nowUs();
    const auto remaining = _operationDeadline.untilUs > now ? _operationDeadline.untilUs - now : 0;
    const auto untilDeadline = std::chrono::microseconds(std::min<uint64_t>(remaining, INT64_MAX));
    if (!_runtime._io.clock.wait(std::min(duration, untilDeadline))) {
        _error = GPSProtocolError::Cancelled;
        _errorDetail.clear();
    }
}

bool GPSCommandChannel::writeCommand(GPSConfigurationStep step, QByteArrayView bytes)
{
    const auto scope = deadlineScope(step.timeout);
    beginCommand(std::move(step));
    return write(bytes);
}

GPSConfigurationEvidence GPSCommandChannel::awaitReply(GPSReplyPoll reply)
{
    if (_commandCompleted) {
        return _command;
    }
    const auto scope = deadlineScope(_commandDeadline);
    const uint64_t deadline = _operationDeadline.untilUs;
    GPSCommandOutcome outcome = GPSCommandOutcome::Pending;
    for (;;) {
        if (const auto failure = failureOutcome(); failure != GPSCommandOutcome::Pending) {
            outcome = failure;
            break;
        }
        if (const auto response = reply(); response != GPSCommandOutcome::Pending) {
            outcome = response;
            break;
        }
        if (nowUs() >= deadline) {
            outcome = GPSCommandOutcome::TimedOut;
            break;
        }
        (void) _readAndDecode(_commandDeadline.remaining(nowUs()));
    }
    return completeCommand(outcome);
}

void GPSCommandChannel::_clearReply()
{
    _reply.reset();
    _replyMatcher = {};
}

GPSConfigurationEvidence GPSCommandChannel::_transact(GPSConfigurationStep step, QByteArrayView bytes,
                                                      ReplyMatcher matcher)
{
    const auto guard = qScopeGuard([this] { _clearReply(); });
    _replyMatcher = std::move(matcher);
    if (failed()) {
        return _failedAttempt(std::move(step));
    }
    _reply = GPSCommandOutcome::Pending;
    if (!writeCommand(std::move(step), bytes)) {
        return completeCommand(failed() ? failureOutcome() : GPSCommandOutcome::TransportError);
    }
    return awaitReply([this] { return _reply.value_or(GPSCommandOutcome::Pending); });
}

void GPSCommandChannel::_offerText(std::string_view reply)
{
    const auto* text = std::get_if<GPSTextMatcher>(&_replyMatcher);
    if (text && *text && _replyPending()) {
        _resolveReply((*text)(reply));
    }
}

void GPSCommandChannel::_offerFrame(const GPSFrame& frame)
{
    const auto* frameReply = std::get_if<GPSFrameMatcher>(&_replyMatcher);
    if (frameReply && *frameReply && _replyPending()) {
        _resolveReply((*frameReply)(frame));
    }
}

void GPSCommandChannel::_offerRaw(std::span<const uint8_t> bytes)
{
    if (auto* const* raw = std::get_if<GPSRawAckMatcher*>(&_replyMatcher)) {
        (*raw)->append(bytes);
        _resolveReply((*raw)->outcome());
    }
}

GPSConfigurationEvidence GPSCommandChannel::transact(const GPSCommandSequence::Command& command)
{
    if (const auto* raw = std::get_if<GPSCommandSequence::RawReply>(&command.reply)) {
        GPSRawAckMatcher matcher(raw->accepted, raw->rejected);
        return _transact(command.step, command.wire, &matcher);
    }
    return transact(command.step, command.wire, std::get<GPSTextMatcher>(command.reply));
}

GPSCommandSequence::Result GPSCommandChannel::runSequence(const GPSCommandSequence& sequence)
{
    for (size_t index = 0; index < sequence.steps.size(); ++index) {
        const auto& command = sequence.steps[index];
        GPSConfigurationEvidence result;
        for (unsigned attempt = 0; attempt < (std::max) (command.attempts, 1U); ++attempt) {
            result = transact(command);
            if (result.succeeded() || failed()) {
                break;
            }
        }
        if (!result.succeeded() && (command.step.required || failed())) {
            return GPSCommandSequence::Result{
                .failedStep = index, .failedLabel = command.step.command, .outcome = result.outcome};
        }
    }
    return GPSCommandSequence::Result{};
}

bool GPSCommandChannel::runRequired(const GPSCommandSequence& sequence)
{
    const auto result = runSequence(sequence);
    if (!result.succeeded()) {
        failSequence(result);
    }
    return result.succeeded();
}

GPSBaudDetection GPSCommandChannel::detectBaud(std::span<const unsigned> candidates, unsigned baud,
                                               GPSBaudProbeFunction probe)
{
    std::vector<unsigned> order;
    if (baud) {
        order.push_back(baud);
    } else {
        order.assign(candidates.begin(), candidates.end());
        if (_detectedBaud != 0) {
            // Detection may recognise a receiver by its traffic at a rate the family does not list.
            std::erase(order, _detectedBaud);
            order.insert(order.begin(), _detectedBaud);
        }
    }
    GPSBaudDetection result{.baud = baud};
    for (const unsigned candidate : order) {
        result.baud = candidate;
        const GPSBaudStatus status = _setBaudrate(candidate);
        if (status == GPSBaudStatus::Unsupported && !baud) {
            continue;
        }
        if (status != GPSBaudStatus::Configured) {
            result.linkFailed = true;
            return result;
        }
        result.probed.push_back(candidate);
        const GPSBaudProbe outcome = probe();
        if (outcome == GPSBaudProbe::Found) {
            result.found = true;
            return result;
        }
        if (outcome == GPSBaudProbe::Stop || failed()) {
            return result;
        }
    }
    return result;
}

GPSReceiveUpdates GPSCommandChannel::receiveCycle(std::chrono::milliseconds timeout)
{
    const auto scope = deadlineScope(timeout);
    if (failed()) {
        return GPSReceiveUpdates{};
    }
    GPSFamilyProtocol& protocol = _runtime.protocol();
    GPSReceiveUpdates handled;
    std::array<uint8_t, READ_CHUNK_SIZE> buffer{};
    for (;;) {
        if (protocol.completeReceiveCycle(handled)) {
            return handled;
        }
        const int count = read(buffer, protocol.nextReadSlice(timeout));
        if (count < 0) {
            return handled;
        }
        handled |= _runtime.consume(std::span<const uint8_t>(buffer).first(static_cast<size_t>(count)));
        if (nowUs() >= _operationDeadline.untilUs) {
            return handled;
        }
    }
}

void GPSCommandChannel::receiveFor(std::chrono::milliseconds duration)
{
    const GPSDeadline until = GPSDeadline::after(nowUs(), duration);
    while (nowUs() < until.untilUs) {
        (void) _runtime.protocol().receive(*this, until.remaining(nowUs()));
        if (failed()) {
            return;
        }
    }
}

bool GPSCommandChannel::receiveUntil(std::function<bool()> done, std::chrono::milliseconds duration)
{
    const GPSDeadline until = GPSDeadline::after(nowUs(), duration);
    while (!done() && nowUs() < until.untilUs) {
        (void) receiveCycle(until.remaining(nowUs()));
        if (failed()) {
            return false;
        }
    }
    return done();
}

void GPSCommandChannel::serviceControls()
{
    if (_servicingControls || failed()) {
        return;
    }
    {
        const QScopedValueRollback servicing(_servicingControls, true);
        // This budget belongs to pending receiver commands, independently of the completed read slice.
        const auto scope = deadlineScope(SERVICE_TIMEOUT);
        _runtime.protocol().serviceStreaming(*this);
    }
    (void) flush();
}

GPSReceiveUpdates GPSCommandChannel::flush()
{
    return _runtime.consume({});
}

GPSStreamDemux& GPSCommandChannel::stream()
{
    return _runtime.stream();
}

GPSDecodeContext& GPSCommandChannel::context()
{
    return _runtime._context;
}

GPSLogCategory GPSCommandChannel::logCategory() const
{
    return _runtime.logCategory();
}
