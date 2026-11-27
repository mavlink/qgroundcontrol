#include "ScaleBarUnitsTest.h"

#include <memory>

#include <QtCore/QScopeGuard>
#include <QtQml/QQmlComponent>
#include <QtQml/QQmlEngine>

#include "SettingsManager.h"
#include "UnitsSettings.h"
#include "Viewer3DCameraController.h"

namespace {

constexpr const char* kMapScaleQml = R"(
import QtQuick
import QtPositioning
import QGroundControl.FlightMap

MapScale {
    mapControl: Item {
        property real zoomLevel: 15
        property bool isSatelliteMap: false
        function toCoordinate(point, clipToViewPort) { return QtPositioning.coordinate(0, point.x * 0.0001) }
    }
}
)";

/// A map as a QtLocation Map is before it is laid out: it has no coordinates until it is ready
constexpr const char* kMapScaleOfMapNotReadyQml = R"(
import QtQuick
import QtPositioning
import QGroundControl.FlightMap

MapScale {
    mapControl: Item {
        property real zoomLevel: 15
        property bool isSatelliteMap: false
        property bool mapReady: false
        function toCoordinate(point, clipToViewPort) {
            return mapReady ? QtPositioning.coordinate(0, point.x * 0.0001) : QtPositioning.coordinate()
        }
    }
}
)";

constexpr const char* kViewer3DScaleBarQml = R"(
import QtQuick
import QGroundControl.Viewer3D

Viewer3DScaleBar {}
)";

}  // namespace

void ScaleBarUnitsTest::_scaleTextFollowsUnitsChange_data()
{
    QTest::addColumn<QByteArray>("qml");
    QTest::addColumn<bool>("viewer3D");

    QTest::newRow("MapScale") << QByteArray(kMapScaleQml) << false;
    QTest::newRow("Viewer3DScaleBar") << QByteArray(kViewer3DScaleBarQml) << true;
}

void ScaleBarUnitsTest::_scaleTextFollowsUnitsChange()
{
    QFETCH(QByteArray, qml);
    QFETCH(bool, viewer3D);

    Fact* const horizontalUnitsFact = SettingsManager::instance()->unitsSettings()->horizontalDistanceUnits();
    const QVariant savedUnits = horizontalUnitsFact->rawValue();
    const auto restoreUnits =
        qScopeGuard([horizontalUnitsFact, savedUnits] { horizontalUnitsFact->setRawValue(savedUnits); });
    horizontalUnitsFact->setRawValue(UnitsSettings::HorizontalDistanceUnitsMeters);

    Viewer3DCameraController controller;
    controller.setViewportSize(QSizeF(800.0, 600.0));
    controller.lookAt(QVector3D(0, 0, 0), 0.0, 60.0, 1000.0);

    QQmlEngine engine;
    engine.addImportPath(QStringLiteral("qrc:/qml"));
    QQmlComponent component(&engine);
    component.setData(qml, QUrl());
    QVariantMap properties;
    if (viewer3D) {
        properties.insert(QStringLiteral("controller"), QVariant::fromValue(&controller));
    }
    const std::unique_ptr<QObject> item(component.createWithInitialProperties(properties));
    QVERIFY2(item, qPrintable(component.errorString()));

    const QObject* const scaleText = item->findChild<QObject*>(QStringLiteral("scaleText"));
    QVERIFY(scaleText);
    const QString metersText = scaleText->property("text").toString();
    QVERIFY2(metersText != QStringLiteral("0 m") && metersText.endsWith(QStringLiteral(" m")), qPrintable(metersText));

    horizontalUnitsFact->setRawValue(UnitsSettings::HorizontalDistanceUnitsFeet);

    const QString feetText = scaleText->property("text").toString();
    QVERIFY2(feetText.endsWith(QStringLiteral(" ft")), qPrintable(feetText));
}

// The map of a tab that is shown for the first time gets its size, which the scale follows, before it has coordinates
void ScaleBarUnitsTest::_scaleCalculatedOnceMapIsReady()
{
    Fact* const horizontalUnitsFact = SettingsManager::instance()->unitsSettings()->horizontalDistanceUnits();
    const QVariant savedUnits = horizontalUnitsFact->rawValue();
    const auto restoreUnits =
        qScopeGuard([horizontalUnitsFact, savedUnits] { horizontalUnitsFact->setRawValue(savedUnits); });
    horizontalUnitsFact->setRawValue(UnitsSettings::HorizontalDistanceUnitsMeters);

    QQmlEngine engine;
    engine.addImportPath(QStringLiteral("qrc:/qml"));
    QQmlComponent component(&engine);
    component.setData(QByteArray(kMapScaleOfMapNotReadyQml), QUrl());
    const std::unique_ptr<QObject> item(component.create());
    QVERIFY2(item, qPrintable(component.errorString()));

    // No distance between coordinates that the map does not have
    const QObject* const scaleText = item->findChild<QObject*>(QStringLiteral("scaleText"));
    QVERIFY(scaleText);
    QCOMPARE(scaleText->property("text").toString(), QStringLiteral("0 m"));

    QObject* const mapControl = item->property("mapControl").value<QObject*>();
    QVERIFY(mapControl);
    QVERIFY(mapControl->setProperty("mapReady", true));
    const QString text = scaleText->property("text").toString();
    QVERIFY2(text != QStringLiteral("0 m") && text.endsWith(QStringLiteral(" m")), qPrintable(text));
    QCOMPARE_GT(item->property("width").toReal(), 50.0);
}

UT_REGISTER_TEST(ScaleBarUnitsTest, TestLabel::Unit)
