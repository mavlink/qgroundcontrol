#include "GPSProtocolRuntime.h"

#include <algorithm>
#include <thread>
#include <utility>

#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(GPSProtocolRuntimeLog, "GPS.Protocols.Runtime")

using namespace std::chrono_literals;

GPSProtocolRuntime::GPSProtocolRuntime(const GPSReceiverFamily& family, GPSRuntimeIO io, GPSRuntimeObserver observer)
    : GPSProtocolRuntime(family, family.create ? family.create() : nullptr, std::move(io), std::move(observer))
{}

GPSProtocolRuntime::GPSProtocolRuntime(const GPSReceiverFamily& family, std::unique_ptr<GPSFamilyProtocol> protocol,
                                       GPSRuntimeIO io, GPSRuntimeObserver observer)
    : _family(family)
    , _io(std::move(io))
    , _observer(std::move(observer))
    , _demux(family.stream)
    , _protocol(std::move(protocol))
    , _channel(*this)
    , _context(_channel)
{
    if (!_io.read) {
        _io.read = [](std::span<uint8_t>, GPSDeadline) { return GPSReadResult{GPSReadStatus::Error}; };
    }
    if (!_io.write) {
        _io.write = [](std::span<const uint8_t>, GPSDeadline) { return GPSWriteResult{}; };
    }
    if (!_io.setBaudrate) {
        _io.setBaudrate = [](unsigned) { return GPSBaudStatus::Unsupported; };
    }
    if (!_io.clock.wait) {
        _io.clock.wait = [token = _io.cancelToken](std::chrono::microseconds duration) {
            const auto stopped = [&token] { return token.isCancelled(); };
            const auto until = std::chrono::steady_clock::now() + duration;
            while (!stopped() && std::chrono::steady_clock::now() < until) {
                std::this_thread::sleep_for(
                    std::min<std::chrono::steady_clock::duration>(until - std::chrono::steady_clock::now(), 20ms));
            }
            return !stopped();
        };
    }
    if (!_protocol) {
        qCWarning(logCategory()) << "No protocol for receiver family" << _family.type;
    }
}

GPSProtocolRuntime::~GPSProtocolRuntime() = default;

GPSLogCategory GPSProtocolRuntime::logCategory() const
{
    return _family.logCategory ? _family.logCategory : &GPSProtocolRuntimeLog;
}

bool GPSProtocolRuntime::configure(const GPSConfig& config, unsigned& baud)
{
    _channel._resetError();
    if (!_protocol) {
        return false;
    }
    _channel._detectedBaud = config.detectedBaud;
    const bool configured = _protocol->configure(_channel, config, baud);
    _channel.finishCommand(GPSCommandOutcome::Written);
    if (configured) {
        (void) consume({});
    } else {
        // Every failed configuration reports a failure, keeping the detail the family left.
        _channel._describeFailure();
        _channel.failControl(_channel.errorDetail());
    }
    return configured;
}

bool GPSProtocolRuntime::probe(std::chrono::milliseconds budget)
{
    _channel._resetError();
    if (!_protocol) {
        return false;
    }
    bool answered = false;
    {
        const auto scope = _channel.deadlineScope(budget);
        answered = _protocol->probe(_channel);
    }
    _channel.finishCommand(GPSCommandOutcome::Written);
    return answered;
}

GPSReceiveUpdates GPSProtocolRuntime::receive(std::chrono::milliseconds timeout)
{
    if (!_protocol) {
        return {};
    }
    return _protocol->receive(_channel, timeout);
}

bool GPSProtocolRuntime::armDecodeOnly(const GPSConfig& config)
{
    if (!_protocol) {
        return false;
    }
    return _protocol->armDecodeOnly(config, _context);
}

bool GPSProtocolRuntime::armNavigationDecode(GPSNavigationDecode decode)
{
    return _protocol && _protocol->armNavigationDecode(decode, _context);
}

GPSEventBatch GPSProtocolRuntime::decode(std::span<const uint8_t> bytes)
{
    if (!_protocol) {
        return {};
    }
    _protocol->flush(_context);
    for (const uint8_t byte : bytes) {
        _demux.push(byte, *this);
    }
    _channel._offerRaw(bytes);
    return _context._takeBatch();
}

GPSReceiveUpdates GPSProtocolRuntime::consume(std::span<const uint8_t> bytes)
{
    return _deliver(decode(bytes));
}

GPSReceiveUpdates GPSProtocolRuntime::consumeFrame(const GPSFrame& frame)
{
    if (!_protocol) {
        return {};
    }
    GPSProtocolRuntime::frame(frame);
    return _deliver(_context._takeBatch());
}

GPSReceiveUpdates GPSProtocolRuntime::_deliver(GPSEventBatch batch)
{
    if (_observer.decoded) {
        _observer.decoded(batch);
    }
    // A nested consume from the observer may already have refilled the context's storage.
    _context._recycle(std::move(batch.events));
    return batch.updates;
}

bool GPSProtocolRuntime::receiverReady() const
{
    return _protocol && !_context.failed() && _protocol->receiverReady();
}

QString GPSProtocolRuntime::identity() const
{
    return _protocol ? _protocol->identity().trimmed() : QString();
}

void GPSProtocolRuntime::frame(const GPSFrame& frame)
{
    _context.markUpdates(_protocol->onFrame(frame, _context));
    _channel._offerFrame(frame);
}

void GPSProtocolRuntime::_commandFinished(const GPSConfigurationEvidence& result)
{
    if (_observer.commandFinished) {
        _observer.commandFinished(result);
    }
}

uint64_t GPSDecodeContext::nowUs() const
{
    return _channel.nowUs();
}

GPSStreamDemux& GPSDecodeContext::stream()
{
    return _channel.stream();
}

bool GPSDecodeContext::replyPending() const
{
    return _channel._replyPending();
}

void GPSDecodeContext::resolveReply(GPSCommandOutcome outcome)
{
    _channel._resolveReply(outcome);
}

void GPSDecodeContext::offerReply(std::string_view reply)
{
    _channel._offerText(reply);
}

bool GPSDecodeContext::failed() const
{
    return _channel.failed();
}

void GPSDecodeContext::failControl(const QString& detail)
{
    _channel.failControl(detail);
}

GPSReceiveUpdates GPSFamilyProtocol::receive(GPSCommandChannel& channel, std::chrono::milliseconds timeout)
{
    const GPSReceiveUpdates updates = channel.receiveCycle(timeout);
    channel.serviceControls();
    return updates;
}

void GPSFamilyProtocol::serviceStreaming(GPSCommandChannel&) {}
