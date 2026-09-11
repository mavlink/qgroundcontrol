#pragma once

#include <type_traits>
#include <variant>

#include "GPSReceiverAttempt.h"

struct GPSReceiverErrorDetail
{
    GPSConnectionError error;
    QString detail;
};

/// Qt delivery tags worker events before they cross the attempt admission boundary.
struct GPSReceiverEvent
{
    using Value = std::variant<GPSOpenResult, GPSConfigurationResult, GPSReadResult, GPSReceiverErrorDetail,
                               GPSReceiverAttempt::Phase, GPSReceiverFailure>;
    quint64 generation = 0;
    Value value;
};

/// Pure transition logic: stale and terminal attempts cannot accept worker events.
inline bool gpsReduceReceiverAttempt(GPSReceiverAttempt& attempt, const GPSReceiverEvent& event)
{
    if (!event.generation || event.generation != attempt.generation || attempt.terminal())
        return false;
    return std::visit(
        [&attempt](const auto& value) {
            using T = std::decay_t<decltype(value)>;
            if constexpr (std::is_same_v<T, GPSOpenResult>) {
                if (attempt.transportOpen)
                    return false;
                attempt.transportOpen = value;
            } else if constexpr (std::is_same_v<T, GPSConfigurationResult>) {
                if (attempt.configurationResult)
                    return false;
                attempt.configurationResult = value;
            } else if constexpr (std::is_same_v<T, GPSReadResult>) {
                if (attempt.transportRead)
                    return false;
                attempt.transportRead = value;
            } else if constexpr (std::is_same_v<T, GPSReceiverErrorDetail>) {
                attempt.error = value.error;
                attempt.errorDetail = value.detail;
            } else if constexpr (std::is_same_v<T, GPSReceiverFailure>) {
                if (value.generation != attempt.generation)
                    return false;
                attempt.failure = value;
                attempt.error = value.error;
                attempt.errorDetail = value.detail;
                attempt.phase = value.retry == GPSRetryDisposition::Cancel ? GPSReceiverAttempt::Phase::Cancelled
                                                                           : GPSReceiverAttempt::Phase::Failed;
            } else {
                using Phase = GPSReceiverAttempt::Phase;
                if (value == attempt.phase || value == Phase::Idle || value == Phase::Failed)
                    return false;
                if (value == Phase::Connecting && attempt.phase != Phase::Idle)
                    return false;
                if (value == Phase::Ready && attempt.phase != Phase::Connecting && attempt.phase != Phase::Configuring)
                    return false;
                if (value == Phase::Ready && attempt.configurationResult &&
                    attempt.configurationResult->status != GPSConfigurationStatus::Ready)
                    return false;
                if (value == Phase::Configuring && attempt.phase != Phase::Connecting)
                    return false;
                attempt.phase = value;
                if (value == Phase::Ready || value == Phase::Cancelled) {
                    attempt.error = GPSConnectionError::None;
                    attempt.errorDetail.clear();
                }
            }
            return true;
        },
        event.value);
}
