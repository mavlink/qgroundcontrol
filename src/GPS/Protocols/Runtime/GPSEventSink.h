#pragma once

#include <chrono>
#include <cstddef>
#include <functional>
#include <optional>
#include <span>
#include <utility>
#include <vector>

#include "GPSEllipsoidPosition.h"
#include "GPSProtocolEvent.h"

/// Collects the events of one decode chunk. The runtime stops feeding bytes while fewer than FRAME_RESERVE slots
/// remain, so a completed frame can always publish its events; deferred output such as queued RTCM frames and
/// satellite systems drains only while it leaves DEFERRED_RESERVE slots for the frame being decoded. Events are never
/// coalesced: every position epoch is its own event, and consumers decide whether to drop superseded ones.
class GPSEventSink
{
public:
    static constexpr size_t MAX_EVENTS = 8;
    /// One completed frame can publish relative or survey data plus position, satellites and integrity.
    static constexpr size_t FRAME_RESERVE = 4;
    /// Space left for the position and vendor event of the current frame while deferred output drains.
    static constexpr size_t DEFERRED_RESERVE = 2;

    explicit GPSEventSink(std::function<uint64_t()> nowUs)
        : _nowUs(std::move(nowUs))
    {}

    [[nodiscard]] size_t size() const { return _batch.events.size(); }

    [[nodiscard]] bool hasRoomFor(size_t count) const { return _batch.events.size() + count <= MAX_EVENTS; }

    /// Whether one deferred event fits while keeping DEFERRED_RESERVE slots free.
    [[nodiscard]] bool hasRoomForDeferred() const { return hasRoomFor(1 + DEFERRED_RESERVE); }

    void publishPosition(const GPSDecodedPosition& report)
    {
        _batch.updates |= GPSReceiveUpdate::Position;
        _batch.events.emplace_back(report);
    }

    void publishSatellites(const GPSDecodedSatellites& report)
    {
        _batch.updates |= GPSReceiveUpdate::Satellites;
        _batch.events.emplace_back(report);
    }

    void publishSatelliteUsage(std::optional<int> count)
    {
        _batch.updates |= GPSReceiveUpdate::Satellites;
        _batch.events.emplace_back(GPSDecodedSatelliteUsage{_nowUs(), count});
    }

    /// Stamps the family's working @a report with the receipt time and publishes a copy.
    void publishIntegrity(GPSIntegrityReport& report)
    {
        report.timestampUs = _nowUs();
        _batch.events.emplace_back(report);
        _batch.updates |= GPSReceiveUpdate::Activity;
    }

    /// Stamps @a status with the receipt time and publishes a copy. Survey progress is not an update flag.
    void publishSurvey(GPSDecodedSurvey& status)
    {
        status.timestamp = _nowUs();
        _batch.events.emplace_back(status);
    }

    /// Publishes survey-in progress; unknown coordinates remain NaN.
    void publishSurvey(bool active, bool valid, std::chrono::seconds duration,
                       const GPSEllipsoidPosition& position = {})
    {
        GPSDecodedSurvey status{};
        status.survey.position = position;
        status.survey.duration = duration;
        status.survey.valid = valid;
        status.survey.active = active;
        publishSurvey(status);
    }

    void publishRTCM(std::span<const uint8_t> frame)
    {
        _batch.events.emplace_back(GPSRTCMFrame{
            QByteArray(reinterpret_cast<const char*>(frame.data()), static_cast<qsizetype>(frame.size()))});
        _batch.updates |= GPSReceiveUpdate::Activity;
    }

    /// Publishes @a event exactly as given: its timestamps are kept and no update flag is set. For reports a family
    /// buffered with their original receipt, such as a Quectel survey status held during boot verification.
    void publishAsReceived(GPSProtocolEvent event) { _batch.events.push_back(std::move(event)); }

    /// Adds updates a decoder reports for a frame without publishing an event, such as protocol activity.
    void markUpdates(GPSReceiveUpdates updates) { _batch.updates |= updates; }

    [[nodiscard]] GPSEventBatch takeBatch() { return std::exchange(_batch, {}); }

    /// Returns delivered storage so steady-state decoding does not allocate.
    void recycle(std::vector<GPSProtocolEvent>&& storage)
    {
        storage.clear();
        if (_batch.events.empty() && _batch.events.capacity() < storage.capacity()) {
            _batch.events.swap(storage);
        }
    }

private:
    std::function<uint64_t()> _nowUs;
    GPSEventBatch _batch;
};
