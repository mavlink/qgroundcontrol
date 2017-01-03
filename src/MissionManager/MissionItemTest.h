/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
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

/// Unit test for the MissionItem Object
class MissionItemTest : public UnitTest
{
    Q_OBJECT
    
public:
    MissionItemTest(void);
    
private slots:
    void _testSetGet(void);
    void _testSignals(void);
    void _testFactSignals(void);
    void _testLoadFromStream(void);
    void _testSimpleLoadFromStream(void);
    void _testLoadFromJsonV1(void);
    void _testLoadFromJsonV2(void);
    void _testSimpleLoadFromJson(void);
    void _testSaveToJson(void);

private:
    void _checkExpectedMissionItem(const MissionItem& missionItem);

    int _seq = 10;
};

#endif
