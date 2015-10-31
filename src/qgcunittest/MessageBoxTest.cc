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

#include "MessageBoxTest.h"
#include "QGCMessageBox.h"

MessageBoxTest::MessageBoxTest(void)
{
    
}

void MessageBoxTest::_messageBoxExpected_test(void)
{
    setExpectedMessageBox(QMessageBox::Ok);
    
    QCOMPARE(QGCMessageBox::information(QString(), QString()), QMessageBox::Ok);
    
    checkExpectedMessageBox();
}

void MessageBoxTest::_messageBoxUnexpected_test(void)
{
    // This should cause an expected failure in the cleanup method
    QGCMessageBox::information(QString(), QString());
    _expectMissedMessageBox = true;
}

void MessageBoxTest::_previousMessageBox_test(void)
{
    // This is the previous unexpected message box
    QGCMessageBox::information(QString(), QString());
    
    // Setup for an expected message box.
    QEXPECT_FAIL("", "Expecting failure due to previous message boxes", Continue);
    setExpectedMessageBox(QMessageBox::Ok);
}

void MessageBoxTest::_noMessageBox_test(void)
{
    setExpectedMessageBox(QMessageBox::Ok);
    checkExpectedMessageBox(expectFailNoDialog);
}

void MessageBoxTest::_badResponseButton_test(void)
{
    setExpectedMessageBox(QMessageBox::Cancel);

    // Will return Ok even though Cancel was specified, since that was wrong
    QCOMPARE(QGCMessageBox::information(QString(), QString()), QMessageBox::Ok);
    
    checkExpectedMessageBox(expectFailBadResponseButton);
}

