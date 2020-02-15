/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#ifndef MissionManagerTest_H
#define MissionManagerTest_H

#include "UnitTest.h"
#include "MockLink.h"
#include "MissionManager.h"
#include "MultiSignalSpy.h"
#include "MissionControllerManagerTest.h"

#include <QGeoCoordinate>

class MissionManagerTest : public MissionControllerManagerTest
{
    Q_OBJECT
    
public:
    MissionManagerTest(void);
    
private slots:
    void _testWriteFailureHandlingPX4(void);
    void _testWriteFailureHandlingAPM(void);
    void _testReadFailureHandlingPX4(void);
    void _testReadFailureHandlingAPM(void);

private:
    void _roundTripItems(MockLinkMissionItemHandler::FailureMode_t failureMode, bool shouldFail);
    void _writeItems(MockLinkMissionItemHandler::FailureMode_t failureMode, bool shouldFail);
    void _testWriteFailureHandlingWorker(void);
    void _testReadFailureHandlingWorker(void);
    
    static const TestCase_t _rgTestCases[];
    static const size_t     _cTestCases;
};

#endif
