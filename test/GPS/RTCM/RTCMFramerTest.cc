#include "RTCMFramerTest.h"

#include <QtCore/QByteArrayView>

#include "RTCMFramer.h"
#include "RTCMTestFixtures.h"

void RTCMFramerTest::_frameAccess_data()
{
    QTest::addColumn<int>("messageId");
    QTest::addColumn<int>("extraPayload");
    QTest::newRow("minimum-id") << 0 << 0;
    QTest::newRow("base-position") << 1005 << 4;
    QTest::newRow("observations") << 1077 << 8;
    QTest::newRow("maximum-id") << 4095 << 0;
    QTest::newRow("near-maximum-length") << 1230 << 1020;
    QTest::newRow("maximum-length") << 1230 << 1021;
}

void RTCMFramerTest::_frameAccess()
{
    QFETCH(int, messageId);
    QFETCH(int, extraPayload);
    const auto frame = GpsTestHelpers::buildRtcmFrame(static_cast<uint16_t>(messageId), extraPayload);
    RTCMFramer framer;
    for (qsizetype index = 0; index < frame.size(); ++index) {
        QCOMPARE(framer.addByte(static_cast<uint8_t>(frame[index])), index == frame.size() - 1);
    }
    QVERIFY(framer.valid());
    QVERIFY(framer.valid());
    QVERIFY(RTCMFramer::isValidFrame(frame));
    QVERIFY(RTCMFramer::isValidFrame(framer.frame()));
    QCOMPARE(framer.messageId(), messageId);
    QCOMPARE(framer.payloadLength(), extraPayload + 2);
    QCOMPARE(QByteArrayView(framer.frame()), QByteArrayView(frame));

    const auto savedFrame = QByteArrayView(framer.frame()).toByteArray();
    framer.reset();
    const auto replacement = GpsTestHelpers::buildRtcmFrame(1006, extraPayload);
    for (const char byte : replacement) {
        framer.addByte(static_cast<uint8_t>(byte));
    }
    QCOMPARE(QByteArrayView(framer.frame()), QByteArrayView(replacement));
    QCOMPARE(savedFrame, frame);
    QVERIFY(framer.valid());
    QVERIFY(!framer.addByte(0));
    QVERIFY(!framer.valid());
    QVERIFY(!framer.hasPartialFrame());

    auto corrupted = replacement;
    corrupted.back() ^= 1;
    for (const char byte : corrupted) {
        framer.addByte(static_cast<uint8_t>(byte));
    }
    QVERIFY(!framer.valid());
    QVERIFY(!framer.valid());
    for (const char byte : replacement) {
        framer.addByte(static_cast<uint8_t>(byte));
    }
    QVERIFY(framer.valid());
}

void RTCMFramerTest::_frameViewAndReset()
{
    const auto frame = QByteArray::fromHex("d300023ed0a4e000");
    RTCMFramer framer;
    QVERIFY(framer.frame().empty());
    QVERIFY(!framer.valid());
    QVERIFY(!framer.hasPartialFrame());
    QVERIFY(!framer.nextFrame());
    QVERIFY(!framer.valid());
    for (qsizetype index = 0; index < frame.size() - 1; ++index) {
        QVERIFY(!framer.addByte(static_cast<uint8_t>(frame[index])));
        QVERIFY(framer.hasPartialFrame());
        QVERIFY(framer.frame().empty());
        QVERIFY(!framer.valid());
        QCOMPARE(framer.bufferedSize(), index + 1);
    }
    QVERIFY(framer.addByte(static_cast<uint8_t>(frame.back())));
    QVERIFY(!framer.hasPartialFrame());
    QVERIFY(framer.valid());
    const RTCMFramer& constFramer = framer;
    const std::span<const uint8_t> view = constFramer.frame();
    QCOMPARE(QByteArray(reinterpret_cast<const char*>(view.data()), static_cast<qsizetype>(view.size())), frame);
    QVERIFY(!framer.nextFrame());
    QVERIFY(framer.frame().empty());
    QCOMPARE(framer.bufferedSize(), 0);

    QVERIFY(!framer.addByte(RTCMFramer::PREAMBLE));
    framer.reset();
    QVERIFY(!framer.hasPartialFrame());
    QVERIFY(framer.frame().empty());
    QCOMPARE(framer.bufferedSize(), 0);
    QCOMPARE(framer.payloadLength(), 0);
    QCOMPARE(framer.messageId(), 0);
    QVERIFY(!framer.valid());
}

void RTCMFramerTest::_implicitAdvance_data()
{
    QTest::addColumn<QByteArray>("stream");
    QTest::addColumn<QList<QByteArray>>("expectedFrames");
    QTest::addColumn<QList<QByteArray>>("expectedRejected");

    const auto frame = QByteArray::fromHex("d300023ed0a4e000");
    QTest::newRow("consecutive-frames") << frame + frame << QList<QByteArray>{frame, frame} << QList<QByteArray>{};
    const auto nested = QByteArray::fromHex("d3000a4350d300023ed0a4e0000d07f0");
    const auto interrupted = QByteArray::fromHex("d300134350") + frame + nested.first(12);
    QTest::newRow("fresh-bytes-outside-recovery")
        << interrupted + nested.mid(12) << QList<QByteArray>{frame, nested} << QList<QByteArray>{interrupted};
}

void RTCMFramerTest::_implicitAdvance()
{
    QFETCH(QByteArray, stream);
    QFETCH(QList<QByteArray>, expectedFrames);
    QFETCH(QList<QByteArray>, expectedRejected);
    RTCMFramer framer;
    QList<QByteArray> frames;
    QList<QByteArray> rejected;
    for (const char byte : stream) {
        if (framer.addByte(static_cast<uint8_t>(byte))) {
            const auto view = framer.frame();
            const QByteArray candidate(reinterpret_cast<const char*>(view.data()), static_cast<qsizetype>(view.size()));
            if (framer.valid()) {
                frames.append(candidate);
            } else {
                rejected.append(candidate);
            }
        }
        QVERIFY(framer.bufferedSize() <= RTCMFramer::MAX_FRAME_SIZE);
    }
    QCOMPARE(frames, expectedFrames);
    QCOMPARE(rejected, expectedRejected);
}

UT_REGISTER_TEST(RTCMFramerTest, TestLabel::Unit)
