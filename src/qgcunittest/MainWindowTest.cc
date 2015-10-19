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

/// @file
///     @brief Simple MainWindow unit test
///
///     @author Don Gagne <don@thegagnes.com>

#include "MainWindowTest.h"
#include "MockLink.h"
#include "QGCMessageBox.h"
#include "MultiVehicleManager.h"

UT_REGISTER_TEST(MainWindowTest)

MainWindowTest::MainWindowTest(void) :
    _mainWindow()
{
    
}

void MainWindowTest::init(void)
{
    UnitTest::init();

    _mainWindow = MainWindow::_create();
    Q_CHECK_PTR(_mainWindow);
}

void MainWindowTest::cleanup(void)
{
    _mainWindow->close();
    QTest::qWait(200);
    delete _mainWindow;
    
    UnitTest::cleanup();
}

void MainWindowTest::_connectWindowClose_test(MAV_AUTOPILOT autopilot)
{
    LinkManager* linkMgr = LinkManager::instance();
    Q_CHECK_PTR(linkMgr);
    
    MockLink* link = new MockLink();
    Q_CHECK_PTR(link);
    link->setAutopilotType(autopilot);
    LinkManager::instance()->_addLink(link);
    
    linkMgr->connectLink(link);
    
    // Wait for the Vehicle to work it's way through the various threads
    
    QSignalSpy spyVehicle(MultiVehicleManager::instance(), SIGNAL(activeVehicleChanged(Vehicle*)));
    QCOMPARE(spyVehicle.wait(5000), true);
    
    // On MainWindow close we should get a message box telling the user to disconnect first. Cancel should do nothing.
    setExpectedMessageBox(QGCMessageBox::Cancel);
    _mainWindow->close();
    QTest::qWait(1000); // Need to allow signals to move between threads    
    checkExpectedMessageBox();

    linkMgr->disconnectLink(link);
    QTest::qWait(1000); // Need to allow signals to move between threads
}

void MainWindowTest::_connectWindowClosePX4_test(void) {
    _connectWindowClose_test(MAV_AUTOPILOT_PX4);
}

void MainWindowTest::_connectWindowCloseGeneric_test(void) {
    _connectWindowClose_test(MAV_AUTOPILOT_ARDUPILOTMEGA);
}
