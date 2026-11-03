#include "BatteryFactGroupTest.h"

#include <QtTest/QTest>

#include "BatteryFactGroupListModel.h"
#include "MAVLinkLib.h"

using CellList = QList<uint16_t>;

void BatteryFactGroupTest::_batteryStatusVoltage_data()
{
    QTest::addColumn<CellList>("cells");
    QTest::addColumn<CellList>("extCells");
    QTest::addColumn<double>("expected");

    const CellList tenCells(10, 4000);

    QTest::newRow("4S, ext unused") << CellList(4, 4000) << CellList(4, 0) << 16.0;
    QTest::newRow("4S, ext UINT16_MAX") << CellList(4, 4000) << CellList(4, UINT16_MAX) << 16.0;
    QTest::newRow("4S, ext garbage") << CellList(4, 4000) << CellList(4, 4000) << 16.0;
    QTest::newRow("10S") << tenCells << CellList(4, 0) << 40.0;
    QTest::newRow("12S") << tenCells << CellList{4000, 4000, 0, 0} << 48.0;
    QTest::newRow("12S, ext UINT16_MAX") << tenCells << CellList{4000, 4000, UINT16_MAX, UINT16_MAX} << 48.0;
    QTest::newRow("14S") << tenCells << CellList(4, 4000) << 56.0;
}

void BatteryFactGroupTest::_batteryStatusVoltage()
{
    QFETCH(CellList, cells);
    QFETCH(CellList, extCells);
    QFETCH(double, expected);

    mavlink_battery_status_t batteryStatus{};
    for (qsizetype i = 0; i < 10; i++) {
        batteryStatus.voltages[i] = (i < cells.size()) ? cells[i] : UINT16_MAX;
    }
    for (qsizetype i = 0; i < 4; i++) {
        batteryStatus.voltages_ext[i] = extCells[i];
    }

    mavlink_message_t message{};
    (void) mavlink_msg_battery_status_encode(1, 1, &message, &batteryStatus);

    BatteryFactGroup battery(0);
    battery.handleMessage(nullptr, message);

    QCOMPARE(battery.voltage()->rawValue().toDouble(), expected);
}

UT_REGISTER_TEST(BatteryFactGroupTest, TestLabel::Unit, TestLabel::Vehicle)
