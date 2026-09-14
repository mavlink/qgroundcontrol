#include "RTCMFramerTest.h"

#include "RTCMFramer.h"

void RTCMFramerTest::_frameViewAndReset()
{
    const auto frame = QByteArray::fromHex("d300023ed0a4e000");
    RTCMFramer framer;
    QVERIFY(framer.frame().empty());
    QVERIFY(!framer.hasPartialFrame());
    QVERIFY(!framer.nextFrame());
    for (qsizetype index = 0; index < frame.size() - 1; ++index) {
        QVERIFY(!framer.addByte(static_cast<uint8_t>(frame[index])));
        QVERIFY(framer.hasPartialFrame());
        QVERIFY(framer.frame().empty());
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
}

void RTCMFramerTest::_implicitAdvance()
{
    const auto frame = QByteArray::fromHex("d300023ed0a4e000");
    RTCMFramer framer;
    int completed = 0;
    for (const char byte : frame + frame) {
        if (framer.addByte(static_cast<uint8_t>(byte))) {
            QVERIFY(framer.valid());
            ++completed;
        }
    }
    QCOMPARE(completed, 2);
}

#ifdef QGC_GPS_STANDALONE_TEST
QTEST_GUILESS_MAIN(RTCMFramerTest)
#else
UT_REGISTER_TEST(RTCMFramerTest, TestLabel::Unit)
#endif
