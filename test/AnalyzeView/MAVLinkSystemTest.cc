#include "MAVLinkSystemTest.h"
#include <QtTest/QSignalSpy>


#include "MAVLinkMessage.h"
#include "MAVLinkSystem.h"
#include "MAVLinkTestHelpers.h"
#include "QmlObjectListModel.h"

void MAVLinkSystemTest::_constructionTest()
{
    QGCMAVLinkSystem system(7);

    QCOMPARE(system.id(), static_cast<quint8>(7));
}

void MAVLinkSystemTest::_initiallyEmptyMessagesTest()
{
    QGCMAVLinkSystem system(1);

    QVERIFY(system.messages() != nullptr);
    QCOMPARE(system.messages()->count(), 0);
}

void MAVLinkSystemTest::_initiallyNoCompIDsTest()
{
    QGCMAVLinkSystem system(1);

    QVERIFY(system.compIDs().isEmpty());
    QVERIFY(system.compIDsStr().isEmpty());
}

void MAVLinkSystemTest::_appendAndFindMessageTest()
{
    QGCMAVLinkSystem system(1);

    QGCMAVLinkMessage *msg = MAVLinkTestHelpers::makeHeartbeatMsg(1, 1);
    system.append(msg);

    QCOMPARE(system.messages()->count(), 1);

    QGCMAVLinkMessage *found = system.findMessage(MAVLINK_MSG_ID_HEARTBEAT, 1);
    QVERIFY(found != nullptr);
    QCOMPARE(found->id(), static_cast<quint32>(MAVLINK_MSG_ID_HEARTBEAT));
    QCOMPARE(found->compId(), static_cast<quint8>(1));
}

void MAVLinkSystemTest::_findMessageNotFoundTest()
{
    QGCMAVLinkSystem system(1);

    // findMessage on an empty system returns nullptr
    QGCMAVLinkMessage *found = system.findMessage(MAVLINK_MSG_ID_HEARTBEAT, 1);
    QVERIFY(found == nullptr);

    // findMessage with a wrong compId also returns nullptr
    QGCMAVLinkMessage *msg = MAVLinkTestHelpers::makeHeartbeatMsg(1, 1);
    system.append(msg);

    QGCMAVLinkMessage *wrongComp = system.findMessage(MAVLINK_MSG_ID_HEARTBEAT, 99);
    QVERIFY(wrongComp == nullptr);
}

void MAVLinkSystemTest::_findMessageByPointerTest()
{
    QGCMAVLinkSystem system(1);

    QGCMAVLinkMessage *msg = MAVLinkTestHelpers::makeHeartbeatMsg(1, 1);
    system.append(msg);

    const int idx = system.findMessage(msg);
    QCOMPARE(idx, 0);

    // A pointer that was never appended returns -1
    QGCMAVLinkMessage fakeMsg(mavlink_message_t{});
    QCOMPARE(system.findMessage(&fakeMsg), -1);
}

void MAVLinkSystemTest::_setSelectedAndSelectedMsgTest()
{
    QGCMAVLinkSystem system(1);

    // Two messages with different msg IDs so they sort differently
    mavlink_message_t raw1{};
    mavlink_msg_heartbeat_pack_chan(1, 1, MAVLINK_COMM_0, &raw1,
                                   MAV_TYPE_QUADROTOR, MAV_AUTOPILOT_PX4, 0, 0, MAV_STATE_ACTIVE);
    auto *msg1 = new QGCMAVLinkMessage(raw1);

    // Use a second heartbeat with a different compId so they coexist
    mavlink_message_t raw2{};
    mavlink_msg_heartbeat_pack_chan(1, 2, MAVLINK_COMM_0, &raw2,
                                   MAV_TYPE_QUADROTOR, MAV_AUTOPILOT_PX4, 0, 0, MAV_STATE_ACTIVE);
    auto *msg2 = new QGCMAVLinkMessage(raw2);

    system.append(msg1);
    system.append(msg2);

    QSignalSpy selectedSpy(&system, &QGCMAVLinkSystem::selectedChanged);

    system.setSelected(0);
    QVERIFY(selectedSpy.count() >= 1);

    QGCMAVLinkMessage *sel = system.selectedMsg();
    QVERIFY(sel != nullptr);
    // The selected message must be the one at index 0 of the sorted list
    QCOMPARE(system.findMessage(sel), 0);
}

void MAVLinkSystemTest::_setSelectedOutOfBoundsTest()
{
    QGCMAVLinkSystem system(1);

    QGCMAVLinkMessage *msg = MAVLinkTestHelpers::makeHeartbeatMsg();
    system.append(msg);

    QSignalSpy selectedSpy(&system, &QGCMAVLinkSystem::selectedChanged);

    // Index equal to count is out of bounds - must be silently ignored
    system.setSelected(1);
    QCOMPARE(selectedSpy.count(), 0);
}

void MAVLinkSystemTest::_appendUpdatesCompIDsTest()
{
    QGCMAVLinkSystem system(1);

    QSignalSpy compSpy(&system, &QGCMAVLinkSystem::compIDsChanged);

    QGCMAVLinkMessage *msg = MAVLinkTestHelpers::makeHeartbeatMsg(1, 42);
    system.append(msg);

    QCOMPARE(compSpy.count(), 1);
    QVERIFY(system.compIDs().contains(42));
    QVERIFY(!system.compIDsStr().isEmpty());

    // Appending another message with the same compId must not emit again
    QGCMAVLinkMessage *msg2 = MAVLinkTestHelpers::makeHeartbeatMsg(1, 42);
    // Give it a different msgid so it is a distinct entry - use a raw msg
    mavlink_message_t raw{};
    mavlink_msg_heartbeat_pack_chan(1, 42, MAVLINK_COMM_1, &raw,
                                   MAV_TYPE_FIXED_WING, MAV_AUTOPILOT_GENERIC, 0, 0, MAV_STATE_ACTIVE);
    auto *msg3 = new QGCMAVLinkMessage(raw);
    delete msg2;  // not used
    system.append(msg3);

    QCOMPARE(compSpy.count(), 1);
}

void MAVLinkSystemTest::_selectedMsgOnEmptySystemTest()
{
    QGCMAVLinkSystem system(1);

    QVERIFY(system.selectedMsg() == nullptr);
}

UT_REGISTER_TEST(MAVLinkSystemTest, TestLabel::Unit, TestLabel::AnalyzeView)
