/*=====================================================================
 
 QGroundControl Open Source Ground Control Station
 
 (c) 2009 - 2014 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 
 This file is part of the QGROUNDCONTROL project
 
 QGROUNDCONTROL is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 QGROUNDCONTROL is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.
 
 ======================================================================*/

#include "MissionManagerTest.h"
#include "LinkManager.h"
#include "MultiVehicleManager.h"

UT_REGISTER_TEST(MissionManagerTest)

const MissionManagerTest::TestCase_t MissionManagerTest::_rgTestCases[] = {
    { "0\t0\t3\t16\t10\t20\t30\t40\t-10\t-20\t-30\t1\r\n",  { 0, QGeoCoordinate(-10.0, -20.0, -30.0), MAV_CMD_NAV_WAYPOINT,     10.0, 20.0, 30.0, 40.0, true, false, MAV_FRAME_GLOBAL_RELATIVE_ALT } },
    { "1\t0\t3\t17\t10\t20\t30\t40\t-10\t-20\t-30\t1\r\n",  { 1, QGeoCoordinate(-10.0, -20.0, -30.0), MAV_CMD_NAV_LOITER_UNLIM, 10.0, 20.0, 30.0, 40.0, true, false, MAV_FRAME_GLOBAL_RELATIVE_ALT } },
    { "2\t0\t3\t18\t10\t20\t30\t40\t-10\t-20\t-30\t1\r\n",  { 2, QGeoCoordinate(-10.0, -20.0, -30.0), MAV_CMD_NAV_LOITER_TURNS, 10.0, 20.0, 30.0, 40.0, true, false, MAV_FRAME_GLOBAL_RELATIVE_ALT } },
    { "3\t0\t3\t19\t10\t20\t30\t40\t-10\t-20\t-30\t1\r\n",  { 3, QGeoCoordinate(-10.0, -20.0, -30.0), MAV_CMD_NAV_LOITER_TIME,  10.0, 20.0, 30.0, 40.0, true, false, MAV_FRAME_GLOBAL_RELATIVE_ALT } },
    { "4\t0\t3\t21\t10\t20\t30\t40\t-10\t-20\t-30\t1\r\n",  { 4, QGeoCoordinate(-10.0, -20.0, -30.0), MAV_CMD_NAV_LAND,         10.0, 20.0, 30.0, 40.0, true, false, MAV_FRAME_GLOBAL_RELATIVE_ALT } },
    { "5\t0\t3\t22\t10\t20\t30\t40\t-10\t-20\t-30\t1\r\n",  { 5, QGeoCoordinate(-10.0, -20.0, -30.0), MAV_CMD_NAV_TAKEOFF,      10.0, 20.0, 30.0, 40.0, true, false, MAV_FRAME_GLOBAL_RELATIVE_ALT } },
    { "6\t0\t2\t112\t10\t20\t30\t40\t-10\t-20\t-30\t1\r\n", { 6, QGeoCoordinate(-10.0, -20.0, -30.0), MAV_CMD_CONDITION_DELAY,  10.0, 20.0, 30.0, 40.0, true, false, MAV_FRAME_MISSION } },
    { "7\t0\t2\t177\t3\t20\t30\t40\t-10\t-20\t-30\t1\r\n",  { 7, QGeoCoordinate(-10.0, -20.0, -30.0), MAV_CMD_DO_JUMP,          3,    20.0, 30.0, 40.0, true, false, MAV_FRAME_MISSION } },
};
const size_t MissionManagerTest::_cTestCases = sizeof(_rgTestCases)/sizeof(_rgTestCases[0]);

MissionManagerTest::MissionManagerTest(void)
{
    
}

void MissionManagerTest::_writeItems(MockLinkMissionItemHandler::FailureMode_t failureMode, MissionManager::ErrorCode_t errorCode, bool failFirstTimeOnly)
{
    _mockLink->setMissionItemFailureMode(failureMode, failFirstTimeOnly);
    if (failFirstTimeOnly) {
        // Should fail first time, then retry should succed
        failureMode = MockLinkMissionItemHandler::FailNone;
    }
    
    // Setup our test case data
    QmlObjectListModel* list = new QmlObjectListModel();
    
    // Editor has a home position item on the front, so we do the same
    MissionItem* homeItem = new MissionItem(this);
    homeItem->setHomePositionSpecialCase(true);
    homeItem->setHomePositionValid(false);
    homeItem->setCommand(MavlinkQmlSingleton::MAV_CMD_NAV_WAYPOINT);
    homeItem->setLatitude(47.3769);
    homeItem->setLongitude(8.549444);
    homeItem->setSequenceNumber(0);
    list->insert(0, homeItem);

    for (size_t i=0; i<_cTestCases; i++) {
        const TestCase_t* testCase = &_rgTestCases[i];
        
        MissionItem* item = new MissionItem(list);
        
        QTextStream loadStream(testCase->itemStream, QIODevice::ReadOnly);
        QVERIFY(item->load(loadStream));

        // Mission Manager expects to get 1-base sequence numbers for write

        item->setSequenceNumber(item->sequenceNumber() + 1);
        if (item->command() == MavlinkQmlSingleton::MAV_CMD_DO_JUMP) {
            item->setParam1((int)item->param1() + 1);
        }
        
        list->append(item);
    }
    
    // Send the items to the vehicle
    _missionManager->writeMissionItems(*list);
    
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

        QCOMPARE(_missionManager->missionItems()->count(), expectedCount);
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
        QCOMPARE(signalArgs[0].toInt(), (int)errorCode);

        checkExpectedMessageBox();
    }
    
    QCOMPARE(_missionManager->canEdit(), true);
    
    delete list;
    list = NULL;
    _multiSpyMissionManager->clearAllSignals();
}

void MissionManagerTest::_roundTripItems(MockLinkMissionItemHandler::FailureMode_t failureMode, MissionManager::ErrorCode_t errorCode, bool failFirstTimeOnly)
{
    _writeItems(MockLinkMissionItemHandler::FailNone, MissionManager::InternalError, false);
    
    _mockLink->setMissionItemFailureMode(failureMode, failFirstTimeOnly);
    if (failFirstTimeOnly) {
        // Should fail first time, then retry should succed
        failureMode = MockLinkMissionItemHandler::FailNone;
    }

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
        QCOMPARE(_multiSpyMissionManager->checkNoSignalByMask(canEditChangedSignalMask), true);
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
        QCOMPARE(signalArgs[0].toInt(), (int)errorCode);
        
        checkExpectedMessageBox();
    }
    
    _multiSpyMissionManager->clearAllSignals();

    // Validate returned items
    
    size_t cMissionItemsExpected;
    
    if (failureMode == MockLinkMissionItemHandler::FailNone || failFirstTimeOnly == true) {
        cMissionItemsExpected = (int)_cTestCases;
        if (_mockLink->getFirmwareType() == MAV_AUTOPILOT_ARDUPILOTMEGA) {
            // Home position at position 0 comes from vehicle
            cMissionItemsExpected++;
        }
    } else {
        switch (failureMode) {
            case MockLinkMissionItemHandler::FailReadRequestListNoResponse:
            case MockLinkMissionItemHandler::FailReadRequest0NoResponse:
            case MockLinkMissionItemHandler::FailReadRequest0IncorrectSequence:
            case MockLinkMissionItemHandler::FailReadRequest0ErrorAck:
                cMissionItemsExpected = 0;
                break;
            case MockLinkMissionItemHandler::FailReadRequest1NoResponse:
            case MockLinkMissionItemHandler::FailReadRequest1IncorrectSequence:
            case MockLinkMissionItemHandler::FailReadRequest1ErrorAck:
                cMissionItemsExpected = 1;
                break;
            default:
                // Internal error
                Q_ASSERT(false);
                break;
        }
    }
    
    QCOMPARE(_missionManager->missionItems()->count(), (int)cMissionItemsExpected);
    QCOMPARE(_missionManager->canEdit(), true);

    size_t firstActualItem = 0;
    if (_mockLink->getFirmwareType() == MAV_AUTOPILOT_ARDUPILOTMEGA) {
        // First item is home position, don't validate it
        firstActualItem++;
    }

    int testCaseIndex = 0;
    for (size_t actualItemIndex=firstActualItem; actualItemIndex<cMissionItemsExpected; actualItemIndex++) {
        const TestCase_t* testCase = &_rgTestCases[testCaseIndex];

        int expectedSequenceNumber = testCase->expectedItem.sequenceNumber;
        int expectedParam1 = (int)testCase->expectedItem.param1;
        if (_mockLink->getFirmwareType() == MAV_AUTOPILOT_ARDUPILOTMEGA) {
            // Account for home position in first item
            expectedSequenceNumber++;
            if (testCase->expectedItem.command == MAV_CMD_DO_JUMP) {
                // Expected data in test case is for PX4
                expectedParam1++;
            }
        }

        MissionItem* actual = qobject_cast<MissionItem*>(_missionManager->missionItems()->get(actualItemIndex));
        
        qDebug() << "Test case" << testCaseIndex;
        QCOMPARE(actual->sequenceNumber(),          expectedSequenceNumber);
        QCOMPARE(actual->coordinate().latitude(),   testCase->expectedItem.coordinate.latitude());
        QCOMPARE(actual->coordinate().longitude(),  testCase->expectedItem.coordinate.longitude());
        QCOMPARE(actual->coordinate().altitude(),   testCase->expectedItem.coordinate.altitude());
        QCOMPARE((int)actual->command(),       (int)testCase->expectedItem.command);
        QCOMPARE((int)actual->param1(),        (int)expectedParam1);
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
        MissionManager::ErrorCode_t                 errorCode;
    } TestCase_t;
    
    static const TestCase_t rgTestCases[] = {
        { "No Failure",                         MockLinkMissionItemHandler::FailNone,                           MissionManager::AckTimeoutError },
        { "FailWriteRequest0NoResponse",        MockLinkMissionItemHandler::FailWriteRequest0NoResponse,        MissionManager::AckTimeoutError },
        { "FailWriteRequest1NoResponse",        MockLinkMissionItemHandler::FailWriteRequest1NoResponse,        MissionManager::AckTimeoutError },
        { "FailWriteRequest0IncorrectSequence", MockLinkMissionItemHandler::FailWriteRequest0IncorrectSequence, MissionManager::ItemMismatchError },
        { "FailWriteRequest1IncorrectSequence", MockLinkMissionItemHandler::FailWriteRequest1IncorrectSequence, MissionManager::ItemMismatchError },
        { "FailWriteRequest0ErrorAck",          MockLinkMissionItemHandler::FailWriteRequest0ErrorAck,          MissionManager::VehicleError },
        { "FailWriteRequest1ErrorAck",          MockLinkMissionItemHandler::FailWriteRequest1ErrorAck,          MissionManager::VehicleError },
        { "FailWriteFinalAckNoResponse",        MockLinkMissionItemHandler::FailWriteFinalAckNoResponse,        MissionManager::AckTimeoutError },
        { "FailWriteFinalAckErrorAck",          MockLinkMissionItemHandler::FailWriteFinalAckErrorAck,          MissionManager::VehicleError },
        { "FailWriteFinalAckMissingRequests",   MockLinkMissionItemHandler::FailWriteFinalAckMissingRequests,   MissionManager::MissingRequestsError },
    };

    for (size_t i=0; i<sizeof(rgTestCases)/sizeof(rgTestCases[0]); i++) {
        qDebug() << "TEST CASE " << rgTestCases[i].failureText << "errorCode:" << rgTestCases[i].errorCode << "failFirstTimeOnly:false";
        _writeItems(rgTestCases[i].failureMode, rgTestCases[i].errorCode, false);
        _mockLink->resetMissionItemHandler();
        qDebug() << "TEST CASE " << rgTestCases[i].failureText << "errorCode:" << rgTestCases[i].errorCode << "failFirstTimeOnly:true";
        _writeItems(rgTestCases[i].failureMode, rgTestCases[i].errorCode, true);
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
        MissionManager::ErrorCode_t                 errorCode;
    } TestCase_t;
    
    static const TestCase_t rgTestCases[] = {
        { "No Failure",                         MockLinkMissionItemHandler::FailNone,                           MissionManager::AckTimeoutError },
        { "FailReadRequestListNoResponse",      MockLinkMissionItemHandler::FailReadRequestListNoResponse,      MissionManager::AckTimeoutError },
        { "FailReadRequest0NoResponse",         MockLinkMissionItemHandler::FailReadRequest0NoResponse,         MissionManager::AckTimeoutError },
        { "FailReadRequest1NoResponse",         MockLinkMissionItemHandler::FailReadRequest1NoResponse,         MissionManager::AckTimeoutError },
        { "FailReadRequest0IncorrectSequence",  MockLinkMissionItemHandler::FailReadRequest0IncorrectSequence,  MissionManager::ItemMismatchError },
        { "FailReadRequest1IncorrectSequence",  MockLinkMissionItemHandler::FailReadRequest1IncorrectSequence,  MissionManager::ItemMismatchError },
        { "FailReadRequest0ErrorAck",           MockLinkMissionItemHandler::FailReadRequest0ErrorAck,           MissionManager::VehicleError },
        { "FailReadRequest1ErrorAck",           MockLinkMissionItemHandler::FailReadRequest1ErrorAck,           MissionManager::VehicleError },
    };
    
    for (size_t i=0; i<sizeof(rgTestCases)/sizeof(rgTestCases[0]); i++) {
        qDebug() << "TEST CASE " << rgTestCases[i].failureText << "errorCode:" << rgTestCases[i].errorCode << "failFirstTimeOnly:false";
        _roundTripItems(rgTestCases[i].failureMode, rgTestCases[i].errorCode, false);
        _mockLink->resetMissionItemHandler();
        _multiSpyMissionManager->clearAllSignals();
        qDebug() << "TEST CASE " << rgTestCases[i].failureText << "errorCode:" << rgTestCases[i].errorCode << "failFirstTimeOnly:true";
        _roundTripItems(rgTestCases[i].failureMode, rgTestCases[i].errorCode, true);
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
