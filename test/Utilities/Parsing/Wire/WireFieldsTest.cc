#include "WireFieldsTest.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <span>
#include <tuple>

#include "WireFields.h"

namespace {
struct WireFieldsTestBlock
{
    uint16_t value;
    uint8_t tag[2];
};

struct WireFieldsTestRecord
{
    uint32_t id;
    int8_t delta;
    float scale;
    WireFieldsTestBlock blocks[2];
};

struct WireFieldsTestOverlap
{
    uint16_t first;
    uint16_t second;
};

struct WireFieldsTestOversized
{
    uint32_t value;
};

struct WireFieldsTestForeign
{
    uint16_t value;
};
}  // namespace

namespace Wire {
template <>
struct Layout<WireFieldsTestBlock>
{
    using T = WireFieldsTestBlock;
    static constexpr size_t SIZE = 4;
    static constexpr auto FIELDS = std::tuple{Field<&T::value, 0>{}, Field<&T::tag, 2>{}};
};

// Byte 5 has no field, so decode skips it and encode leaves it zero.
template <>
struct Layout<WireFieldsTestRecord>
{
    using T = WireFieldsTestRecord;
    static constexpr size_t SIZE = 18;
    static constexpr auto FIELDS =
        std::tuple{Field<&T::id, 0>{}, Field<&T::delta, 4>{}, Field<&T::scale, 6>{}, Field<&T::blocks, 10>{}};
};

template <>
struct Layout<WireFieldsTestOverlap>
{
    using T = WireFieldsTestOverlap;
    static constexpr size_t SIZE = 4;
    static constexpr auto FIELDS = std::tuple{Field<&T::first, 0>{}, Field<&T::second, 1>{}};
};

template <>
struct Layout<WireFieldsTestOversized>
{
    using T = WireFieldsTestOversized;
    static constexpr size_t SIZE = 4;
    static constexpr auto FIELDS = std::tuple{Field<&T::value, 2>{}};
};

template <>
struct Layout<WireFieldsTestForeign>
{
    static constexpr size_t SIZE = 2;
    static constexpr auto FIELDS = std::tuple{Field<&WireFieldsTestBlock::value, 0>{}};
};
}  // namespace Wire

namespace {
constexpr WireFieldsTestRecord SAMPLE{
    .id = 0x04030201, .delta = -2, .scale = 1.5f, .blocks = {{0x1122, {0xa1, 0xa2}}, {0x3344, {0xb1, 0xb2}}}};

constexpr std::array<uint8_t, 18> SAMPLE_BYTES{0x01, 0x02, 0x03, 0x04, 0xfe, 0x00, 0x00, 0x00, 0xc0,
                                               0x3f, 0x22, 0x11, 0xa1, 0xa2, 0x44, 0x33, 0xb1, 0xb2};

static_assert(Wire::SIZE<WireFieldsTestRecord> == SAMPLE_BYTES.size());
static_assert(Wire::VALID_LAYOUT<WireFieldsTestRecord>);
static_assert(!Wire::VALID_LAYOUT<WireFieldsTestOverlap>);
static_assert(!Wire::VALID_LAYOUT<WireFieldsTestOversized>);
static_assert(!Wire::VALID_LAYOUT<WireFieldsTestForeign>);
static_assert(!Wire::Record<uint32_t> && !Wire::Record<WireFieldsTestRecord[2]>);
static_assert(Wire::encode(SAMPLE) == SAMPLE_BYTES);
static_assert(Wire::decode<WireFieldsTestRecord>(SAMPLE_BYTES).blocks[1].tag[1] == 0xb2);
}  // namespace

void WireFieldsTest::_roundTrip()
{
    QVERIFY(Wire::encode(SAMPLE) == SAMPLE_BYTES);

    std::array<uint8_t, 3 + SAMPLE_BYTES.size()> shifted{0xee, 0xee, 0xee};
    std::copy(SAMPLE_BYTES.begin(), SAMPLE_BYTES.end(), shifted.begin() + 3);
    const auto decoded = Wire::decode<WireFieldsTestRecord>(shifted, 3);
    QVERIFY(Wire::encode(decoded) == SAMPLE_BYTES);
}

void WireFieldsTest::_truncatedInput_data()
{
    QTest::addColumn<size_t>("offset");
    QTest::addColumn<size_t>("available");

    QTest::newRow("complete") << size_t{0} << SAMPLE_BYTES.size();
    QTest::newRow("partial scalar") << size_t{0} << size_t{8};
    QTest::newRow("partial nested block") << size_t{0} << size_t{15};
    QTest::newRow("empty") << size_t{0} << size_t{0};
    QTest::newRow("offset past end") << SAMPLE_BYTES.size() + 1 << SAMPLE_BYTES.size();
}

void WireFieldsTest::_truncatedInput()
{
    QFETCH(size_t, offset);
    QFETCH(size_t, available);

    const auto decoded = Wire::decode<WireFieldsTestRecord>(std::span(SAMPLE_BYTES).first(available), offset);
    const auto expected = [&](auto value, size_t end) {
        return offset == 0 && end <= available ? value : decltype(value){};
    };
    QCOMPARE(decoded.id, expected(SAMPLE.id, 4));
    QCOMPARE(decoded.delta, expected(SAMPLE.delta, 5));
    QCOMPARE(decoded.scale, expected(SAMPLE.scale, 10));
    QCOMPARE(decoded.blocks[0].tag[1], expected(SAMPLE.blocks[0].tag[1], 14));
    QCOMPARE(decoded.blocks[1].value, expected(SAMPLE.blocks[1].value, 16));
    QCOMPARE(decoded.blocks[1].tag[1], expected(SAMPLE.blocks[1].tag[1], 18));
}

UT_REGISTER_TEST(WireFieldsTest, TestLabel::Unit, TestLabel::Utilities)
