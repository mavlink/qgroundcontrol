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
///     @brief Base class for all unit tests
///
///     @author Don Gagne <don@thegagnes.com>

#include "UnitTest.h"
#include "QGCApplication.h"
#include "MAVLinkProtocol.h"

bool UnitTest::_messageBoxRespondedTo = false;
bool UnitTest::_badResponseButton = false;
QMessageBox::StandardButton UnitTest::_messageBoxResponseButton = QMessageBox::NoButton;
int UnitTest::_missedMessageBoxCount = 0;

bool UnitTest::_fileDialogRespondedTo = false;
bool UnitTest::_fileDialogResponseSet = false;
QStringList UnitTest::_fileDialogResponse;
enum UnitTest::FileDialogType UnitTest::_fileDialogExpectedType = getOpenFileName;
int UnitTest::_missedFileDialogCount = 0;

UnitTest::UnitTest(void) :
    _expectMissedFileDialog(false),
    _expectMissedMessageBox(false),
    _unitTestRun(false),
    _initCalled(false),
    _cleanupCalled(false)
{
    
}

UnitTest::~UnitTest()
{
    if (_unitTestRun) {
        // Derived classes must call base class implementations
        Q_ASSERT(_initCalled);
        Q_ASSERT(_cleanupCalled);
    }
}

void UnitTest::_addTest(QObject* test)
{
	QList<QObject*>& tests = _testList();

    Q_ASSERT(!tests.contains(test));
    
    tests.append(test);
}

void UnitTest::_unitTestCalled(void)
{
    _unitTestRun = true;
}

/// @brief Returns the list of unit tests.
QList<QObject*>& UnitTest::_testList(void)
{
	static QList<QObject*> tests;
	return tests;
}

int UnitTest::run(QString& singleTest)
{
    int ret = 0;
    
    foreach (QObject* test, _testList()) {
        if (singleTest.isEmpty() || singleTest == test->objectName()) {
            QStringList args;
            args << "*" << "-maxwarnings" << "0";
            ret += QTest::qExec(test, args);
        }
    }
    
    return ret;
}

/// @brief Called before each test.
///         Make sure to call first in your derived class
void UnitTest::init(void)
{
    _initCalled = true;
    
    _messageBoxRespondedTo = false;
    _missedMessageBoxCount = 0;
    _badResponseButton = false;
    _messageBoxResponseButton = QMessageBox::NoButton;
    
    _fileDialogRespondedTo = false;
    _missedFileDialogCount = 0;
    _fileDialogResponseSet = false;
    _fileDialogResponse.clear();

    _expectMissedFileDialog = false;
    _expectMissedMessageBox = false;
    
    // Each test gets a clean global state
    qgcApp()->_destroySingletons();
    qgcApp()->_createSingletons();
    
    MAVLinkProtocol::deleteTempLogFiles();
}

/// @brief Called after each test.
///         Make sure to call first in your derived class
void UnitTest::cleanup(void)
{
    _cleanupCalled = true;

    // Keep in mind that any code below these QCOMPARE may be skipped if the compare fails
    if (_expectMissedMessageBox) {
        QEXPECT_FAIL("", "Expecting failure due internal testing", Continue);
    }
    QCOMPARE(_missedMessageBoxCount, 0);
    if (_expectMissedFileDialog) {
        QEXPECT_FAIL("", "Expecting failure due internal testing", Continue);
    }
    QCOMPARE(_missedFileDialogCount, 0);
    
    qgcApp()->_destroySingletons();
}

void UnitTest::setExpectedMessageBox(QMessageBox::StandardButton response)
{
    // This means that there was an expected message box but no call to checkExpectedMessageBox
    Q_ASSERT(!_messageBoxRespondedTo);
    
    Q_ASSERT(response != QMessageBox::NoButton);
    Q_ASSERT(_messageBoxResponseButton == QMessageBox::NoButton);
    
    // Make sure we haven't missed any previous message boxes
    int missedMessageBoxCount = _missedMessageBoxCount;
    _missedMessageBoxCount = 0;
    QCOMPARE(missedMessageBoxCount, 0);
    
    _messageBoxResponseButton = response;
}

void UnitTest::setExpectedFileDialog(enum FileDialogType type, QStringList response)
{
    // This means that there was an expected file dialog but no call to checkExpectedFileDialog
    Q_ASSERT(!_fileDialogRespondedTo);
    
    // Multiple responses must be expected getOpenFileNames
    Q_ASSERT(response.count() <= 1 || type == getOpenFileNames);

    // Make sure we haven't missed any previous file dialogs
    int missedFileDialogCount = _missedFileDialogCount;
    _missedFileDialogCount = 0;
    QCOMPARE(missedFileDialogCount, 0);
    
    _fileDialogResponseSet = true;
    _fileDialogResponse = response;
    _fileDialogExpectedType = type;
}

void UnitTest::checkExpectedMessageBox(int expectFailFlags)
{
    // Previous call to setExpectedMessageBox should have already checked this
    Q_ASSERT(_missedMessageBoxCount == 0);
    
    // Check for a valid response
    
    if (expectFailFlags & expectFailBadResponseButton) {
        QEXPECT_FAIL("", "Expecting failure due to bad button response", Continue);
    }
    QCOMPARE(_badResponseButton, false);
    
    if (expectFailFlags & expectFailNoDialog) {
        QEXPECT_FAIL("", "Expecting failure due to no message box", Continue);
    }
    
    // Clear this flag before QCOMPARE since anything after QCOMPARE will be skipped on failure
    bool messageBoxRespondedTo = _messageBoxRespondedTo;
    _messageBoxRespondedTo = false;
    
    QCOMPARE(messageBoxRespondedTo, true);
}

void UnitTest::checkExpectedFileDialog(int expectFailFlags)
{
    // Internal testing
    
    if (expectFailFlags & expectFailNoDialog) {
        QEXPECT_FAIL("", "Expecting failure due to no file dialog", Continue);
    }
    if (expectFailFlags & expectFailWrongFileDialog) {
        QEXPECT_FAIL("", "Expecting failure due to incorrect file dialog", Continue);
    } else {
        // Previous call to setExpectedFileDialog should have already checked this
        Q_ASSERT(_missedFileDialogCount == 0);
    }
    
    // Clear this flag before QCOMPARE since anything after QCOMPARE will be skipped on failure
    bool fileDialogRespondedTo = _fileDialogRespondedTo;
    _fileDialogRespondedTo = false;
    
    QCOMPARE(fileDialogRespondedTo, true);
}

QMessageBox::StandardButton UnitTest::_messageBox(QMessageBox::Icon icon, const QString& title, const QString& text, QMessageBox::StandardButtons buttons, QMessageBox::StandardButton defaultButton)
{
    QMessageBox::StandardButton retButton;
    
    Q_UNUSED(icon);
    Q_UNUSED(title);
    Q_UNUSED(text);

    if (_messageBoxResponseButton == QMessageBox::NoButton) {
        // If no response button is set it means we were not expecting this message box. Response with default
        _missedMessageBoxCount++;
        retButton = defaultButton;
    } else {
        if (_messageBoxResponseButton & buttons) {
            // Everything is correct, use the specified response
            retButton = _messageBoxResponseButton;
        } else {
            // Trying to respond with a button not in the dialog. This is an error. Respond with default
            _badResponseButton = true;
            retButton = defaultButton;
        }
        _messageBoxRespondedTo = true;
    }
    
    // Clear response for next message box
    _messageBoxResponseButton = QMessageBox::NoButton;
    
    return retButton;
}

/// @brief Response to a file dialog which returns a single file
QString UnitTest::_fileDialogResponseSingle(enum FileDialogType type)
{
    QString retFile;
    
    if (!_fileDialogResponseSet || _fileDialogExpectedType != type) {
        // If no response is set or the type does not match what we expected it means we were not expecting this file dialog.
        // Respond with no selection.
        _missedFileDialogCount++;
    } else {
        Q_ASSERT(_fileDialogResponse.count() <= 1);
        if (_fileDialogResponse.count() == 1) {
            retFile = _fileDialogResponse[0];
        }
        _fileDialogRespondedTo = true;
    }
    
    // Clear response for next message box
    _fileDialogResponse.clear();
    _fileDialogResponseSet = false;
    
    return retFile;
}

QString UnitTest::_getExistingDirectory(
    QWidget* parent,
    const QString& caption,
    const QString& dir,
    QFileDialog::Options options)
{
    Q_UNUSED(parent);
    Q_UNUSED(caption);
    Q_UNUSED(dir);
    Q_UNUSED(options);

    return _fileDialogResponseSingle(getExistingDirectory);
}

QString UnitTest::_getOpenFileName(
    QWidget* parent,
    const QString& caption,
    const QString& dir,
    const QString& filter,
    QFileDialog::Options options)
{
    Q_UNUSED(parent);
    Q_UNUSED(caption);
    Q_UNUSED(dir);
    Q_UNUSED(filter);
    Q_UNUSED(options);
    
    return _fileDialogResponseSingle(getOpenFileName);
}

QStringList UnitTest::_getOpenFileNames(
    QWidget* parent,
    const QString& caption,
    const QString& dir,
    const QString& filter,
    QFileDialog::Options options)
{
    Q_UNUSED(parent);
    Q_UNUSED(caption);
    Q_UNUSED(dir);
    Q_UNUSED(filter);
    Q_UNUSED(options);

    QStringList retFiles;
    
    if (!_fileDialogResponseSet || _fileDialogExpectedType != getOpenFileNames) {
        // If no response is set or the type does not match what we expected it means we were not expecting this file dialog.
        // Respond with no selection.
        _missedFileDialogCount++;
        retFiles.clear();
    } else {
        retFiles = _fileDialogResponse;
        _fileDialogRespondedTo = true;
    }
    
    // Clear response for next message box
    _fileDialogResponse.clear();
    _fileDialogResponseSet = false;
    
    return retFiles;
}

QString UnitTest::_getSaveFileName(
    QWidget* parent,
    const QString& caption,
    const QString& dir,
    const QString& filter,
    const QString& defaultSuffix,
    QFileDialog::Options options)
{
    Q_UNUSED(parent);
    Q_UNUSED(caption);
    Q_UNUSED(dir);
    Q_UNUSED(filter);
    Q_UNUSED(options);

    if(!defaultSuffix.isEmpty()) {
        Q_ASSERT(defaultSuffix.startsWith(".") == false);
    }

    return _fileDialogResponseSingle(getSaveFileName);
}
