#include "RtpPacketCounterTest.h"

#include <optional>

#include <QtCore/QList>

#include "RtpPacketCounter.h"

namespace {

constexpr int kRtpClockRate = 90000;
constexpr quint32 kRtpSsrc = 0x1234ABCD;
// Added to a sequence number of _testRtpPacketCounter: the packet is of another SSRC.
constexpr int kOtherRtpSender = 0x10000;

}  // namespace

void RtpPacketCounterTest::_testRtpPacketCounter_data()
{
    QTest::addColumn<QList<int>>("sequences");  // Plus kOtherRtpSender for a packet of another SSRC.
    QTest::addColumn<quint64>("received");
    QTest::addColumn<quint64>("lost");

    const auto row = [](const char* name, const QList<int>& sequences, quint64 received, quint64 lost) {
        QTest::newRow(name) << sequences << received << lost;
    };
    const int other = kOtherRtpSender;
    row("first packet", {1000}, 1, 0);
    row("in order", {10, 11, 12, 13}, 4, 0);
    row("loss", {10, 11, 14, 15}, 4, 2);
    row("late packet still missing", {10, 12}, 2, 1);
    row("late packet", {10, 12, 11, 13}, 4, 0);
    row("duplicate", {10, 11, 11, 12}, 3, 0);
    row("duplicate does not hide a loss", {10, 12, 12, 13}, 3, 1);
    row("duplicate of a late packet", {10, 12, 11, 11}, 3, 0);
    row("wraparound", {65534, 65535, 0, 1}, 4, 0);
    row("loss across the wraparound", {65534, 1}, 2, 2);
    row("late packet across the wraparound", {65535, 1, 0}, 3, 0);
    row("before the first packet", {100, 99, 101}, 2, 0);
    const int window = RtpPacketCounter::kReorderWindow;
    row("late at the end of the reorder window", {200, 200 + window, 201}, 3, quint64(window - 2));
    row("too late for the reorder window", {200, 201 + window, 201}, 2, quint64(window));
    row("longest loss", {0, 2999}, 2, 2998);
    // Too far ahead to be loss: counted once a second packet shows the sender went on from there.
    row("jump, unconfirmed", {0, 3000}, 1, 0);
    row("sender restarted", {0, 1, 5000, 5001}, 4, 0);
    row("sender restarted to lower numbers", {30000, 30001, 100, 101}, 4, 0);
    row("sender restarted, next packet lost", {0, 1, 40000, 40002}, 4, 1);
    row("stray packet", {10, 11, 40000, 12, 13}, 4, 0);
    row("stray packets apart", {10, 40000, 11, 40001, 12}, 3, 0);
    row("stray packet twice", {10, 40000, 40000, 11}, 2, 0);
    row("losses add up across a restart", {10, 12, 40000, 40001, 40003}, 5, 2);
    // Another sender's numbers continue nothing, however close they are.
    row("new SSRC", {10, 11, other + 500, other + 501}, 4, 0);
    row("new SSRC, numbers go on after a gap", {10, 12, other + 15, other + 16}, 4, 1);
    row("new SSRC, next packet lost", {10, 11, other + 500, other + 502}, 4, 1);
    // It takes two packets in a row to be the stream: one that strays in leaves the sequence, and its loss, alone.
    row("stray packet of another SSRC", {10, other + 500, 12, 13}, 3, 1);
    row("stray packets of another SSRC apart", {10, other + 500, 11, other + 501, 12}, 3, 0);
    row("another SSRC, unconfirmed", {10, 11, other + 500}, 2, 0);
}

void RtpPacketCounterTest::_testRtpPacketCounter()
{
    QFETCH(QList<int>, sequences);
    QFETCH(quint64, received);
    QFETCH(quint64, lost);

    RtpPacketCounter counter(kRtpClockRate);
    QCOMPARE(counter.packetsReceived(), quint64(0));
    QCOMPARE(counter.packetsLost(), quint64(0));
    for (const int sequence : sequences) {
        counter.add(static_cast<quint16>(sequence), 0, (sequence >= kOtherRtpSender) ? (kRtpSsrc + 1) : kRtpSsrc,
                    std::nullopt);
    }
    QCOMPARE(counter.packetsReceived(), received);
    QCOMPARE(counter.packetsLost(), lost);
    QVERIFY(!counter.jitterNs());
}

void RtpPacketCounterTest::_testRtpPacketCounterDropoutFollowsPacketRate_data()
{
    QTest::addColumn<int>("packetsPerSecond");
    QTest::addColumn<int>("packetsMissing");
    QTest::addColumn<bool>("countedAsLost");

    // Up to ten seconds of packets may be lost; a longer jump is a sender that started over.
    QTest::newRow("3 s of a 1500 packets/s stream") << 1500 << 4500 << true;
    QTest::newRow("12 s of a 1500 packets/s stream") << 1500 << 18000 << false;
    QTest::newRow("5 s of a 100 packets/s stream") << 100 << 500 << true;
    QTest::newRow("20 s of a 100 packets/s stream") << 100 << 2000 << false;
    // Not less than RtpPacketCounter::kMinDropout, however few packets a stream has.
    QTest::newRow("20 s of a 1 packet/s stream") << 1 << 20 << true;
}

void RtpPacketCounterTest::_testRtpPacketCounterDropoutFollowsPacketRate()
{
    QFETCH(int, packetsPerSecond);
    QFETCH(int, packetsMissing);
    QFETCH(bool, countedAsLost);

    // Three seconds of the stream measure its packet rate.
    const int streamed = 3 * packetsPerSecond;
    const auto timestamp = [packetsPerSecond](int packet) {
        return static_cast<quint32>(qint64(packet) * kRtpClockRate / packetsPerSecond);
    };
    RtpPacketCounter counter(kRtpClockRate);
    for (int packet = 0; packet <= streamed; ++packet) {
        counter.add(static_cast<quint16>(packet), timestamp(packet), kRtpSsrc, std::nullopt);
    }
    QCOMPARE(counter.packetsLost(), quint64(0));

    // The stream goes on after the missing packets; a second packet confirms a sender that started over.
    const int resumed = streamed + packetsMissing + 1;
    counter.add(static_cast<quint16>(resumed), timestamp(resumed), kRtpSsrc, std::nullopt);
    counter.add(static_cast<quint16>(resumed + 1), timestamp(resumed + 1), kRtpSsrc, std::nullopt);
    QCOMPARE(counter.packetsReceived(), quint64(streamed + 3));
    QCOMPARE(counter.packetsLost(), quint64(countedAsLost ? packetsMissing : 0));
}

void RtpPacketCounterTest::_testRtpPacketCounterJitter_data()
{
    QTest::addColumn<QList<quint32>>("timestamps");
    QTest::addColumn<QList<qint64>>("arrivalsMs");  // -1: unknown
    QTest::addColumn<qint64>("jitterNs");           // -1: not measured

    // 9000 ticks of the 90 kHz clock are 100 ms.
    QTest::newRow("one packet") << QList<quint32>{0} << QList<qint64>{0} << qint64(-1);
    QTest::newRow("steady") << QList<quint32>{0, 9000, 18000} << QList<qint64>{0, 100, 200} << qint64(0);
    // RFC 3550: a sixteenth of each deviation goes into the estimate.
    QTest::newRow("late by 16 ms") << QList<quint32>{0, 9000, 18000} << QList<qint64>{0, 100, 216} << qint64(1'000'000);
    QTest::newRow("decays") << QList<quint32>{0, 9000, 18000, 27000, 36000} << QList<qint64>{0, 100, 216, 316, 416}
                            << qint64(878'906);
    QTest::newRow("packets of one frame")
        << QList<quint32>{0, 0, 9000} << QList<qint64>{0, 16, 100} << qint64(1'937'500);
    QTest::newRow("timestamp wraparound")
        << QList<quint32>{0xFFFFFF00, 0x00002228} << QList<qint64>{0, 100} << qint64(0);
    QTest::newRow("sent before the previous packet")
        << QList<quint32>{9000, 0} << QList<qint64>{100, 100} << qint64(6'250'000);
    QTest::newRow("no arrival times") << QList<quint32>{0, 9000, 18000} << QList<qint64>{-1, -1, -1} << qint64(-1);
    QTest::newRow("arrival time missing") << QList<quint32>{0, 9000, 18000} << QList<qint64>{0, -1, 200} << qint64(0);
}

void RtpPacketCounterTest::_testRtpPacketCounterJitter()
{
    QFETCH(QList<quint32>, timestamps);
    QFETCH(QList<qint64>, arrivalsMs);
    QFETCH(qint64, jitterNs);

    RtpPacketCounter counter(kRtpClockRate);
    for (qsizetype i = 0; i < timestamps.size(); ++i) {
        const qint64 arrivalMs = arrivalsMs.at(i);
        counter.add(static_cast<quint16>(i), timestamps.at(i), kRtpSsrc,
                    (arrivalMs >= 0) ? std::optional(arrivalMs * 1'000'000) : std::nullopt);
    }
    QCOMPARE(counter.jitterNs().value_or(-1), jitterNs);
}

UT_REGISTER_TEST(RtpPacketCounterTest, TestLabel::Unit)
