#pragma once

#include "NMEASentence.h"

namespace NMEAFields {
constexpr char hexDigit(unsigned value)
{
    return "0123456789ABCDEF"[value & 0xf];
}

/// Empty fields retain their absence rather than shifting later proprietary fields.
class Cursor
{
public:
    explicit Cursor(std::string_view fields)
        : _remaining(fields)
    {}

    template <class T>
    bool read(T& value)
    {
        const auto separator = _remaining.find_first_of(",*\r\n");
        const auto field = _remaining.substr(0, separator);
        _remaining = separator == std::string_view::npos || _remaining[separator] != ','
                         ? std::string_view{}
                         : _remaining.substr(separator + 1);
        if (field.empty()) {
            return false;
        }
        if constexpr (std::is_same_v<T, char>) {
            if (field.size() == 1) {
                value = field.front();
                return true;
            }
        } else if (const auto parsed = NMEA::number<T>(field)) {
            value = *parsed;
            return true;
        }
        _valid = false;
        return false;
    }

    bool valid() const { return _valid; }

private:
    std::string_view _remaining;
    bool _valid = true;
};
}  // namespace NMEAFields
