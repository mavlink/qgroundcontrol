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
///     @author Don Gagne <don@thegagnes.com>

#include "SetupViewTest.h"
#include "MockLink.h"
#include "QGCMessageBox.h"

UT_REGISTER_TEST(SetupViewTest)

SetupViewTest::SetupViewTest(void) :
    _mainWindow(NULL),
    _mainToolBar(NULL)
{
    
}

void SetupViewTest::init(void)
{
    UnitTest::init();

    _mainWindow = MainWindow::_create(NULL);
    Q_CHECK_PTR(_mainWindow);
    
    _mainToolBar = _mainWindow->getMainToolBar();
    Q_ASSERT(_mainToolBar);
}

void SetupViewTest::cleanup(void)
{
    _mainWindow->close();
    delete _mainWindow;
    
    UnitTest::cleanup();
}

void SetupViewTest::_clickThrough_test(void)
{
    LinkManager* linkMgr = LinkManager::instance();
    Q_CHECK_PTR(linkMgr);
    
    MockLink* link = new MockLink();
    Q_CHECK_PTR(link);
    link->setAutopilotType(MAV_AUTOPILOT_PX4);
    LinkManager::instance()->addLink(link);
    linkMgr->connectLink(link);
    QTest::qWait(5000); // Give enough time for UI to settle and heartbeats to go through
    
    // Switch to the Setup view
    _mainToolBar->onSetupView();
    QTest::qWait(1000);
    
    // Click through all the setup buttons
    // FIXME: NYI
    
    // On MainWindow close we should get a message box telling the user to disconnect first. Disconnect will then pop
    // the log file save dialog.
    
    setExpectedMessageBox(QGCMessageBox::Yes);
    setExpectedFileDialog(getSaveFileName, QStringList());
    
    _mainWindow->close();
    QTest::qWait(1000); // Need to allow signals to move between threads
    checkExpectedMessageBox();
    checkExpectedFileDialog();
}
