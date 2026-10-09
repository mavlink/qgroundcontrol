#pragma once

#include <cstdint>

/// GPS system time as receivers stamp their epochs: a week number and the milliseconds into that week.
namespace GPSTime {

/// Milliseconds in a GPS week; every time of week is below it.
inline constexpr uint32_t WEEK_MS = 604800000;

/// Milliseconds since the GPS epoch of @a towMs into @a week.
[[nodiscard]] constexpr uint64_t epochMs(uint32_t week, uint32_t towMs)
{
    return uint64_t(week) * WEEK_MS + towMs;
}

/// Whether time of week @a later follows @a earlier by less than half a week, across the week rollover.
[[nodiscard]] constexpr bool towAdvances(uint32_t later, uint32_t earlier)
{
    const uint64_t difference = (uint64_t(later) + WEEK_MS - earlier) % WEEK_MS;
    return difference != 0 && difference < WEEK_MS / 2;
}

}  // namespace GPSTime
