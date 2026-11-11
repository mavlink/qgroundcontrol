#include "GoldenTranscript.h"

#include <algorithm>
#include <span>
#include <type_traits>
#include <utility>

#include "GPSDriver.h"
#include "GPSProtocolRuntime.h"
#include "GPSReceiverDetector.h"
#include "GPSReceiverFamilies.h"
#include "Protocols/Support/GPSProtocolTestBase.h"
#include "Protocols/Support/GPSTestClock.h"
#include "Protocols/Support/ReceiverBench.h"
#include "Protocols/Support/ScriptedReceiver.h"

using namespace GPSTest;

namespace GPSTest::Golden {
namespace {

Failure failureOf(GPSProtocolError error)
{
    switch (error) {
        case GPSProtocolError::None:
            return Failure::None;
        case GPSProtocolError::Cancelled:
            return Failure::Cancelled;
        case GPSProtocolError::Transport:
            return Failure::Transport;
        case GPSProtocolError::Protocol:
            return Failure::Protocol;
        case GPSProtocolError::InvalidArgument:
            return Failure::InvalidArgument;
        case GPSProtocolError::ConsentRequired:
            return Failure::ConsentRequired;
    }
    return Failure::Protocol;
}

Event eventOf(const GPSProtocolEvent& decoded)
{
    return std::visit(
        [](const auto& report) -> Event {
            using Report = std::decay_t<decltype(report)>;
            if constexpr (std::is_same_v<Report, GPSDecodedPosition>) {
                return Position{report.navigation, report.velocityValid};
            } else if constexpr (std::is_same_v<Report, GPSIntegrityReport>) {
                return report;
            } else if constexpr (std::is_same_v<Report, GPSDecodedSatellites>) {
                Satellites satellites{.fullSnapshot = report.fullSnapshot};
                const size_t count = std::min<size_t>(report.count, report.constellations.size());
                for (size_t index = 0; index < count; ++index) {
                    const auto& system = report.constellations[index];
                    satellites.systems.push_back({system.constellation, system.inView, system.inUse,
                                                  system.inViewTimestampUs, system.inUseTimestampUs});
                }
                return satellites;
            } else if constexpr (std::is_same_v<Report, GPSDecodedSatelliteUsage>) {
                return SatelliteUsage{report.timestampUs, report.usedCount};
            } else if constexpr (std::is_same_v<Report, GPSBaseStatus>) {
                return Survey{report.timestampUs, report};
            } else if constexpr (std::is_same_v<Report, GPSRTCMFrame>) {
                return RTCM{report.bytes};
            } else {
                return report;
            }
        },
        decoded);
}

/// GPSDriver for a scenario on its Link, and the record of what it does. It is the driver's transport, which forwards
/// to Link::receiver and records each write and baud change, and it observes the driver's commands and decoded events.
/// While offline, transport services fail and are only counted.
class Session final : public GPSTransport
{
public:
    Session(const Scenario& scenario, const Link& link, Run& run)
        : GPSTransport(link.receiver.cancelToken())
        , _link(link)
        , _run(run)
        , _driver(scenario.type, *this, scenario.config, {}, _clock(), _observer())
    {
        // Transport reads wait on the link's clock: one that finds no data ends at its deadline.
        link.receiver.setClock(&link.clock);
    }

    Phase phase = Phase::Configure;
    bool offline = false;
    int transportCalls = 0;
    /// Decoded events go here instead of the records.
    std::vector<Event>* events = nullptr;

    /// GPSDriver::configure(), whose outcome the observer records.
    bool configure()
    {
        _run.startedAtUs = _link.clock.nowUs();
        _run.configured = _driver.configure();
        _run.configuredAtUs = _link.clock.nowUs();
        _run.error = _driver.configurationError();
        if (!_run.configured) {
            _runtime = nullptr;
        }
        return _run.configured;
    }

    GPSReceiveResult receive() { return _driver.receiveOutcome(RECEIVE_TIMEOUT); }

    /// The driver's runtime after a successful configure(), else null.
    GPSProtocolRuntime* runtime() { return _runtime; }

    GPSOpenResult open() override { return _link.receiver.open(); }

    bool fatalError() const override { return _link.receiver.fatalError(); }

    unsigned fixedBaudRate() const override { return _link.receiver.fixedBaudRate(); }

    std::chrono::milliseconds configurationWriteTimeout() const override
    {
        return _link.receiver.configurationWriteTimeout();
    }

    bool setBaudRate(unsigned rate) override
    {
        if (offline) {
            ++transportCalls;
            return false;
        }
        const bool accepted = _link.receiver.setBaudRate(rate);
        _run.records.push_back({phase, Baud{rate, accepted}});
        return accepted;
    }

    GPSReadResult read(std::span<uint8_t> bytes, std::chrono::milliseconds timeout) override
    {
        if (offline) {
            ++transportCalls;
            return {GPSReadStatus::Error};
        }
        return _link.receiver.read(bytes, timeout);
    }

protected:
    GPSWriteResult writeData(QByteArrayView bytes, QDeadlineTimer deadline) override
    {
        if (offline) {
            ++transportCalls;
            return {GPSWriteStatus::Error};
        }
        const size_t index = _run.records.size();
        _run.records.push_back({phase, Write{bytes.toByteArray()}});
        const auto result = _link.receiver.write(bytes, deadline);
        auto& recorded = std::get<Write>(_run.records[index].value);
        recorded.status = result.status;
        recorded.acceptedBytes = result.acceptedBytes;
        recorded.writtenBytes = result.writtenBytes;
        return result;
    }

private:
    GPSClock _clock()
    {
        return {.nowUs = [this] { return _link.clock.nowUs(); },
                .wait =
                    [this](std::chrono::microseconds duration) {
                        if (offline) {
                            ++transportCalls;
                            return true;
                        }
                        if (_link.wait) {
                            return _link.wait(duration);
                        }
                        _link.clock.advanceBy(duration);
                        return true;
                    }};
    }

    GPSDriverObserver _observer()
    {
        GPSDriverObserver observer;
        observer.commandFinished = [this](const GPSConfigurationEvidence& command) {
            _run.records.push_back({phase, command});
        };
        observer.decoded = [this](const GPSEventBatch& batch) {
            for (const auto& event : batch.events) {
                if (events) {
                    events->push_back(eventOf(event));
                } else {
                    _run.records.push_back({phase, eventOf(event)});
                }
            }
        };
        observer.detected = [this](const GPSReceiverDetection& detection, unsigned baud) {
            _run.requestedBaud = baud;
            _run.detectionRecords = _run.records.size();
            _run.failure = failureOf(detection.error);
            if (detection.found()) {
                _run.detected = detection.family->type;
                _run.detectedBaud = detection.baud;
                _run.detectedEvidence = detection.evidence;
            }
        };
        observer.configured = [this](GPSProtocolRuntime& runtime, unsigned requestedBaud, unsigned baud) {
            _runtime = &runtime;
            _run.requestedBaud = requestedBaud;
            _run.baud = baud;
            _run.failure = failureOf(runtime.error());
            _run.ready = runtime.receiverReady();
            _run.identity = runtime.identity();
        };
        return observer;
    }

    const Link& _link;
    Run& _run;
    GPSProtocolRuntime* _runtime = nullptr;
    GPSDriver _driver;
};

/// Decodes @a bytes in chunks through @a runtime into Decode::events, then nothing after FRESHNESS_HORIZON into
/// Decode::expired. @a target is the list the runtime's observer records into.
void decodeOffline(GPSProtocolRuntime& runtime, const Link& link, QByteArrayView bytes, qsizetype chunk,
                   std::vector<Event>*& target, Decode& result)
{
    target = &result.events;
    const auto* data = reinterpret_cast<const uint8_t*>(bytes.data());
    for (qsizetype offset = 0; offset < bytes.size();) {
        const qsizetype count = chunk > 0 ? std::min(chunk, bytes.size() - offset) : bytes.size() - offset;
        (void) runtime.consume({data + offset, static_cast<size_t>(count)});
        offset += count;
    }
    link.clock.advanceBy(static_cast<uint64_t>(FRESHNESS_HORIZON.count()));
    target = &result.expired;
    (void) runtime.consume({});
}

}  // namespace

Run runGolden(const Scenario& scenario, const Link& link)
{
    // Diagnostics are not transcribed; failure scenarios warn by design.
    const GPSProtocolLogCapture diagnostics;
    Run run;
    Session session(scenario, link, run);
    if (!session.configure()) {
        return run;
    }
    session.phase = Phase::Stream;
    GPSReceiveResult outcome;
    for (const auto& step : scenario.stream) {
        if (!step.bytes.isEmpty()) {
            link.receiver.queueReply(step.bytes);
        }
        const uint64_t until =
            link.clock.nowUs() +
            static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::microseconds>(step.duration).count());
        do {
            outcome = session.receive();
            ++run.receiveCalls;
        } while (outcome.error == GPSProtocolError::None && link.clock.nowUs() < until &&
                 run.receiveCalls < MAX_RECEIVE_CALLS);
        if (outcome.error != GPSProtocolError::None || run.receiveCalls >= MAX_RECEIVE_CALLS) {
            break;
        }
    }
    run.streamFailure = failureOf(outcome.error);
    run.streamReady = session.runtime()->receiverReady();
    return run;
}

Decode decodeGolden(const Scenario& setup, const Link& link, QByteArrayView bytes, qsizetype chunk)
{
    const GPSProtocolLogCapture diagnostics;
    Decode result;
    Run run;
    Session session(setup, link, run);
    result.configured = session.configure();
    if (result.configured) {
        session.offline = true;
        decodeOffline(*session.runtime(), link, bytes, chunk, session.events, result);
        result.transportCalls = session.transportCalls;
    }
    return result;
}

Decode armedDecodeGolden(const Scenario& setup, const Link& link, QByteArrayView bytes, qsizetype chunk)
{
    const GPSProtocolLogCapture diagnostics;
    Decode result;
    const GPSReceiverFamily* family = gpsReceiverFamily(setup.type);
    if (!family) {
        return result;
    }
    std::vector<Event>* target = nullptr;
    GPSRuntimeObserver observer;
    observer.decoded = [&target](const GPSEventBatch& batch) {
        for (const auto& event : batch.events) {
            target->push_back(eventOf(event));
        }
    };
    // Touching the device is a model violation, which fails the test.
    GPSProtocolRuntime runtime(*family, makeDecoderOnlyIO(link.clock), std::move(observer));
    result.configured = runtime.armDecodeOnly(
        {.base = setup.config.base, .allowPersistentChanges = setup.config.allowPersistentChanges});
    if (result.configured) {
        decodeOffline(runtime, link, bytes, chunk, target, result);
    }
    return result;
}

}  // namespace GPSTest::Golden
