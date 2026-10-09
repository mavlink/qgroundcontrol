#include "RTCMMAVLinkTest.h"

#include "RTCMMAVLink.h"

namespace {

uint8_t sequenceId(uint8_t flags)
{
    return (flags >> 3) & 0x1FU;
}

QByteArray makePayload(qsizetype size, char fill = 'R')
{
    return QByteArray(size, fill);
}

}  // namespace

void RTCMMAVLinkTest::_testOutputFanout()
{
    RTCMMAVLink sender;
    const auto bytes = makePayload(360);
    sender.submitToOutputs(bytes);
    QCOMPARE(sender.totalBytesSubmitted(), 0ULL);
    int firstCalls = 0;
    int secondCalls = 0;
    sender.setOutputProvider([&]() {
        return QList<RTCMMAVLink::Output>{[&](const GPSRTCMPacket&) {
                                              ++firstCalls;
                                              return true;
                                          },
                                          [&](const GPSRTCMPacket&) {
                                              ++secondCalls;
                                              return true;
                                          }};
    });
    sender.submitToOutputs(bytes);
    // Two fragments and the zero-length terminator to each output.
    QCOMPARE(firstCalls, 3);
    QCOMPARE(secondCalls, 3);
    QCOMPARE(sender.totalBytesSubmitted(), 720ULL);
}

void RTCMMAVLinkTest::_testPartialOutput_data()
{
    QTest::addColumn<int>("rejectAt");
    QTest::addColumn<quint64>("queuedBytes");
    QTest::newRow("unavailable-before-first-packet") << 1 << quint64(0);
    QTest::newRow("disappears-after-first-packet") << 2 << quint64(180);
    QTest::newRow("terminator-not-admitted") << 3 << quint64(360);
}

void RTCMMAVLinkTest::_testPartialOutput()
{
    QFETCH(int, rejectAt);
    QFETCH(quint64, queuedBytes);
    RTCMMAVLink sender;
    int calls = 0;
    sender.setOutputProvider(
        [&]() { return QList<RTCMMAVLink::Output>{[&](const GPSRTCMPacket&) { return ++calls < rejectAt; }}; });
    sender.submitToOutputs(makePayload(360));
    // The output gets no further packet of the frame once it refuses one.
    QCOMPARE(calls, rejectAt);
    QCOMPARE(sender.totalBytesSubmitted(), queuedBytes);
}

void RTCMMAVLinkTest::_testEmpty()
{
    RTCMMAVLink sender;
    int calls = 0;
    sender.setOutputProvider([&]() {
        ++calls;
        return QList<RTCMMAVLink::Output>();
    });
    sender.submitToOutputs({});
    QCOMPARE(calls, 0);
    QCOMPARE(sender.totalBytesSubmitted(), 0ULL);
}

void RTCMMAVLinkTest::_testSequenceAdvances()
{
    RTCMMAVLink sender;
    QList<uint8_t> firstSequences;
    QList<uint8_t> secondSequences;
    const auto recordSequence = [](QList<uint8_t>& sequences) {
        return [&sequences](const GPSRTCMPacket& packet) {
            if (((packet.flags >> 1) & 3) == 0) {
                sequences.append(sequenceId(packet.flags));
            }
            return true;
        };
    };
    sender.setOutputProvider(
        [&]() { return QList<RTCMMAVLink::Output>{recordSequence(firstSequences), recordSequence(secondSequences)}; });
    const auto bytes = makePayload(360);
    for (int index = 0; index < 33; ++index) {
        sender.submitToOutputs(bytes);
        QCOMPARE(sender.totalBytesSubmitted(), quint64((index + 1) * 2 * bytes.size()));
        QCOMPARE(firstSequences.size(), index + 1);
        QCOMPARE(firstSequences.last(), uint8_t(index & 31));
    }
    QCOMPARE(firstSequences, secondSequences);
}

UT_REGISTER_TEST_LIGHTWEIGHT(RTCMMAVLinkTest, TestLabel::Unit)
