/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "MissionControllerManagerTest.h"

class MissionManagerTest : public MissionControllerManagerTest
{
    Q_OBJECT
    
public:
    MissionManagerTest(void);
    
private slots:
    //void _testWriteFailureHandlingPX4(void);
    //void _testWriteFailureHandlingAPM(void);
    void _testReadFailureHandlingPX4(void);
    //void _testReadFailureHandlingAPM(void);
    //void _testErrorAckFailureStrings(void);

private:
    void _testWriteFailureHandlingPX4(void);
    void _testWriteFailureHandlingAPM(void);
    //void _testReadFailureHandlingPX4(void);
    void _testReadFailureHandlingAPM(void);
    void _testErrorAckFailureStrings(void);
    void _roundTripItems(MockLinkMissionItemHandler::FailureMode_t failureMode, MAV_MISSION_RESULT failureAckResult, bool shouldFail);
    void _writeItems(MockLinkMissionItemHandler::FailureMode_t failureMode, MAV_MISSION_RESULT failureAckResult, bool shouldFail);
    void _testWriteFailureHandlingWorker(void);
    void _testReadFailureHandlingWorker(void);
    
    static const TestCase_t _rgTestCases[];
    static const size_t     _cTestCases;
};
