#include "CommsTest.h"

#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

#include "LinkManager.h"
#include "MultiVehicleManager.h"
#include <QtCore/QLoggingCategory>
#include "Vehicle.h"

Q_STATIC_LOGGING_CATEGORY(CommsTestLog, "Test.CommsTest")

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
}

void CommsTest::cleanup()
{
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

    if (!spy.wait(TestTimeout::longMs())) {
        qCWarning(CommsTestLog) << "createMockLinkAndWaitForVehicle: Timeout waiting for vehicle";
        return nullptr;
    }

    return MultiVehicleManager::instance()->activeVehicle();
}

void CommsTest::disconnectAllLinks()
{
    if (LinkManager::instance()->links().count() > 0) {
        QSignalSpy spy(MultiVehicleManager::instance(), &MultiVehicleManager::activeVehicleChanged);
        LinkManager::instance()->disconnectAll();

        if (MultiVehicleManager::instance()->vehicles()->count() > 0) {
            spy.wait(TestTimeout::mediumMs());
        }
    }
}

Vehicle* CommsTest::waitForVehicleConnect(int timeoutMs)
{
    if (timeoutMs <= 0) {
        timeoutMs = TestTimeout::longMs();
    }

    if (MultiVehicleManager::instance()->activeVehicle()) {
        return MultiVehicleManager::instance()->activeVehicle();
    }

    QSignalSpy spy(MultiVehicleManager::instance(), &MultiVehicleManager::activeVehicleChanged);
    if (spy.wait(timeoutMs)) {
        return MultiVehicleManager::instance()->activeVehicle();
    }

    return nullptr;
}

bool CommsTest::waitForAllVehiclesDisconnect(int timeoutMs)
{
    if (timeoutMs <= 0) {
        timeoutMs = TestTimeout::longMs();
    }

    if (MultiVehicleManager::instance()->vehicles()->count() == 0) {
        return true;
    }

    QSignalSpy spy(MultiVehicleManager::instance(), &MultiVehicleManager::activeVehicleChanged);

    while (MultiVehicleManager::instance()->vehicles()->count() > 0) {
        if (!spy.wait(timeoutMs)) {
            return false;
        }
    }

    return true;
}
