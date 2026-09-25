#include "RTCMMAVLinkTest.h"

#include <QtCore/QPointer>
#include <QtCore/QScopeGuard>

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
    QVERIFY(sender.submitToOutputs(bytes).isEmpty());
    QCOMPARE(sender.totalBytesSubmitted(), 0ULL);
    int firstCalls = 0;
    int secondCalls = 0;
    sender.setOutputProvider([&]() {
        const RTCMMAVLink::Output first{QStringLiteral("link1"), 1, [&](const GPSRTCMPacket&) {
                                            ++firstCalls;
                                            return true;
                                        }};
        const RTCMMAVLink::Output second{QStringLiteral("link2"), 2, [&](const GPSRTCMPacket&) {
                                             ++secondCalls;
                                             return true;
                                         }};
        return QList<RTCMMAVLink::Output>{first, second, first};
    });
    const auto admitted = sender.submitToOutputs(bytes);
    QCOMPARE(admitted.size(), 2);
    QCOMPARE(firstCalls, 3);
    QCOMPARE(secondCalls, 3);
    QVERIFY(admitted[0].complete);
    QVERIFY(admitted[1].complete);
    QCOMPARE(admitted[0].queuedBytes, 360ULL);
    QCOMPARE(admitted[1].queuedBytes, 360ULL);
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
    sender.setOutputProvider([&]() {
        return QList<RTCMMAVLink::Output>{
            {QStringLiteral("link"), 7, [&](const GPSRTCMPacket&) { return ++calls < rejectAt; }}};
    });
    const auto admitted = sender.submitToOutputs(makePayload(360));
    QCOMPARE(admitted.size(), 1);
    QCOMPARE(admitted[0].queuedBytes, queuedBytes);
    QCOMPARE(admitted[0].session, 7ULL);
    QVERIFY(!admitted[0].complete);
    QCOMPARE(calls, rejectAt);
    QCOMPARE(sender.totalBytesSubmitted(), queuedBytes);
}

void RTCMMAVLinkTest::_testOutputReplacementAndDeletion()
{
    RTCMMAVLink sender;
    int oldCalls = 0;
    int replacementCalls = 0;
    sender.setOutputProvider([&]() {
        return QList<RTCMMAVLink::Output>{{QStringLiteral("old"), 1, [&](const GPSRTCMPacket&) {
                                               ++oldCalls;
                                               sender.setOutputProvider([&]() {
                                                   return QList<RTCMMAVLink::Output>{
                                                       {QStringLiteral("new"), 2, [&](const GPSRTCMPacket& packet) {
                                                            ++replacementCalls;
                                                            return sequenceId(packet.flags) == uint8_t(1);
                                                        }}};
                                               });
                                               return true;
                                           }}};
    });
    const auto first = sender.submitToOutputs(makePayload(360));
    QCOMPARE(first.size(), 1);
    QCOMPARE(first[0].queuedBytes, 180ULL);
    QVERIFY(!first[0].complete);
    QCOMPARE(oldCalls, 1);
    QCOMPARE(replacementCalls, 0);
    QCOMPARE(sender.totalBytesSubmitted(), 180ULL);
    const auto second = sender.submitToOutputs(makePayload(360));
    QVERIFY(second[0].complete);
    QCOMPARE(replacementCalls, 3);
    QPointer<RTCMMAVLink> destroyed = new RTCMMAVLink;
    destroyed->setOutputProvider([&]() {
        return QList<RTCMMAVLink::Output>{{QStringLiteral("deleted"), 1, [&](const GPSRTCMPacket&) {
                                               delete destroyed.data();
                                               return true;
                                           }}};
    });
    const auto retired = destroyed->submitToOutputs(makePayload(360));
    QVERIFY(destroyed.isNull());
    QCOMPARE(retired.size(), 1);
    QCOMPARE(retired[0].queuedBytes, 180ULL);
    QVERIFY(!retired[0].complete);
}

void RTCMMAVLinkTest::_testFinalAdmissionRetirement_data()
{
    QTest::addColumn<int>("size");
    QTest::addColumn<bool>("destroy");
    QTest::newRow("single-payload-replace") << 30 << false;
    QTest::newRow("final-payload-replace") << 181 << false;
    QTest::newRow("terminator-replace") << 360 << false;
    QTest::newRow("single-payload-delete") << 30 << true;
    QTest::newRow("final-payload-delete") << 181 << true;
    QTest::newRow("terminator-delete") << 360 << true;
}

void RTCMMAVLinkTest::_testFinalAdmissionRetirement()
{
    QFETCH(int, size);
    QFETCH(bool, destroy);
    QPointer<RTCMMAVLink> sender = new RTCMMAVLink;
    const auto cleanup = qScopeGuard([&]() { delete sender.data(); });
    const auto bytes = makePayload(size);
    const auto packetCount = RTCMMAVLinkPacket::pack(bytes, 0).packets.size();
    qsizetype calls = 0;
    int laterCalls = 0;
    sender->setOutputProvider([&]() {
        return QList<RTCMMAVLink::Output>{{QStringLiteral("retired"), 7,
                                           [&](const GPSRTCMPacket&) {
                                               if (++calls == packetCount) {
                                                   if (destroy) {
                                                       delete sender.data();
                                                   } else {
                                                       sender->setOutputProvider({});
                                                   }
                                               }
                                               return true;
                                           }},
                                          {QStringLiteral("later"), 8, [&](const GPSRTCMPacket&) {
                                               ++laterCalls;
                                               return true;
                                           }}};
    });
    const auto admissions = sender->submitToOutputs(bytes);
    QCOMPARE(admissions.size(), 1);
    QVERIFY(admissions.first().complete);
    QCOMPARE(admissions.first().queuedBytes, quint64(size));
    QCOMPARE(admissions.first().session, 7ULL);
    QCOMPARE(calls, packetCount);
    QCOMPARE(laterCalls, 0);
    QCOMPARE(sender.isNull(), destroy);
    if (sender) {
        QCOMPARE(sender->totalBytesSubmitted(), quint64(size));
    }
}

void RTCMMAVLinkTest::_testEmpty()
{
    RTCMMAVLink sender;
    int calls = 0;
    sender.setOutputProvider([&]() {
        ++calls;
        return QList<RTCMMAVLink::Output>();
    });
    QVERIFY(sender.submitToOutputs({}).isEmpty());
    QCOMPARE(calls, 0);
    QCOMPARE(sender.totalBytesSent(), 0ULL);
    QCOMPARE(sender.totalBytesSubmitted(), 0ULL);
}

void RTCMMAVLinkTest::_testSequenceAdvances()
{
    RTCMMAVLink sender;
    QList<uint8_t> firstSequences;
    QList<uint8_t> secondSequences;
    sender.setOutputProvider([&]() {
        return QList<RTCMMAVLink::Output>{{QStringLiteral("first"), 1,
                                           [&](const GPSRTCMPacket& packet) {
                                               if (((packet.flags >> 1) & 3) == 0) {
                                                   firstSequences.append(sequenceId(packet.flags));
                                               }
                                               return true;
                                           }},
                                          {QStringLiteral("second"), 2, [&](const GPSRTCMPacket& packet) {
                                               if (((packet.flags >> 1) & 3) == 0) {
                                                   secondSequences.append(sequenceId(packet.flags));
                                               }
                                               return true;
                                           }}};
    });
    const auto bytes = makePayload(360);
    for (int index = 0; index < 33; ++index) {
        const auto admissions = sender.submitToOutputs(bytes);
        QCOMPARE(admissions.size(), 2);
        for (const auto& admission : admissions) {
            QVERIFY(admission.complete);
            QCOMPARE(admission.queuedBytes, quint64(bytes.size()));
        }
        QCOMPARE(sender.totalBytesSubmitted(), quint64((index + 1) * 2 * bytes.size()));
        QCOMPARE(firstSequences.size(), index + 1);
        QCOMPARE(firstSequences.last(), uint8_t(index & 31));
    }
    QCOMPARE(firstSequences, secondSequences);
}

UT_REGISTER_TEST(RTCMMAVLinkTest, TestLabel::Unit)
