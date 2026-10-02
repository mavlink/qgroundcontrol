#include "FlyViewOccluderUITest.h"

#include <functional>
#include <optional>

#include <QtCore/QPointer>
#include <QtCore/QRegularExpression>
#include <QtCore/QScopeGuard>
#include <QtPositioning/QGeoCoordinate>
#include <QtQuick/QQuickItem>
#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

#include "Fact.h"
#include "FlyViewSettings.h"
#include "GeoMapCamera.h"
#include "GeoScene.h"
#include "MapPositionTracker.h"
#include "MockLink.h"
#include "SettingsManager.h"
#include "Vehicle.h"
#include "VideoSettings.h"

UT_REGISTER_TEST(FlyViewOccluderUITest, TestLabel::Integration)

namespace {
// Generous ceilings: first appearance of loader/engine-gated items vs. QTRY settle polling
constexpr int kItemAppearTimeoutMs = 10000;
constexpr int kSettleTimeoutMs = 5000;
}  // namespace

void FlyViewOccluderUITest::_testVehicleUnderWidgetRecenters_data()
{
    QTest::addColumn<bool>("geoMapEngine");

    QTest::newRow("GeoMap") << true;
    QTest::newRow("QtLocation") << false;
}

// Map following treats the Fly View widgets as occluders: a vehicle hidden under
// the bottom right instrument row recenters the map, a vehicle in the clear does not
void FlyViewOccluderUITest::_testVehicleUnderWidgetRecenters()
{
    QFETCH(bool, geoMapEngine);

    Fact* const geoEngineFact = SettingsManager::instance()->flyViewSettings()->useGeoMapEngine();
    const QVariant savedEnabled = geoEngineFact->rawValue();
    const auto guard = qScopeGuard([geoEngineFact, savedEnabled] { geoEngineFact->setRawValue(savedEnabled); });
    ignoreLogMessage("API.QGCApplication.AppMessage", QtDebugMsg,
                     QRegularExpression(QStringLiteral("Restart application for changes to take effect")));
    geoEngineFact->setRawValue(geoMapEngine);

    runWithMockLink(
        [] { return MockLink::startPX4MockLink(); },
        [this, geoMapEngine](QPointer<MockLink>, Vehicle* vehicle) {
            QQuickItem* mapItem = nullptr;
            MapPositionTracker* positionTracker = nullptr;
            // Moves the map so the vehicle lands on screenPoint, in mapItem coordinates
            std::function<bool(const QPointF&)> placeVehicleAt;
            if (geoMapEngine) {
                const std::optional<bool> rhiBased = expectSoftwareBackendWarnings(/*strict*/ false);
                QVERIFY2(rhiBased.has_value(), "No renderer interface on the main window");

                mapItem = findVisibleItem(_rootItem, QStringLiteral("geoMapViewport"), kItemAppearTimeoutMs);
                QVERIFY2(mapItem, "GeoMap 3D viewport not visible");
                QQuickItem* const geoMap = mapItem->parentItem();
                auto* const cam = geoMap->findChild<GeoMapCamera*>(QStringLiteral("geoMapCamera"));
                QVERIFY2(cam, "GeoMapCamera not found");
                auto* const scene = geoMap->findChild<GeoScene*>(QStringLiteral("geoMapScene"));
                QVERIFY2(scene, "GeoScene not found");
                positionTracker = geoMap->findChild<MapPositionTracker*>();
                placeVehicleAt = [geoMap, cam, scene](const QPointF& screenPoint) {
                    const auto vehicleCoordinate =
                        geoMap->property("_trackedVehicleCoordinate").value<QGeoCoordinate>();
                    cam->setCenter(scene->centerForCoordinateAtScreenPoint(vehicleCoordinate, screenPoint));
                    return true;
                };
            } else {
                mapItem = findVisibleItem(_rootItem, QStringLiteral("flyViewMap"), kItemAppearTimeoutMs);
                QVERIFY2(mapItem, "QtLocation fly view map not visible");
                positionTracker =
                    qobject_cast<MapPositionTracker*>(mapItem->property("positionTracker").value<QObject*>());
                placeVehicleAt = [mapItem, vehicle](const QPointF& screenPoint) {
                    QPointF vehiclePoint;
                    QGeoCoordinate center;
                    const QPointF mapCenter(mapItem->width() / 2, mapItem->height() / 2);
                    return QMetaObject::invokeMethod(mapItem, "fromCoordinate", Q_RETURN_ARG(QPointF, vehiclePoint),
                                                     Q_ARG(QGeoCoordinate, vehicle->coordinate()),
                                                     Q_ARG(bool, false)) &&
                           QMetaObject::invokeMethod(mapItem, "toCoordinate", Q_RETURN_ARG(QGeoCoordinate, center),
                                                     Q_ARG(QPointF, mapCenter + vehiclePoint - screenPoint),
                                                     Q_ARG(bool, false)) &&
                           mapItem->setProperty("center", QVariant::fromValue(center));
                };
            }
            QVERIFY2(positionTracker, "MapPositionTracker not found");

            QQuickItem* const bottomRightRow =
                findVisibleItem(_rootItem, QStringLiteral("flyViewBottomRightRowLayout"), kItemAppearTimeoutMs);
            QVERIFY2(bottomRightRow, "Bottom right instrument row not visible");
            QTRY_VERIFY_WITH_TIMEOUT(bottomRightRow->width() > 0 && bottomRightRow->height() > 0, kSettleTimeoutMs);

            // The one-shot centering on the first vehicle position would also move the map
            QTRY_VERIFY_WITH_TIMEOUT(positionTracker->firstVehiclePositionReceived(), kItemAppearTimeoutMs);

            QSignalSpy recenterSpy(positionTracker, &MapPositionTracker::recenterVehicleTo);
            QVERIFY(placeVehicleAt(QPointF(mapItem->width() / 2, mapItem->height() / 2)));
            QVERIFY_NO_SIGNAL_WAIT(recenterSpy, TestTimeout::shortMs());

            QVERIFY(placeVehicleAt(mapItem->mapFromItem(
                bottomRightRow, QPointF(bottomRightRow->width() / 2, bottomRightRow->height() / 2))));
            QTRY_VERIFY_WITH_TIMEOUT(recenterSpy.count() > 0, kSettleTimeoutMs);
        });
}

// A collapsed PiP only covers the map with its restore button, so only that is an occluder
void FlyViewOccluderUITest::_testCollapsedPipOccludesRestoreButtonOnly()
{
    // A configured video stream is what makes the Fly View offer PiP
    Fact* const videoSource = SettingsManager::instance()->videoSettings()->videoSource();
    const QVariant savedVideoSource = videoSource->rawValue();
    const auto videoGuard =
        qScopeGuard([videoSource, savedVideoSource] { videoSource->setRawValue(savedVideoSource); });
    videoSource->setRawValue(QString::fromLatin1(VideoSettings::videoSourceUDPH264));

    startUI();
    if (QTest::currentTestFailed())
        return;

    const QPointer<QQuickItem> pipView =
        findVisibleItem(_rootItem, QStringLiteral("flyViewPipView"), kItemAppearTimeoutMs);
    QVERIFY2(pipView, "PiP view not visible");
    QQuickItem* const widgetLayer = findVisibleItem(_rootItem, QStringLiteral("flyViewWidgetLayer"));
    QVERIFY2(widgetLayer, "Fly View widget layer not visible");
    QObject* const occluders = widgetLayer->property("occluders").value<QObject*>();
    QVERIFY2(occluders, "Widget layer occluders not found");

    // The collapsed state is persisted: restore it even if an assertion bails out early
    const auto expandGuard = qScopeGuard([pipView] {
        if (pipView) {
            QMetaObject::invokeMethod(pipView, "_setPipIsExpanded", Q_ARG(QVariant, true));
        }
    });
    const auto pipOccluderSize = [occluders] { return occluders->property("pipView").toRectF().size(); };
    const auto pipOccupiedSize = [pipView] { return pipView->property("occupiedRect").toRectF().size(); };

    QCOMPARE(pipOccupiedSize(), pipView->size());
    QCOMPARE(pipOccluderSize(), pipView->size());

    // The hide button only shows on hover
    QVERIFY(QMetaObject::invokeMethod(pipView, "_setPipIsExpanded", Q_ARG(QVariant, false)));
    QCOMPARE_LT(pipOccupiedSize().height(), pipView->height() / 2);
    QCOMPARE(pipOccluderSize(), pipOccupiedSize());

    QVERIFY(QMetaObject::invokeMethod(pipView, "_setPipIsExpanded", Q_ARG(QVariant, true)));
    QCOMPARE(pipOccluderSize(), pipView->size());

    stopUI();
}
