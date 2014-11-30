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
#include "MainWindow.h"

UT_REGISTER_TEST(MainWindowTest)

MainWindowTest::MainWindowTest(void)
{
    
}

void MainWindowTest::init(void)
{
    UnitTest::init();
}

void MainWindowTest::cleanup(void)
{
    UnitTest::cleanup();
}

void MainWindowTest::_simpleDisplay_test(void)
{
    MainWindow* mainWindow = MainWindow::_create(NULL, MainWindow::CUSTOM_MODE_PX4);
    Q_CHECK_PTR(mainWindow);
    
    mainWindow->close();
    
    delete mainWindow;
}
