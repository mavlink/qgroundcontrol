#pragma once

#include "MissionControllerManagerTest.h"

class MissionManagerTest : public MissionControllerManagerTest
{
    Q_OBJECT

private slots:
    void _testWriteFailureHandlingPX4();
    void _testWriteFailureHandlingAPM();
    void _testReadFailureHandlingPX4();
    void _testReadFailureHandlingAPM();
    void _testErrorAckFailureStrings();

private:
    void _roundTripItems(MockLinkMissionItemHandler::FailureMode_t failureMode, MAV_MISSION_RESULT failureAckResult,
                         bool shouldFail);
    void _writeItems(MockLinkMissionItemHandler::FailureMode_t failureMode, MAV_MISSION_RESULT failureAckResult,
                     bool shouldFail);
    void _testWriteFailureHandlingWorker();
    void _testReadFailureHandlingWorker();

    static const TestCase_t _rgTestCases[];
    static const size_t _cTestCases;
};
