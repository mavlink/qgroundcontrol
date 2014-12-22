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
///     @brief Unit test for QGCMessageBox catching mechanism.
///
///     @author Don Gagne <don@thegagnes.com>

#ifndef MESSAGEBOXTEST_H
#define MESSAGEBOXTEST_H

#include "UnitTest.h"

class MessageBoxTest : public UnitTest
{
    Q_OBJECT
    
public:
    MessageBoxTest(void);
    
private slots:
    void _messageBoxExpected_test(void);
    void _messageBoxUnexpected_test(void);
    void _previousMessageBox_test(void);
    void _noMessageBox_test(void);
    void _badResponseButton_test(void);
};

#endif
