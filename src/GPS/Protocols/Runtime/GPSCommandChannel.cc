#include "GPSCommandChannel.h"

#include <algorithm>
#include <cstdint>
#include <utility>
#include <vector>

#include "GPSFamilyProtocol.h"
#include "GPSProtocolRuntime.h"
#include "GPSRawAckMatcher.h"
#include "GPSReceiverCapabilities.h"
#include "GPSReceiverConfig.h"
#include "QGCLoggingCategory.h"

GPSDeadlineScope::GPSDeadlineScope(GPSCommandChannel& channel, std::chrono::milliseconds timeout)
    : _channel(channel)
    , _previous(channel._operationDeadline)
{
    _channel._operationDeadline.untilUs =
        std::min(_previous.untilUs, GPSDeadline::after(_channel.nowUs(), timeout).untilUs);
}

GPSDeadlineScope::~GPSDeadlineScope()
{
    _channel._operationDeadline = _previous;
}

void GPSDeadlineScope::limitUntil(uint64_t untilUs)
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

void GPSCommandChannel::beginCommand(GPSConfigurationStep step)
{
    if (failed()) {
        return;
    }
    finishCommand(GPSCommandOutcome::Written);
    _command = {};
    _command.evidence.startedAtUs = nowUs();
    _command.evidence.command = std::move(step.command);
    _command.affectedSettings = step.affectedSettings;
    _command.evidence.required = step.required;
    _commandDeadline.untilUs =
        std::min(_operationDeadline.untilUs, GPSDeadline::after(_command.evidence.startedAtUs, step.timeout).untilUs);
    _commandCompleted = false;
}

GPSCommandResult GPSCommandChannel::completeCommand(GPSCommandOutcome outcome)
{
    if (_commandCompleted) {
        return _command;
    }
    _command.evidence.outcome = outcome;
    _command.evidence.finishedAtUs = nowUs();
    _commandCompleted = true;
    // Completion observers may finish evidence again or begin another attempt.
    const auto result = _command;
    _runtime._commandFinished(result);
    return result;
}

bool GPSCommandChannel::_prepareRead(ReadAwaitable& awaitable)
{
    const auto buffer = awaitable._request.readBuffer;
    if (failed() || !buffer.data() || buffer.empty()) {
        awaitable._immediate = failed() ? -1 : 0;
        awaitable._ready = true;
        return true;
    }
    awaitable._request.deadline = {
        std::min(_operationDeadline.untilUs, GPSDeadline::after(nowUs(), awaitable._timeout).untilUs)};
    return false;
}

int GPSCommandChannel::_finishRead(const GPSIORequest& request)
{
    const auto& result = request.readResult;
    _errorDetail = result.detail;
    if (result.status == GPSReadStatus::Data && result.bytesRead >= 0 &&
        static_cast<size_t>(result.bytesRead) <= request.readBuffer.size()) {
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

GPSReceiveUpdates GPSCommandChannel::_decodeRead(std::span<const uint8_t> buffer, int count)
{
    if (count < 0) {
        return {};
    }
    return _runtime.consume(buffer.first(static_cast<size_t>(count)));
}

bool GPSCommandChannel::_prepareWrite(WriteAwaitable& awaitable)
{
    if (failed()) {
        awaitable._ready = true;
        return true;
    }
    if (!awaitable._bytes.data()) {
        _errorDetail.clear();
        _error = GPSProtocolError::InvalidArgument;
        awaitable._ready = true;
        return true;
    }
    awaitable._request.writeBytes = {reinterpret_cast<const uint8_t*>(awaitable._bytes.data()),
                                     static_cast<size_t>(awaitable._bytes.size())};
    awaitable._request.deadline = {
        std::min(_operationDeadline.untilUs, _commandCompleted ? UINT64_MAX : _commandDeadline.untilUs)};
    return false;
}

bool GPSCommandChannel::_finishWrite(const GPSIORequest& request)
{
    const auto& result = request.writeResult;
    const auto length = static_cast<qsizetype>(request.writeBytes.size());
    _errorDetail = result.detail;
    _command.evidence.acceptedBytes += result.acceptedBytes;
    _command.evidence.writtenBytes += result.writtenBytes;
    _command.evidence.uncertainBytes += result.uncertainBytes();
    if (result.status == GPSWriteStatus::Completed && result.acceptedBytes == length && result.writtenBytes == length &&
        result.uncertainBytes() == 0) {
        return true;
    }
    if (result.status == GPSWriteStatus::Unsupported && result.acceptedBytes == 0 && result.writtenBytes == 0) {
        finishCommand(GPSCommandOutcome::TransportError);
        return false;
    }
    _error = result.status == GPSWriteStatus::Cancelled ? GPSProtocolError::Cancelled : GPSProtocolError::Transport;
    finishCommand(result.status == GPSWriteStatus::Cancelled ? GPSCommandOutcome::Cancelled
                                                             : GPSCommandOutcome::TransportError);
    return false;
}

bool GPSCommandChannel::_prepareBaudrate(BaudrateAwaitable& awaitable)
{
    if (failed()) {
        awaitable._ready = true;
        return true;
    }
    _errorDetail.clear();
    return false;
}

bool GPSCommandChannel::_finishBaudrate(const GPSIORequest& request)
{
    if (request.baudStatus == GPSBaudStatus::Configured) {
        return true;
    }
    if (request.baudStatus != GPSBaudStatus::Unsupported) {
        _error =
            request.baudStatus == GPSBaudStatus::Cancelled ? GPSProtocolError::Cancelled : GPSProtocolError::Transport;
    }
    return false;
}

bool GPSCommandChannel::_prepareWait(WaitAwaitable& awaitable)
{
    if (failed()) {
        awaitable._ready = true;
        return true;
    }
    const auto now = nowUs();
    const auto remaining = _operationDeadline.untilUs > now ? _operationDeadline.untilUs - now : 0;
    awaitable._request.duration =
        std::min(awaitable._request.duration, std::chrono::microseconds(std::min<uint64_t>(remaining, INT64_MAX)));
    return false;
}

void GPSCommandChannel::_finishWait(const GPSIORequest& request)
{
    if (!request.waited) {
        _error = GPSProtocolError::Cancelled;
        _errorDetail.clear();
    }
}

void GPSCommandChannel::_post(GPSIORequest& request, std::coroutine_handle<> continuation)
{
    _runtime._post(request, continuation);
}

GPSTask<bool> GPSCommandChannel::writeCommand(GPSConfigurationStep step, QByteArrayView bytes)
{
    const auto scope = deadlineScope(step.timeout);
    beginCommand(std::move(step));
    co_return co_await write(bytes);
}

GPSTask<GPSCommandResult> GPSCommandChannel::awaitReply(GPSReplyPoll reply)
{
    if (_commandCompleted) {
        co_return _command;
    }
    auto scope = deadlineScope(remainingUntil(_commandDeadline.untilUs));
    scope.limitUntil(_commandDeadline.untilUs);
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
        (void) co_await readAndDecode(remainingUntil(_commandDeadline.untilUs));
    }
    co_return completeCommand(outcome);
}

void GPSCommandChannel::_clearReply()
{
    _reply.reset();
    _rawReply = nullptr;
    _textReply = {};
    _frameReply = {};
}

GPSTask<GPSCommandResult> GPSCommandChannel::_transact(GPSConfigurationStep step, QByteArrayView bytes,
                                                       ReplyMatcher matcher)
{
    struct ReplyGuard
    {
        GPSCommandChannel& channel;

        ~ReplyGuard() { channel._clearReply(); }
    };

    const ReplyGuard guard{*this};
    if (auto* const* raw = std::get_if<GPSRawAckMatcher*>(&matcher)) {
        _rawReply = *raw;
    } else if (auto* text = std::get_if<GPSTextMatcher>(&matcher)) {
        _textReply = std::move(*text);
    } else if (auto* frame = std::get_if<GPSFrameMatcher>(&matcher)) {
        _frameReply = std::move(*frame);
    }
    if (failed()) {
        GPSCommandResult failure;
        failure.evidence.command = std::move(step.command);
        failure.evidence.required = step.required;
        failure.evidence.outcome = failureOutcome();
        co_return failure;
    }
    _reply = GPSCommandOutcome::Pending;
    if (!co_await writeCommand(std::move(step), bytes)) {
        co_return completeCommand(failed() ? failureOutcome() : GPSCommandOutcome::TransportError);
    }
    co_return co_await awaitReply([this] { return _reply.value_or(GPSCommandOutcome::Pending); });
}

void GPSCommandChannel::_offerText(std::string_view reply)
{
    if (_textReply && replyPending()) {
        resolveReply(_textReply(reply));
    }
}

void GPSCommandChannel::_offerFrame(const GPSFrame& frame)
{
    if (_frameReply && replyPending()) {
        resolveReply(_frameReply(frame));
    }
}

void GPSCommandChannel::_offerRaw(std::span<const uint8_t> bytes)
{
    if (_rawReply) {
        _rawReply->append(bytes);
        resolveReply(_rawReply->outcome());
    }
}

GPSTask<GPSCommandSequence::Result> GPSCommandChannel::runSequence(const GPSCommandSequence& sequence)
{
    for (size_t index = 0; index < sequence.steps.size(); ++index) {
        if (const auto* command = std::get_if<GPSCommandSequence::Command>(&sequence.steps[index])) {
            GPSCommandResult result;
            for (unsigned attempt = 0; attempt < std::max(command->attempts, 1U); ++attempt) {
                if (const auto* raw = std::get_if<GPSCommandSequence::RawReply>(&command->reply)) {
                    GPSRawAckMatcher matcher(raw->accepted, raw->rejected);
                    result = co_await transact(command->step, command->wire, matcher);
                } else if (const auto* frame = std::get_if<GPSFrameMatcher>(&command->reply)) {
                    result = co_await transact(command->step, command->wire, *frame);
                } else {
                    result = co_await transact(command->step, command->wire, std::get<GPSTextMatcher>(command->reply));
                }
                if (result.succeeded() || failed()) {
                    break;
                }
            }
            if (!result.succeeded() && (command->step.required || failed())) {
                co_return GPSCommandSequence::Result{
                    .failedStep = index, .failedLabel = command->step.command, .outcome = result.evidence.outcome};
            }
            continue;
        }
        const auto& custom = std::get<GPSCommandSequence::Custom>(sequence.steps[index]);
        const bool succeeded = co_await custom.run();
        if ((!succeeded && custom.required) || failed()) {
            co_return GPSCommandSequence::Result{
                .failedStep = index, .failedLabel = custom.label, .outcome = failureOutcome()};
        }
    }
    co_return GPSCommandSequence::Result{};
}

GPSTask<GPSBaudDetection> GPSCommandChannel::detectBaud(std::span<const unsigned> candidates, unsigned baud,
                                                        GPSBaudProbeFunction probe)
{
    std::vector<unsigned> order(candidates.begin(), candidates.end());
    if (baud == 0 && _detectedBaud != 0) {
        // Detection may recognise a receiver by its traffic at a rate the family does not list.
        std::erase(order, _detectedBaud);
        order.insert(order.begin(), _detectedBaud);
    }
    GPSBaudDetection result{.baud = baud};
    for (const unsigned candidate : order) {
        result.baud = baud ? baud : candidate;
        if (!co_await setBaudrate(result.baud)) {
            result.linkFailed = true;
            co_return result;
        }
        const GPSBaudProbe outcome = co_await probe(result.baud);
        if (outcome == GPSBaudProbe::Found) {
            result.found = true;
            co_return result;
        }
        if (outcome == GPSBaudProbe::Stop || baud || failed()) {
            co_return result;
        }
    }
    co_return result;
}

GPSTask<GPSReceiveUpdates> GPSCommandChannel::receiveCycle(std::chrono::milliseconds timeout)
{
    const auto scope = deadlineScope(timeout);
    if (failed()) {
        co_return GPSReceiveUpdates{};
    }
    GPSFamilyProtocol& protocol = _runtime.protocol();
    const uint64_t started = nowUs();
    GPSReceiveUpdates handled;
    std::array<uint8_t, READ_CHUNK_SIZE> buffer{};
    for (;;) {
        if (protocol.completeReceiveCycle(handled)) {
            co_return handled;
        }
        const auto slice =
            std::min(protocol.nextReadSlice(timeout), remainingUntil(GPSDeadline::after(started, timeout).untilUs));
        const int count = co_await read(buffer, slice);
        if (count < 0) {
            co_return handled;
        }
        handled |= _runtime.consume(std::span<const uint8_t>(buffer).first(static_cast<size_t>(count)));
        if (nowUs() >= _operationDeadline.untilUs) {
            co_return handled;
        }
    }
}

GPSTask<GPSReceiveUpdates> GPSCommandChannel::receive(std::chrono::milliseconds timeout)
{
    return _runtime.protocol().receive(*this, timeout);
}

GPSTask<void> GPSCommandChannel::receiveFor(std::chrono::milliseconds duration)
{
    const uint64_t until = GPSDeadline::after(nowUs(), duration).untilUs;
    while (nowUs() < until) {
        (void) co_await receive(duration);
        if (failed()) {
            co_return;
        }
    }
}

GPSTask<bool> GPSCommandChannel::receiveUntil(std::function<bool()> done, std::chrono::milliseconds duration)
{
    const uint64_t until = GPSDeadline::after(nowUs(), duration).untilUs;
    while (!done() && nowUs() < until) {
        (void) co_await receiveCycle(duration);
        if (failed()) {
            co_return false;
        }
    }
    co_return done();
}

GPSTask<void> GPSCommandChannel::serviceControls()
{
    if (_servicingControls || failed()) {
        co_return;
    }
    {
        struct ServicingGuard
        {
            bool& servicing;

            explicit ServicingGuard(bool& flag)
                : servicing(flag)
            {
                servicing = true;
            }

            ~ServicingGuard() { servicing = false; }
        };

        const ServicingGuard servicing(_servicingControls);
        // This budget belongs to pending receiver commands, independently of the completed read slice.
        const auto scope = deadlineScope(SERVICE_TIMEOUT);
        co_await _runtime.protocol().serviceStreaming(*this);
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

bool GPSCommandChannel::validateConfiguration(const GPSConfig& config) const
{
    const auto& support = _runtime.family().support;
    const GPSReceiverConfig physical{.role = GPSReceiverConfig::Role::RTKBase,
                                     .base = config.base,
                                     .allowPersistentChanges = config.allowPersistentChanges};
    const GPSReceiverCapabilities supported{.surveyIn = true,
                                            .receiverAveraging = support.receiverAveraging,
                                            .persistentConfiguration = support.persistentChanges,
                                            .compactObservations = support.compactObservations};
    const auto error = gpsValidateReceiverPhysicalConfig(physical, supported);
    if (error != GPSReceiverConfigError::None) {
        qCWarning(logCategory()).noquote()
            << QStringLiteral("Invalid receiver physical configuration (%1)").arg(static_cast<int>(error));
        return false;
    }
    return true;
}
