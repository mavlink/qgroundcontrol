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

UT_REGISTER_TEST(ScaleBarUnitsTest, TestLabel::Unit)
