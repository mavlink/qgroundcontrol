#include "WireFieldsTest.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cstdint>
#include <limits>
#include <span>
#include <tuple>

#include "LittleEndian.h"
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

void WireFieldsTest::_littleEndianScalars()
{
    const std::array<uint8_t, 9> data{0, 0xfe, 0xff, 0xff, 0xff, 0, 0, 0x80, 0xbf};
    QCOMPARE(LittleEndian::read<int32_t>(data, 1), -2);
    QCOMPARE_EQ(LittleEndian::read<float>(data, 5), -1.0f);
    QVERIFY(!LittleEndian::read<double>(data, 2));
    QVERIFY(!LittleEndian::read<uint8_t>(data, SIZE_MAX));
    std::array<uint8_t, 9> output{};
    QVERIFY(LittleEndian::write(output, 1, int32_t(-2)));
    QVERIFY(LittleEndian::write(output, 5, -1.0f));
    QCOMPARE(output, data);
    QVERIFY(!LittleEndian::write(output, 2, double(1.0)));
    QCOMPARE(output, data);
    QVERIFY(LittleEndian::write(output, 1, std::numeric_limits<uint64_t>::max()));
    QCOMPARE(LittleEndian::read<uint64_t>(output, 1), std::numeric_limits<uint64_t>::max());
    QVERIFY(LittleEndian::write(output, 1, std::bit_cast<double>(uint64_t(0x7ff8000000000001))));
    QCOMPARE(std::bit_cast<uint64_t>(*LittleEndian::read<double>(output, 1)), 0x7ff8000000000001);
}

QGC_REGISTER_PORTABLE_TEST(WireFieldsTest, TestLabel::Unit, TestLabel::Utilities)
