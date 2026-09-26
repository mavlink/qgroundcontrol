#pragma once

#include <chrono>
#include <coroutine>
#include <cstdint>
#include <memory>
#include <span>
#include <vector>

#include <QtCore/QString>

#include "GPSCommandChannel.h"
#include "GPSConfigurationEvidence.h"
#include "GPSEventSink.h"
#include "GPSFamilyProtocol.h"
#include "GPSProtocolError.h"
#include "GPSProtocolEvent.h"
#include "GPSReceiverFamily.h"
#include "GPSRuntimeIO.h"
#include "GPSStreamDemux.h"
#include "GPSTask.h"

/// One bounded decode step: the bytes it consumed and the events they produced.
struct [[nodiscard]] GPSDecodedChunk
{
    size_t bytesConsumed = 0;
    GPSEventBatch batch;
};

/// Hosts one receiver family on a blocking transport. It owns the family protocol, the stream demultiplexer, the
/// event sink, the command channel with its deadlines, evidence and sticky failure, and the I/O request the running
/// coroutine waits for. configure() and receive() drive the family's coroutines to completion on the caller thread,
/// performing each requested I/O operation and resuming the coroutine with its result.
class GPSProtocolRuntime final : private GPSStreamDemux::Handler
{
public:
    /// Creates the family's protocol with its factory.
    GPSProtocolRuntime(const GPSReceiverFamily& family, GPSRuntimeIO io, GPSRuntimeObserver observer = {},
                       const GPSFamilyOptions& options = {});
    /// Hosts @a protocol, for families configured by the caller such as test doubles.
    GPSProtocolRuntime(const GPSReceiverFamily& family, std::unique_ptr<GPSFamilyProtocol> protocol, GPSRuntimeIO io,
                       GPSRuntimeObserver observer = {});
    ~GPSProtocolRuntime();

    GPSProtocolRuntime(const GPSProtocolRuntime&) = delete;
    GPSProtocolRuntime& operator=(const GPSProtocolRuntime&) = delete;

    /// Configures the receiver after clearing any sticky failure. A zero @a baud requests detection and returns the
    /// detected rate. A command still pending at the end is retired as Written, and a successful configuration flushes
    /// once more so a fixed-base or survey-start event is delivered without another read.
    /// @return true when the receiver is ready; error() and errorDetail() describe a failure.
    [[nodiscard]] bool configure(const GPSConfig& config, unsigned& baud);

    /// Runs the family's identity probe (GPSReceiverFamily::probe) within @a budget at the link's current rate, after
    /// clearing any sticky failure; its commands are the configuration evidence. @return whether a receiver of this
    /// family answered; false for a family without a probe.
    [[nodiscard]] bool probe(std::chrono::milliseconds budget);

    /// One streaming receive of up to @a timeout, including the family's streaming services. Failures are reported
    /// through error(). @return the updates decoded.
    GPSReceiveUpdates receive(std::chrono::milliseconds timeout);

    /// Prepares decode() and consume() for traffic from a receiver configured with @a config, without I/O
    /// (GPSFamilyProtocol::armDecodeOnly()). @return false when the family needs a live configuration to report base
    /// status, or does not support @a config.
    [[nodiscard]] bool armDecodeOnly(const GPSConfig& config);

    /// Decodes one bounded chunk of @a bytes without I/O or observer callbacks; call again with the remainder.
    GPSDecodedChunk decode(std::span<const uint8_t> bytes);

    /// Decodes all of @a bytes, delivering each chunk's batch to the observer. An empty span flushes.
    GPSReceiveUpdates consume(std::span<const uint8_t> bytes);

    [[nodiscard]] bool receiverReady() const;

    /// Model and firmware reported during configuration, trimmed; empty when unknown.
    [[nodiscard]] QString identity() const;

    [[nodiscard]] GPSProtocolError error() const { return _channel.error(); }

    [[nodiscard]] const QString& errorDetail() const { return _channel.errorDetail(); }

    /// Evidence of the latest configure() attempt, in completion order.
    [[nodiscard]] const std::vector<GPSConfigurationEvidence>& configurationEvidence() const { return _evidence; }

    [[nodiscard]] const GPSReceiverFamily& family() const { return _family; }

    [[nodiscard]] GPSFamilyProtocol& protocol() { return *_protocol; }

    [[nodiscard]] GPSStreamDemux& stream() { return _demux; }

    [[nodiscard]] uint64_t nowUs() const { return _io.nowUs(); }

    /// The family's logging category, or the runtime's when the family has none.
    [[nodiscard]] GPSLogCategory logCategory() const;

private:
    friend class GPSCommandChannel;
    friend class GPSDecodeContext;

    void frame(const GPSFrame& frame) override;
    bool acceptDeferred() override;

    template <typename T>
    T _drive(GPSTask<T>& task);
    void _perform(GPSIORequest& request);
    void _post(GPSIORequest& request, std::coroutine_handle<> continuation);
    void _commandFinished(const GPSCommandResult& result);

    const GPSReceiverFamily& _family;
    GPSRuntimeIO _io;
    GPSRuntimeObserver _observer;
    GPSEventSink _sink;
    GPSStreamDemux _demux;
    std::unique_ptr<GPSFamilyProtocol> _protocol;
    GPSDecodeContext _context;
    GPSCommandChannel _channel;
    std::vector<GPSConfigurationEvidence> _evidence;
    GPSIORequest* _request = nullptr;
    std::coroutine_handle<> _continuation;
    bool _busy = false;
    bool _configuring = false;
};
