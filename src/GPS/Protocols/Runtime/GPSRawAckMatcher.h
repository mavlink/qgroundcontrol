#pragma once

#include <algorithm>
#include <array>
#include <span>
#include <string_view>

#include "GPSCommandTransaction.h"

/// Bounded raw-byte matching for receivers whose replies need not be complete ASCII lines.
class GPSRawAckMatcher
{
public:
    GPSRawAckMatcher(std::string_view accepted, std::string_view rejected)
        : _accepted(accepted)
        , _rejected(rejected)
    {}

    bool valid() const
    {
        return !_accepted.empty() && _accepted.size() <= _window.size() && _rejected.size() <= _window.size();
    }

    void append(std::span<const uint8_t> bytes)
    {
        if (!valid()) {
            return;
        }
        for (const uint8_t byte : bytes) {
            if (_size == _window.size()) {
                std::move(_window.begin() + 1, _window.end(), _window.begin());
                --_size;
            }
            _window[_size++] = static_cast<char>(byte);
            const std::string_view received(_window.data(), _size);
            if (!_rejected.empty() && received.ends_with(_rejected)) {
                _outcome = GPSCommandOutcome::Rejected;
            } else if (_outcome != GPSCommandOutcome::Rejected && received.ends_with(_accepted)) {
                _outcome = GPSCommandOutcome::Acknowledged;
            }
        }
    }

    GPSCommandOutcome outcome() const { return _outcome; }

private:
    std::string_view _accepted;
    std::string_view _rejected;
    std::array<char, 256> _window{};
    size_t _size = 0;
    GPSCommandOutcome _outcome = GPSCommandOutcome::Pending;
};
