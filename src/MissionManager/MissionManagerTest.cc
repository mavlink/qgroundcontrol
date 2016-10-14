/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
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

void MissionManagerTest::_writeItems(MockLinkMissionItemHandler::FailureMode_t failureMode)
{
    _mockLink->setMissionItemFailureMode(failureMode);
    
    // Setup our test case data
    QList<MissionItem*> missionItems;
    
    // Editor has a home position item on the front, so we do the same
    MissionItem* homeItem = new MissionItem(NULL /* Vehicle */, this);
    homeItem->setCommand(MAV_CMD_NAV_WAYPOINT);
    homeItem->setCoordinate(QGeoCoordinate(47.3769, 8.549444, 0));
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
    //      newMissionItemsAvailable
    QVERIFY(_missionManager->inProgress());
    QCOMPARE(_multiSpyMissionManager->checkSignalByMask(inProgressChangedSignalMask | newMissionItemsAvailableSignalMask), true);
    _checkInProgressValues(true);
    
    _multiSpyMissionManager->clearAllSignals();
    
    if (failureMode == MockLinkMissionItemHandler::FailNone) {
        // This should be clean run
        
        // Wait for write sequence to complete. We should get:
        //      inProgressChanged(false) signal
        _multiSpyMissionManager->waitForSignalByIndex(inProgressChangedSignalIndex, _missionManagerSignalWaitTime);
        QCOMPARE(_multiSpyMissionManager->checkOnlySignalByMask(inProgressChangedSignalMask), true);
        
        // Validate inProgressChanged signal value
        _checkInProgressValues(false);

        // Validate item count in mission manager

        int expectedCount = (int)_cTestCases;
        if (_mockLink->getFirmwareType() == MAV_AUTOPILOT_ARDUPILOTMEGA) {
            // Home position at position 0 comes from vehicle
            expectedCount++;
        }

        QCOMPARE(_missionManager->missionItems().count(), expectedCount);
    } else {
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
    }
    
    _multiSpyMissionManager->clearAllSignals();
}

void MissionManagerTest::_roundTripItems(MockLinkMissionItemHandler::FailureMode_t failureMode)
{
    _writeItems(MockLinkMissionItemHandler::FailNone);
    
    _mockLink->setMissionItemFailureMode(failureMode);

    // Read the items back from the vehicle
    _missionManager->requestMissionItems();
    
    // requestMissionItems should emit inProgressChanged signal before returning so no need to wait for it
    QVERIFY(_missionManager->inProgress());
    QCOMPARE(_multiSpyMissionManager->checkOnlySignalByMask(inProgressChangedSignalMask), true);
    _checkInProgressValues(true);
    
    _multiSpyMissionManager->clearAllSignals();
    
    if (failureMode == MockLinkMissionItemHandler::FailNone) {
        // This should be clean run
        
        // Now wait for read sequence to complete. We should get:
        //      inProgressChanged(false) signal to signal completion
        //      newMissionItemsAvailable signal
        _multiSpyMissionManager->waitForSignalByIndex(inProgressChangedSignalIndex, _missionManagerSignalWaitTime);
        QCOMPARE(_multiSpyMissionManager->checkSignalByMask(newMissionItemsAvailableSignalMask | inProgressChangedSignalMask), true);
        _checkInProgressValues(false);

    } else {
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
    }
    
    _multiSpyMissionManager->clearAllSignals();

    // Validate returned items
    
    size_t cMissionItemsExpected;
    
    if (failureMode == MockLinkMissionItemHandler::FailNone) {
        cMissionItemsExpected = (int)_cTestCases;
        if (_mockLink->getFirmwareType() == MAV_AUTOPILOT_ARDUPILOTMEGA) {
            // Home position at position 0 comes from vehicle
            cMissionItemsExpected++;
        }
    } else {
        cMissionItemsExpected = 0;
    }
    
    QCOMPARE(_missionManager->missionItems().count(), (int)cMissionItemsExpected);

    size_t firstActualItem = 0;
    if (_mockLink->getFirmwareType() == MAV_AUTOPILOT_ARDUPILOTMEGA) {
        // First item is home position, don't validate it
        firstActualItem++;
    }

    int testCaseIndex = 0;
    for (size_t actualItemIndex=firstActualItem; actualItemIndex<cMissionItemsExpected; actualItemIndex++) {
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
    } TestCase_t;
    
    static const TestCase_t rgTestCases[] = {
        { "No Failure",                         MockLinkMissionItemHandler::FailNone },
        { "FailWriteRequest0NoResponse",        MockLinkMissionItemHandler::FailWriteRequest0NoResponse },
        { "FailWriteRequest1NoResponse",        MockLinkMissionItemHandler::FailWriteRequest1NoResponse },
        { "FailWriteRequest0IncorrectSequence", MockLinkMissionItemHandler::FailWriteRequest0IncorrectSequence },
        { "FailWriteRequest1IncorrectSequence", MockLinkMissionItemHandler::FailWriteRequest1IncorrectSequence },
        { "FailWriteRequest0ErrorAck",          MockLinkMissionItemHandler::FailWriteRequest0ErrorAck },
        { "FailWriteRequest1ErrorAck",          MockLinkMissionItemHandler::FailWriteRequest1ErrorAck },
        { "FailWriteFinalAckNoResponse",        MockLinkMissionItemHandler::FailWriteFinalAckNoResponse },
        { "FailWriteFinalAckErrorAck",          MockLinkMissionItemHandler::FailWriteFinalAckErrorAck },
        { "FailWriteFinalAckMissingRequests",   MockLinkMissionItemHandler::FailWriteFinalAckMissingRequests },
    };

    for (size_t i=0; i<sizeof(rgTestCases)/sizeof(rgTestCases[0]); i++) {
        qDebug() << "TEST CASE " << rgTestCases[i].failureText;
        _writeItems(rgTestCases[i].failureMode);
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
    } TestCase_t;
    
    static const TestCase_t rgTestCases[] = {
        { "No Failure",                         MockLinkMissionItemHandler::FailNone },
        { "FailReadRequestListNoResponse",      MockLinkMissionItemHandler::FailReadRequestListNoResponse },
        { "FailReadRequest0NoResponse",         MockLinkMissionItemHandler::FailReadRequest0NoResponse },
        { "FailReadRequest1NoResponse",         MockLinkMissionItemHandler::FailReadRequest1NoResponse },
        { "FailReadRequest0IncorrectSequence",  MockLinkMissionItemHandler::FailReadRequest0IncorrectSequence },
        { "FailReadRequest1IncorrectSequence",  MockLinkMissionItemHandler::FailReadRequest1IncorrectSequence  },
        { "FailReadRequest0ErrorAck",           MockLinkMissionItemHandler::FailReadRequest0ErrorAck },
        { "FailReadRequest1ErrorAck",           MockLinkMissionItemHandler::FailReadRequest1ErrorAck },
    };
    
    for (size_t i=0; i<sizeof(rgTestCases)/sizeof(rgTestCases[0]); i++) {
        qDebug() << "TEST CASE " << rgTestCases[i].failureText;
        _roundTripItems(rgTestCases[i].failureMode);
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
