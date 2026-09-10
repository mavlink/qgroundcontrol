#include "GPSReplayDriver.h"

#include <algorithm>
#include <utility>

std::unique_ptr<GPSDriver> createGPSReplayDriver(GPSReplayTransport& transport, GPSDriverSinks sinks, QString& error)
{
    const auto& trace = transport.trace();
    if (!trace.profile || !trace.profile->configured || trace.profile->driverType < 0) {
        error = QStringLiteral("A configured receiver profile is required; passive traces replay through NMEA instead");
        return {};
    }
    const auto& metadata = *trace.profile;
    if (trace.limitReached || std::any_of(trace.recordedEvents.cbegin(), trace.recordedEvents.cend(),
                                          [](const auto& event) { return event.resumed; })) {
        error = QStringLiteral("Driver configuration cannot be reproduced from a truncated or resumed capture");
        return {};
    }
    using K = GPSRecordingEvent::Kind;
    const auto started = std::find_if(trace.recordedEvents.cbegin(), trace.recordedEvents.cend(),
                                      [](const auto& event) { return event.kind == K::ConfigurationStarted; });
    if (started == trace.recordedEvents.cend()) {
        error = QStringLiteral("Capture must include the receiver configuration start");
        return {};
    }
    if (metadata.transport == GPSRecordingMetadata::Transport::Unknown && !metadata.fixedBaud) {
        error = QStringLiteral("Capture does not identify whether transport baud is fixed; supply explicit metadata");
        return {};
    }
    error = metadata.receiver.validationError();
    if (!error.isEmpty()) {
        return {};
    }
    GPSType type;
    switch (metadata.driverType) {
        case static_cast<int>(GPSType::u_blox):
            type = GPSType::u_blox;
            break;
        case static_cast<int>(GPSType::trimble):
            type = GPSType::trimble;
            break;
        case static_cast<int>(GPSType::septentrio):
            type = GPSType::septentrio;
            break;
        case static_cast<int>(GPSType::femto):
            type = GPSType::femto;
            break;
        default:
            error = QStringLiteral("Unknown receiver driver");
            return {};
    }
    GPSDriverClock clock;
    clock.nowUs = [&transport] { return transport.clock().nowUs(); };
    clock.wait = [&transport](std::chrono::microseconds duration) { transport.clock().advanceBy(duration.count()); };
    return std::make_unique<GPSDriver>(type, transport, metadata.receiver, std::move(sinks), std::move(clock));
}
