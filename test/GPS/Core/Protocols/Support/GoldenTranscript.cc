#include "GoldenTranscript.h"

#include <algorithm>
#include <memory>
#include <span>
#include <type_traits>
#include <utility>

#include <QtCore/QMetaEnum>

#include "GPSProtocolRuntime.h"
#include "GPSReceiverDescriptor.h"
#include "GPSReceiverDetector.h"
#include "GPSReceiverFamilies.h"
#include "Protocols/Support/GPSProtocolTestBase.h"
#include "Protocols/Support/GPSTestClock.h"
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
            } else if constexpr (std::is_same_v<Report, GPSSurveyReport>) {
                return Survey{report.timestampUs, report};
            } else if constexpr (std::is_same_v<Report, GPSRTCMFrame>) {
                return RTCM{report.bytes};
            } else {
                return report;
            }
        },
        decoded);
}

/// Records every protocol service call. While offline, transport services fail and are only counted.
class Session
{
public:
    Session(const Link& link, std::vector<Record>& records)
        : _link(link)
        , _records(&records)
    {}

    Phase phase = Phase::Configure;
    bool offline = false;
    int transportCalls = 0;
    std::vector<Event>* events = nullptr;

    GPSRuntimeObserver observer()
    {
        GPSRuntimeObserver result;
        result.commandFinished = [this](const GPSConfigurationEvidence& command) {
            _records->push_back({phase, command});
        };
        result.decoded = [this](const GPSEventBatch& batch) {
            for (const auto& event : batch.events) {
                if (events) {
                    events->push_back(eventOf(event));
                } else {
                    _records->push_back({phase, eventOf(event)});
                }
            }
        };
        return result;
    }

    GPSRuntimeIO io()
    {
        const auto transport = _link.receiver.makeIO({});
        GPSRuntimeIO services;
        services.clock.nowUs = [this] { return _link.clock.nowUs(); };
        services.clock.wait = [this](std::chrono::microseconds duration) {
            if (offline) {
                ++transportCalls;
                return true;
            }
            if (_link.wait) {
                return _link.wait(duration);
            }
            _link.clock.advanceBy(static_cast<uint64_t>(duration.count()));
            return true;
        };
        services.read = [this, read = transport.read](std::span<uint8_t> bytes, GPSDeadline deadline) {
            if (offline) {
                ++transportCalls;
                return GPSReadResult{GPSReadStatus::Error};
            }
            const auto result = read(bytes, deadline);
            // A blocking read that times out has consumed its whole deadline.
            if (result.status == GPSReadStatus::TimedOut && _link.clock.nowUs() < deadline.untilUs) {
                _link.clock.advanceTo(deadline.untilUs);
            }
            return result;
        };
        services.write = [this, write = transport.write](std::span<const uint8_t> bytes, GPSDeadline deadline) {
            if (offline) {
                ++transportCalls;
                return GPSWriteResult{GPSWriteStatus::Error};
            }
            const size_t index = _records->size();
            _records->push_back({phase, Write{QByteArray(reinterpret_cast<const char*>(bytes.data()),
                                                         static_cast<qsizetype>(bytes.size()))}});
            const auto result = write(bytes, deadline);
            auto& recorded = std::get<Write>((*_records)[index].value);
            recorded.status = result.status;
            recorded.acceptedBytes = result.acceptedBytes;
            recorded.writtenBytes = result.writtenBytes;
            return result;
        };
        services.setBaudrate = [this, setBaudrate = transport.setBaudrate](unsigned rate) {
            if (offline) {
                ++transportCalls;
                return GPSBaudStatus::Error;
            }
            const auto status = setBaudrate(rate);
            _records->push_back({phase, Baud{rate, status}});
            return status;
        };
        return services;
    }

private:
    const Link& _link;
    std::vector<Record>* _records;
};

/// Configures @a family as GPSDriver::_configureFamily() does; @a detected prefixes its error.
std::unique_ptr<GPSProtocolRuntime> configureFamily(const GPSReceiverFamily& family, const GPSConfig& config,
                                                    unsigned baud, const QString& detected, const Link& link,
                                                    Session& session, Run& run)
{
    auto runtime = std::make_unique<GPSProtocolRuntime>(family, session.io(), session.observer());
    run.requestedBaud = baud;
    run.configured = runtime->configure(config, baud);
    run.configuredAtUs = link.clock.nowUs();
    run.baud = baud;
    run.failure = failureOf(runtime->error());
    run.ready = runtime->receiverReady();
    run.identity = runtime->identity();
    if (!run.configured) {
        run.error = runtime->errorDetail();
        if (run.error.isEmpty()) {
            run.error = QStringLiteral("Receiver configuration failed");
        }
        if (!detected.isEmpty()) {
            run.error = QStringLiteral("%1: %2").arg(detected, run.error);
        }
    }
    return runtime;
}

/// GPSDriver::_configureDetected() on the virtual clock.
std::unique_ptr<GPSProtocolRuntime> configureDetected(const Scenario& scenario, unsigned baud, const Link& link,
                                                      Session& session, Run& run)
{
    run.requestedBaud = baud;
    const auto detection =
        GPSReceiverDetector(gpsReceiverFamilies(), session.io(), session.observer().commandFinished).detect(baud);
    run.configuredAtUs = link.clock.nowUs();
    run.detectionRecords = run.records.size();
    if (!detection.found()) {
        run.failure = failureOf(detection.error);
        run.error = !detection.errorDetail.isEmpty() ? detection.errorDetail
                    : detection.error == GPSProtocolError::Cancelled
                        ? QStringLiteral("Receiver detection cancelled")
                        : QStringLiteral("No supported receiver was identified");
        return {};
    }
    const GPSType type = detection.family->type;
    run.detected = type;
    run.detectedBaud = detection.baud;
    run.detectedEvidence = detection.evidence;
    const GPSReceiverConfig config = gpsReceiverConfigForDetected(type, scenario.config);
    if (const QString error = gpsDetectedReceiverConfigError(type, config); !error.isEmpty()) {
        run.error = error;
        return {};
    }
    return configureFamily(
        *detection.family,
        {.base = config.base, .allowPersistentChanges = config.allowPersistentChanges, .detectedBaud = detection.baud},
        baud, QStringLiteral("Detected %1 receiver at %2 baud").arg(gpsReceiverName(type)).arg(detection.baud), link,
        session, run);
}

/// GPSDriver::configure() on the virtual clock, through the same family table and runtime, for a request the driver
/// accepts (GPSDriverTest covers refused requests).
std::unique_ptr<GPSProtocolRuntime> configure(const Scenario& scenario, const Link& link, Session& session, Run& run)
{
    run.startedAtUs = link.clock.nowUs();
    run.configuredAtUs = run.startedAtUs;
    unsigned baud = scenario.config.baudRate ? scenario.config.baudRate : link.receiver.fixedBaudrate();
    if (scenario.type == GPSType::automatic) {
        return configureDetected(scenario, baud, link, session, run);
    }
    const GPSReceiverFamily* family = gpsReceiverFamily(scenario.type);
    if (!family) {
        run.error =
            QStringLiteral("Unsupported GPS type: %1")
                .arg(QLatin1StringView(QMetaEnum::fromType<GPSType>().valueToKey(static_cast<int>(scenario.type))));
        return {};
    }
    if (family->autoBaudRate && !baud) {
        baud = family->autoBaudRate;
    }
    return configureFamily(
        *family, {.base = scenario.config.base, .allowPersistentChanges = scenario.config.allowPersistentChanges}, baud,
        {}, link, session, run);
}

bool terminal(const GPSProtocolRuntime& runtime, const ScriptedReceiver& receiver)
{
    return runtime.error() != GPSProtocolError::None || receiver.isCancelled() || receiver.fatalError();
}

/// Decodes @a bytes in chunks with the receiver unreachable, then nothing after FRESHNESS_HORIZON.
void decodeOffline(GPSProtocolRuntime& runtime, Session& session, const Link& link, QByteArrayView bytes,
                   qsizetype chunk, Decode& result)
{
    session.offline = true;
    session.events = &result.events;
    const auto* data = reinterpret_cast<const uint8_t*>(bytes.data());
    for (qsizetype offset = 0; offset < bytes.size();) {
        const qsizetype count = chunk > 0 ? std::min(chunk, bytes.size() - offset) : bytes.size() - offset;
        (void) runtime.consume({data + offset, static_cast<size_t>(count)});
        offset += count;
    }
    link.clock.advanceBy(static_cast<uint64_t>(FRESHNESS_HORIZON.count()));
    session.events = &result.expired;
    (void) runtime.consume({});
    result.transportCalls = session.transportCalls;
}

}  // namespace

Run runGolden(const Scenario& scenario, const Link& link)
{
    // Diagnostics are not transcribed; failure scenarios warn by design.
    const GPSProtocolLogCapture diagnostics;
    Run run;
    Session session(link, run.records);
    const auto runtime = configure(scenario, link, session, run);
    if (!runtime || !run.configured) {
        return run;
    }
    session.phase = Phase::Stream;
    for (const auto& step : scenario.stream) {
        if (!step.bytes.isEmpty()) {
            link.receiver.queueReply(step.bytes);
        }
        const uint64_t until =
            link.clock.nowUs() +
            static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::microseconds>(step.duration).count());
        do {
            (void) runtime->receive(RECEIVE_TIMEOUT);
            ++run.receiveCalls;
        } while (!terminal(*runtime, link.receiver) && link.clock.nowUs() < until &&
                 run.receiveCalls < MAX_RECEIVE_CALLS);
        if (terminal(*runtime, link.receiver) || run.receiveCalls >= MAX_RECEIVE_CALLS) {
            break;
        }
    }
    run.streamFailure = failureOf(runtime->error());
    run.streamReady = runtime->receiverReady();
    return run;
}

Decode decodeGolden(const Scenario& setup, const Link& link, QByteArrayView bytes, qsizetype chunk)
{
    const GPSProtocolLogCapture diagnostics;
    Decode result;
    Run run;
    Session session(link, run.records);
    const auto runtime = configure(setup, link, session, run);
    result.configured = run.configured;
    if (runtime) {
        decodeOffline(*runtime, session, link, bytes, chunk, result);
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
    std::vector<Record> records;
    Session session(link, records);
    session.offline = true;
    GPSProtocolRuntime runtime(*family, session.io(), session.observer());
    result.configured = runtime.armDecodeOnly(
        {.base = setup.config.base, .allowPersistentChanges = setup.config.allowPersistentChanges});
    if (result.configured) {
        decodeOffline(runtime, session, link, bytes, chunk, result);
    }
    return result;
}

}  // namespace GPSTest::Golden
