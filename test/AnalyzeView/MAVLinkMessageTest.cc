#include "MAVLinkMessageTest.h"
#include <QtTest/QSignalSpy>


#include "MAVLinkMessage.h"
#include "MAVLinkTestHelpers.h"
#include "QmlObjectListModel.h"

void MAVLinkMessageTest::_constructionTest()
{
    const mavlink_message_t msg = MAVLinkTestHelpers::makeHeartbeat(2, 3);
    QGCMAVLinkMessage message(msg);

    QCOMPARE(message.id(), static_cast<quint32>(MAVLINK_MSG_ID_HEARTBEAT));
    QCOMPARE(message.sysId(), static_cast<quint8>(2));
    QCOMPARE(message.compId(), static_cast<quint8>(3));
    QCOMPARE(message.name(), QStringLiteral("HEARTBEAT"));
}

void MAVLinkMessageTest::_countStartsAtOneTest()
{
    const mavlink_message_t msg = MAVLinkTestHelpers::makeHeartbeat();
    QGCMAVLinkMessage message(msg);

    QCOMPARE(message.count(), static_cast<quint64>(1));
}

void MAVLinkMessageTest::_updateIncrementsCountTest()
{
    const mavlink_message_t msg = MAVLinkTestHelpers::makeHeartbeat();
    QGCMAVLinkMessage message(msg);

    QSignalSpy countSpy(&message, &QGCMAVLinkMessage::countChanged);

    message.update(msg);
    QCOMPARE(message.count(), static_cast<quint64>(2));
    QCOMPARE(countSpy.count(), 1);

    message.update(msg);
    QCOMPARE(message.count(), static_cast<quint64>(3));
    QCOMPARE(countSpy.count(), 2);
}

void MAVLinkMessageTest::_setSelectedTest()
{
    const mavlink_message_t msg = MAVLinkTestHelpers::makeHeartbeat();
    QGCMAVLinkMessage message(msg);

    QSignalSpy selectedSpy(&message, &QGCMAVLinkMessage::selectedChanged);

    QVERIFY(!message.selected());

    message.setSelected(true);
    QVERIFY(message.selected());
    QCOMPARE(selectedSpy.count(), 1);

    // Setting to the same value must not emit again
    message.setSelected(true);
    QCOMPARE(selectedSpy.count(), 1);

    message.setSelected(false);
    QVERIFY(!message.selected());
    QCOMPARE(selectedSpy.count(), 2);
}

void MAVLinkMessageTest::_fieldsPopulatedTest()
{
    const mavlink_message_t msg = MAVLinkTestHelpers::makeHeartbeat();
    QGCMAVLinkMessage message(msg);

    const QmlObjectListModel *fields = message.fields();
    QVERIFY(fields != nullptr);

    // HEARTBEAT has 6 fields: type, autopilot, base_mode, custom_mode, system_status, mavlink_version
    const mavlink_message_info_t *info = mavlink_get_message_info(&msg);
    QVERIFY(info != nullptr);
    QCOMPARE(fields->count(), static_cast<int>(info->num_fields));
}

void MAVLinkMessageTest::_setTargetRateHzTest()
{
    const mavlink_message_t msg = MAVLinkTestHelpers::makeHeartbeat();
    QGCMAVLinkMessage message(msg);

    QSignalSpy rateSpy(&message, &QGCMAVLinkMessage::targetRateHzChanged);

    QCOMPARE(message.targetRateHz(), static_cast<int32_t>(0));

    message.setTargetRateHz(10);
    QCOMPARE(message.targetRateHz(), static_cast<int32_t>(10));
    QCOMPARE(rateSpy.count(), 1);

    // Same value must not emit
    message.setTargetRateHz(10);
    QCOMPARE(rateSpy.count(), 1);
}

void MAVLinkMessageTest::_updateFreqTest()
{
    const mavlink_message_t msg = MAVLinkTestHelpers::makeHeartbeat();
    QGCMAVLinkMessage message(msg);

    // Initial frequency is zero
    QCOMPARE_FUZZY(message.actualRateHz(), 0.0, 1e-9);

    // Simulate one update since last freq refresh
    message.update(msg);
    message.updateFreq();

    // _count started at 1, update() made it 2, _lastCount was 0
    // msgCount = 2 - 0 = 2; EWMA = 0.2*0 + 0.8*2 = 1.6
    QCOMPARE_FUZZY(message.actualRateHz(), 1.6, 1e-9);
}

UT_REGISTER_TEST(MAVLinkMessageTest, TestLabel::Unit, TestLabel::AnalyzeView)
