/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "MockLink.h"
#include "MAVLinkLib.h"

#include <QtCore/QLoggingCategory>
#include <QtCore/QObject>
#include <QtPositioning/QGeoCoordinate>

#define UT_REGISTER_TEST(className) static UnitTestWrapper<className> className(#className, false);
#define UT_REGISTER_TEST_STANDALONE(className) static UnitTestWrapper<className> className(#className, true);  // Test will only be run with specifically called to from command line

Q_DECLARE_LOGGING_CATEGORY(UnitTestLog)

class Vehicle;
class Fact;
class LinkInterface;
class MissionItem;

class UnitTest : public QObject
{
    Q_OBJECT

public:
    UnitTest(QObject *parent = nullptr);
    virtual ~UnitTest();

    /// @brief Called to run all the registered unit tests
    ///     @param singleTest Name of test to just run a single test
    static int run(QStringView singleTest);

    bool standalone() const { return _standalone; }
    void setStandalone(bool standalone) { _standalone = standalone; }

    /// @brief Adds a unit test to the list. Should only be called by UnitTestWrapper.
    static void _addTest(UnitTest *test);

    /// Will throw qWarning at location where files differ
    /// @return true: files are alike, false: files differ
    static bool fileCompare(const QString &file1, const QString &file2);

    /// Changes the Facts rawValue such that it emits a valueChanged signal.
    ///     @param increment 0 use standard increment, other increment by specified amount if double value
    void changeFactValue(Fact *fact, double increment = 0);

    QGeoCoordinate changeCoordinateValue(const QGeoCoordinate &coordinate);

    /// Returns true is the position of the two coordinates is less then a meter from each other.
    /// Does not check altitude.
    static bool fuzzyCompareLatLon(const QGeoCoordinate &coord1, const QGeoCoordinate &coord2);

protected slots:
    // These are all pure virtuals to force the derived class to implement each one and in turn
    // call the UnitTest private implementation.

    /// @brief Called before each test.
    ///         Make sure to call UnitTest::init first in your derived class.
    virtual void init();

    /// @brief Called after each test.
    ///         Make sure to call UnitTest::cleanup last in your derived class.
    virtual void cleanup();

protected:
    void _connectMockLink(MAV_AUTOPILOT autopilot = MAV_AUTOPILOT_PX4, MockConfiguration::FailureMode_t failureMode = MockConfiguration::FailNone);
    void _connectMockLinkNoInitialConnectSequence() { _connectMockLink(MAV_AUTOPILOT_INVALID); }
    void _disconnectMockLink();
    static void _missionItemsEqual(const MissionItem &actual, const MissionItem &expected);

    MockLink *_mockLink = nullptr;
    Vehicle *_vehicle = nullptr;

private slots:
    void _linkDeleted(const LinkInterface *link);

private:
    void _unitTestCalled() { _unitTestRun = true; }

    /// @brief Returns the list of unit tests.
    static QList<UnitTest*> &_testList();

    bool _unitTestRun = false;      ///< true: Unit Test was run
    bool _initCalled = false;       ///< true: UnitTest::_init was called
    bool _cleanupCalled = false;    ///< true: UnitTest::_cleanup was called
    bool _standalone = false;       ///< true: Only run when requested specifically from command line
};

template <class T>
class UnitTestWrapper
{
public:
    UnitTestWrapper(const QString &name, bool standalone)
        : _unitTest(new T)
    {
        _unitTest->setObjectName(name);
        _unitTest->setStandalone(standalone);
        UnitTest::_addTest(_unitTest.data());
    }

private:
    QSharedPointer<T> _unitTest;
};
