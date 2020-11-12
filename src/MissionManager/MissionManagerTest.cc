/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#include "MissionManagerTest.h"
#include "LinkManager.h"
#include "MultiVehicleManager.h"

const MissionManagerTest::TestCase_t MissionManagerTest::_rgTestCases[] = {
    { "0\t0\t3\t16\t10\t20\t30\t40\t-10\t-20\t-30\t1\r\n",  { 0, QGeoCoordinate(-10.0, -20.0, -30.0), MAV_CMD_NAV_WAYPOINT,     10.0, 20.0, 30.0, 40.0, true, false, MAV_FRAME_GLOBAL_RELATIVE_ALT } },
    { "1\t0\t3\t17\t10\t20\t30\t40\t-10\t-20\t-30\t1\r\n",  { 1, QGeoCoordinate(-10.0, -20.0, -30.0), MAV_CMD_NAV_LOITER_UNLIM, 10.0, 20.0, 30.0, 40.0, true, false, MAV_FRAME_GLOBAL_RELATIVE_ALT } },
    { "2\t0\t3\t18\t10\t20\t30\t40\t-10\t-20\t-30\t1\r\n",  { 2, QGeoCoordinate(-10.0, -20.0, -30.0), MAV_CMD_NAV_LOITER_TURNS, 10.0, 20.0, 30.0, 40.0, true, false, MAV_FRAME_GLOBAL_RELATIVE_ALT } },
    { "3\t0\t3\t19\t10\t20\t30\t40\t-10\t-20\t-30\t1\r\n",  { 3, QGeoCoordinate(-10.0, -20.0, -30.0), MAV_CMD_NAV_LOITER_TIME,  10.0, 20.0, 30.0, 40.0, true, false, MAV_FRAME_GLOBAL_RELATIVE_ALT } },
    { "4\t0\t3\t21\t10\t20\t30\t40\t-10\t-20\t-30\t1\r\n",  { 4, QGeoCoordinate(-10.0, -20.0, -30.0), MAV_CMD_NAV_LAND,         10.0, 20.0, 30.0, 40.0, true, false, MAV_FRAME_GLOBAL_RELATIVE_ALT } },
    { "6\t0\t2\t112\t10\t20\t30\t40\t-10\t-20\t-30\t1\r\n", { 5, QGeoCoordinate(-10.0, -20.0, -30.0), MAV_CMD_CONDITION_DELAY,  10.0, 20.0, 30.0, 40.0, true, false, MAV_FRAME_MISSION } },
};
const size_t MissionManagerTest::_cTestCases = sizeof(_rgTestCases)/sizeof(_rgTestCases[0]);

MissionManagerTest::MissionManagerTest(void)
{
    
}

void MissionManagerTest::_writeItems(MockLinkMissionItemHandler::FailureMode_t failureMode, MAV_MISSION_RESULT failureAckResult, bool shouldFail)
{
    _mockLink->setMissionItemFailureMode(failureMode, failureAckResult);
    
    // Setup our test case data
    QList<MissionItem*> missionItems;
    
    // Editor has a home position item on the front, so we do the same
    MissionItem* homeItem = new MissionItem(nullptr /* Vehicle */, this);
    homeItem->setCommand(MAV_CMD_NAV_WAYPOINT);
    homeItem->setParam5(47.3769);
    homeItem->setParam6(8.549444);
    homeItem->setParam7(0);
    homeItem->setSequenceNumber(0);
    missionItems.append(homeItem);

    for (size_t i=0; i<_cTestCases; i++) {
        const TestCase_t* testCase = &_rgTestCases[i];
        
        MissionItem* missionItem = new MissionItem(this);
        
        QTextStream loadStream(testCase->itemStream, QIODevice::ReadOnly);
        QVERIFY(missionItem->load(loadStream));

        // Mission Manager expects to get 1-base sequence numbers for write
        missionItem->setSequenceNumber(missionItem->sequenceNumber() + 1);
        
        missionItems.append(missionItem);
    }
    
    // Send the items to the vehicle
    _missionManager->writeMissionItems(missionItems);
    
    // writeMissionItems should emit these signals before returning:
    //      inProgressChanged
    QVERIFY(_missionManager->inProgress());
    QCOMPARE(_multiSpyMissionManager->checkSignalByMask(inProgressChangedSignalMask), true);
    _checkInProgressValues(true);
    
    _multiSpyMissionManager->clearAllSignals();
    
    if (shouldFail) {
        // This should be a failed run

        setExpectedMessageBox(QMessageBox::Ok);

        // Wait for write sequence to complete. We should get:
        //      inProgressChanged(false) signal
        //      error(errorCode, QString) signal
        _multiSpyMissionManager->waitForSignalByIndex(inProgressChangedSignalIndex, _missionManagerSignalWaitTime);
        QCOMPARE(_multiSpyMissionManager->checkSignalByMask(inProgressChangedSignalMask | errorSignalMask), true);

        // Validate inProgressChanged signal value
        _checkInProgressValues(false);

        // Validate error signal values
        QSignalSpy* spy = _multiSpyMissionManager->getSpyByIndex(errorSignalIndex);
        QList<QVariant> signalArgs = spy->takeFirst();
        QCOMPARE(signalArgs.count(), 2);
        qDebug() << signalArgs[1].toString();

        checkExpectedMessageBox();
    } else {
        // This should be clean run

        // Wait for write sequence to complete. We should get:
        //      inProgressChanged(false) signal
        //      sednComplete signal
        _multiSpyMissionManager->waitForSignalByIndex(sendCompleteSignalIndex, _missionManagerSignalWaitTime);
        QCOMPARE(_multiSpyMissionManager->checkSignalByMask(inProgressChangedSignalMask | sendCompleteSignalMask), true);

        // Validate inProgressChanged signal value
        _checkInProgressValues(false);

        // Validate item count in mission manager

        int expectedCount = (int)_cTestCases;
        if (_mockLink->getFirmwareType() == MAV_AUTOPILOT_ARDUPILOTMEGA) {
            // Home position at position 0 comes from vehicle
            expectedCount++;
        }

        QCOMPARE(_missionManager->missionItems().count(), expectedCount);
    }
    
    _multiSpyMissionManager->clearAllSignals();
}

void MissionManagerTest::_roundTripItems(MockLinkMissionItemHandler::FailureMode_t failureMode, MAV_MISSION_RESULT failureAckResult, bool shouldFail)
{
    _writeItems(MockLinkMissionItemHandler::FailNone, failureAckResult, false);
    
    _mockLink->setMissionItemFailureMode(failureMode, failureAckResult);

    // Read the items back from the vehicle
    _missionManager->loadFromVehicle();
    
    // requestMissionItems should emit inProgressChanged signal before returning so no need to wait for it
    QVERIFY(_missionManager->inProgress());
    QCOMPARE(_multiSpyMissionManager->checkOnlySignalByMask(inProgressChangedSignalMask), true);
    _checkInProgressValues(true);
    
    _multiSpyMissionManager->clearAllSignals();
    
    if (shouldFail) {
        // This should be a failed run

        setExpectedMessageBox(QMessageBox::Ok);

        // Wait for read sequence to complete. We should get:
        //      inProgressChanged(false) signal to signal completion
        //      error(errorCode, QString) signal
        //      newMissionItemsAvailable signal
        _multiSpyMissionManager->waitForSignalByIndex(inProgressChangedSignalIndex, _missionManagerSignalWaitTime);
        QCOMPARE(_multiSpyMissionManager->checkSignalByMask(newMissionItemsAvailableSignalMask | inProgressChangedSignalMask | errorSignalMask), true);

        // Validate inProgressChanged signal value
        _checkInProgressValues(false);

        // Validate error signal values
        QSignalSpy* spy = _multiSpyMissionManager->getSpyByIndex(errorSignalIndex);
        QList<QVariant> signalArgs = spy->takeFirst();
        QCOMPARE(signalArgs.count(), 2);
        qDebug() << signalArgs[1].toString();

        checkExpectedMessageBox();
    } else {
        // This should be clean run

        // Now wait for read sequence to complete. We should get:
        //      inProgressChanged(false) signal to signal completion
        //      newMissionItemsAvailable signal
        _multiSpyMissionManager->waitForSignalByIndex(inProgressChangedSignalIndex, _missionManagerSignalWaitTime);
        QCOMPARE(_multiSpyMissionManager->checkSignalByMask(newMissionItemsAvailableSignalMask | inProgressChangedSignalMask), true);
        _checkInProgressValues(false);
    }
    
    _multiSpyMissionManager->clearAllSignals();

    // Validate returned items
    
    int cMissionItemsExpected;
    
    if (shouldFail) {
        cMissionItemsExpected = 0;
    } else {
        cMissionItemsExpected = (int)_cTestCases;
        if (_mockLink->getFirmwareType() == MAV_AUTOPILOT_ARDUPILOTMEGA) {
            // Home position at position 0 comes from vehicle
            cMissionItemsExpected++;
        }
    }
    
    QCOMPARE(_missionManager->missionItems().count(), (int)cMissionItemsExpected);

    int firstActualItem = 0;
    if (_mockLink->getFirmwareType() == MAV_AUTOPILOT_ARDUPILOTMEGA) {
        // First item is home position, don't validate it
        firstActualItem++;
    }

    int testCaseIndex = 0;
    for (int actualItemIndex=firstActualItem; actualItemIndex<cMissionItemsExpected; actualItemIndex++) {
        const TestCase_t* testCase = &_rgTestCases[testCaseIndex];

        int expectedSequenceNumber = testCase->expectedItem.sequenceNumber;
        if (_mockLink->getFirmwareType() == MAV_AUTOPILOT_ARDUPILOTMEGA) {
            // Account for home position in first item
            expectedSequenceNumber++;
        }

        MissionItem* actual = _missionManager->missionItems()[actualItemIndex];
        
        qDebug() << "Test case" << testCaseIndex;
        QCOMPARE(actual->sequenceNumber(),          expectedSequenceNumber);
        QCOMPARE(actual->coordinate().latitude(),   testCase->expectedItem.coordinate.latitude());
        QCOMPARE(actual->coordinate().longitude(),  testCase->expectedItem.coordinate.longitude());
        QCOMPARE(actual->coordinate().altitude(),   testCase->expectedItem.coordinate.altitude());
        QCOMPARE((int)actual->command(),       (int)testCase->expectedItem.command);
        QCOMPARE(actual->param1(),                  testCase->expectedItem.param1);
        QCOMPARE(actual->param2(),                  testCase->expectedItem.param2);
        QCOMPARE(actual->param3(),                  testCase->expectedItem.param3);
        QCOMPARE(actual->param4(),                  testCase->expectedItem.param4);
        QCOMPARE(actual->autoContinue(),            testCase->expectedItem.autocontinue);
        QCOMPARE(actual->frame(),                   testCase->expectedItem.frame);

        testCaseIndex++;
    }
    
}

void MissionManagerTest::_testWriteFailureHandlingWorker(void)
{
    /*
    /// Called to send a MISSION_ACK message while the MissionManager is in idle state
    void sendUnexpectedMissionAck(MAV_MISSION_RESULT ackType) { _missionItemHandler.sendUnexpectedMissionAck(ackType); }
    
    /// Called to send a MISSION_ITEM message while the MissionManager is in idle state
    void sendUnexpectedMissionItem(void) { _missionItemHandler.sendUnexpectedMissionItem(); }
    
    /// Called to send a MISSION_REQUEST message while the MissionManager is in idle state
    void sendUnexpectedMissionRequest(void) { _missionItemHandler.sendUnexpectedMissionRequest(); }
    */
    
    typedef struct {
        const char*                                 failureText;
        MockLinkMissionItemHandler::FailureMode_t   failureMode;
        bool                                        shouldFail;
    } WriteTestCase_t;
    
    static const WriteTestCase_t rgTestCases[] = {
        { "No Failure",                         MockLinkMissionItemHandler::FailNone,                           false },
        { "FailWriteMissionCountNoResponse",    MockLinkMissionItemHandler::FailWriteMissionCountNoResponse,    true },
        { "FailWriteMissionCountFirstResponse", MockLinkMissionItemHandler::FailWriteMissionCountFirstResponse, false },
        { "FailWriteRequest1NoResponse",        MockLinkMissionItemHandler::FailWriteRequest1NoResponse,        true },
        { "FailWriteRequest0IncorrectSequence", MockLinkMissionItemHandler::FailWriteRequest0IncorrectSequence, true },
        { "FailWriteRequest1IncorrectSequence", MockLinkMissionItemHandler::FailWriteRequest1IncorrectSequence, true },
        { "FailWriteRequest0ErrorAck",          MockLinkMissionItemHandler::FailWriteRequest0ErrorAck,          true },
        { "FailWriteRequest1ErrorAck",          MockLinkMissionItemHandler::FailWriteRequest1ErrorAck,          true },
        { "FailWriteFinalAckNoResponse",        MockLinkMissionItemHandler::FailWriteFinalAckNoResponse,        true },
        { "FailWriteFinalAckErrorAck",          MockLinkMissionItemHandler::FailWriteFinalAckErrorAck,          true },
        { "FailWriteFinalAckMissingRequests",   MockLinkMissionItemHandler::FailWriteFinalAckMissingRequests,   true },
    };

    for (size_t i=0; i<sizeof(rgTestCases)/sizeof(rgTestCases[0]); i++) {
        const WriteTestCase_t* pCase = &rgTestCases[i];
        qDebug() << "TEST CASE _testWriteFailureHandlingWorker" << pCase->failureText;
        _writeItems(pCase->failureMode, MAV_MISSION_ERROR, pCase->shouldFail);
        _mockLink->resetMissionItemHandler();
    }
}

void MissionManagerTest::_testReadFailureHandlingWorker(void)
{
    /*
     /// Called to send a MISSION_ACK message while the MissionManager is in idle state
     void sendUnexpectedMissionAck(MAV_MISSION_RESULT ackType) { _missionItemHandler.sendUnexpectedMissionAck(ackType); }
     
     /// Called to send a MISSION_ITEM message while the MissionManager is in idle state
     void sendUnexpectedMissionItem(void) { _missionItemHandler.sendUnexpectedMissionItem(); }
     
     /// Called to send a MISSION_REQUEST message while the MissionManager is in idle state
     void sendUnexpectedMissionRequest(void) { _missionItemHandler.sendUnexpectedMissionRequest(); }
     */
    
    typedef struct {
        const char*                                 failureText;
        MockLinkMissionItemHandler::FailureMode_t   failureMode;
        bool                                        shouldFail;
    } ReadTestCase_t;
    
    /*
    static const ReadTestCase_t rgTestCases[] = {
        { "FailReadRequest1FirstResponse",      MockLinkMissionItemHandler::FailReadRequest1FirstResponse,      false },
    };*/

    static const ReadTestCase_t rgTestCases[] = {
        { "No Failure",                         MockLinkMissionItemHandler::FailNone,                           false },
        { "FailReadRequestListNoResponse",      MockLinkMissionItemHandler::FailReadRequestListNoResponse,      true },
        { "FailReadRequestListFirstResponse",   MockLinkMissionItemHandler::FailReadRequestListFirstResponse,   false },
        { "FailReadRequest0NoResponse",         MockLinkMissionItemHandler::FailReadRequest0NoResponse,         true },
        { "FailReadRequest1NoResponse",         MockLinkMissionItemHandler::FailReadRequest1NoResponse,         true },
        { "FailReadRequest1FirstResponse",      MockLinkMissionItemHandler::FailReadRequest1FirstResponse,      false },
        { "FailReadRequest0IncorrectSequence",  MockLinkMissionItemHandler::FailReadRequest0IncorrectSequence,  true },
        { "FailReadRequest1IncorrectSequence",  MockLinkMissionItemHandler::FailReadRequest1IncorrectSequence,  true  },
        { "FailReadRequest0ErrorAck",           MockLinkMissionItemHandler::FailReadRequest0ErrorAck,           true },
        { "FailReadRequest1ErrorAck",           MockLinkMissionItemHandler::FailReadRequest1ErrorAck,           true },
    };

    for (size_t i=0; i<sizeof(rgTestCases)/sizeof(rgTestCases[0]); i++) {
        const ReadTestCase_t* pCase = &rgTestCases[i];
        qDebug() << "TEST CASE _testReadFailureHandlingWorker" << pCase->failureText;
        _roundTripItems(pCase->failureMode, MAV_MISSION_ERROR, pCase->shouldFail);
        _mockLink->resetMissionItemHandler();
        _multiSpyMissionManager->clearAllSignals();
    }
}

void MissionManagerTest::_testWriteFailureHandlingAPM(void)
{
    _initForFirmwareType(MAV_AUTOPILOT_ARDUPILOTMEGA);
    _testWriteFailureHandlingWorker();
}

void MissionManagerTest::_testReadFailureHandlingAPM(void)
{
    _initForFirmwareType(MAV_AUTOPILOT_ARDUPILOTMEGA);
    _testReadFailureHandlingWorker();
}


void MissionManagerTest::_testWriteFailureHandlingPX4(void)
{
    _initForFirmwareType(MAV_AUTOPILOT_PX4);
    _testWriteFailureHandlingWorker();
}

void MissionManagerTest::_testReadFailureHandlingPX4(void)
{
    _initForFirmwareType(MAV_AUTOPILOT_PX4);
    _testReadFailureHandlingWorker();
}

void MissionManagerTest::_testErrorAckFailureStrings(void)
{
    _initForFirmwareType(MAV_AUTOPILOT_PX4);

    typedef struct {
        const char*                                 ackResultStr;
        MAV_MISSION_RESULT                          ackResult;
    } ErrorStringTestCase_t;

    static const ErrorStringTestCase_t rgTestCases[] = {
        { "MAV_MISSION_UNSUPPORTED_FRAME",  MAV_MISSION_UNSUPPORTED_FRAME },
        { "MAV_MISSION_UNSUPPORTED",        MAV_MISSION_UNSUPPORTED },
        { "MAV_MISSION_INVALID_PARAM1",     MAV_MISSION_INVALID_PARAM1 },
        { "MAV_MISSION_INVALID_PARAM2",     MAV_MISSION_INVALID_PARAM2 },
        { "MAV_MISSION_INVALID_PARAM3",     MAV_MISSION_INVALID_PARAM3 },
        { "MAV_MISSION_INVALID_PARAM4",     MAV_MISSION_INVALID_PARAM4 },
        { "MAV_MISSION_INVALID_PARAM5_X",   MAV_MISSION_INVALID_PARAM5_X },
        { "MAV_MISSION_INVALID_PARAM6_Y",   MAV_MISSION_INVALID_PARAM6_Y },
        { "MAV_MISSION_INVALID_PARAM7",     MAV_MISSION_INVALID_PARAM7 },
        { "MAV_MISSION_INVALID_SEQUENCE",   MAV_MISSION_INVALID_SEQUENCE },
    };

    for (size_t i=0; i<sizeof(rgTestCases)/sizeof(rgTestCases[0]); i++) {
        const ErrorStringTestCase_t* pCase = &rgTestCases[i];
        qDebug() << "TEST CASE _testErrorAckFailureStrings" << pCase->ackResultStr;
        _writeItems(MockLinkMissionItemHandler::FailWriteRequest1ErrorAck, pCase->ackResult, true /* shouldFail */);
        _mockLink->resetMissionItemHandler();
    }

}
