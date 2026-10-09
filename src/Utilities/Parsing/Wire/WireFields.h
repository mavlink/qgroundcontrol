#pragma once

#include <array>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <span>
#include <tuple>
#include <type_traits>

#include "LittleEndian.h"

/// Table-driven little-endian records. A record specializes Layout<T> with its wire SIZE and a FIELDS tuple of
/// Field<&T::member, offset>; members are scalars, records or fixed arrays of either. Bytes without a field are
/// skipped on decode and zero on encode.
namespace Wire {

template <typename T>
struct Layout;

template <typename T>
concept Record = requires {
    { Layout<T>::SIZE } -> std::convertible_to<std::size_t>;
    Layout<T>::FIELDS;
};

template <Record T>
inline constexpr std::size_t SIZE = Layout<T>::SIZE;

namespace detail {
template <typename>
struct MemberPointer;

template <typename Class, typename Value>
struct MemberPointer<Value Class::*>
{
    using ClassType = Class;
    using ValueType = Value;
};

template <typename T>
constexpr std::size_t wireSize()
{
    if constexpr (std::is_array_v<T>) {
        return std::extent_v<T> * wireSize<std::remove_extent_t<T>>();
    } else if constexpr (Record<T>) {
        return SIZE<T>;
    } else {
        static_assert(LittleEndian::Scalar<T>, "wire fields are scalars, records or fixed arrays of them");
        return sizeof(T);
    }
}
}  // namespace detail

template <auto Member, std::size_t Offset>
struct Field
{
    using Class = typename detail::MemberPointer<decltype(Member)>::ClassType;
    using Value = typename detail::MemberPointer<decltype(Member)>::ValueType;
    static constexpr auto MEMBER = Member;
    static constexpr std::size_t OFFSET = Offset;
    static constexpr std::size_t SIZE = detail::wireSize<Value>();
};

/// Every field belongs to T, fits within SIZE<T> and overlaps no other field.
template <Record T>
inline constexpr bool VALID_LAYOUT = std::apply(
    [](auto... field) {
        const std::array<std::size_t, sizeof...(field)> begin{decltype(field)::OFFSET...};
        const std::array<std::size_t, sizeof...(field)> end{(decltype(field)::OFFSET + decltype(field)::SIZE)...};
        for (std::size_t i = 0; i < begin.size(); ++i) {
            if (end[i] > SIZE<T>) {
                return false;
            }
            for (std::size_t j = 0; j < i; ++j) {
                if (begin[i] < end[j] && begin[j] < end[i]) {
                    return false;
                }
            }
        }
        return (std::same_as<typename decltype(field)::Class, T> && ...);
    },
    Layout<T>::FIELDS);

namespace detail {
template <typename Value>
constexpr void read(Value& value, std::span<const uint8_t> bytes, std::size_t offset)
{
    if constexpr (std::is_array_v<Value>) {
        for (std::size_t i = 0; i < std::extent_v<Value>; ++i) {
            detail::read(value[i], bytes, offset + i * wireSize<std::remove_extent_t<Value>>());
        }
    } else if constexpr (Record<Value>) {
        static_assert(VALID_LAYOUT<Value>, "Layout<T> fields must belong to T, fit in SIZE and not overlap");
        const auto record = offset <= bytes.size() ? bytes.subspan(offset) : std::span<const uint8_t>{};
        std::apply(
            [&](auto... field) {
                (detail::read(value.*decltype(field)::MEMBER, record, decltype(field)::OFFSET), ...);
            },
            Layout<Value>::FIELDS);
    } else {
        value = LittleEndian::read<Value>(bytes, offset).value_or(Value{});
    }
}

template <typename Value>
constexpr void write(std::span<uint8_t> bytes, std::size_t offset, const Value& value)
{
    if constexpr (std::is_array_v<Value>) {
        for (std::size_t i = 0; i < std::extent_v<Value>; ++i) {
            detail::write(bytes, offset + i * wireSize<std::remove_extent_t<Value>>(), value[i]);
        }
    } else if constexpr (Record<Value>) {
        static_assert(VALID_LAYOUT<Value>, "Layout<T> fields must belong to T, fit in SIZE and not overlap");
        std::apply(
            [&](auto... field) {
                (detail::write(bytes, offset + decltype(field)::OFFSET, value.*decltype(field)::MEMBER), ...);
            },
            Layout<Value>::FIELDS);
    } else {
        (void) LittleEndian::write(bytes, offset, value);
    }
}
}  // namespace detail

/// Decodes the record at @a offset. Fields whose bytes are unavailable decode as zero.
template <Record T>
constexpr T decode(std::span<const uint8_t> bytes, std::size_t offset = 0)
{
    T value{};
    detail::read(value, bytes, offset);
    return value;
}

template <Record T>
constexpr std::array<uint8_t, SIZE<T>> encode(const T& value)
{
    std::array<uint8_t, SIZE<T>> bytes{};
    detail::write(bytes, 0, value);
    return bytes;
}

}  // namespace Wire
