/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/// @file
///     @brief Simple MainWindow unit test
///
///     @author Don Gagne <don@thegagnes.com>

#include "MainWindowTest.h"
#include "MockLink.h"
#include "QGCMessageBox.h"
#include "MultiVehicleManager.h"

void MainWindowTest::_connectWindowClose_test(MAV_AUTOPILOT autopilot)
{
    _createMainWindow();
    _connectMockLink(autopilot);
    
    // On MainWindow close we should get a message box telling the user to disconnect first. Cancel should do nothing.
    setExpectedMessageBox(QGCMessageBox::Cancel);
    _closeMainWindow(true /* cancelExpected */);
    checkExpectedMessageBox();
}

void MainWindowTest::_connectWindowClosePX4_test(void) {
    _connectWindowClose_test(MAV_AUTOPILOT_PX4);
}

void MainWindowTest::_connectWindowCloseGeneric_test(void) {
    _connectWindowClose_test(MAV_AUTOPILOT_ARDUPILOTMEGA);
}
