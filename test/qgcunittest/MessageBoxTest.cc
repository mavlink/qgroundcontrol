/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


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

