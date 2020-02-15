/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#ifndef MissionItemTest_H
#define MissionItemTest_H

#include "UnitTest.h"
#include "MultiSignalSpy.h"
#include "MissionItem.h"
#include "Vehicle.h"

/// Unit test for the MissionItem Object
class MissionItemTest : public UnitTest
{
    Q_OBJECT
    
public:
    MissionItemTest(void);
    
    void init(void) override;
    void cleanup(void) override;

private slots:
    void _testSetGet(void);
    void _testSignals(void);
    void _testFactSignals(void);
    void _testLoadFromStream(void);
    void _testSimpleLoadFromStream(void);
    void _testLoadFromJsonV1(void);
    void _testLoadFromJsonV2(void);
    void _testLoadFromJsonV3(void);
    void _testLoadFromJsonV3NaN(void);
    void _testSimpleLoadFromJson(void);
    void _testSaveToJson(void);

private:
    void _checkExpectedMissionItem(const MissionItem& missionItem, bool allNaNs = false);
    QJsonObject _createV1Json(void);
    QJsonObject _createV2Json(void);
    QJsonObject _createV3Json(bool allNaNs = false);

    int         _seq = 10;
    Vehicle*    _offlineVehicle;
};

#endif
