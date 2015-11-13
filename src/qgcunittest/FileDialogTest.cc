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
///     @brief Unit test for QGCFileDialog catching mechanism.
///
///     @author Don Gagne <don@thegagnes.com>

#include "FileDialogTest.h"
#include "QGCFileDialog.h"

FileDialogTest::FileDialogTest(void)
{
    
}

void FileDialogTest::_fileDialogExpected_test(void)
{
    QStringList response;
    response << "" << "response";
    
    for (int i=0; i<response.count(); i++) {
        setExpectedFileDialog(getExistingDirectory, QStringList(response[i]));
        QCOMPARE(QGCFileDialog::getExistingDirectory(), response[i]);
        checkExpectedFileDialog();
    }

    for (int i=0; i<response.count(); i++) {
        setExpectedFileDialog(getOpenFileName, QStringList(response[i]));
        QCOMPARE(QGCFileDialog::getOpenFileName(), response[i]);
        checkExpectedFileDialog();
    }
    
    for (int i=0; i<response.count(); i++) {
        setExpectedFileDialog(getSaveFileName, QStringList(response[i]));
        QCOMPARE(QGCFileDialog::getSaveFileName(), response[i]);
        checkExpectedFileDialog();
    }
    
    QList<QStringList> responseList;
    QStringList list;
    
    responseList.append(QStringList());
    responseList.append(QStringList("response1"));
    list << "response1" << "response2";
    responseList.append(list);

    for (int i=0; i<responseList.count(); i++) {
        setExpectedFileDialog(getOpenFileNames, responseList[i]);
        QStringList retResponse = QGCFileDialog::getOpenFileNames();
        checkExpectedFileDialog();
        QCOMPARE(retResponse.count(), responseList[i].count());
         for (int j=0; j<retResponse.count(); j++) {
             QCOMPARE(retResponse[j], responseList[i][j]);
         }
    }
}

void FileDialogTest::_fileDialogUnexpected_test(void)
{
    // This should cause an expected failure in the cleanup method
    QGCFileDialog::getOpenFileName();
    _expectMissedFileDialog = true;
}

void FileDialogTest::_previousFileDialog_test(void)
{
    // This is the previous unexpected file dialog
    QGCFileDialog::getOpenFileName();
    
    // Setup for an expected message box.
    QEXPECT_FAIL("", "Expecting failure due to previous file dialog", Continue);
    setExpectedFileDialog(getOpenFileName, QStringList());
}

void FileDialogTest::_noFileDialog_test(void)
{
    setExpectedFileDialog(getOpenFileName, QStringList());
    checkExpectedFileDialog(expectFailNoDialog);
}

void FileDialogTest::_fileDialogExpectedIncorrect_test(void)
{
    // Expecting save but get open dialog
    setExpectedFileDialog(getSaveFileName, QStringList());
    QGCFileDialog::getOpenFileName();
    checkExpectedFileDialog(expectFailWrongFileDialog);
    
    // This is going to fail in cleanup as well since we have a missed file dialog
    _expectMissedFileDialog = true;
}
