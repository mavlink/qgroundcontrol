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

#include "MissionItemTest.h"
#include "LinkManager.h"
#include "MultiVehicleManager.h"
#include "MissionItem.h"
#include "SimpleMissionItem.h"

#if 0
const MissionItemTest::TestCase_t MissionItemTest::_rgTestCases[] = {
    { "0\t0\t3\t16\t10\t20\t30\t40\t-10\t-20\t-30\t1\r\n",  { 0, QGeoCoordinate(-10.0, -20.0, -30.0), MAV_CMD_NAV_WAYPOINT,     10.0, 20.0, 30.0, 40.0, true, false, MAV_FRAME_GLOBAL_RELATIVE_ALT } },
    { "1\t0\t3\t17\t10\t20\t30\t40\t-10\t-20\t-30\t1\r\n",  { 1, QGeoCoordinate(-10.0, -20.0, -30.0), MAV_CMD_NAV_LOITER_UNLIM, 10.0, 20.0, 30.0, 40.0, true, false, MAV_FRAME_GLOBAL_RELATIVE_ALT } },
    { "2\t0\t3\t18\t10\t20\t30\t40\t-10\t-20\t-30\t1\r\n",  { 2, QGeoCoordinate(-10.0, -20.0, -30.0), MAV_CMD_NAV_LOITER_TURNS, 10.0, 20.0, 30.0, 40.0, true, false, MAV_FRAME_GLOBAL_RELATIVE_ALT } },
    { "3\t0\t3\t19\t10\t20\t30\t40\t-10\t-20\t-30\t1\r\n",  { 3, QGeoCoordinate(-10.0, -20.0, -30.0), MAV_CMD_NAV_LOITER_TIME,  10.0, 20.0, 30.0, 40.0, true, false, MAV_FRAME_GLOBAL_RELATIVE_ALT } },
    { "4\t0\t3\t21\t10\t20\t30\t40\t-10\t-20\t-30\t1\r\n",  { 4, QGeoCoordinate(-10.0, -20.0, -30.0), MAV_CMD_NAV_LAND,         10.0, 20.0, 30.0, 40.0, true, false, MAV_FRAME_GLOBAL_RELATIVE_ALT } },
    { "6\t0\t2\t112\t10\t20\t30\t40\t-10\t-20\t-30\t1\r\n", { 5, QGeoCoordinate(-10.0, -20.0, -30.0), MAV_CMD_CONDITION_DELAY,  10.0, 20.0, 30.0, 40.0, true, false, MAV_FRAME_MISSION } },
};
const size_t MissionItemTest::_cTestCases = sizeof(_rgTestCases)/sizeof(_rgTestCases[0]);
#endif

MissionItemTest::MissionItemTest(void)
{
    
}

// Test property get/set
void MissionItemTest::_testSetGet(void)
{
    MissionItem missionItem;

    missionItem.setSequenceNumber(1);
    QCOMPARE(missionItem.sequenceNumber(), 1);

    missionItem.setCommand(MAV_CMD_NAV_WAYPOINT);
    QCOMPARE(missionItem.command(), MAV_CMD_NAV_WAYPOINT);

    missionItem.setFrame(MAV_FRAME_LOCAL_NED);
    QCOMPARE(missionItem.frame(), MAV_FRAME_LOCAL_NED);
    QCOMPARE(missionItem.relativeAltitude(), false);
    missionItem.setFrame(MAV_FRAME_GLOBAL_RELATIVE_ALT);
    QCOMPARE(missionItem.relativeAltitude(), true);

    missionItem.setParam1(1.0);
    QCOMPARE(missionItem.param1(), 1.0);

    missionItem.setParam2(2.0);
    QCOMPARE(missionItem.param2(), 2.0);

    missionItem.setParam3(3.0);
    QCOMPARE(missionItem.param3(), 3.0);

    missionItem.setParam4(4.0);
    QCOMPARE(missionItem.param4(), 4.0);

    missionItem.setParam5(5.0);
    QCOMPARE(missionItem.param5(), 5.0);

    missionItem.setParam6(6.0);
    QCOMPARE(missionItem.param6(), 6.0);

    missionItem.setParam7(7.0);
    QCOMPARE(missionItem.param7(), 7.0);

    QCOMPARE(missionItem.coordinate(), QGeoCoordinate(5.0, 6.0, 7.0));

    missionItem.setAutoContinue(false);
    QCOMPARE(missionItem.autoContinue(), false);

    missionItem.setIsCurrentItem(true);
    QCOMPARE(missionItem.isCurrentItem(), true);
}

// Test basic signalling
void MissionItemTest::_testSignals(void)
{
    MissionItem missionItem(1,                                  // sequenceNumber
                            MAV_CMD_NAV_WAYPOINT,               // command
                            MAV_FRAME_GLOBAL_RELATIVE_ALT,      // MAV_FRAME
                            1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0,  // params
                            true,                               // autoContinue
                            true);                              // isCurrentItem

    enum {
        isCurrentItemChangedIndex = 0,
        sequenceNumberChangedIndex,
        maxSignalIndex
    };

    enum {
        isCurrentItemChangedMask =          1 << isCurrentItemChangedIndex,
        sequenceNumberChangedIndexMask =    1 << sequenceNumberChangedIndex
    };

    static const size_t cMissionItemSignals = maxSignalIndex;
    const char*         rgMissionItemSignals[cMissionItemSignals];

    rgMissionItemSignals[isCurrentItemChangedIndex] =   SIGNAL(isCurrentItemChanged(bool));
    rgMissionItemSignals[sequenceNumberChangedIndex] =  SIGNAL(sequenceNumberChanged(int));

    MultiSignalSpy* multiSpyMissionItem = new MultiSignalSpy();
    Q_CHECK_PTR(multiSpyMissionItem);
    QCOMPARE(multiSpyMissionItem->init(&missionItem, rgMissionItemSignals, cMissionItemSignals), true);

    // Validate isCurrentItemChanged signalling
    missionItem.setIsCurrentItem(true);
    QVERIFY(multiSpyMissionItem->checkNoSignals());
    missionItem.setIsCurrentItem(false);
    QVERIFY(multiSpyMissionItem->checkOnlySignalByMask(isCurrentItemChangedMask));
    QSignalSpy* spy = multiSpyMissionItem->getSpyByIndex(isCurrentItemChangedIndex);
    QList<QVariant> signalArgs = spy->takeFirst();
    QCOMPARE(signalArgs.count(), 1);
    QCOMPARE(signalArgs[0].toBool(), false);

    multiSpyMissionItem->clearAllSignals();

    // Validate sequenceNumberChanged signalling
    missionItem.setSequenceNumber(1);
    QVERIFY(multiSpyMissionItem->checkNoSignals());
    missionItem.setSequenceNumber(2);
    QVERIFY(multiSpyMissionItem->checkOnlySignalByMask(sequenceNumberChangedIndexMask));
    spy = multiSpyMissionItem->getSpyByIndex(sequenceNumberChangedIndex);
    signalArgs = spy->takeFirst();
    QCOMPARE(signalArgs.count(), 1);
    QCOMPARE(signalArgs[0].toInt(), 2);
}

// Test signalling associated with contained facts
void MissionItemTest::_testFactSignals(void)
{
    MissionItem missionItem(1,                                  // sequenceNumber
                            MAV_CMD_NAV_WAYPOINT,               // command
                            MAV_FRAME_GLOBAL_RELATIVE_ALT,      // MAV_FRAME
                            1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0,  // params
                            true,                               // autoContinue
                            true);                              // isCurrentItem


    // command
    QSignalSpy commandSpy(&missionItem._commandFact, SIGNAL(valueChanged(QVariant)));
    missionItem.setCommand(MAV_CMD_NAV_WAYPOINT);
    QCOMPARE(commandSpy.count(), 0);
    missionItem.setCommand(MAV_CMD_NAV_ALTITUDE_WAIT);
    QCOMPARE(commandSpy.count(), 1);
    QList<QVariant> arguments = commandSpy.takeFirst();
    QCOMPARE(arguments.count(), 1);
    QCOMPARE((MAV_CMD)arguments.at(0).toInt(), MAV_CMD_NAV_ALTITUDE_WAIT);

    // frame
    QSignalSpy frameSpy(&missionItem._frameFact, SIGNAL(valueChanged(QVariant)));
    missionItem.setFrame(MAV_FRAME_GLOBAL_RELATIVE_ALT);
    QCOMPARE(frameSpy.count(), 0);
    missionItem.setFrame(MAV_FRAME_BODY_NED);
    QCOMPARE(frameSpy.count(), 1);
    arguments = frameSpy.takeFirst();
    QCOMPARE(arguments.count(), 1);
    QCOMPARE((MAV_FRAME)arguments.at(0).toInt(), MAV_FRAME_BODY_NED);

    // param1
    QSignalSpy param1Spy(&missionItem._param1Fact, SIGNAL(valueChanged(QVariant)));
    missionItem.setParam1(1.0);
    QCOMPARE(param1Spy.count(), 0);
    missionItem.setParam1(2.0);
    QCOMPARE(param1Spy.count(), 1);
    arguments = param1Spy.takeFirst();
    QCOMPARE(arguments.count(), 1);
    QCOMPARE(arguments.at(0).toDouble(), 2.0);

    // param2
    QSignalSpy param2Spy(&missionItem._param2Fact, SIGNAL(valueChanged(QVariant)));
    missionItem.setParam2(2.0);
    QCOMPARE(param2Spy.count(), 0);
    missionItem.setParam2(3.0);
    QCOMPARE(param2Spy.count(), 1);
    arguments = param2Spy.takeFirst();
    QCOMPARE(arguments.count(), 1);
    QCOMPARE(arguments.at(0).toDouble(), 3.0);

    // param3
    QSignalSpy param3Spy(&missionItem._param3Fact, SIGNAL(valueChanged(QVariant)));
    missionItem.setParam3(3.0);
    QCOMPARE(param3Spy.count(), 0);
    missionItem.setParam3(4.0);
    QCOMPARE(param3Spy.count(), 1);
    arguments = param3Spy.takeFirst();
    QCOMPARE(arguments.count(), 1);
    QCOMPARE(arguments.at(0).toDouble(), 4.0);

    // param4
    QSignalSpy param4Spy(&missionItem._param4Fact, SIGNAL(valueChanged(QVariant)));
    missionItem.setParam4(4.0);
    QCOMPARE(param4Spy.count(), 0);
    missionItem.setParam4(5.0);
    QCOMPARE(param4Spy.count(), 1);
    arguments = param4Spy.takeFirst();
    QCOMPARE(arguments.count(), 1);
    QCOMPARE(arguments.at(0).toDouble(), 5.0);

    // param6
    QSignalSpy param6Spy(&missionItem._param6Fact, SIGNAL(valueChanged(QVariant)));
    missionItem.setParam6(6.0);
    QCOMPARE(param6Spy.count(), 0);
    missionItem.setParam6(7.0);
    QCOMPARE(param6Spy.count(), 1);
    arguments = param6Spy.takeFirst();
    QCOMPARE(arguments.count(), 1);
    QCOMPARE(arguments.at(0).toDouble(), 7.0);

    // param7
    QSignalSpy param7Spy(&missionItem._param7Fact, SIGNAL(valueChanged(QVariant)));
    missionItem.setParam7(7.0);
    QCOMPARE(param7Spy.count(), 0);
    missionItem.setParam7(8.0);
    QCOMPARE(param7Spy.count(), 1);
    arguments = param7Spy.takeFirst();
    QCOMPARE(arguments.count(), 1);
    QCOMPARE(arguments.at(0).toDouble(), 8.0);
}

void MissionItemTest::_checkExpectedMissionItem(const MissionItem& missionItem)
{
    QCOMPARE(missionItem.sequenceNumber(), 10);
    QCOMPARE(missionItem.isCurrentItem(), false);
    QCOMPARE(missionItem.frame(), (MAV_FRAME)3);
    QCOMPARE(missionItem.command(), (MAV_CMD)80);
    QCOMPARE(missionItem.param1(), 10.0);
    QCOMPARE(missionItem.param2(), 20.0);
    QCOMPARE(missionItem.param3(), 30.0);
    QCOMPARE(missionItem.param4(), 40.0);
    QCOMPARE(missionItem.param5(), -10.0);
    QCOMPARE(missionItem.param6(), -20.0);
    QCOMPARE(missionItem.param7(), -30.0);
    QCOMPARE(missionItem.autoContinue(), true);
}

void MissionItemTest::_testLoadFromStream(void)
{
    MissionItem missionItem;

    QString testString("10\t0\t3\t80\t10\t20\t30\t40\t-10\t-20\t-30\t1\r\n");
    QTextStream testStream(&testString, QIODevice::ReadOnly);

    QVERIFY(missionItem.load(testStream));
    _checkExpectedMissionItem(missionItem);
}

void MissionItemTest::_testSimpleLoadFromStream(void)
{
    // We specifically test SimpleMissionItem loading as well since it has additional
    // signalling which can affect values.
    SimpleMissionItem simpleMissionItem(NULL);

    QString testString("10\t0\t3\t80\t10\t20\t30\t40\t-10\t-20\t-30\t1\r\n");
    QTextStream testStream(&testString, QIODevice::ReadOnly);

    QVERIFY(simpleMissionItem.load(testStream));
    _checkExpectedMissionItem(simpleMissionItem.missionItem());
}

void MissionItemTest::_testLoadFromJson(void)
{
    MissionItem missionItem;
    QString     errorString;
    QJsonArray  coordinateArray;
    coordinateArray << -10.0 << -20.0 <<-30.0;
    QJsonObject jsonObject;
    jsonObject.insert("autoContinue", true);
    jsonObject.insert("command", 80);
    jsonObject.insert("frame", 3);
    jsonObject.insert("id", 10);
    jsonObject.insert("param1", 10);
    jsonObject.insert("param2", 20);
    jsonObject.insert("param3", 30);
    jsonObject.insert("param4", 40);
    jsonObject.insert("type", "missionItem");
    jsonObject.insert("coordinate", coordinateArray);


    // Test missing key detection

    QStringList removeKeys;
    removeKeys << "autoContinue" << "command" << "frame" << "id" << "param1" << "param2" << "param3" << "param4" << "type" << "coordinate";
    foreach(const QString& removeKey, removeKeys) {
        QJsonObject badObject = jsonObject;
        badObject.remove(removeKey);
        QCOMPARE(missionItem.load(badObject, errorString), false);
        QVERIFY(!errorString.isEmpty());
        qDebug() << errorString;
    }

    // Test bad coordinate variations

    QJsonObject badObject = jsonObject;
    badObject.remove("coordinate");
    badObject["coordinate"] = 10;
    QCOMPARE(missionItem.load(badObject, errorString), false);
    QVERIFY(!errorString.isEmpty());
    qDebug() << errorString;

    QJsonArray  badCoordinateArray;
    badCoordinateArray << -10.0 << -20.0 ;
    badObject = jsonObject;
    badObject.remove("coordinate");
    badObject["coordinate"] = badCoordinateArray;
    QCOMPARE(missionItem.load(badObject, errorString), false);
    QVERIFY(!errorString.isEmpty());
    qDebug() << errorString;

    QJsonArray badCoordinateArray_second;
    badCoordinateArray_second << -10.0 << -20.0 << true;
    badObject = jsonObject;
    badObject.remove("coordinate");
    badObject["coordinate"] = badCoordinateArray_second;
    QCOMPARE(missionItem.load(badObject, errorString), false);
    QVERIFY(!errorString.isEmpty());
    qDebug() << errorString;

    QJsonArray  badCoordinateArray2;
    badCoordinateArray2 << 1 << 2;
    QJsonArray badCoordinateArray_third;
    badCoordinateArray_third << -10.0 << -20.0 << badCoordinateArray2;
    badObject = jsonObject;
    badObject.remove("coordinate");
    badObject["coordinate"] = badCoordinateArray_third;
    QCOMPARE(missionItem.load(badObject, errorString), false);
    QVERIFY(!errorString.isEmpty());
    qDebug() << errorString;

    // Test bad type

    badObject = jsonObject;
    badObject.remove("type");
    badObject["type"] = "foo";
    QCOMPARE(missionItem.load(badObject, errorString), false);
    QVERIFY(!errorString.isEmpty());
    qDebug() << errorString;

    // Test good load

    QVERIFY(missionItem.load(jsonObject, errorString));
    _checkExpectedMissionItem(missionItem);
}

void MissionItemTest::_testSimpleLoadFromJson(void)
{
    // We specifically test SimpleMissionItem loading as well since it has additional
    // signalling which can affect values.

    SimpleMissionItem simpleMissionItem(NULL);
    QString     errorString;
    QJsonArray  coordinateArray;
    coordinateArray << -10.0 << -20.0 << -30.0;
    QJsonObject jsonObject;
    jsonObject.insert("autoContinue", true);
    jsonObject.insert("command", 80);
    jsonObject.insert("frame", 3);
    jsonObject.insert("id", 10);
    jsonObject.insert("param1", 10);
    jsonObject.insert("param2", 20);
    jsonObject.insert("param3", 30);
    jsonObject.insert("param4", 40);
    jsonObject.insert("type", "missionItem");
    jsonObject.insert("coordinate", coordinateArray);


    QVERIFY(simpleMissionItem.load(jsonObject, errorString));
    _checkExpectedMissionItem(simpleMissionItem.missionItem());
}

void MissionItemTest::_testSaveToJson(void)
{
    MissionItem missionItem;

    missionItem.setSequenceNumber(10);
    missionItem.setIsCurrentItem(true);
    missionItem.setFrame((MAV_FRAME)3);
    missionItem.setCommand((MAV_CMD)80);
    missionItem.setParam1(10.1234567);
    missionItem.setParam2(20.1234567);
    missionItem.setParam3(30.1234567);
    missionItem.setParam4(40.1234567);
    missionItem.setParam5(-10.1234567);
    missionItem.setParam6(-20.1234567);
    missionItem.setParam7(-30.1234567);
    missionItem.setAutoContinue(true);

    // Round trip item
    QJsonObject jsonObject;
    QString errorString;
    missionItem.save(jsonObject);
    QVERIFY(missionItem.load(jsonObject, errorString));

    QCOMPARE(missionItem.sequenceNumber(), 10);
    QCOMPARE(missionItem.isCurrentItem(), false);
    QCOMPARE(missionItem.frame(), (MAV_FRAME)3);
    QCOMPARE(missionItem.command(), (MAV_CMD)80);
    QCOMPARE(missionItem.param1(), 10.1234567);
    QCOMPARE(missionItem.param2(), 20.1234567);
    QCOMPARE(missionItem.param3(), 30.1234567);
    QCOMPARE(missionItem.param4(), 40.1234567);
    QCOMPARE(missionItem.param5(), -10.1234567);
    QCOMPARE(missionItem.param6(), -20.1234567);
    QCOMPARE(missionItem.param7(), -30.1234567);
    QCOMPARE(missionItem.autoContinue(), true);
}

#if 0
void MissionItemTest::_writeItems(MockLinkMissionItemHandler::FailureMode_t failureMode)
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

void MissionItemTest::_roundTripItems(MockLinkMissionItemHandler::FailureMode_t failureMode)
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

void MissionItemTest::_testWriteFailureHandlingWorker(void)
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

void MissionItemTest::_testReadFailureHandlingWorker(void)
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

void MissionItemTest::_testWriteFailureHandlingAPM(void)
{
    _initForFirmwareType(MAV_AUTOPILOT_ARDUPILOTMEGA);
    _testWriteFailureHandlingWorker();
}

void MissionItemTest::_testReadFailureHandlingAPM(void)
{
    _initForFirmwareType(MAV_AUTOPILOT_ARDUPILOTMEGA);
    _testReadFailureHandlingWorker();
}


void MissionItemTest::_testWriteFailureHandlingPX4(void)
{
    _initForFirmwareType(MAV_AUTOPILOT_PX4);
    _testWriteFailureHandlingWorker();
}

void MissionItemTest::_testReadFailureHandlingPX4(void)
{
    _initForFirmwareType(MAV_AUTOPILOT_PX4);
    _testReadFailureHandlingWorker();
}
#endif
