#include "GPSProtocolRuntime.h"

#include <algorithm>
#include <exception>
#include <thread>
#include <utility>

#include <QtCore/QScopedValueRollback>

#include "MonotonicClock.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(GPSProtocolRuntimeLog, "GPS.Driver.Protocols.Runtime")

using namespace std::chrono_literals;

GPSProtocolRuntime::GPSProtocolRuntime(const GPSReceiverFamily& family, GPSRuntimeIO io, GPSRuntimeObserver observer,
                                       const GPSFamilyOptions& options)
    : GPSProtocolRuntime(family, family.create ? family.create(options) : nullptr, std::move(io), std::move(observer))
{}

GPSProtocolRuntime::GPSProtocolRuntime(const GPSReceiverFamily& family, std::unique_ptr<GPSFamilyProtocol> protocol,
                                       GPSRuntimeIO io, GPSRuntimeObserver observer)
    : _family(family)
    , _io(std::move(io))
    , _observer(std::move(observer))
    , _sink([this] { return nowUs(); })
    , _demux(family.stream)
    , _protocol(std::move(protocol))
    , _context(*this)
    , _channel(*this)
{
    if (!_io.nowUs) {
        _io.nowUs = MonotonicClock::nowUs;
    }
    if (!_io.wait) {
        _io.wait = [cancelled = _io.isCancelled](std::chrono::microseconds duration) {
            const auto stopped = [&cancelled] { return cancelled && cancelled(); };
            const auto until = std::chrono::steady_clock::now() + duration;
            while (!stopped() && std::chrono::steady_clock::now() < until) {
                std::this_thread::sleep_for(
                    std::min<std::chrono::steady_clock::duration>(until - std::chrono::steady_clock::now(), 20ms));
            }
            return !stopped();
        };
    }
    if (!_protocol) {
        qCWarning(logCategory()) << "No protocol for receiver family" << _family.name;
    }
}

GPSProtocolRuntime::~GPSProtocolRuntime() = default;

GPSLogCategory GPSProtocolRuntime::logCategory() const
{
    return _family.logCategory ? _family.logCategory : &GPSProtocolRuntimeLog;
}

bool GPSProtocolRuntime::configure(const GPSConfig& config, unsigned& baud)
{
    if (_busy) {
        qCWarning(logCategory()) << "Receiver operation already in progress; configuration rejected";
        return false;
    }
    const QScopedValueRollback busy(_busy, true);
    _evidence.clear();
    _channel._resetError();
    if (!_protocol) {
        return false;
    }
    const QScopedValueRollback configuring(_configuring, true);
    _channel._detectedBaud = config.detectedBaud;
    auto task = _protocol->configure(_channel, config, baud);
    const bool configured = _drive(task);
    _channel.finishConfigurationEvidence();
    if (configured) {
        (void) consume({});
    }
    return configured;
}

namespace {

GPSTask<bool> probeWithin(GPSCommandChannel& channel, GPSFamilyProtocol& protocol, GPSProbeFunction probe,
                          std::chrono::milliseconds budget)
{
    const auto scope = channel.deadlineScope(budget);
    co_return co_await probe(channel, protocol);
}

}  // namespace

bool GPSProtocolRuntime::probe(std::chrono::milliseconds budget)
{
    if (_busy) {
        qCWarning(logCategory()) << "Receiver operation already in progress; probe rejected";
        return false;
    }
    const QScopedValueRollback busy(_busy, true);
    _evidence.clear();
    _channel._resetError();
    if (!_protocol || !_family.probe) {
        return false;
    }
    const QScopedValueRollback configuring(_configuring, true);
    auto task = probeWithin(_channel, *_protocol, _family.probe, budget);
    const bool answered = _drive(task);
    _channel.finishConfigurationEvidence();
    return answered;
}

GPSReceiveUpdates GPSProtocolRuntime::receive(std::chrono::milliseconds timeout)
{
    if (_busy) {
        qCWarning(logCategory()) << "Receiver operation already in progress; receive rejected";
        return {};
    }
    if (!_protocol) {
        return {};
    }
    const QScopedValueRollback busy(_busy, true);
    auto task = _protocol->receive(_channel, timeout);
    return _drive(task);
}

bool GPSProtocolRuntime::armDecodeOnly(const GPSConfig& config)
{
    if (_busy || !_protocol || !_channel.validateConfiguration(config)) {
        return false;
    }
    return _protocol->armDecodeOnly(config, _context);
}

GPSDecodedChunk GPSProtocolRuntime::decode(std::span<const uint8_t> bytes)
{
    if (!_protocol) {
        return {bytes.size(), {}};
    }
    _protocol->flush(_context);
    _demux.drainDeferred(*this);
    size_t consumed = 0;
    while (consumed < bytes.size() && _sink.hasRoomFor(GPSEventSink::FRAME_RESERVE)) {
        _demux.push(bytes[consumed++], *this);
    }
    _channel._offerRaw(bytes.first(consumed));
    return {consumed, _sink.takeBatch()};
}

GPSReceiveUpdates GPSProtocolRuntime::consume(std::span<const uint8_t> bytes)
{
    GPSReceiveUpdates updates;
    do {
        auto result = decode(bytes);
        bytes = bytes.subspan(result.bytesConsumed);
        updates |= result.batch.updates;
        if (_observer.decoded) {
            _observer.decoded(result.batch);
        }
        // A nested consume from the observer may already have refilled the sink's storage.
        _sink.recycle(std::move(result.batch.events));
    } while (!bytes.empty());
    return updates;
}

bool GPSProtocolRuntime::receiverReady() const
{
    return _protocol && _protocol->receiverReady(_context);
}

QString GPSProtocolRuntime::identity() const
{
    return _protocol ? _protocol->identity().trimmed() : QString();
}

void GPSProtocolRuntime::frame(const GPSFrame& frame)
{
    _sink.markUpdates(_protocol->onFrame(frame, _context));
    _channel._offerFrame(frame);
}

bool GPSProtocolRuntime::acceptDeferred()
{
    return _sink.hasRoomForDeferred();
}

template <typename T>
T GPSProtocolRuntime::_drive(GPSTask<T>& task)
{
    task.start();
    while (!task.done()) {
        if (!_request || !_continuation) {
            // A coroutine awaited something other than a channel operation, so nothing can ever resume it.
            qCCritical(logCategory()) << "Receiver coroutine suspended without an I/O request";
            std::terminate();
        }
        GPSIORequest& request = *std::exchange(_request, nullptr);
        const auto continuation = std::exchange(_continuation, {});
        _perform(request);
        continuation.resume();
    }
    return task.result();
}

void GPSProtocolRuntime::_perform(GPSIORequest& request)
{
    switch (request.kind) {
        case GPSIORequest::Kind::Read:
            request.readResult =
                _io.read ? _io.read(request.readBuffer, request.deadline) : GPSReadResult{GPSReadStatus::Error};
            break;
        case GPSIORequest::Kind::Write:
            request.writeResult = _io.write ? _io.write(request.writeBytes, request.deadline) : GPSWriteResult{};
            break;
        case GPSIORequest::Kind::SetBaudrate:
            request.baudStatus = _io.setBaudrate ? _io.setBaudrate(request.baudrate) : GPSBaudStatus::Unsupported;
            break;
        case GPSIORequest::Kind::Wait:
            request.waited = _io.wait(request.duration);
            break;
    }
}

void GPSProtocolRuntime::_post(GPSIORequest& request, std::coroutine_handle<> continuation)
{
    _request = &request;
    _continuation = continuation;
}

void GPSProtocolRuntime::_commandFinished(const GPSCommandResult& result)
{
    if (_configuring) {
        _evidence.push_back(result.evidence);
    }
    if (_observer.commandFinished) {
        _observer.commandFinished(result);
    }
}

uint64_t GPSDecodeContext::nowUs() const
{
    return _runtime.nowUs();
}

GPSEventSink& GPSDecodeContext::sink()
{
    return _runtime._sink;
}

GPSStreamDemux& GPSDecodeContext::stream()
{
    return _runtime._demux;
}

void GPSDecodeContext::drainDeferredFrames()
{
    _runtime._demux.drainDeferred(_runtime);
}

bool GPSDecodeContext::replyPending() const
{
    return _runtime._channel.replyPending();
}

void GPSDecodeContext::resolveReply(GPSCommandOutcome outcome)
{
    _runtime._channel.resolveReply(outcome);
}

void GPSDecodeContext::offerReply(std::string_view reply)
{
    _runtime._channel._offerText(reply);
}

bool GPSDecodeContext::failed() const
{
    return _runtime._channel.failed();
}

GPSProtocolError GPSDecodeContext::error() const
{
    return _runtime._channel.error();
}

const QString& GPSDecodeContext::errorDetail() const
{
    return _runtime._channel.errorDetail();
}

void GPSDecodeContext::failControl()
{
    _runtime._channel.failControl();
}

void GPSDecodeContext::setErrorDetail(const QString& detail)
{
    _runtime._channel.setErrorDetail(detail);
}

GPSTask<GPSReceiveUpdates> GPSFamilyProtocol::receive(GPSCommandChannel& channel, std::chrono::milliseconds timeout)
{
    const GPSReceiveUpdates updates = co_await channel.receiveCycle(timeout);
    co_await channel.serviceControls();
    co_return updates;
}

GPSTask<void> GPSFamilyProtocol::serviceStreaming(GPSCommandChannel&)
{
    co_return;
}
