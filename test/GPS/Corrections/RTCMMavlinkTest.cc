#include "RTCMMavlinkTest.h"

#include "RTCMMavlink.h"

namespace {

bool isFragmented(uint8_t flags)
{
    return (flags & 0x01U) != 0;
}

uint8_t fragmentId(uint8_t flags)
{
    return (flags >> 1) & 0x03U;
}

uint8_t sequenceId(uint8_t flags)
{
    return (flags >> 3) & 0x1FU;
}

QByteArray makePayload(qsizetype size, char fill = 'R')
{
    return QByteArray(size, fill);
}

}  // namespace

void RTCMMavlinkTest::_testEmpty()
{
    const auto packed = RTCMMavlink::pack({}, 3);
    QCOMPARE(packed.packets.size(), 0);
    QCOMPARE(packed.nextSequenceId, static_cast<uint8_t>(3));
}

void RTCMMavlinkTest::_testUnfragmentedSmall()
{
    const QByteArray data = makePayload(100);
    const auto packed = RTCMMavlink::pack(data, 5);

    QCOMPARE(packed.packets.size(), 1);
    QCOMPARE(packed.nextSequenceId, static_cast<uint8_t>(6));
    QVERIFY(!isFragmented(packed.packets[0].flags));
    QCOMPARE(sequenceId(packed.packets[0].flags), static_cast<uint8_t>(5));
    QCOMPARE(packed.packets[0].data, data);
}

void RTCMMavlinkTest::_testUnfragmentedExact180()
{
    // Full single fragment fits unfragmented — must not force fragmentation.
    const QByteArray data = makePayload(RTCMMavlink::kFragmentLen);
    const auto packed = RTCMMavlink::pack(data, 0);

    QCOMPARE(packed.packets.size(), 1);
    QVERIFY(!isFragmented(packed.packets[0].flags));
    QCOMPARE(packed.packets[0].data.size(), RTCMMavlink::kFragmentLen);
}

void RTCMMavlinkTest::_testFragmented181()
{
    const QByteArray data = makePayload(181);
    const auto packed = RTCMMavlink::pack(data, 2);

    QCOMPARE(packed.packets.size(), 2);
    QVERIFY(isFragmented(packed.packets[0].flags));
    QVERIFY(isFragmented(packed.packets[1].flags));
    QCOMPARE(fragmentId(packed.packets[0].flags), static_cast<uint8_t>(0));
    QCOMPARE(fragmentId(packed.packets[1].flags), static_cast<uint8_t>(1));
    QCOMPARE(sequenceId(packed.packets[0].flags), static_cast<uint8_t>(2));
    QCOMPARE(sequenceId(packed.packets[1].flags), static_cast<uint8_t>(2));
    QCOMPARE(packed.packets[0].data.size(), RTCMMavlink::kFragmentLen);
    QCOMPARE(packed.packets[1].data.size(), 1);
    QCOMPARE(packed.packets[0].data + packed.packets[1].data, data);
    QCOMPARE(packed.nextSequenceId, static_cast<uint8_t>(3));
}

void RTCMMavlinkTest::_testExactMultiple360Terminator()
{
    // 2 * 180: two full fragments plus required zero-length terminator.
    const QByteArray data = makePayload(360);
    const auto packed = RTCMMavlink::pack(data, 7);

    QCOMPARE(packed.packets.size(), 3);
    for (int i = 0; i < 3; ++i) {
        QVERIFY(isFragmented(packed.packets[i].flags));
        QCOMPARE(sequenceId(packed.packets[i].flags), static_cast<uint8_t>(7));
        QCOMPARE(fragmentId(packed.packets[i].flags), static_cast<uint8_t>(i));
    }
    QCOMPARE(packed.packets[0].data.size(), 180);
    QCOMPARE(packed.packets[1].data.size(), 180);
    QCOMPARE(packed.packets[2].data.size(), 0);
    QCOMPARE(packed.packets[0].data + packed.packets[1].data, data);
}

void RTCMMavlinkTest::_testExactMultiple540Terminator()
{
    const QByteArray data = makePayload(540);
    const auto packed = RTCMMavlink::pack(data, 1);

    QCOMPARE(packed.packets.size(), 4);  // 3 full + terminator
    QCOMPARE(fragmentId(packed.packets[3].flags), static_cast<uint8_t>(3));
    QCOMPARE(packed.packets[3].data.size(), 0);
    QByteArray reassembled;
    for (int i = 0; i < 3; ++i) {
        reassembled += packed.packets[i].data;
    }
    QCOMPARE(reassembled, data);
}

void RTCMMavlinkTest::_testFourFragmentsPartialTail()
{
    // 541..719: four fragments with a non-full last fragment — the short tail
    // itself marks completion, so no terminator.
    const QByteArray data = makePayload(700);  // 3 * 180 + 160
    const auto packed = RTCMMavlink::pack(data, 9);

    QCOMPARE(packed.packets.size(), 4);
    QByteArray reassembled;
    for (int i = 0; i < 4; ++i) {
        QVERIFY(isFragmented(packed.packets[i].flags));
        QCOMPARE(fragmentId(packed.packets[i].flags), static_cast<uint8_t>(i));
        reassembled += packed.packets[i].data;
    }
    QCOMPARE(packed.packets[3].data.size(), 160);
    QCOMPARE(reassembled, data);
    QCOMPARE(packed.nextSequenceId, static_cast<uint8_t>(10));
}

void RTCMMavlinkTest::_testExact720NoTerminator()
{
    // All four full fragments complete by the "all fragments present" rule.
    const QByteArray data = makePayload(720);
    const auto packed = RTCMMavlink::pack(data, 4);

    QCOMPARE(packed.packets.size(), 4);
    for (int i = 0; i < 4; ++i) {
        QVERIFY(isFragmented(packed.packets[i].flags));
        QCOMPARE(fragmentId(packed.packets[i].flags), static_cast<uint8_t>(i));
        QCOMPARE(packed.packets[i].data.size(), 180);
    }
}

void RTCMMavlinkTest::_testOversizedStreamsUnfragmented()
{
    // >720 cannot use the 4-fragment protocol; stream as unfragmented chunks.
    const QByteArray data = makePayload(900);  // 5 * 180
    const auto packed = RTCMMavlink::pack(data, 10);

    QCOMPARE(packed.packets.size(), 5);
    QCOMPARE(packed.nextSequenceId, static_cast<uint8_t>(15));

    QByteArray reassembled;
    for (int i = 0; i < packed.packets.size(); ++i) {
        QVERIFY(!isFragmented(packed.packets[i].flags));
        QCOMPARE(sequenceId(packed.packets[i].flags), static_cast<uint8_t>(10 + i));
        QCOMPARE(packed.packets[i].data.size(), 180);
        reassembled += packed.packets[i].data;
    }
    QCOMPARE(reassembled, data);

    // Non-multiple oversized: last chunk short, still unfragmented.
    const QByteArray odd = makePayload(721);
    const auto packedOdd = RTCMMavlink::pack(odd, 0);
    QCOMPARE(packedOdd.packets.size(), 5);  // 180*4 + 1
    QVERIFY(!isFragmented(packedOdd.packets[4].flags));
    QCOMPARE(packedOdd.packets[4].data.size(), 1);
}

void RTCMMavlinkTest::_testSequenceAdvances()
{
    uint8_t seq = 30;
    auto packed = RTCMMavlink::pack(makePayload(10), seq);
    QCOMPARE(packed.nextSequenceId, static_cast<uint8_t>(31));

    packed = RTCMMavlink::pack(makePayload(10), packed.nextSequenceId);
    // Sequence field is 5 bits; packing stores (seq & 0x1f) in flags but nextSequenceId
    // is free to wrap as uint8_t — only the low 5 bits appear in flags.
    QCOMPARE(sequenceId(packed.packets[0].flags), static_cast<uint8_t>(31 & 0x1F));
}

void RTCMMavlinkTest::_testFlagsBitLayout()
{
    const auto packed = RTCMMavlink::pack(makePayload(200), 0x15);  // seq 21

    QCOMPARE(packed.packets.size(), 2);
    // flags: bit0=1, bits1-2=fragId, bits3-7=seq
    QCOMPARE(packed.packets[0].flags, static_cast<uint8_t>(0x01 | (0 << 1) | (0x15 << 3)));
    QCOMPARE(packed.packets[1].flags, static_cast<uint8_t>(0x01 | (1 << 1) | (0x15 << 3)));
}

UT_REGISTER_TEST(RTCMMavlinkTest, TestLabel::Unit)
