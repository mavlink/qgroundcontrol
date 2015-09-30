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
    { "1\t0\t3\t16\t10\t20\t30\t40\t-10\t-20\t-30\t1\r\n",  { 1, QGeoCoordinate(-10.0, -20.0, -30.0), MAV_CMD_NAV_WAYPOINT,     10.0, 20.0, 30.0, 40.0, true, false, MAV_FRAME_GLOBAL_RELATIVE_ALT } },
    { "1\t0\t3\t17\t10\t20\t30\t40\t-10\t-20\t-30\t1\r\n",  { 1, QGeoCoordinate(-10.0, -20.0, -30.0), MAV_CMD_NAV_LOITER_UNLIM, 10.0, 20.0, 30.0, 40.0, true, false, MAV_FRAME_GLOBAL_RELATIVE_ALT } },
    { "1\t0\t3\t18\t10\t20\t30\t40\t-10\t-20\t-30\t1\r\n",  { 1, QGeoCoordinate(-10.0, -20.0, -30.0), MAV_CMD_NAV_LOITER_TURNS, 10.0, 20.0, 30.0, 40.0, true, false, MAV_FRAME_GLOBAL_RELATIVE_ALT } },
    { "1\t0\t3\t19\t10\t20\t30\t40\t-10\t-20\t-30\t1\r\n",  { 1, QGeoCoordinate(-10.0, -20.0, -30.0), MAV_CMD_NAV_LOITER_TIME,  10.0, 20.0, 30.0, 40.0, true, false, MAV_FRAME_GLOBAL_RELATIVE_ALT } },
    { "1\t0\t3\t21\t10\t20\t30\t40\t-10\t-20\t-30\t1\r\n",  { 1, QGeoCoordinate(-10.0, -20.0, -30.0), MAV_CMD_NAV_LAND,         10.0, 20.0, 30.0, 40.0, true, false, MAV_FRAME_GLOBAL_RELATIVE_ALT } },
    { "1\t0\t3\t22\t10\t20\t30\t40\t-10\t-20\t-30\t1\r\n",  { 1, QGeoCoordinate(-10.0, -20.0, -30.0), MAV_CMD_NAV_TAKEOFF,      10.0, 20.0, 30.0, 40.0, true, false, MAV_FRAME_GLOBAL_RELATIVE_ALT } },
    { "1\t0\t2\t112\t10\t20\t30\t40\t-10\t-20\t-30\t1\r\n", { 1, QGeoCoordinate(-10.0, -20.0, -30.0), MAV_CMD_CONDITION_DELAY,  10.0, 20.0, 30.0, 40.0, true, false, MAV_FRAME_MISSION } },
    { "1\t0\t2\t177\t10\t20\t30\t40\t-10\t-20\t-30\t1\r\n", { 1, QGeoCoordinate(-10.0, -20.0, -30.0), MAV_CMD_DO_JUMP,          10.0, 20.0, 30.0, 40.0, true, false, MAV_FRAME_MISSION } },
};

MissionManagerTest::MissionManagerTest(void)
    : _mockLink(NULL)
{
    
}

void MissionManagerTest::init(void)
{
    UnitTest::init();
    
    LinkManager* linkMgr = LinkManager::instance();
    Q_CHECK_PTR(linkMgr);
    
    _mockLink = new MockLink();
    Q_CHECK_PTR(_mockLink);
    LinkManager::instance()->_addLink(_mockLink);
    
    linkMgr->connectLink(_mockLink);
    
    // Wait for the Vehicle to work it's way through the various threads
    
    QSignalSpy spyVehicle(MultiVehicleManager::instance(), SIGNAL(activeVehicleChanged(Vehicle*)));
    QCOMPARE(spyVehicle.wait(5000), true);
    
    // Wait for the Mission Manager to finish it's initial load
    
    _missionManager = MultiVehicleManager::instance()->activeVehicle()->missionManager();
    QVERIFY(_missionManager);
    
    _rgSignals[canEditChangedSignalIndex] =             SIGNAL(canEditChanged(bool));
    _rgSignals[newMissionItemsAvailableSignalIndex] =   SIGNAL(newMissionItemsAvailable(void));
    _rgSignals[inProgressChangedSignalIndex] =          SIGNAL(inProgressChanged(bool));

    _multiSpy = new MultiSignalSpy();
    Q_CHECK_PTR(_multiSpy);
    QCOMPARE(_multiSpy->init(_missionManager, _rgSignals, _cSignals), true);
    
    if (_missionManager->inProgress()) {
        _multiSpy->waitForSignalByIndex(newMissionItemsAvailableSignalIndex, 1000);
        _multiSpy->waitForSignalByIndex(inProgressChangedSignalIndex, 1000);
        QCOMPARE(_multiSpy->checkSignalByMask(newMissionItemsAvailableSignalMask | inProgressChangedSignalMask), true);
        QCOMPARE(_multiSpy->checkNoSignalByMask(canEditChangedSignalIndex), true);
    }
    
    QVERIFY(!_missionManager->inProgress());
    QCOMPARE(_missionManager->missionItems()->count(), 0);
    _multiSpy->clearAllSignals();
}

void MissionManagerTest::cleanup(void)
{
    delete _multiSpy;
    _multiSpy = NULL;
    
    LinkManager::instance()->disconnectLink(_mockLink);
    _mockLink = NULL;
    QTest::qWait(1000); // Need to allow signals to move between threads
    
    UnitTest::cleanup();
}

/// Checks the state of the inProgress value and signal to match the specified value
void MissionManagerTest::_checkInProgressValues(bool inProgress)
{
    QCOMPARE(_missionManager->inProgress(), inProgress);
    QSignalSpy* spy = _multiSpy->getSpyByIndex(inProgressChangedSignalIndex);
    QList<QVariant> signalArgs = spy->takeFirst();
    QCOMPARE(signalArgs.count(), 1);
    QCOMPARE(signalArgs[0].toBool(), inProgress);
}

void MissionManagerTest::_readEmptyVehicle(void)
{
    _missionManager->requestMissionItems();

    // requestMissionItems should emit inProgressChanged signal before returning so no need to wait for it
    QVERIFY(_missionManager->inProgress());
    QCOMPARE(_multiSpy->checkOnlySignalByMask(inProgressChangedSignalMask), true);
    _checkInProgressValues(true);
    
    _multiSpy->clearAllSignals();
    
    // Now wait for read sequence to complete. We should get both a newMissionItemsAvailable and a
    // inProgressChanged signal to signal completion.
    _multiSpy->waitForSignalByIndex(newMissionItemsAvailableSignalIndex, 1000);
    _multiSpy->waitForSignalByIndex(inProgressChangedSignalIndex, 1000);
    QCOMPARE(_multiSpy->checkSignalByMask(newMissionItemsAvailableSignalMask | inProgressChangedSignalMask), true);
    QCOMPARE(_multiSpy->checkNoSignalByMask(canEditChangedSignalMask), true);
    _checkInProgressValues(false);
    
    // Vehicle should have no items at this point
    QCOMPARE(_missionManager->missionItems()->count(), 0);
    QCOMPARE(_missionManager->canEdit(), true);
}

void MissionManagerTest::_roundTripItems(void)
{
    // Setup our test case data
    const size_t cTestCases = sizeof(_rgTestCases)/sizeof(_rgTestCases[0]);
    QmlObjectListModel* list = new QmlObjectListModel();
    
    for (size_t i=0; i<cTestCases; i++) {
        const TestCase_t* testCase = &_rgTestCases[i];
        
        MissionItem* item = new MissionItem(list);
        
        QTextStream loadStream(testCase->itemStream, QIODevice::ReadOnly);
        QVERIFY(item->load(loadStream));
        
        list->append(item);
    }
    
    // Send the items to the vehicle
    _missionManager->writeMissionItems(*list);

    // writeMissionItems should emit inProgressChanged signal before returning so no need to wait for it
    QVERIFY(_missionManager->inProgress());
    QCOMPARE(_multiSpy->checkOnlySignalByMask(inProgressChangedSignalMask), true);
    _checkInProgressValues(true);
    
    _multiSpy->clearAllSignals();
    
    // Now wait for write sequence to complete. We should only get an inProgressChanged signal to signal completion.
    _multiSpy->waitForSignalByIndex(inProgressChangedSignalIndex, 1000);
    QCOMPARE(_multiSpy->checkOnlySignalByMask(inProgressChangedSignalMask), true);
    _checkInProgressValues(false);
    
    QCOMPARE(_missionManager->missionItems()->count(), (int)cTestCases);
    QCOMPARE(_missionManager->canEdit(), true);
    
    delete list;
    list = NULL;
    _multiSpy->clearAllSignals();
    
    // Read the items back from the vehicle
    _missionManager->requestMissionItems();
    
    // requestMissionItems should emit inProgressChanged signal before returning so no need to wait for it
    QVERIFY(_missionManager->inProgress());
    QCOMPARE(_multiSpy->checkOnlySignalByMask(inProgressChangedSignalMask), true);
    _checkInProgressValues(true);
    
    _multiSpy->clearAllSignals();
    
    // Now wait for read sequence to complete. We should get both a newMissionItemsAvailable and a
    // inProgressChanged signal to signal completion.
    _multiSpy->waitForSignalByIndex(newMissionItemsAvailableSignalIndex, 1000);
    _multiSpy->waitForSignalByIndex(inProgressChangedSignalIndex, 1000);
    QCOMPARE(_multiSpy->checkSignalByMask(newMissionItemsAvailableSignalMask | inProgressChangedSignalMask), true);
    QCOMPARE(_multiSpy->checkNoSignalByMask(canEditChangedSignalMask), true);
    _checkInProgressValues(false);
    
    QCOMPARE(_missionManager->missionItems()->count(), (int)cTestCases);
    QCOMPARE(_missionManager->canEdit(), true);
    
    // Validate the returned items against our test data
    
    for (size_t i=0; i<cTestCases; i++) {
        const TestCase_t* testCase = &_rgTestCases[i];
        MissionItem* actual = qobject_cast<MissionItem*>(_missionManager->missionItems()->get(i));
        
        qDebug() << "Test case" << i;
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
    }
}
