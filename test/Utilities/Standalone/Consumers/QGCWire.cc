#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>

#include "LittleEndian.h"

namespace {

template <typename T>
bool equalValue(T actual, T expected)
{
    if constexpr (std::floating_point<T>) {
        if (std::isnan(expected)) {
            return std::isnan(actual);
        }
        return actual == expected && std::signbit(actual) == std::signbit(expected);
    }
    return actual == expected;
}

template <typename T>
bool checkScalar(const char* name, const std::array<uint8_t, sizeof(T)>& bytes, T expected)
{
    alignas(T) std::array<uint8_t, sizeof(T) + 1> unaligned{};
    std::copy(bytes.begin(), bytes.end(), unaligned.begin() + 1);
    const auto value = LittleEndian::read<T>(unaligned, 1);
    if (!value || !equalValue(*value, expected)) {
        std::cerr << name << ": incorrect unaligned scalar decoding\n";
        return false;
    }
    alignas(T) std::array<uint8_t, sizeof(T) + 2> encoded;
    encoded.fill(0xa5);
    if (!LittleEndian::write(encoded, 1, *value) || encoded.front() != 0xa5 || encoded.back() != 0xa5 ||
        !std::equal(bytes.begin(), bytes.end(), encoded.begin() + 1)) {
        std::cerr << name << ": incorrect unaligned scalar encoding or overwritten guard bytes\n";
        return false;
    }
    const auto unchanged = encoded;
    const auto payload = std::span<const uint8_t>(unaligned).subspan(1);
    for (std::size_t size = 0; size < sizeof(T); ++size) {
        if (LittleEndian::read<T>(payload.first(size))) {
            std::cerr << name << ": accepted a truncated scalar\n";
            return false;
        }
        if (LittleEndian::write(std::span(encoded).subspan(1, size), 0, expected) || encoded != unchanged) {
            std::cerr << name << ": truncated write changed the destination\n";
            return false;
        }
    }
    for (const auto offset :
         {std::size_t{1}, payload.size(), payload.size() + 1, (std::numeric_limits<std::size_t>::max)()}) {
        if (LittleEndian::read<T>(payload, offset)) {
            std::cerr << name << ": accepted an invalid offset\n";
            return false;
        }
        if (LittleEndian::write(std::span(encoded).subspan(1, sizeof(T)), offset, expected) || encoded != unchanged) {
            std::cerr << name << ": invalid-offset write changed the destination\n";
            return false;
        }
    }
    return true;
}

}  // namespace

static_assert(!LittleEndian::Scalar<bool>);
static_assert(!LittleEndian::Scalar<const bool>);
static_assert(!LittleEndian::Scalar<volatile bool>);
static_assert(!LittleEndian::Scalar<const volatile bool>);
static_assert(!LittleEndian::Scalar<void*>);
static_assert(LittleEndian::read<uint16_t>(std::array<uint8_t, 2>{0x34, 0x12}) == 0x1234);
static_assert(!LittleEndian::read<uint64_t>({}));
static_assert([] {
    std::array<uint8_t, 2> bytes{};
    return LittleEndian::write(bytes, 0, uint16_t{0x1234}) && bytes == std::array<uint8_t, 2>{0x34, 0x12};
}());
static_assert(!LittleEndian::write({}, 0, uint64_t{1}));

int main()
{
    const bool valid =
        checkScalar<int8_t>("int8", {0x80}, -128) && checkScalar<uint8_t>("uint8", {0xfe}, 254) &&
        checkScalar<int16_t>("int16", {0x2e, 0xfb}, -1234) && checkScalar<uint16_t>("uint16", {0x50, 0xc3}, 50000) &&
        checkScalar<int32_t>("int32", {0xc0, 0x1d, 0xfe, 0xff}, -123456) &&
        checkScalar<uint32_t>("uint32", {0x00, 0x28, 0x6b, 0xee}, 4000000000u) &&
        checkScalar<int64_t>("int64", {0x35, 0xfb, 0x04, 0x8e, 0xe0, 0xfe, 0xff, 0xff}, -1234567890123LL) &&
        checkScalar<uint64_t>("uint64", {0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x81}, 0x8102030405060708ULL) &&
        checkScalar<float>("float", {0x00, 0x00, 0xc0, 0xbf}, -1.5f) &&
        checkScalar<double>("double", {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf8, 0xbf}, -1.5) &&
        checkScalar<float>("float-subnormal", {0x01, 0x00, 0x00, 0x00}, std::numeric_limits<float>::denorm_min()) &&
        checkScalar<double>("double-subnormal", {0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
                            std::numeric_limits<double>::denorm_min()) &&
        checkScalar<float>("float-negative-zero", {0x00, 0x00, 0x00, 0x80}, -0.0f) &&
        checkScalar<double>("double-negative-zero", {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80}, -0.0) &&
        checkScalar<float>("float-infinity", {0x00, 0x00, 0x80, 0x7f}, std::numeric_limits<float>::infinity()) &&
        checkScalar<double>("double-infinity", {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0x7f},
                            std::numeric_limits<double>::infinity()) &&
        checkScalar<float>("float-nan", {0x01, 0x00, 0xc0, 0x7f}, std::numeric_limits<float>::quiet_NaN()) &&
        checkScalar<double>("double-nan", {0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf8, 0x7f},
                            std::numeric_limits<double>::quiet_NaN());
    return valid ? 0 : 1;
}
