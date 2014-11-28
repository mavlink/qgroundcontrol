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

bool UnitTest::_messageBoxRespondedTo = false;
bool UnitTest::_badResponseButton = false;
QMessageBox::StandardButton UnitTest::_messageBoxResponseButton = QMessageBox::NoButton;
int UnitTest::_missedMessageBoxCount = 0;
UnitTest::UnitTestList_t UnitTest::_tests;

UnitTest::UnitTest(void) :
    _unitTestRun(false),
    _initTestCaseCalled(false),
    _cleanupTestCaseCalled(false),
    _initCalled(false),
    _cleanupCalled(false)
{
    
}

UnitTest::~UnitTest()
{
    if (_unitTestRun) {
        // Derived classes must call base class implementations
        Q_ASSERT(_initTestCaseCalled);
        Q_ASSERT(_cleanupTestCaseCalled);
        Q_ASSERT(_initCalled);
        Q_ASSERT(_cleanupCalled);
    }
}

void UnitTest::_addTest(QObject* test)
{
    Q_ASSERT(!_tests.contains(test));
    
    _tests.append(test);
}

void UnitTest::_unitTestCalled(void)
{
    _unitTestRun = true;
}

int UnitTest::run(int argc, char *argv[], QString& singleTest)
{
    int ret = 0;
    
    foreach (QObject* test, _tests) {
        if (singleTest.isEmpty() || singleTest == test->objectName()) {
            ret += QTest::qExec(test, argc, argv);
        }
    }
    
    return ret;
}

/// @brief Called at the initialization of the entire unit test.
///         Make sure to call first in your derived class
void UnitTest::initTestCase(void)
{
    _initTestCaseCalled = true;
    
    _missedMessageBoxCount = 0;
    _badResponseButton = false;
}

/// @brief Called at the end of the entire unit test.
///         Make sure to call first in your derived class
void UnitTest::cleanupTestCase(void)
{
    _cleanupTestCaseCalled = true;
        
    int missedMessageBoxCount = _missedMessageBoxCount;
    _missedMessageBoxCount = 0;
    
    // Make sure to reset any needed variables since this can fall and cause the rest of the method to be skipped
    QCOMPARE(missedMessageBoxCount, 0);
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

    // Each test gets a clean global state
    qgcApp()->destroySingletonsForUnitTest();
    qgcApp()->createSingletonsForUnitTest();
}

/// @brief Called after each test.
///         Make sure to call first in your derived class
void UnitTest::cleanup(void)
{
    _cleanupCalled = true;
    
    int missedMessageBoxCount = _missedMessageBoxCount;
    _missedMessageBoxCount = 0;
    
    // Make sure to reset any needed variables since this can fall and cause the rest of the method to be skipped
    QCOMPARE(missedMessageBoxCount, 0);
}

void UnitTest::setExpectedMessageBox(QMessageBox::StandardButton response)
{
    Q_ASSERT(!_messageBoxRespondedTo);
    
    // We use an obsolete StandardButton value to signal that response button has not been set. So you can't use this.
    Q_ASSERT(response != QMessageBox::NoButton);
    Q_ASSERT(_messageBoxResponseButton == QMessageBox::NoButton);
    
    int missedMessageBoxCount = _missedMessageBoxCount;
    _missedMessageBoxCount = 0;
    
    QCOMPARE(missedMessageBoxCount, 0);
    
    _messageBoxResponseButton = response;
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
    
    if (expectFailFlags & expectFailNoMessageBox) {
        QEXPECT_FAIL("", "Expecting failure due to no message box", Continue);
    }
    QCOMPARE(_messageBoxRespondedTo, true);
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
