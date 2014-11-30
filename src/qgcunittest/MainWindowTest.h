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

#ifndef MAINWINDOWTEST_H
#define MAINWINDOWTEST_H

#include "UnitTest.h"

class MainWindowTest : public UnitTest
{
    Q_OBJECT
    
public:
    MainWindowTest(void);
    
private slots:
    UT_DECLARE_DEFAULT_initTestCase
    UT_DECLARE_DEFAULT_cleanupTestCase
    void init(void);
    void cleanup(void);
    
    void _simpleDisplay_test(void);
};

#endif
