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

// TODO: This is not working in Windows. Needs to be updated for new tool bar any way.
//UT_REGISTER_TEST(SetupViewTest)

SetupViewTest::SetupViewTest(void)
{
    
}

void SetupViewTest::init(void)
{
    UnitTest::init();

    _mainWindow = MainWindow::_create(NULL);
    Q_CHECK_PTR(_mainWindow);
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
    
    // Find the Setup button and click it
    
    // Tool Bar is now a QQuickWidget and cannot be manipulated like below
#if 0
    QGCToolBar* toolbar = _mainWindow->findChild<QGCToolBar*>();
    Q_ASSERT(toolbar);
    
    QList<QToolButton*> buttons = toolbar->findChildren<QToolButton*>();
    QToolButton* setupButton = NULL;
    foreach(QToolButton* button, buttons) {
        if (button->text() == "Setup") {
            setupButton = button;
            break;
        }
    }
    
    Q_ASSERT(setupButton);
    QTest::mouseClick(setupButton, Qt::LeftButton);
    QTest::qWait(1000);
#endif
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
