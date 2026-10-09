#pragma once

#include <algorithm>
#include <chrono>
#include <concepts>
#include <cstdint>
#include <functional>
#include <optional>
#include <span>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#include <QtCore/QByteArray>
#include <QtCore/QByteArrayView>

#include "GPSCancellation.h"
#include "GPSProtocolRuntime.h"
#include "Protocols/Support/DetectionReceiver.h"
#include "Protocols/Support/GPSModelViolations.h"
#include "Protocols/Support/GPSTestClock.h"
#include "Protocols/Support/ScriptedFaults.h"
#include "Protocols/Support/ScriptedReceiver.h"

// The rig the protocol suites, the golden scenarios and the driver and worker suites drive a receiver family on: a
// receiver model behind fault rules on a scripted transport and a virtual clock, and a log of what the runtime
// delivered.
namespace GPSTest {

/// Runtime services on @p clock: waits advance it instead of sleeping and every baud rate is accepted. Tests add the
/// link, or take it from a ReceiverBench.
inline GPSRuntimeIO makeGPSRuntimeTestIO(GPSTestClock& clock)
{
    GPSRuntimeIO io;
    io.clock = clock.source();
    io.setBaudrate = [](unsigned) { return GPSBaudStatus::Configured; };
    return io;
}

/// Runtime services on @p clock for decoding without a receiver: touching the link records a violation and fails.
inline GPSRuntimeIO makeDecoderOnlyIO(GPSTestClock& clock)
{
    auto io = makeGPSRuntimeTestIO(clock);
    io.read = [](std::span<uint8_t>, GPSDeadline) {
        GPSTest::ModelViolations::record("Decoder read the device");
        return GPSReadResult{GPSReadStatus::Error};
    };
    io.write = [](std::span<const uint8_t>, GPSDeadline) {
        GPSTest::ModelViolations::record("Decoder wrote to the device");
        return GPSWriteResult{GPSWriteStatus::Error};
    };
    io.setBaudrate = [](unsigned) {
        GPSTest::ModelViolations::record("Decoder changed the baud rate");
        return GPSBaudStatus::Error;
    };
    return io;
}

/// What a runtime delivered: every event in order, and the evidence of every finished command.
struct EventLog
{
    std::vector<GPSProtocolEvent> events;
    std::vector<GPSConfigurationEvidence> commands;
    /// The latest position delivered.
    GPSDecodedPosition position;

    /// Records into this log, which must outlive the runtime.
    GPSRuntimeObserver observer()
    {
        GPSRuntimeObserver result;
        result.decoded = [this](const GPSEventBatch& batch) {
            for (const auto& event : batch.events) {
                if (const auto* report = std::get_if<GPSDecodedPosition>(&event)) {
                    position = *report;
                }
            }
            events.insert(events.end(), batch.events.begin(), batch.events.end());
        };
        result.commandFinished = [this](const GPSConfigurationEvidence& evidence) { commands.push_back(evidence); };
        return result;
    }

    template <typename T>
    std::vector<T> reports() const
    {
        std::vector<T> result;
        for (const auto& event : events) {
            if (const auto* report = std::get_if<T>(&event)) {
                result.push_back(*report);
            }
        }
        return result;
    }

    template <typename T>
    size_t count() const
    {
        return static_cast<size_t>(
            std::ranges::count_if(events, [](const auto& event) { return std::holds_alternative<T>(event); }));
    }

    /// The latest @a T delivered, or a default one.
    template <typename T>
    T latest() const
    {
        for (auto event = events.rbegin(); event != events.rend(); ++event) {
            if (const auto* report = std::get_if<T>(&*event)) {
                return *report;
            }
        }
        return T{};
    }

    /// Forgets the @a T events delivered so far.
    template <typename T>
    void clear()
    {
        std::erase_if(events, [](const auto& event) { return std::holds_alternative<T>(event); });
    }
};

/// A receiver family on the runtime, with a log of what it delivered.
struct LoggedRuntime : EventLog
{
    LoggedRuntime(const GPSReceiverFamily& family, GPSRuntimeIO io)
        : runtime(family, std::move(io), observer())
    {}

    GPSProtocolRuntime* operator->() { return &runtime; }

    GPSProtocolRuntime& operator*() { return runtime; }

    GPSProtocolRuntime runtime;
};

/// A scripted receiver on a virtual clock, with a log of what the runtime driving it delivered. It is the transport of
/// a GPSDriver or GPSReceiverWorker, or provides the runtime services of a GPSProtocolRuntime.
class ReceiverBench : public ScriptedReceiver
{
public:
    ReceiverBench(const ReceiverBench&) = delete;
    ReceiverBench& operator=(const ReceiverBench&) = delete;

    /// Prepares the receiver for a new connection.
    virtual void startSession() {}

    /// Starts a session without earlier output or commands, and returns runtime services over this receiver.
    GPSRuntimeIO io()
    {
        startSession();
        clearReplies();
        clearCommands();
        GPSRuntimeIO services = makeGPSRuntimeTestIO(clock);
        if (wait) {
            services.clock.wait = wait;
        }
        return makeIO(std::move(services));
    }

    GPSRuntimeObserver observer() { return log.observer(); }

    /// @a family's runtime over a new session of this receiver, delivering to log.
    GPSProtocolRuntime runtime(const GPSReceiverFamily& family) { return {family, io(), observer()}; }

    /// Whether a command starting with @a prefix was written.
    bool sent(QByteArrayView prefix) const
    {
        return std::ranges::any_of(commands(),
                                   [prefix](const QByteArray& command) { return command.startsWith(prefix); });
    }

    GPSTestClock& clock;
    /// Runtime waits; empty, they advance the clock.
    std::function<bool(std::chrono::microseconds)> wait;
    EventLog log;

protected:
    ReceiverBench(GPSCancelSource cancelSource, GPSTestClock& testClock)
        : ScriptedReceiver(std::move(cancelSource))
        , clock(testClock)
    {
        setClock(&clock);
    }

    ReceiverBench(GPSCancelToken cancelToken, GPSTestClock& testClock)
        : ScriptedReceiver(std::move(cancelToken))
        , clock(testClock)
    {
        setClock(&clock);
    }
};

/// A model whose receiver keeps state across connections and runs its own events: prepareSession() starts a
/// connection, wait() runs its events, and faultedRead() is a read its faults force.
template <typename ReceiverModel>
concept SessionModel = requires(ReceiverModel& model) {
    model.prepareSession();
    { model.wait(std::chrono::microseconds{}) } -> std::convertible_to<bool>;
    { model.faultedRead() } -> std::convertible_to<std::optional<GPSReadResult>>;
};

/// A ReceiverBench driven by @a ReceiverModel behind ScriptedFaults rules. The model is built from the constructor's
/// extra arguments, followed by the clock when it takes one.
template <typename ReceiverModel>
class ModelReceiver final : public ReceiverBench
{
public:
    template <typename... Args>
    explicit ModelReceiver(GPSTestClock& testClock, Args&&... args)
        : ReceiverBench(GPSCancelSource(), testClock)
        , model(_makeModel(testClock, std::forward<Args>(args)...))
        , faults(model)
    {
        _attach();
    }

    /// A transport that @a cancelToken stops, as a GPSReceiverWorker creates.
    template <typename... Args>
    ModelReceiver(GPSCancelToken cancelToken, GPSTestClock& testClock, Args&&... args)
        : ReceiverBench(std::move(cancelToken), testClock)
        , model(_makeModel(testClock, std::forward<Args>(args)...))
        , faults(model)
    {
        _attach();
    }

    void startSession() override
    {
        if constexpr (SessionModel<ReceiverModel>) {
            model.prepareSession();
        }
    }

    /// Puts receiver detection in front of the fault rules: the receiver parses only @a dialect, and answers and
    /// streams only while atRate() holds.
    DetectionReceiver& detect(Dialect dialect)
    {
        detection.emplace(faults, dialect, clock, [this] { return atRate(); });
        setModel(&*detection);
        setFixedBaudrate(0);
        detection->attach(*this);
        return *detection;
    }

    ReceiverModel model;
    ScriptedFaults faults;
    std::optional<DetectionReceiver> detection;
    /// For detect(): defaults to the rate setReceiverBaudrate() gave the receiver.
    std::function<bool()> atRate = [this] { return hostBaudrate() == receiverBaudrate(); };

private:
    template <typename... Args>
    static ReceiverModel _makeModel(GPSTestClock& clock, Args&&... args)
    {
        if constexpr (std::is_constructible_v<ReceiverModel, Args..., GPSTestClock&>) {
            return ReceiverModel(std::forward<Args>(args)..., clock);
        } else {
            return ReceiverModel(std::forward<Args>(args)...);
        }
    }

    void _attach()
    {
        setModel(&faults);
        if constexpr (SessionModel<ReceiverModel>) {
            wait = [this](std::chrono::microseconds delay) { return model.wait(delay); };
            setReadHandler([this](uint8_t*, int, std::chrono::milliseconds) { return model.faultedRead(); });
        }
    }
};

}  // namespace GPSTest
