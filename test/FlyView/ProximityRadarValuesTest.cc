#include "ProximityRadarValuesTest.h"

#include <QtQml/QQmlComponent>
#include <QtQml/QQmlEngine>
#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

#include "FactGroup.h"
#include "MultiVehicleManager.h"
#include "UnitTest.h"
#include "Vehicle.h"

void ProximityRadarValuesTest::_testRadarValues()
{
    QVERIFY2(!_mockLink, "MockLink already connected");

    QSignalSpy activeVehicleSpy(MultiVehicleManager::instance(), &MultiVehicleManager::activeVehicleChanged);
    QVERIFY(activeVehicleSpy.isValid());

    _mockLink = MockLink::startPX4MockLink(MockConfiguration::OptionEnableProximity);
    QVERIFY(_mockLink);

    QVERIFY_SIGNAL_WAIT(activeVehicleSpy, TestTimeout::longMs());
    _vehicle = MultiVehicleManager::instance()->activeVehicle();
    QVERIFY(_vehicle);

    QQmlEngine engine;
    engine.addImportPath(QStringLiteral("qrc:/qml"));
    QQmlComponent component(&engine);
    component.setData(R"(
        import QtQuick
        import QGroundControl.FlyView

        ProximityRadarValues {}
    )", QUrl());
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));

    const QScopedPointer<QObject> radarValues(
        component.createWithInitialProperties({{ QStringLiteral("vehicle"), QVariant::fromValue(_vehicle) }}));
    QVERIFY(radarValues);

    QSignalSpy rotationSpy(radarValues.data(), SIGNAL(rotationValueChanged()));
    QVERIFY(rotationSpy.isValid());

    // DISTANCE_SENSOR messages are sent from the 1Hz task loop
    QTRY_VERIFY_WITH_TIMEOUT(radarValues->property("telemetryAvailable").toBool(), TestTimeout::mediumMs());
    QTRY_VERIFY_WITH_TIMEOUT(rotationSpy.count() > 0, TestTimeout::mediumMs());

    // rgRotationValues sector order: None, Yaw45, Yaw90, Yaw135, Yaw180, Yaw225, Yaw270, Yaw315.
    // MockLink populates all but Yaw135 (index 3) and Yaw225 (index 5).
    // Note: the QML values are cooked (unit-converted per app settings), so verify against
    // the fact group's cooked values rather than raw meters.
    const auto sectorPopulated = [&radarValues](int index) {
        return !qIsNaN(radarValues->property("rgRotationValues").toList().value(index).toDouble());
    };
    for (const int populatedIndex : { 0, 1, 2, 4, 6, 7 }) {
        QTRY_VERIFY2_WITH_TIMEOUT(sectorPopulated(populatedIndex),
                                  qPrintable(QStringLiteral("sector %1 never populated").arg(populatedIndex)),
                                  TestTimeout::mediumMs());
    }

    const QVariantList rotationValues = radarValues->property("rgRotationValues").toList();
    QCOMPARE(rotationValues.size(), 8);
    for (const int populatedIndex : { 0, 1, 2, 4, 6, 7 }) {
        const double distance = rotationValues.value(populatedIndex).toDouble();
        QVERIFY2(distance > 0.0,
                 qPrintable(QStringLiteral("sector %1: distance %2 should be positive").arg(populatedIndex).arg(distance)));
    }
    for (const int unpopulatedIndex : { 3, 5 }) {
        QVERIFY2(qIsNaN(rotationValues.value(unpopulatedIndex).toDouble()),
                 qPrintable(QStringLiteral("sector %1: should never receive data").arg(unpopulatedIndex)));
    }

    FactGroup *const distanceSensors = _vehicle->distanceSensorFactGroup();
    QVERIFY(distanceSensors);
    QCOMPARE(radarValues->property("maxDistance").toDouble(),
             distanceSensors->getFact(QStringLiteral("maxDistance"))->cookedValue().toDouble());

    _disconnectMockLink();
}

UT_REGISTER_TEST(ProximityRadarValuesTest, TestLabel::Integration, TestLabel::Vehicle)
