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
#include "QGCToolBar.h"
#include "MockLink.h"
#include "QGCMessageBox.h"

UT_REGISTER_TEST(MainWindowTest)

MainWindowTest::MainWindowTest(void)
{
    
}

void MainWindowTest::init(void)
{
    UnitTest::init();

    _mainWindow = MainWindow::_create(NULL, MainWindow::CUSTOM_MODE_PX4);
    Q_CHECK_PTR(_mainWindow);
}

void MainWindowTest::cleanup(void)
{
    _mainWindow->close();
    delete _mainWindow;
    
    UnitTest::cleanup();
}

void MainWindowTest::_clickThrough_test(void)
{
    QGCToolBar* toolbar = _mainWindow->findChild<QGCToolBar*>();
    Q_ASSERT(toolbar);
    
    QList<QToolButton*> buttons = toolbar->findChildren<QToolButton*>();
    foreach(QToolButton* button, buttons) {
        if (!button->menu()) {
            QTest::mouseClick(button, Qt::LeftButton);
            QTest::qWait(1000);
        }
    }
    
}

void MainWindowTest::_connectWindowClose_test(MAV_AUTOPILOT autopilot)
{
    LinkManager* linkMgr = LinkManager::instance();
    Q_CHECK_PTR(linkMgr);
    
    MockLink* link = new MockLink();
    Q_CHECK_PTR(link);
    link->setAutopilotType(autopilot);
    LinkManager::instance()->addLink(link);
    linkMgr->connectLink(link);
    QTest::qWait(5000); // Give enough time for UI to settle and heartbeats to go through
    
    // On MainWindow close we should get a message box telling the user to disconnect first. Cancel should do nothing.
    setExpectedMessageBox(QGCMessageBox::Cancel);
    _mainWindow->close();
    QTest::qWait(1000); // Need to allow signals to move between threads    
    checkExpectedMessageBox();

    // We are going to disconnect the link which is going to pop a save file dialog
    setExpectedFileDialog(getSaveFileName, QStringList());
    linkMgr->disconnectLink(link);
    QTest::qWait(1000); // Need to allow signals to move between threads
    checkExpectedFileDialog();
}

void MainWindowTest::_connectWindowClosePX4_test(void) {
    _connectWindowClose_test(MAV_AUTOPILOT_PX4);
}

void MainWindowTest::_connectWindowCloseGeneric_test(void) {
    _connectWindowClose_test(MAV_AUTOPILOT_ARDUPILOTMEGA);
}
