#include <chrono>
#include <cmath>
#include <limits>

#include "GPSExecutionContext.h"
#include "GPSProtocolIO.h"
#include "GPSReceiverConfig.h"
#include "GPSRelativeReport.h"
#include "MonotonicClock.h"

int main()
{
    static_assert(MonotonicClock::ageMilliseconds(0, 1000) == -1);
    static_assert(MonotonicClock::ageMilliseconds(1001, 1000) == -1);
    static_assert(MonotonicClock::ageMilliseconds(1000, 1000) == 0);
    static_assert(MonotonicClock::ageMilliseconds(1, 1000) == 0);
    static_assert(MonotonicClock::ageMilliseconds(1, 1001) == 1);
    static_assert(MonotonicClock::ageMilliseconds(1, std::numeric_limits<uint64_t>::max()) ==
                  static_cast<int64_t>((std::numeric_limits<uint64_t>::max() - 1) / 1000));
    const GPSExecutionContext context;
    const auto before = MonotonicClock::nowUs();
    const auto sampled = context.nowUs();
    if (sampled < before || sampled > MonotonicClock::nowUs()) {
        return 4;
    }
    const GPSRelativeReport relative;
    if (relative.timestamp != 0 || relative.timestamp_sample != 0 || relative.time_utc_usec != 0 ||
        relative.gnss_fix_ok || relative.differential_solution || relative.relative_position_valid ||
        relative.carrier_solution_floating || relative.carrier_solution_fixed || relative.heading_valid ||
        relative.reference_station_id || relative.moving_base_mode || relative.reference_position_miss ||
        relative.reference_observations_miss || relative.relative_position_normalized ||
        !std::isnan(relative.heading) || !std::isnan(relative.heading_accuracy) ||
        !std::isnan(relative.position_length) || !std::isnan(relative.accuracy_length)) {
        return 5;
    }
    for (int i = 0; i < 3; ++i) {
        if (!std::isnan(relative.position[i]) || !std::isnan(relative.position_accuracy[i])) {
            return 6;
        }
    }
    constexpr GPSReceiverConfig config;
    static_assert(config.role == GPSReceiverConfig::Role::RTKBase);
    GPSDecodedBatch batch;
    batch.events.emplace_back(GPSPositionReport{});
    if (std::get<GPSPositionReport>(batch.events.front()).satellites_used != std::numeric_limits<uint8_t>::max()) {
        return 1;
    }
    const GPSDeadline deadline{.untilUs = 2001};
    if (deadline.remainingMilliseconds(1000) != 2 || deadline.remainingMilliseconds(2001) != 0) {
        return 2;
    }
    uint64_t now = 0;
    const auto outcome = GPSCommandTransaction::await(
        2000, [&]() { return now; }, []() { return GPSCommandOutcome::Pending; }, [&]() { now += 1000; },
        []() { return GPSCommandOutcome::Pending; });
    return outcome == GPSCommandOutcome::TimedOut && now == 2000 ? 0 : 3;
}
