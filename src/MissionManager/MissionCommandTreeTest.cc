/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "MissionCommandTreeTest.h"
#include "QGCApplication.h"
#include "MissionCommandUIInfo.h"
#include "MissionCommandList.h"
#include "FactMetaData.h"

MissionCommandTreeTest::MissionCommandTreeTest(void)
{
    
}

void MissionCommandTreeTest::init(void)
{
    _commandTree = new MissionCommandTree(qgcApp(), qgcApp()->toolbox(), true /* unitTest */);
    _commandTree->setToolbox(qgcApp()->toolbox());
}

void MissionCommandTreeTest::cleanup(void)
{
    delete _commandTree;
}

QString MissionCommandTreeTest::_rawName(int id)
{
    return QString("UNITTEST_%1").arg(id);
}

QString MissionCommandTreeTest::_friendlyName(int id)
{
    return QString("Unit Test %1").arg(id);
}

QString MissionCommandTreeTest::_paramLabel(int index)
{
    return QString("param%1").arg(index);
}

/// Verifies that all values have been set
void MissionCommandTreeTest::_checkFullInfoMap(const MissionCommandUIInfo* uiInfo)
{
    QVERIFY(uiInfo->_infoMap.contains(MissionCommandUIInfo::_rawNameJsonKey));
    QVERIFY(uiInfo->_infoMap.contains(MissionCommandUIInfo::_categoryJsonKey));
    QVERIFY(uiInfo->_infoMap.contains(MissionCommandUIInfo::_descriptionJsonKey));
    QVERIFY(uiInfo->_infoMap.contains(MissionCommandUIInfo::_friendlyEditJsonKey));
    QVERIFY(uiInfo->_infoMap.contains(MissionCommandUIInfo::_friendlyNameJsonKey));
    QVERIFY(uiInfo->_infoMap.contains(MissionCommandUIInfo::_standaloneCoordinateJsonKey));
    QVERIFY(uiInfo->_infoMap.contains(MissionCommandUIInfo::_specifiesCoordinateJsonKey));
}

// Verifies that values match settings for base tree
void MissionCommandTreeTest::_checkBaseValues(const MissionCommandUIInfo* uiInfo, int command)
{
    QVERIFY(uiInfo != nullptr);
    _checkFullInfoMap(uiInfo);
    QCOMPARE(uiInfo->command(), (MAV_CMD)command);
    QCOMPARE(uiInfo->rawName(), _rawName(command));
    QCOMPARE(uiInfo->category(), QStringLiteral("category"));
    QCOMPARE(uiInfo->description(), QStringLiteral("description"));
    QCOMPARE(uiInfo->friendlyEdit(), true);
    QCOMPARE(uiInfo->friendlyName(), _friendlyName(command));
    QCOMPARE(uiInfo->isStandaloneCoordinate(), true);
    QCOMPARE(uiInfo->specifiesCoordinate(), true);
    for (int i=1; i<=7; i++) {
        bool showUI;
        const MissionCmdParamInfo* paramInfo = uiInfo->getParamInfo(i, showUI);
        QVERIFY(showUI);
        QVERIFY(paramInfo);
        QCOMPARE(paramInfo->decimalPlaces(), 1);
        QCOMPARE(paramInfo->defaultValue(), 1.0);
        QCOMPARE(paramInfo->enumStrings().count(), 2);
        QCOMPARE(paramInfo->enumStrings()[0], QStringLiteral("1"));
        QCOMPARE(paramInfo->enumStrings()[1], QStringLiteral("2"));
        QCOMPARE(paramInfo->enumValues().count(), 2);
        QCOMPARE(paramInfo->enumValues()[0].toDouble(), 1.0);
        QCOMPARE(paramInfo->enumValues()[1].toDouble(), 2.0);
        QCOMPARE(paramInfo->label(), _paramLabel(i));
        QCOMPARE(paramInfo->param(), i);
        QCOMPARE(paramInfo->units(), QStringLiteral("units"));
    }
}

// Verifies that values match settings for an override
void MissionCommandTreeTest::_checkOverrideParamValues(const MissionCommandUIInfo* uiInfo, int command, int paramIndex)
{
    QString overrideString = QString("override fw %1 %2").arg(command).arg(paramIndex);

    bool showUI;
    const MissionCmdParamInfo* paramInfo = uiInfo->getParamInfo(paramIndex, showUI);
    QVERIFY(showUI);
    QVERIFY(paramInfo);
    QCOMPARE(paramInfo->decimalPlaces(), 1);
    QCOMPARE(paramInfo->defaultValue(), 1.0);
    QCOMPARE(paramInfo->enumStrings().count(), 2);
    QCOMPARE(paramInfo->enumStrings()[0], QStringLiteral("1"));
    QCOMPARE(paramInfo->enumStrings()[1], QStringLiteral("2"));
    QCOMPARE(paramInfo->enumValues().count(), 2);
    QCOMPARE(paramInfo->enumValues()[0].toDouble(), 1.0);
    QCOMPARE(paramInfo->enumValues()[1].toDouble(), 2.0);
    QCOMPARE(paramInfo->label(), overrideString);
    QCOMPARE(paramInfo->param(), paramIndex);
    QCOMPARE(paramInfo->units(), overrideString);
}

// Verifies that values match settings for an override
void MissionCommandTreeTest::_checkOverrideValues(const MissionCommandUIInfo* uiInfo, int command)
{
    bool showUI;
    QString overrideString = QString("override fw %1").arg(command);

    QVERIFY(uiInfo != nullptr);
    _checkFullInfoMap(uiInfo);
    QCOMPARE(uiInfo->command(), (MAV_CMD)command);
    QCOMPARE(uiInfo->rawName(), _rawName(command));
    QCOMPARE(uiInfo->category(), overrideString);
    QCOMPARE(uiInfo->description(), overrideString);
    QCOMPARE(uiInfo->friendlyEdit(), true);
    QCOMPARE(uiInfo->friendlyName(), _friendlyName(command));
    QCOMPARE(uiInfo->isStandaloneCoordinate(), false);
    QCOMPARE(uiInfo->specifiesCoordinate(), false);
    QVERIFY(uiInfo->getParamInfo(2, showUI));
    QCOMPARE(showUI, false);
    QVERIFY(uiInfo->getParamInfo(4, showUI));
    QCOMPARE(showUI, false);
    QVERIFY(uiInfo->getParamInfo(6, showUI));
    QCOMPARE(showUI, false);
    _checkOverrideParamValues(uiInfo, command, 1);
    _checkOverrideParamValues(uiInfo, command, 3);
    _checkOverrideParamValues(uiInfo, command, 5);
}

void MissionCommandTreeTest::testJsonLoad(void)
{
    bool showUI;

    // Test loading from the bad command list
    MissionCommandList* commandList = _commandTree->_staticCommandTree[MAV_AUTOPILOT_GENERIC][MAV_TYPE_GENERIC];
    QVERIFY(commandList != nullptr);

    // Command 1 should have all values defaulted, no params
    MissionCommandUIInfo* uiInfo = commandList->getUIInfo((MAV_CMD)1);
    QVERIFY(uiInfo != nullptr);
    _checkFullInfoMap(uiInfo);
    QCOMPARE(uiInfo->command(), (MAV_CMD)1);
    QCOMPARE(uiInfo->rawName(), _rawName(1));
    QVERIFY(uiInfo->category() == MissionCommandUIInfo::_advancedCategory);
    QVERIFY(uiInfo->description().isEmpty());
    QCOMPARE(uiInfo->friendlyEdit(), false);
    QCOMPARE(uiInfo->friendlyName(), uiInfo->rawName());
    QCOMPARE(uiInfo->isStandaloneCoordinate(), false);
    QCOMPARE(uiInfo->specifiesCoordinate(), false);
    for (int i=1; i<=7; i++) {
        QVERIFY(uiInfo->getParamInfo(i, showUI) == nullptr);
        QCOMPARE(showUI, false);
    }

    // Command 2 should all values defaulted for param 1
    uiInfo = commandList->getUIInfo((MAV_CMD)2);
    QVERIFY(uiInfo != nullptr);
    const MissionCmdParamInfo* paramInfo = uiInfo->getParamInfo(1, showUI);
    QVERIFY(paramInfo);
    QCOMPARE(showUI, true);
    QCOMPARE(paramInfo->decimalPlaces(), -1);
    QCOMPARE(paramInfo->defaultValue(), 0.0);
    QCOMPARE(paramInfo->enumStrings().count(), 0);
    QCOMPARE(paramInfo->enumValues().count(), 0);
    QCOMPARE(paramInfo->label(), _paramLabel(1));
    QCOMPARE(paramInfo->param(), 1);
    QVERIFY(paramInfo->units().isEmpty());
    for (int i=2; i<=7; i++) {
        QVERIFY(uiInfo->getParamInfo(i, showUI) == nullptr);
        QCOMPARE(showUI, false);
    }

    // Command 3 should have all values set
    _checkBaseValues(commandList->getUIInfo((MAV_CMD)3), 3);
}

void MissionCommandTreeTest::testOverride(void)
{
    // Generic/Generic should not have any overrides
    Vehicle* vehicle = new Vehicle(MAV_AUTOPILOT_GENERIC, MAV_TYPE_GENERIC, qgcApp()->toolbox()->firmwarePluginManager());
    _checkBaseValues(_commandTree->getUIInfo(vehicle, QGCMAVLink::VehicleClassGeneric, (MAV_CMD)4), 4);
    delete vehicle;

    // Generic/FixedWing should have overrides
    vehicle = new Vehicle(MAV_AUTOPILOT_GENERIC, MAV_TYPE_FIXED_WING, qgcApp()->toolbox()->firmwarePluginManager());
    _checkOverrideValues(_commandTree->getUIInfo(vehicle, QGCMAVLink::VehicleClassGeneric, (MAV_CMD)4), 4);
    delete vehicle;
}

void MissionCommandTreeTest::testAllTrees(void)
{
    QList<MAV_AUTOPILOT>    firmwareList;
    QList<MAV_TYPE>         vehicleList;

    firmwareList << MAV_AUTOPILOT_GENERIC << MAV_AUTOPILOT_PX4 << MAV_AUTOPILOT_ARDUPILOTMEGA;
    vehicleList << MAV_TYPE_GENERIC << MAV_TYPE_QUADROTOR << MAV_TYPE_FIXED_WING << MAV_TYPE_GROUND_ROVER << MAV_TYPE_SUBMARINE << MAV_TYPE_VTOL_QUADROTOR;

    // This will cause all of the variants of collapsed trees to be built
    for(MAV_AUTOPILOT firmwareType: firmwareList) {
        for (MAV_TYPE vehicleType: vehicleList) {
            if (firmwareType == MAV_AUTOPILOT_ARDUPILOTMEGA && vehicleType == MAV_TYPE_VTOL_QUADROTOR) {
                // VTOL in ArduPilot shows up as plane so we can test this pair
                continue;
            }
            qDebug() << firmwareType << vehicleType;
            Vehicle* vehicle = new Vehicle(firmwareType, vehicleType, qgcApp()->toolbox()->firmwarePluginManager());
            QVERIFY(qgcApp()->toolbox()->missionCommandTree()->getUIInfo(vehicle, QGCMAVLink::VehicleClassMultiRotor, MAV_CMD_NAV_WAYPOINT) != nullptr);
            delete vehicle;
        }
    }

}

