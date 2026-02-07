#include "MultiVehicleTest.h"

#include <QtCore/QCoreApplication>
#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

#include "LinkManager.h"
#include "MultiVehicleManager.h"
#include <QtCore/QLoggingCategory>
#include "QmlObjectListModel.h"
#include "Vehicle.h"

Q_STATIC_LOGGING_CATEGORY(MultiVehicleTestLog, "Test.MultiVehicleTest")

MultiVehicleTest::MultiVehicleTest(QObject* parent) : UnitTest(parent)
{
}

void MultiVehicleTest::cleanup()
{
    disconnectAllVehicles();

    _links.clear();
    _vehicles.clear();

    UnitTest::cleanup();
}

int MultiVehicleTest::vehicleCount() const
{
    return MultiVehicleManager::instance()->vehicles()->count();
}

Vehicle* MultiVehicleTest::vehicleAt(int index) const
{
    QmlObjectListModel* vehicles = MultiVehicleManager::instance()->vehicles();
    if (index >= 0 && index < vehicles->count()) {
        return vehicles->value<Vehicle*>(index);
    }
    return nullptr;
}

QList<Vehicle*> MultiVehicleTest::vehicles() const
{
    QList<Vehicle*> result;
    QmlObjectListModel* vehicleList = MultiVehicleManager::instance()->vehicles();
    for (int i = 0; i < vehicleList->count(); ++i) {
        result.append(vehicleList->value<Vehicle*>(i));
    }
    return result;
}

Vehicle* MultiVehicleTest::activeVehicle() const
{
    return MultiVehicleManager::instance()->activeVehicle();
}

bool MultiVehicleTest::createVehicles(int count, MAV_AUTOPILOT autopilot)
{
    for (int i = 0; i < count; ++i) {
        const int systemId = 128 + i;
        if (!createVehicle(systemId, autopilot)) {
            return false;
        }
    }
    return true;
}

Vehicle* MultiVehicleTest::createVehicle(int systemId, MAV_AUTOPILOT autopilot)
{
    Q_UNUSED(systemId);  // MockConfiguration auto-increments vehicle IDs

    const QString name = QString("Vehicle_%1").arg(_vehicles.count() + 1);

    MockConfiguration* mockConfig = new MockConfiguration(name);
    mockConfig->setDynamic(true);
    mockConfig->setFirmwareType(autopilot);
    mockConfig->setIncrementVehicleId(true);

    SharedLinkConfigurationPtr sharedConfig(mockConfig);

    const int previousCount = vehicleCount();
    QSignalSpy spy(MultiVehicleManager::instance(), &MultiVehicleManager::vehicleAdded);

    if (!LinkManager::instance()->createConnectedLink(sharedConfig)) {
        qCWarning(MultiVehicleTestLog) << "createVehicle: Failed to create link";
        return nullptr;
    }

    SharedLinkInterfacePtr link = LinkManager::instance()->sharedLinkInterfacePointerForLink(mockConfig->link());
    if (link) {
        _links.append(link);
    }

    // Wait for vehicle to be added
    if (vehicleCount() <= previousCount) {
        if (!spy.wait(TestTimeout::longMs())) {
            qCWarning(MultiVehicleTestLog) << "createVehicle: Timeout waiting for vehicle";
            return nullptr;
        }
    }

    // Get the most recently added vehicle
    Vehicle* newVehicle = vehicleAt(vehicleCount() - 1);
    if (newVehicle) {
        _vehicles.append(newVehicle);
        return newVehicle;
    }

    qCWarning(MultiVehicleTestLog) << "createVehicle: Vehicle not found after creation";
    return nullptr;
}

void MultiVehicleTest::setActiveVehicle(Vehicle* vehicle)
{
    MultiVehicleManager::instance()->setActiveVehicle(vehicle);
}

void MultiVehicleTest::disconnectAllVehicles()
{
    if (LinkManager::instance()->links().count() > 0) {
        LinkManager::instance()->disconnectAll();

        // Wait for all vehicles to disconnect
        QSignalSpy spy(MultiVehicleManager::instance(), &MultiVehicleManager::activeVehicleChanged);
        while (vehicleCount() > 0) {
            if (!spy.wait(TestTimeout::mediumMs())) {
                break;
            }
        }

        // Process cleanup events including deferred deletions
        for (int i = 0; i < 5; ++i) {
            QCoreApplication::processEvents(QEventLoop::AllEvents);
            QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
            if (i < 4) {
                QTest::qWait(20);
            }
        }
    }
}
