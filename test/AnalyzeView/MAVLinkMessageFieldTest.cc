#include "MAVLinkMessageFieldTest.h"

#include <QtTest/QSignalSpy>

#include "MAVLinkLib.h"
#include "MAVLinkMessage.h"
#include "MAVLinkMessageField.h"

namespace {

QGCMAVLinkMessage *makeHeartbeatMsg(QObject *parent = nullptr)
{
    mavlink_message_t msg{};
    mavlink_msg_heartbeat_pack_chan(1, 1, MAVLINK_COMM_0, &msg,
                                   MAV_TYPE_QUADROTOR, MAV_AUTOPILOT_PX4, 0, 0, MAV_STATE_ACTIVE);
    return new QGCMAVLinkMessage(msg, parent);
}

} // namespace

void MAVLinkMessageFieldTest::_constructionTest()
{
    QGCMAVLinkMessage *msg = makeHeartbeatMsg();

    // Pass nullptr as parent to avoid double-free (stack field + parent delete)
    QGCMAVLinkMessageField field(QStringLiteral("type"), QStringLiteral("uint8_t"), msg);
    field.setParent(nullptr);

    QCOMPARE(field.name(), QStringLiteral("type"));
    QCOMPARE(field.type(), QStringLiteral("uint8_t"));

    delete msg;
}

void MAVLinkMessageFieldTest::_initialValueEmptyTest()
{
    QGCMAVLinkMessage *msg = makeHeartbeatMsg();

    QGCMAVLinkMessageField field(QStringLiteral("type"), QStringLiteral("uint8_t"), msg);
    field.setParent(nullptr);

    QVERIFY(field.value().isEmpty());

    delete msg;
}

void MAVLinkMessageFieldTest::_selectableDefaultsTrueTest()
{
    QGCMAVLinkMessage *msg = makeHeartbeatMsg();

    QGCMAVLinkMessageField field(QStringLiteral("type"), QStringLiteral("uint8_t"), msg);
    field.setParent(nullptr);

    QVERIFY(field.selectable());

    delete msg;
}

void MAVLinkMessageFieldTest::_setSelectableTest()
{
    QGCMAVLinkMessage *msg = makeHeartbeatMsg();
    QGCMAVLinkMessageField field(QStringLiteral("type"), QStringLiteral("uint8_t"), msg);
    field.setParent(nullptr);

    QSignalSpy selectableSpy(&field, &QGCMAVLinkMessageField::selectableChanged);

    field.setSelectable(false);
    QVERIFY(!field.selectable());
    QCOMPARE(selectableSpy.count(), 1);

    // Same value must not emit
    field.setSelectable(false);
    QCOMPARE(selectableSpy.count(), 1);

    field.setSelectable(true);
    QVERIFY(field.selectable());
    QCOMPARE(selectableSpy.count(), 2);

    delete msg;
}

void MAVLinkMessageFieldTest::_updateValueChangesValueTest()
{
    QGCMAVLinkMessage *msg = makeHeartbeatMsg();
    QGCMAVLinkMessageField field(QStringLiteral("type"), QStringLiteral("uint8_t"), msg);
    field.setParent(nullptr);

    QSignalSpy valueSpy(&field, &QGCMAVLinkMessageField::valueChanged);

    field.updateValue(QStringLiteral("2"), 2.0);
    QCOMPARE(field.value(), QStringLiteral("2"));
    QCOMPARE(valueSpy.count(), 1);

    delete msg;
}

void MAVLinkMessageFieldTest::_updateValueNoopOnSameValueTest()
{
    QGCMAVLinkMessage *msg = makeHeartbeatMsg();
    QGCMAVLinkMessageField field(QStringLiteral("type"), QStringLiteral("uint8_t"), msg);
    field.setParent(nullptr);

    field.updateValue(QStringLiteral("5"), 5.0);

    QSignalSpy valueSpy(&field, &QGCMAVLinkMessageField::valueChanged);

    // Calling with the same string value must not emit valueChanged
    field.updateValue(QStringLiteral("5"), 5.0);
    QCOMPARE(valueSpy.count(), 0);

    delete msg;
}

void MAVLinkMessageFieldTest::_labelFormatTest()
{
    QGCMAVLinkMessage *msg = makeHeartbeatMsg();
    QGCMAVLinkMessageField field(QStringLiteral("autopilot"), QStringLiteral("uint8_t"), msg);
    field.setParent(nullptr);

    // label() must follow the pattern "<message_name>: <field_name>"
    const QString expected = QStringLiteral("HEARTBEAT: autopilot");
    QCOMPARE(field.label(), expected);

    delete msg;
}

UT_REGISTER_TEST(MAVLinkMessageFieldTest, TestLabel::Unit, TestLabel::AnalyzeView)
