#include <QtCore/QByteArray>
#include <QtTest/QTest>

#include "RTCMMavlinkPacket.h"
#include "UnitTest.h"

class RTCMMavlinkPacketTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _packetization_data();
    void _packetization();
};

void RTCMMavlinkPacketTest::_packetization_data()
{
    QTest::addColumn<int>("size");
    QTest::addColumn<int>("sequence");
    QTest::addColumn<int>("packetCount");
    QTest::addColumn<bool>("fragmented");
    QTest::addColumn<int>("nextSequence");

    QTest::newRow("empty") << 0 << 31 << 0 << false << 31;
    QTest::newRow("single-byte") << 1 << 31 << 1 << false << 32;
    QTest::newRow("exact-single-fragment") << 180 << 31 << 1 << false << 32;
    QTest::newRow("split-tail") << 181 << 31 << 2 << true << 32;
    QTest::newRow("two-full-and-terminator") << 360 << 31 << 3 << true << 32;
    QTest::newRow("three-full-and-terminator") << 540 << 31 << 4 << true << 32;
    QTest::newRow("four-with-tail") << 719 << 31 << 4 << true << 32;
    QTest::newRow("four-full-no-terminator") << 720 << 31 << 4 << true << 32;
    QTest::newRow("oversized-unfragmented-stream") << 721 << 31 << 5 << false << 36;
    QTest::newRow("maximum-rtcm-frame") << 1029 << 31 << 6 << false << 37;
    QTest::newRow("sequence-counter-wrap") << 181 << 255 << 2 << true << 0;
    QTest::newRow("oversized-counter-wrap") << 721 << 255 << 5 << false << 4;
}

void RTCMMavlinkPacketTest::_packetization()
{
    QFETCH(int, size);
    QFETCH(int, sequence);
    QFETCH(int, packetCount);
    QFETCH(bool, fragmented);
    QFETCH(int, nextSequence);
    QByteArray bytes(size, Qt::Uninitialized);
    for (int index = 0; index < size; ++index) {
        bytes[index] = static_cast<char>(index & 0xff);
    }
    const auto packed = RTCMMavlinkPacket::pack(bytes, static_cast<uint8_t>(sequence));
    QCOMPARE(packed.packets.size(), packetCount);
    QCOMPARE(packed.nextSequenceId, nextSequence);
    QByteArray assembled;
    for (qsizetype index = 0; index < packed.packets.size(); ++index) {
        const auto& packet = packed.packets[index];
        QCOMPARE(bool(packet.flags & 1), fragmented);
        QCOMPARE((packet.flags >> 1) & 3, fragmented ? index : 0);
        QCOMPARE(packet.flags >> 3, (sequence + (fragmented ? 0 : index)) & 31);
        QVERIFY(packet.data.size() <= RTCMMavlinkPacket::kFragmentLen);
        QCOMPARE(packet.data.size(), qMin(qsizetype(size) - assembled.size(), RTCMMavlinkPacket::kFragmentLen));
        assembled.append(packet.data);
    }
    QCOMPARE(assembled, bytes);
}

UT_REGISTER_TEST(RTCMMavlinkPacketTest, TestLabel::Unit)

#include "RTCMMavlinkPacketTest.moc"
