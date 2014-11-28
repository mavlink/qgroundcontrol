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

#ifndef UNITTEST_H
#define UNITTEST_H

#include <QObject>
#include <QtTest>
#include <QMessageBox>

#define UT_REGISTER_TEST(className) static UnitTestWrapper<className> t(#className);

/// @brief If you don't need you own specific implemenation of the test case setup methods
///         you can use these macros to declare the default implementation which just calls
///         the base class.

#define UT_DECLARE_DEFAULT_initTestCase 
//virtual void initTestCase(void) { UnitTest::_initTestCase(); }
#define UT_DECLARE_DEFAULT_cleanupTestCase 
// virtual void cleanupTestCase(void) { UnitTest::_cleanupTestCase(); }
#define UT_DECLARE_DEFAULT_init 
//virtual void init(void) { UnitTest::_init(); }
#define UT_DECLARE_DEFAULT_cleanup 
//virtual void cleanup(void) { UnitTest::_cleanup(); }

class QGCMessageBox;
class UnitTest;

static UnitTest* _activeUnitTest = NULL;    ///< Currently active unit test

class UnitTest : public QObject
{
    Q_OBJECT

public:
    UnitTest(void);
    virtual ~UnitTest(void);
    
    /// @brief Called to run all the registered unit tests
    ///     @param argc argc from main
    ///     @param argv argv from main
    ///     @param singleTest Name of test to just run a single test
    static int run(int argc, char *argv[], QString& singleTest);
    
    /// @brief Sets up for an expected QGCMessageBox
    ///     @param response Response to take on message box
    void setExpectedMessageBox(QMessageBox::StandardButton response);
    
    enum {
        expectFailNoFailure =           1 << 0, ///< not expecting any failures
        expectFailNoMessageBox =        1 << 1, ///< expecting a failure due to no message box displayed
        expectFailBadResponseButton =   1 << 2  ///< expecting a failure due to bad button response
    };
    
    /// @brief Check whether a message box was displayed and correctly responded to
    //          @param Expected failure response flags
    void checkExpectedMessageBox(int expectFailFlags = expectFailNoFailure);
    
    // Should only be called by UnitTestWrapper
    static void _addTest(QObject* test);
    
protected slots:
    
    // These are all pure virtuals to force the derived class to implement each one and in turn
    // call the UnitTest private implementation.
    
    /// @brief Called at the initialization of the entire unit test.
    ///         Make sure to call _initTestCase first in your derived class.
    virtual void initTestCase(void);
    
    /// @brief Called at the end of the entire unit test.
    ///         Make sure to call _cleanupTestCase first in your derived class.
    virtual void cleanupTestCase(void);
    
    /// @brief Called before each test.
    ///         Make sure to call _init first in your derived class.
    virtual void init(void);
    
    /// @brief Called after each test.
    ///         Make sure to call _cleanup first in your derived class.
    virtual void cleanup(void);
    
protected:
    /// @brief Must be called first by derived class implementation
    void _initTestCase(void);
    
    /// @brief Must be called first by derived class implementation
    void _cleanupTestCase(void);
    
    /// @brief Must be called first by derived class implementation
    void _init(void);
    
    /// @brief Must be called first by derived class implementation
    void _cleanup(void);
    
private:
    // When the app is running in unit test mode the QGCMessageBox methods are re-routed here.
    static QMessageBox::StandardButton _messageBox(QMessageBox::Icon icon, const QString& title, const QString& text, QMessageBox::StandardButtons buttons, QMessageBox::StandardButton defaultButton);
    
    // This allows the private call to _messageBox
    friend class QGCMessageBox;
    
    void _unitTestCalled(void);
    
    static bool                         _messageBoxRespondedTo;     ///< Message box was responded to
    static bool                         _badResponseButton;         ///< Attempt to repond to expected message box with button not being displayed
    static QMessageBox::StandardButton  _messageBoxResponseButton;  ///< Response to next message box
    static int                          _missedMessageBoxCount;     ///< Count of message box not checked with call to messageBoxWasDisplayed
    
    bool _unitTestRun;              ///< true: Unit Test was run
    bool _initTestCaseCalled;       ///< true: UnitTest::_initTestCase was called
    bool _cleanupTestCaseCalled;    ///< true: UnitTest::_cleanupTestCase was called
    bool _initCalled;               ///< true: UnitTest::_init was called
    bool _cleanupCalled;            ///< true: UnitTest::_cleanup was called
    
    typedef QList<QObject*> UnitTestList_t;
    
    static UnitTestList_t   _tests;
};

template <class T>
class UnitTestWrapper {
public:
    UnitTestWrapper(const QString& name) :
        _unitTest(new T)
    {
        _unitTest->setObjectName(name);
        UnitTest::_addTest(_unitTest.data());
    }

private:
    QSharedPointer<T> _unitTest;
};

#endif
