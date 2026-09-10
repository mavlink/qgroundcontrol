#pragma once

#include <charconv>
#include <cmath>
#include <locale>
#include <optional>
#include <sstream>
#include <string_view>
#include <type_traits>

namespace NMEAFields {
constexpr char hexDigit(unsigned value)
{
    return "0123456789ABCDEF"[value & 0xf];
}

template <class T>
std::optional<T> number(std::string_view field, int base = 10)
{
    if (field.empty())
        return std::nullopt;
    if (field.front() == '+') {
        field.remove_prefix(1);
        if (!field.empty() && (field.front() == '-' || field.front() == '+'))
            return std::nullopt;
    }
    if (field.empty())
        return std::nullopt;
    T value{};
    if constexpr (std::is_integral_v<T>) {
        const auto parsed = std::from_chars(field.data(), field.data() + field.size(), value, base);
        if (parsed.ec != std::errc{} || parsed.ptr != field.data() + field.size())
            return std::nullopt;
    } else if constexpr (requires { std::from_chars(field.data(), field.data() + field.size(), value); }) {
        const auto parsed = std::from_chars(field.data(), field.data() + field.size(), value);
        if (parsed.ec != std::errc{} || parsed.ptr != field.data() + field.size())
            return std::nullopt;
    } else {
        // Older libc++ versions lack floating-point from_chars. Keep parsing locale independent.
        std::istringstream stream{std::string(field)};
        stream.imbue(std::locale::classic());
        stream >> std::noskipws >> value;
        if (stream.fail() || stream.peek() != std::char_traits<char>::eof())
            return std::nullopt;
    }
    if constexpr (std::is_floating_point_v<T>) {
        if (!std::isfinite(value))
            return std::nullopt;
    }
    return value;
}

/// Bounded field cursor; empty fields retain their absence instead of shifting later fields.
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
        if (field.empty())
            return false;
        if constexpr (std::is_same_v<T, char>) {
            if (field.size() == 1) {
                value = field.front();
                return true;
            }
        } else if (const auto parsed = number<T>(field)) {
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
