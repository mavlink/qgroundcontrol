/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "UnitTest.h"
#include "MAVLinkProtocol.h"
#include "MultiVehicleManager.h"
#include "Vehicle.h"
#include "AppSettings.h"
#include "SettingsManager.h"
#include "LinkManager.h"
#include "QGC.h"
#include "Fact.h"
#include "MissionItem.h"
#include "QGCLoggingCategory.h"

#include <QtTest/QTest>
#include <QtTest/QSignalSpy>

QGC_LOGGING_CATEGORY(UnitTestLog, "qgc.test.qgcunittest.unittest")

UnitTest::UnitTest(QObject *parent)
    : QObject(parent)
{
    // qCDebug(UnitTestLog) << Q_FUNC_INFO << this;
}

UnitTest::~UnitTest()
{
    if (_unitTestRun) {
        // Derived classes must call base class implementations
        Q_ASSERT(_initCalled);
        Q_ASSERT(_cleanupCalled);
    }

    // qCDebug(UnitTestLog) << Q_FUNC_INFO << this;
}

void UnitTest::_addTest(UnitTest *test)
{
    QList<UnitTest*> &tests = _testList();

    Q_ASSERT(!tests.contains(test));

    tests.append(test);
}

QList<UnitTest*> &UnitTest::_testList()
{
    static QList<UnitTest*> tests;
    return tests;
}

int UnitTest::run(QStringView singleTest)
{
    int ret = 0;

    for (UnitTest *test: _testList()) {
        if (singleTest.isEmpty() || singleTest == test->objectName()) {
            if (test->standalone() && singleTest.isEmpty()) {
                continue;
            }
            QStringList args;
            args << "*" << "-maxwarnings" << "0";
            ret += QTest::qExec(test, args);
        }
    }

    return ret;
}

void UnitTest::init()
{
    _initCalled = true;

    MultiVehicleManager::instance()->init();
    LinkManager::instance()->setConnectionsAllowed();

    // Force offline vehicle back to defaults
    AppSettings *const appSettings = SettingsManager::instance()->appSettings();
    appSettings->offlineEditingFirmwareClass()->setRawValue(appSettings->offlineEditingFirmwareClass()->rawDefaultValue());
    appSettings->offlineEditingVehicleClass()->setRawValue(appSettings->offlineEditingVehicleClass()->rawDefaultValue());

    MAVLinkProtocol::deleteTempLogFiles();
}

void UnitTest::cleanup()
{
    _cleanupCalled = true;

    _disconnectMockLink();

    // Don't let any lingering signals or events cross to the next unit test.
    // If you have a failure whose stack trace points to this then
    // your test is leaking signals or events. It could cause use-after-free or
    // segmentation faults from wild pointers.
    QCoreApplication::processEvents();
}

void UnitTest::_connectMockLink(MAV_AUTOPILOT autopilot, MockConfiguration::FailureMode_t failureMode)
{
    Q_ASSERT(!_mockLink);

    QSignalSpy spyVehicle(MultiVehicleManager::instance(), &MultiVehicleManager::activeVehicleChanged);

    switch (autopilot) {
    case MAV_AUTOPILOT_PX4:
        _mockLink = MockLink::startPX4MockLink(false, failureMode);
        break;
    case MAV_AUTOPILOT_ARDUPILOTMEGA:
        _mockLink = MockLink::startAPMArduCopterMockLink(false, failureMode);
        break;
    case MAV_AUTOPILOT_GENERIC:
        _mockLink = MockLink::startGenericMockLink(false, failureMode);
        break;
    case MAV_AUTOPILOT_INVALID:
        _mockLink = MockLink::startNoInitialConnectMockLink(false);
        break;
    default:
        qCWarning(UnitTestLog) << "Type not supported";
        break;
    }

    // Wait for the Vehicle to get created
    QCOMPARE(spyVehicle.wait(10000), true);
    _vehicle = MultiVehicleManager::instance()->activeVehicle();
    QVERIFY(_vehicle);

    if (autopilot != MAV_AUTOPILOT_INVALID) {
        // Wait for initial connect sequence to complete
        QSignalSpy spyPlan(_vehicle, &Vehicle::initialConnectComplete);
        QCOMPARE(spyPlan.wait(30000), true);
    }
}

void UnitTest::_disconnectMockLink()
{
    if (_mockLink) {
        QSignalSpy spyVehicle(MultiVehicleManager::instance(), &MultiVehicleManager::activeVehicleChanged);

        _mockLink->disconnect();
        _mockLink = nullptr;

        // Wait for all the vehicle to go away
        QCOMPARE(spyVehicle.wait(10000), true);
        _vehicle = MultiVehicleManager::instance()->activeVehicle();
        QVERIFY(!_vehicle);
    }
}

void UnitTest::_linkDeleted(const LinkInterface *link)
{
    if (link == _mockLink) {
        _mockLink = nullptr;
    }
}

bool UnitTest::fileCompare(const QString &file1, const QString &file2)
{
    QFile f1(file1);
    QFile f2(file2);

    if (QFileInfo(file1).size() != QFileInfo(file2).size()) {
        qCWarning(UnitTestLog) << "UnitTest::fileCompare file sizes differ size1:size2" << QFileInfo(file1).size() << QFileInfo(file2).size();
        return false;
    }

    if (!f1.open(QIODevice::ReadOnly)) {
        qCWarning(UnitTestLog) << "UnitTest::fileCompare unable to open file1:" << f1.errorString();
        return false;
    }
    if (!f2.open(QIODevice::ReadOnly)) {
        qCWarning(UnitTestLog) << "UnitTest::fileCompare unable to open file1:" << f1.errorString();
        return false;
    }

    qint64 bytesRemaining = QFileInfo(file1).size();
    qint64 offset = 0;
    while (bytesRemaining) {
        uint8_t b1, b2;

        qint64 bytesRead = f1.read(reinterpret_cast<char*>(&b1), 1);
        if (bytesRead != 1) {
            qCWarning(UnitTestLog) << "UnitTest::fileCompare file1 read failed:" << f1.errorString();
            return false;
        }
        bytesRead = f2.read(reinterpret_cast<char*>(&b2), 1);
        if (bytesRead != 1) {
            qCWarning(UnitTestLog) << "UnitTest::fileCompare file2 read failed:" << f2.errorString();
            return false;
        }

        if (b1 != b2) {
            qCWarning(UnitTestLog) << "UnitTest::fileCompare mismatch offset:b1:b2" << offset << b1 << b2;
            return false;
        }

        offset++;
        bytesRemaining--;
    }

    return true;
}

void UnitTest::changeFactValue(Fact *fact, double increment)
{
    if (fact->typeIsBool()) {
        fact->setRawValue(!fact->rawValue().toBool());
    } else {
        if (increment == 0) {
            increment = 1;
        }
        fact->setRawValue(fact->rawValue().toDouble() + increment);
    }
}

void UnitTest::_missionItemsEqual(const MissionItem &actual, const MissionItem &expected)
{
    QCOMPARE(static_cast<int>(actual.command()), static_cast<int>(expected.command()));
    QCOMPARE(static_cast<int>(actual.frame()), static_cast<int>(expected.frame()));
    QCOMPARE(actual.autoContinue(), expected.autoContinue());

    QVERIFY(QGC::fuzzyCompare(actual.param1(), expected.param1()));
    QVERIFY(QGC::fuzzyCompare(actual.param2(), expected.param2()));
    QVERIFY(QGC::fuzzyCompare(actual.param3(), expected.param3()));
    QVERIFY(QGC::fuzzyCompare(actual.param4(), expected.param4()));
    QVERIFY(QGC::fuzzyCompare(actual.param5(), expected.param5()));
    QVERIFY(QGC::fuzzyCompare(actual.param6(), expected.param6()));
    QVERIFY(QGC::fuzzyCompare(actual.param7(), expected.param7()));
}

bool UnitTest::fuzzyCompareLatLon(const QGeoCoordinate &coord1, const QGeoCoordinate &coord2)
{
    return coord1.distanceTo(coord2) < 1.0;
}

QGeoCoordinate UnitTest::changeCoordinateValue(const QGeoCoordinate &coordinate)
{
    return coordinate.atDistanceAndAzimuth(1, 0);
}
