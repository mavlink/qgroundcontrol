// Block layouts as the Septentrio SBF reference guide defines them.
#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <tuple>

#include "Checksums.h"
#include "WireFields.h"

/// Offsets count from a block's first sync byte, so a whole block decodes directly. Each layout ends
/// at the last field QGC reads, which is the shortest block QGC can decode.
namespace SBF {

inline constexpr uint8_t SYNC1 = 0x24;
inline constexpr uint8_t SYNC2 = 0x40;

/// Starts every block. The CRC covers id through the end of the block; length counts the whole block.
struct BlockHeader
{
    uint16_t sync;
    uint16_t crc;
    uint16_t id;
    uint16_t length;

    /// Bits 0-12 of the id; bits 13-15 are the block revision.
    [[nodiscard]] constexpr uint16_t number() const { return static_cast<uint16_t>(id & 0x1fff); }
};

}  // namespace SBF

namespace SBF::BlockId {
inline constexpr uint16_t PVT_GEODETIC = 4007;
}  // namespace SBF::BlockId

namespace SBF {

struct PVTGeodetic
{
    uint32_t tow;
    uint16_t wnc;
    uint8_t mode;
    uint8_t error;
    double latitude;
    double longitude;
    double height;
    float undulation;
    float vn;
    float ve;
    float vu;
    float cog;
    uint8_t datum;
    uint8_t nrSV;
    uint16_t hAccuracy;
    uint16_t vAccuracy;

    [[nodiscard]] constexpr uint8_t modeType() const { return static_cast<uint8_t>(mode & 0x0f); }

    [[nodiscard]] constexpr bool modeAutoSet() const { return (mode & 0x40) != 0; }

    [[nodiscard]] constexpr bool mode2D() const { return (mode & 0x80) != 0; }
};

}  // namespace SBF

namespace Wire {
template <>
struct Layout<SBF::BlockHeader>
{
    using T = SBF::BlockHeader;
    static constexpr size_t SIZE = 8;
    static constexpr auto FIELDS =
        std::tuple{Field<&T::sync, 0>{}, Field<&T::crc, 2>{}, Field<&T::id, 4>{}, Field<&T::length, 6>{}};
};

template <>
struct Layout<SBF::PVTGeodetic>
{
    using T = SBF::PVTGeodetic;
    static constexpr size_t SIZE = 94;
    static constexpr auto FIELDS = std::tuple{
        Field<&T::tow, 8>{},       Field<&T::wnc, 12>{},       Field<&T::mode, 14>{},      Field<&T::error, 15>{},
        Field<&T::latitude, 16>{}, Field<&T::longitude, 24>{}, Field<&T::height, 32>{},    Field<&T::undulation, 40>{},
        Field<&T::vn, 44>{},       Field<&T::ve, 48>{},        Field<&T::vu, 52>{},        Field<&T::cog, 56>{},
        Field<&T::datum, 73>{},    Field<&T::nrSV, 74>{},      Field<&T::hAccuracy, 90>{}, Field<&T::vAccuracy, 92>{}};
};
}  // namespace Wire

static_assert(Wire::VALID_LAYOUT<SBF::BlockHeader>);
static_assert(Wire::VALID_LAYOUT<SBF::PVTGeodetic>);

namespace SBF {

/// The header of a block candidate whose length fits the candidate and whose CRC matches; nullopt otherwise.
[[nodiscard]] inline std::optional<BlockHeader> checkedHeader(std::span<const uint8_t> block)
{
    // The block CRC covers everything from the id field to the end of the block.
    constexpr size_t CRC_START = 4;
    if (block.size() < Wire::SIZE<BlockHeader>) {
        return std::nullopt;
    }
    const auto header = Wire::decode<BlockHeader>(block);
    if (header.length < Wire::SIZE<BlockHeader> || header.length > block.size() ||
        header.crc != QGC::crc16Xmodem(block.subspan(CRC_START, header.length - CRC_START))) {
        return std::nullopt;
    }
    return header;
}

}  // namespace SBF
