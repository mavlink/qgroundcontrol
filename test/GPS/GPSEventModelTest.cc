#include "GPSEventModelTest.h"
#include "GPSEventModel.h"

#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

void GPSEventModelTest::testInitialState()
{
    GPSEventModel model;
    QCOMPARE(model.rowCount(), 0);
    QCOMPARE(model.count(), 0);
}

void GPSEventModelTest::testAppend()
{
    GPSEventModel model;
    QSignalSpy countSpy(&model, &GPSEventModel::countChanged);

    model.append(GPSEvent::info(GPSEvent::Source::GPS, QStringLiteral("test")), 10);
    QCOMPARE(model.rowCount(), 1);
    QCOMPARE(countSpy.count(), 1);

    model.append(GPSEvent::warning(GPSEvent::Source::NTRIP, QStringLiteral("warn")), 10);
    QCOMPARE(model.rowCount(), 2);
}

void GPSEventModelTest::testMaxSize()
{
    GPSEventModel model;

    for (int i = 0; i < 5; ++i) {
        model.append(GPSEvent::info(GPSEvent::Source::GPS, QString::number(i)), 3);
    }

    QCOMPARE(model.rowCount(), 3);

    const QModelIndex last = model.index(2, 0);
    QCOMPARE(model.data(last, GPSEventModel::MessageRole).toString(), QStringLiteral("4"));
}

void GPSEventModelTest::testClear()
{
    GPSEventModel model;
    model.append(GPSEvent::info(GPSEvent::Source::GPS, QStringLiteral("a")), 10);
    model.append(GPSEvent::info(GPSEvent::Source::GPS, QStringLiteral("b")), 10);

    QSignalSpy countSpy(&model, &GPSEventModel::countChanged);
    model.clear();

    QCOMPARE(model.rowCount(), 0);
    QCOMPARE(countSpy.count(), 1);
}

void GPSEventModelTest::testRoleNames()
{
    GPSEventModel model;
    const auto roles = model.roleNames();

    QVERIFY(roles.contains(GPSEventModel::TimestampRole));
    QVERIFY(roles.contains(GPSEventModel::SeverityRole));
    QVERIFY(roles.contains(GPSEventModel::SourceRole));
    QVERIFY(roles.contains(GPSEventModel::MessageRole));
    QCOMPARE(roles.value(GPSEventModel::MessageRole), QByteArrayLiteral("message"));
}

void GPSEventModelTest::testDataRoles()
{
    GPSEventModel model;
    model.append(GPSEvent::error(GPSEvent::Source::RTKBase, QStringLiteral("fail")), 10);

    const QModelIndex idx = model.index(0, 0);
    QCOMPARE(model.data(idx, GPSEventModel::SeverityRole).toString(), QStringLiteral("Error"));
    QCOMPARE(model.data(idx, GPSEventModel::SourceRole).toString(), QStringLiteral("RTK Base"));
    QCOMPARE(model.data(idx, GPSEventModel::MessageRole).toString(), QStringLiteral("fail"));
    QVERIFY(!model.data(idx, GPSEventModel::TimestampRole).toString().isEmpty());
}

UT_REGISTER_TEST(GPSEventModelTest, TestLabel::Unit)
