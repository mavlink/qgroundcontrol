#include "VehicleDistanceSensorFactGroupTest.h"

#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

#include "FactGroup.h"
#include "MultiVehicleManager.h"
#include "UnitTest.h"
#include "Vehicle.h"

namespace {

// MockLink::_sendDistanceSensors sweeps current distance between 2000cm - 1500cm and
// 2000cm + 1500cm, with fixed min/max distances of 20cm and 4000cm.
constexpr double kSweepMinMeters = 5.0;
constexpr double kSweepMaxMeters = 35.0;
constexpr double kSensorMinDistanceMeters = 0.2;
constexpr double kSensorMaxDistanceMeters = 40.0;

// Orientations MockLink sends
constexpr const char *kPopulatedFacts[] = {
    "rotationNone", "rotationYaw45", "rotationYaw90", "rotationYaw180", "rotationYaw270", "rotationYaw315",
};

// Orientations MockLink deliberately never sends
constexpr const char *kUnpopulatedFacts[] = {
    "rotationYaw135", "rotationYaw225",
};

} // namespace

void VehicleDistanceSensorFactGroupTest::_startVehicle(MockConfiguration::Options options)
{
    QVERIFY2(!_mockLink, "MockLink already connected");

    QSignalSpy activeVehicleSpy(MultiVehicleManager::instance(), &MultiVehicleManager::activeVehicleChanged);
    QVERIFY(activeVehicleSpy.isValid());

    _mockLink = MockLink::startPX4MockLink(options);
    QVERIFY(_mockLink);

    QVERIFY_SIGNAL_WAIT(activeVehicleSpy, TestTimeout::longMs());
    _vehicle = MultiVehicleManager::instance()->activeVehicle();
    QVERIFY(_vehicle);
}

void VehicleDistanceSensorFactGroupTest::_testProximityTelemetry()
{
    _startVehicle(MockConfiguration::OptionEnableProximity);

    FactGroup *const distanceSensors = _vehicle->distanceSensorFactGroup();
    QVERIFY(distanceSensors);

    // DISTANCE_SENSOR messages are sent from the 1Hz task loop
    QTRY_VERIFY_WITH_TIMEOUT(distanceSensors->telemetryAvailable(), TestTimeout::mediumMs());

    for (const char *factName : kPopulatedFacts) {
        Fact *const fact = distanceSensors->getFact(QString::fromLatin1(factName));
        QVERIFY2(fact, factName);
        QTRY_VERIFY2_WITH_TIMEOUT(!qIsNaN(fact->rawValue().toDouble()), factName, TestTimeout::mediumMs());

        const double distance = fact->rawValue().toDouble();
        QVERIFY2((distance >= kSweepMinMeters) && (distance <= kSweepMaxMeters),
                 qPrintable(QStringLiteral("%1: distance %2 outside expected sweep range").arg(QLatin1String(factName)).arg(distance)));
    }

    for (const char *factName : kUnpopulatedFacts) {
        Fact *const fact = distanceSensors->getFact(QString::fromLatin1(factName));
        QVERIFY2(fact, factName);
        QVERIFY2(qIsNaN(fact->rawValue().toDouble()),
                 qPrintable(QStringLiteral("%1: should never receive data").arg(QLatin1String(factName))));
    }

    Fact *const minDistanceFact = distanceSensors->getFact(QStringLiteral("minDistance"));
    QVERIFY(minDistanceFact);
    QCOMPARE(minDistanceFact->rawValue().toDouble(), kSensorMinDistanceMeters);

    Fact *const maxDistanceFact = distanceSensors->getFact(QStringLiteral("maxDistance"));
    QVERIFY(maxDistanceFact);
    QCOMPARE(maxDistanceFact->rawValue().toDouble(), kSensorMaxDistanceMeters);

    _disconnectMockLink();
}

void VehicleDistanceSensorFactGroupTest::_testProximityValuesUpdate()
{
    _startVehicle(MockConfiguration::OptionEnableProximity);

    FactGroup *const distanceSensors = _vehicle->distanceSensorFactGroup();
    QVERIFY(distanceSensors);

    Fact *const rotationNone = distanceSensors->getFact(QStringLiteral("rotationNone"));
    QVERIFY(rotationNone);
    QTRY_VERIFY_WITH_TIMEOUT(!qIsNaN(rotationNone->rawValue().toDouble()), TestTimeout::mediumMs());

    // MockLink sweeps the simulated distances, so subsequent 1Hz updates must change the value
    const double initialDistance = rotationNone->rawValue().toDouble();
    QTRY_VERIFY_WITH_TIMEOUT(rotationNone->rawValue().toDouble() != initialDistance, TestTimeout::mediumMs());

    _disconnectMockLink();
}

void VehicleDistanceSensorFactGroupTest::_testNoProximityWithoutOption()
{
    _startVehicle(MockConfiguration::OptionNone);

    FactGroup *const distanceSensors = _vehicle->distanceSensorFactGroup();
    QVERIFY(distanceSensors);

    // VIBRATION is sent unconditionally from the same 1Hz task loop as DISTANCE_SENSOR.
    // Waiting for two of them proves the loop has run without proximity messages.
    int vibrationCount = 0;
    const QMetaObject::Connection connection =
        connect(_vehicle, &Vehicle::mavlinkMessageReceived, this, [&vibrationCount](const mavlink_message_t &message) {
            if (message.msgid == MAVLINK_MSG_ID_VIBRATION) {
                vibrationCount++;
            }
        });
    QVERIFY(connection);

    QTRY_VERIFY_WITH_TIMEOUT(vibrationCount >= 2, TestTimeout::mediumMs());
    disconnect(connection);

    QVERIFY2(!distanceSensors->telemetryAvailable(), "Distance sensor telemetry should not arrive without OptionEnableProximity");

    _disconnectMockLink();
}

UT_REGISTER_TEST(VehicleDistanceSensorFactGroupTest, TestLabel::Integration, TestLabel::Vehicle)
