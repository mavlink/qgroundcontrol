/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/// @file
///     @brief Unit test for QGCQFileDialog catching mechanism.
///
///     @author Don Gagne <don@thegagnes.com>

#include "FileDialogTest.h"
#include "QGCQFileDialog.h"

FileDialogTest::FileDialogTest(void)
{
    
}

void FileDialogTest::_fileDialogExpected_test(void)
{
    QStringList response;
    response << "" << "response";
    
    for (int i=0; i<response.count(); i++) {
        setExpectedFileDialog(getExistingDirectory, QStringList(response[i]));
        QCOMPARE(QGCQFileDialog::getExistingDirectory(), response[i]);
        checkExpectedFileDialog();
    }

    for (int i=0; i<response.count(); i++) {
        setExpectedFileDialog(getOpenFileName, QStringList(response[i]));
        QCOMPARE(QGCQFileDialog::getOpenFileName(), response[i]);
        checkExpectedFileDialog();
    }
    
    for (int i=0; i<response.count(); i++) {
        setExpectedFileDialog(getSaveFileName, QStringList(response[i]));
        QCOMPARE(QGCQFileDialog::getSaveFileName(), response[i]);
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
        QStringList retResponse = QGCQFileDialog::getOpenFileNames();
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
    QGCQFileDialog::getOpenFileName();
    _expectMissedFileDialog = true;
}

void FileDialogTest::_previousFileDialog_test(void)
{
    // This is the previous unexpected file dialog
    QGCQFileDialog::getOpenFileName();
    
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
    QGCQFileDialog::getOpenFileName();
    checkExpectedFileDialog(expectFailWrongFileDialog);
    
    // This is going to fail in cleanup as well since we have a missed file dialog
    _expectMissedFileDialog = true;
}
