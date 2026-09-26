#pragma once

#include <algorithm>

/// MAVLink GNSS resilience states (jamming, spoofing and authentication), where 0 and 255 mean unknown.
namespace GPSResilienceState {

[[nodiscard]] constexpr bool reported(int state)
{
    return state > 0 && state < 255;
}

/// The worse of the reported spoofing and jamming states, or 0 when neither is reported.
[[nodiscard]] constexpr int interference(int spoofing, int jamming)
{
    return std::max(reported(spoofing) ? spoofing : 0, reported(jamming) ? jamming : 0);
}

}  // namespace GPSResilienceState
