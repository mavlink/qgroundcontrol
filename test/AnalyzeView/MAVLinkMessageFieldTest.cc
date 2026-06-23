#include "MAVLinkMessageFieldTest.h"
#include <QtTest/QSignalSpy>

#include <memory>


#include "MAVLinkMessage.h"
#include "MAVLinkMessageField.h"
#include "MAVLinkTestHelpers.h"

void MAVLinkMessageFieldTest::_constructionTest()
{
    auto msg = std::unique_ptr<QGCMAVLinkMessage>(MAVLinkTestHelpers::makeHeartbeatMsg());

    // Pass nullptr as parent to avoid double-free (stack field + parent delete)
    QGCMAVLinkMessageField field(QStringLiteral("type"), QStringLiteral("uint8_t"), msg.get());
    field.setParent(nullptr);

    QCOMPARE(field.name(), QStringLiteral("type"));
    QCOMPARE(field.type(), QStringLiteral("uint8_t"));
}

void MAVLinkMessageFieldTest::_initialValueEmptyTest()
{
    auto msg = std::unique_ptr<QGCMAVLinkMessage>(MAVLinkTestHelpers::makeHeartbeatMsg());

    QGCMAVLinkMessageField field(QStringLiteral("type"), QStringLiteral("uint8_t"), msg.get());
    field.setParent(nullptr);

    QVERIFY(field.value().isEmpty());
}

void MAVLinkMessageFieldTest::_selectableDefaultsTrueTest()
{
    auto msg = std::unique_ptr<QGCMAVLinkMessage>(MAVLinkTestHelpers::makeHeartbeatMsg());

    QGCMAVLinkMessageField field(QStringLiteral("type"), QStringLiteral("uint8_t"), msg.get());
    field.setParent(nullptr);

    QVERIFY(field.selectable());
}

void MAVLinkMessageFieldTest::_setSelectableTest()
{
    auto msg = std::unique_ptr<QGCMAVLinkMessage>(MAVLinkTestHelpers::makeHeartbeatMsg());
    QGCMAVLinkMessageField field(QStringLiteral("type"), QStringLiteral("uint8_t"), msg.get());
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
}

void MAVLinkMessageFieldTest::_updateValueChangesValueTest()
{
    auto msg = std::unique_ptr<QGCMAVLinkMessage>(MAVLinkTestHelpers::makeHeartbeatMsg());
    QGCMAVLinkMessageField field(QStringLiteral("type"), QStringLiteral("uint8_t"), msg.get());
    field.setParent(nullptr);

    QSignalSpy valueSpy(&field, &QGCMAVLinkMessageField::valueChanged);

    field.updateValue(QStringLiteral("2"), 2.0);
    QCOMPARE(field.value(), QStringLiteral("2"));
    QCOMPARE(valueSpy.count(), 1);
}

void MAVLinkMessageFieldTest::_updateValueNoopOnSameValueTest()
{
    auto msg = std::unique_ptr<QGCMAVLinkMessage>(MAVLinkTestHelpers::makeHeartbeatMsg());
    QGCMAVLinkMessageField field(QStringLiteral("type"), QStringLiteral("uint8_t"), msg.get());
    field.setParent(nullptr);

    field.updateValue(QStringLiteral("5"), 5.0);

    QSignalSpy valueSpy(&field, &QGCMAVLinkMessageField::valueChanged);

    // Calling with the same string value must not emit valueChanged
    field.updateValue(QStringLiteral("5"), 5.0);
    QCOMPARE(valueSpy.count(), 0);
}

void MAVLinkMessageFieldTest::_labelFormatTest()
{
    auto msg = std::unique_ptr<QGCMAVLinkMessage>(MAVLinkTestHelpers::makeHeartbeatMsg());
    QGCMAVLinkMessageField field(QStringLiteral("autopilot"), QStringLiteral("uint8_t"), msg.get());
    field.setParent(nullptr);

    // label() must follow the pattern "<message_name>: <field_name>"
    const QString expected = QStringLiteral("HEARTBEAT: autopilot");
    QCOMPARE(field.label(), expected);
}

UT_REGISTER_TEST(MAVLinkMessageFieldTest, TestLabel::Unit, TestLabel::AnalyzeView)
