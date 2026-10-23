#include "QmlObjectListModel.h"
#include "CommsTest.h"

#include <QtCore/QElapsedTimer>
#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

#include "LinkManager.h"
#include "MultiVehicleManager.h"
#include "QGCLoggingCategory.h"
#include "Vehicle.h"

QGC_LOGGING_CATEGORY(CommsTestLog, "Test.CommsTest")

CommsTest::CommsTest(QObject* parent) : UnitTest(parent)
{
}

void CommsTest::init()
{
    UnitTest::init();

    // Initialize managers for link creation
    MultiVehicleManager::instance()->init();
    LinkManager::instance()->setConnectionsAllowed();

    // Ensure clean state
    QCOMPARE(LinkManager::instance()->links().count(), 0);
    QCOMPARE(MultiVehicleManager::instance()->vehicles()->count(), 0);
    QCOMPARE(MultiVehicleManager::instance()->activeVehicle(), nullptr);
}

void CommsTest::cleanup()
{
    dumpFailureContextIfTestFailed(QStringLiteral("before CommsTest teardown"));
    disconnectAllLinks();

    _createdLinks.clear();

    UnitTest::cleanup();
}

LinkManager* CommsTest::linkManager() const
{
    return LinkManager::instance();
}

SharedLinkInterfacePtr CommsTest::createMockLink(const QString& name, bool highLatency)
{
    MockConfiguration* mockConfig = new MockConfiguration(name);
    mockConfig->setDynamic(true);
    mockConfig->setHighLatency(highLatency);

    SharedLinkConfigurationPtr sharedConfig(mockConfig);

    if (!LinkManager::instance()->createConnectedLink(sharedConfig)) {
        qCWarning(CommsTestLog) << "createMockLink: Failed to create link";
        return nullptr;
    }

    SharedLinkInterfacePtr link = LinkManager::instance()->sharedLinkInterfacePointerForLink(mockConfig->link());
    if (link) {
        _createdLinks.append(link);
    }

    return link;
}

Vehicle* CommsTest::createMockLinkAndWaitForVehicle(const QString& name, MAV_AUTOPILOT autopilot)
{
    MockConfiguration* mockConfig = new MockConfiguration(name);
    mockConfig->setDynamic(true);
    mockConfig->setFirmwareType(autopilot);

    SharedLinkConfigurationPtr sharedConfig(mockConfig);

    QSignalSpy spy(MultiVehicleManager::instance(), &MultiVehicleManager::activeVehicleChanged);

    if (!LinkManager::instance()->createConnectedLink(sharedConfig)) {
        qCWarning(CommsTestLog) << "createMockLinkAndWaitForVehicle: Failed to create link";
        return nullptr;
    }

    SharedLinkInterfacePtr link = LinkManager::instance()->sharedLinkInterfacePointerForLink(mockConfig->link());
    if (link) {
        _createdLinks.append(link);
    }

    if (!UnitTest::waitForSignal(spy, TestTimeout::longMs(), QStringLiteral("activeVehicleChanged"))) {
        qCWarning(CommsTestLog) << "createMockLinkAndWaitForVehicle: Timeout waiting for vehicle";
        return nullptr;
    }

    return MultiVehicleManager::instance()->activeVehicle();
}

void CommsTest::disconnectAllLinks()
{
    if (LinkManager::instance()->links().count() > 0) {
        LinkManager::instance()->disconnectAll();

        // QSignalSpy::wait returns immediately once the test has failed, so poll
        MultiVehicleManager* const vehicleMgr = MultiVehicleManager::instance();
        if (!QTest::qWaitFor(
                [vehicleMgr] { return (vehicleMgr->vehicles()->count() == 0) && !vehicleMgr->activeVehicle(); },
                TestTimeout::mediumMs())) {
            qCWarning(CommsTestLog) << "Timeout waiting for vehicles to be removed";
        }
    }
}

Vehicle* CommsTest::waitForVehicleConnect(int timeoutMs)
{
    if (timeoutMs <= 0) {
        timeoutMs = TestTimeout::longMs();
    }

    if ((MultiVehicleManager::instance()->vehicles()->count() > 0) && MultiVehicleManager::instance()->activeVehicle()) {
        return MultiVehicleManager::instance()->activeVehicle();
    }

    QSignalSpy spy(MultiVehicleManager::instance(), &MultiVehicleManager::activeVehicleChanged);
    QElapsedTimer waitTimer;
    waitTimer.start();

    while (waitTimer.elapsed() < timeoutMs) {
        const int remaining = timeoutMs - static_cast<int>(waitTimer.elapsed());
        if (remaining <= 0) {
            break;
        }

        if (!UnitTest::waitForSignal(spy, remaining, QStringLiteral("activeVehicleChanged"))) {
            break;
        }

        if ((MultiVehicleManager::instance()->vehicles()->count() > 0) &&
            (MultiVehicleManager::instance()->activeVehicle() != nullptr)) {
            return MultiVehicleManager::instance()->activeVehicle();
        }
    }

    return nullptr;
}

bool CommsTest::waitForAllVehiclesDisconnect(int timeoutMs)
{
    if (timeoutMs <= 0) {
        timeoutMs = TestTimeout::longMs();
    }

    if ((MultiVehicleManager::instance()->vehicles()->count() == 0) &&
        (MultiVehicleManager::instance()->activeVehicle() == nullptr)) {
        return true;
    }

    QSignalSpy spy(MultiVehicleManager::instance(), &MultiVehicleManager::activeVehicleChanged);
    QElapsedTimer waitTimer;
    waitTimer.start();

    while (waitTimer.elapsed() < timeoutMs) {
        if ((MultiVehicleManager::instance()->vehicles()->count() == 0) &&
            (MultiVehicleManager::instance()->activeVehicle() == nullptr)) {
            return true;
        }

        const int remaining = timeoutMs - static_cast<int>(waitTimer.elapsed());
        if (remaining <= 0) {
            break;
        }

        if (!UnitTest::waitForSignal(spy, remaining, QStringLiteral("activeVehicleChanged"))) {
            break;
        }
    }

    return (MultiVehicleManager::instance()->vehicles()->count() == 0) &&
           (MultiVehicleManager::instance()->activeVehicle() == nullptr);
}
