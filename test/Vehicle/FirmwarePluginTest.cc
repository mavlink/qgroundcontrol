#include "FirmwarePluginTest.h"

#include <QtCore/QString>

#include "ArduCopterFirmwarePlugin.h"
#include "ArduSubFirmwarePlugin.h"
#include "FirmwarePlugin.h"

void FirmwarePluginTest::_stableFirmwareNotifiedOncePerVersion_data()
{
    QTest::addColumn<QString>("latest");
    QTest::addColumn<QString>("seen");
    QTest::addColumn<bool>("expected");

    QTest::newRow("never seen") << "4.6.2" << "" << true;
    QTest::newRow("already seen") << "4.6.2" << "4.6.2" << false;
    QTest::newRow("newer than seen") << "4.6.3" << "4.6.2" << true;
    QTest::newRow("older than seen") << "4.6.1" << "4.6.2" << false;
    QTest::newRow("corrupt setting") << "4.6.2" << "junk" << true;
    QTest::newRow("unparseable latest") << "junk" << "" << false;
}

void FirmwarePluginTest::_stableFirmwareNotifiedOncePerVersion()
{
    QFETCH(QString, latest);
    QFETCH(QString, seen);
    QFETCH(bool, expected);

    QCOMPARE(FirmwarePlugin::isStableFirmwareVersionUnseen(latest, seen), expected);
}

void FirmwarePluginTest::_arduSubHasNoRtlFlightMode()
{
    QVERIFY(ArduSubFirmwarePlugin().rtlFlightMode().isEmpty());
    QVERIFY(!ArduCopterFirmwarePlugin().rtlFlightMode().isEmpty());
}

void FirmwarePluginTest::_apmSplineWaypointOnlyForMultiRotor_data()
{
    QTest::addColumn<QGCMAVLink::VehicleClass_t>("vehicleClass");
    QTest::addColumn<bool>("expected");

    QTest::newRow("generic") << QGCMAVLink::VehicleClassGeneric << true;
    QTest::newRow("multirotor") << QGCMAVLink::VehicleClassMultiRotor << true;
    QTest::newRow("fixed wing") << QGCMAVLink::VehicleClassFixedWing << false;
    QTest::newRow("vtol") << QGCMAVLink::VehicleClassVTOL << false;
    QTest::newRow("rover") << QGCMAVLink::VehicleClassRoverBoat << false;
    QTest::newRow("sub") << QGCMAVLink::VehicleClassSub << false;
}

void FirmwarePluginTest::_apmSplineWaypointOnlyForMultiRotor()
{
    QFETCH(QGCMAVLink::VehicleClass_t, vehicleClass);
    QFETCH(bool, expected);

    const QList<MAV_CMD> commands = ArduCopterFirmwarePlugin().supportedMissionCommands(vehicleClass);
    QCOMPARE(commands.contains(MAV_CMD_NAV_SPLINE_WAYPOINT), expected);
}

UT_REGISTER_TEST(FirmwarePluginTest, TestLabel::Unit, TestLabel::Vehicle)
