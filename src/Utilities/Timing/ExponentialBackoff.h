#pragma once

#include <algorithm>
#include <chrono>

/// Retry delays that start at an initial value and grow by a factor per attempt, up to a cap.
class ExponentialBackoff
{
public:
    using Delay = std::chrono::milliseconds;

    /// A positive @a maxRetryAfter lets a server's retry-after hint lengthen a delay, up to that limit.
    constexpr ExponentialBackoff(Delay initial, double factor, Delay cap, Delay maxRetryAfter = Delay::zero())
        : _initial(initial)
        , _factor(factor)
        , _cap(cap)
        , _maxRetryAfter(maxRetryAfter)
        , _delay(initial)
    {}

    /// The delay before the next attempt. A @a retryAfter hint longer than the exponential delay replaces it,
    /// clamped to maxRetryAfter.
    constexpr Delay peek(Delay retryAfter = Delay::zero()) const
    {
        return std::clamp(retryAfter, _delay, (std::max) (_delay, _maxRetryAfter));
    }

    /// Returns peek(@a retryAfter) and counts the attempt.
    constexpr Delay next(Delay retryAfter = Delay::zero())
    {
        const Delay delay = peek(retryAfter);
        const auto grown = _delay * _factor;
        _delay = grown < _cap ? std::chrono::duration_cast<Delay>(grown) : _cap;
        ++_attempts;
        return delay;
    }

    constexpr void reset()
    {
        _delay = _initial;
        _attempts = 0;
    }

    /// Attempts counted since construction or the last reset().
    constexpr int attempts() const { return _attempts; }

private:
    Delay _initial;
    double _factor;
    Delay _cap;
    Delay _maxRetryAfter;
    Delay _delay;
    int _attempts = 0;
};
