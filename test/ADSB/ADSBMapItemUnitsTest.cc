#include "ADSBMapItemUnitsTest.h"

#include <memory>

#include <QtCore/QScopeGuard>
#include <QtPositioning/QGeoCoordinate>
#include <QtQml/QQmlComponent>
#include <QtQml/QQmlEngine>

#include "SettingsManager.h"
#include "UnitsSettings.h"

namespace {

constexpr const char* kFlightMapItemQml = R"(
import QtQuick
import QGroundControl.FlightMap

ADSBVehicleMapItem {
    map: QtObject { property bool isSatelliteMap: false }
}
)";

constexpr const char* kGeoMapItemQml = R"(
import QtQuick
import QGroundControl.GeoMap

GeoMapADSBVehicleItem {}
)";

QString labelText(QObject* item)
{
    const QObject* const label = item->findChild<QObject*>(QStringLiteral("adsbAltitudeLabel"));
    return label ? label->property("text").toString() : QString();
}

}  // namespace

void ADSBMapItemUnitsTest::_altitudeLabelFollowsUnitsChange_data()
{
    QTest::addColumn<QByteArray>("qml");
    QTest::addColumn<QVariantMap>("extraProperties");

    const QVariantMap geoMapProperties = {
        {QStringLiteral("scene"), QVariant::fromValue<QObject*>(nullptr)},
        {QStringLiteral("surfaceModel"), QVariant::fromValue<QObject*>(nullptr)},
    };
    QTest::newRow("FlightMap") << QByteArray(kFlightMapItemQml) << QVariantMap();
    QTest::newRow("GeoMap") << QByteArray(kGeoMapItemQml) << geoMapProperties;
}

void ADSBMapItemUnitsTest::_altitudeLabelFollowsUnitsChange()
{
    QFETCH(QByteArray, qml);
    QFETCH(QVariantMap, extraProperties);

    Fact* const verticalUnitsFact = SettingsManager::instance()->unitsSettings()->verticalDistanceUnits();
    const QVariant savedUnits = verticalUnitsFact->rawValue();
    const auto restoreUnits =
        qScopeGuard([verticalUnitsFact, savedUnits] { verticalUnitsFact->setRawValue(savedUnits); });
    verticalUnitsFact->setRawValue(UnitsSettings::VerticalDistanceUnitsMeters);

    QQmlEngine engine;
    engine.addImportPath(QStringLiteral("qrc:/qml"));
    QQmlComponent component(&engine);
    component.setData(qml, QUrl());
    QVariantMap properties = {
        {QStringLiteral("altitude"), 100.0},
        {QStringLiteral("callsign"), QStringLiteral("TEST")},
        {QStringLiteral("coordinate"), QVariant::fromValue(QGeoCoordinate(47.0, 8.0))},
    };
    properties.insert(extraProperties);
    const std::unique_ptr<QObject> item(component.createWithInitialProperties(properties));
    QVERIFY2(item, qPrintable(component.errorString()));

    QCOMPARE(labelText(item.get()), QStringLiteral("100 m\nTEST"));

    verticalUnitsFact->setRawValue(UnitsSettings::VerticalDistanceUnitsFeet);

    QCOMPARE(labelText(item.get()), QStringLiteral("328 ft\nTEST"));
}

UT_REGISTER_TEST(ADSBMapItemUnitsTest, TestLabel::Unit)
