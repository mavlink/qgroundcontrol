#pragma once

#include <array>
#include <string>
#include <string_view>

#include "GPSCommandTransaction.h"
#include "NMEA/NMEASentence.h"

namespace QuectelCodec {

/// Borrowed fields for the largest supported PQTM reply, including empty trailing fields.
class Fields
{
public:
    explicit Fields(std::string_view body)
        : _count(NMEA::splitFields(body, _fields))
        , _overflowed(_count == 0)
    {}

    size_t size() const { return _count; }

    bool overflowed() const { return _overflowed; }

    std::string_view operator[](size_t index) const { return _fields[index]; }

    std::string_view back() const { return _count ? _fields[_count - 1] : std::string_view{}; }

    void pop_back()
    {
        if (_count) {
            --_count;
        }
    }

private:
    std::array<std::string_view, 12> _fields{};
    size_t _count;
    bool _overflowed;
};

template <typename T>
bool number(std::string_view text, T& result)
{
    if (text.empty() || text.front() == '+') {
        return false;
    }
    const auto parsed = NMEA::number<T>(text);
    if (!parsed) {
        return false;
    }
    result = *parsed;
    return true;
}

std::string frame(std::string_view body);
std::string_view checkedBody(std::string_view line);
bool rejected(const Fields& reply, std::string_view command);
GPSCommandOutcome readback(const Fields& reply, std::string_view command, bool matches);

}  // namespace QuectelCodec
