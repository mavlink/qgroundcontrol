#include "FlyViewGeoUITest.h"

#include <cmath>
#include <optional>

#include <QtCore/QList>
#include <QtCore/QPointer>
#include <QtCore/QRectF>
#include <QtCore/QRegularExpression>
#include <QtCore/QScopeGuard>
#include <QtCore/QSettings>
#include <QtCore/QtMath>
#include <QtCore/QtNumeric>
#include <QtGui/QGuiApplication>
#include <QtGui/QMouseEvent>
#include <QtGui/QPointingDevice>
#include <QtGui/QQuaternion>
#include <QtGui/QVector3D>
#include <QtPositioning/QGeoCoordinate>
#include <QtQuick/QQuickItem>
#include <QtQuick/QQuickWindow>
#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

#include "Fact.h"
#include "FlyViewSettings.h"
#include "GeoMapCamera.h"
#include "MapPositionTracker.h"
#include "MissionController.h"
#include "MissionManager.h"
#include "MockLink.h"
#include "PlanMasterController.h"
#include "QGCMapCircle.h"
#include "QGroundControlQmlGlobal.h"
#include "QmlObjectListModel.h"
#include "SettingsManager.h"
#include "SurfacePatchModel.h"
#include "TileMath.h"
#include "Vehicle.h"
#include "VideoSettings.h"
#include "VisualMissionItem.h"

UT_REGISTER_TEST(FlyViewGeoUITest, TestLabel::Integration)

namespace {
// Generous ceilings: first appearance of loader/engine-gated items vs. QTRY settle polling
constexpr int kItemAppearTimeoutMs = 10000;
constexpr int kSettleTimeoutMs = 5000;
const QGeoCoordinate kMapCenter(47.6329078, -122.0876875);
// Integer window coordinates round each end of a drag by up to half a pixel
constexpr qreal kPixelTolerance = 1.5;

// Screen position of a circle's radius handle, which is drawn due east of the center
std::optional<QPointF> radiusHandleScreenPos(const GeoMapCamera* cam, const QGeoCoordinate& center, double radius,
                                             double handleZ)
{
    return cam->worldToScreen(TileMath::geoToWorld(center.atDistanceAndAzimuth(radius, 90)), handleZ);
}

// Worst radius error from drag rounding: the ground spread of the handle position
// shifted by kPixelTolerance on both screen axes (a tilted camera skews the pixels)
std::optional<double> radiusToleranceMeters(const GeoMapCamera* cam, const QGeoCoordinate& center, double radius,
                                            double handleZ)
{
    const auto pos = radiusHandleScreenPos(cam, center, radius, handleZ);
    if (!pos) {
        return std::nullopt;
    }
    double tolerance = 0.0;
    for (const QPointF& corner : {QPointF(-1, -1), QPointF(-1, 1), QPointF(1, -1), QPointF(1, 1)}) {
        const QGeoCoordinate coord = cam->coordinateAtScreenPoint(*pos + (corner * kPixelTolerance), handleZ);
        if (!coord.isValid()) {
            return std::nullopt;
        }
        tolerance = qMax(tolerance, qAbs(center.distanceTo(coord) - radius));
    }
    return tolerance;
}
}  // namespace

void FlyViewGeoUITest::_testEngineEnabledAtStartup()
{
    Fact* const geoEngineFact = SettingsManager::instance()->flyViewSettings()->useGeoMapEngine();
    const QVariant savedEnabled = geoEngineFact->rawValue();
    const auto guard = qScopeGuard([geoEngineFact, savedEnabled] { geoEngineFact->setRawValue(savedEnabled); });
    Fact* const debugUIFact = SettingsManager::instance()->flyViewSettings()->geoMapDebugUI();
    const QVariant savedDebugUI = debugUIFact->rawValue();
    const auto debugUIGuard = qScopeGuard([debugUIFact, savedDebugUI] { debugUIFact->setRawValue(savedDebugUI); });
    ignoreLogMessage("API.QGCApplication.AppMessage", QtDebugMsg,
                     QRegularExpression(QStringLiteral("Restart application for changes to take effect")));
    geoEngineFact->setRawValue(true);
    debugUIFact->setRawValue(false);

    // Seed a non-default saved map view (QGroundControlQmlGlobal settings). The
    // adapter writes these same values, so expectations are constants, not read-backs.
    const QGeoCoordinate savedCenter(-33.8568, 151.2153);
    constexpr double kSavedZoom = 12.0;
    const QString settingsGroup = QStringLiteral("FlightMapPosition");
    const QStringList settingsKeys = {QStringLiteral("Latitude"), QStringLiteral("Longitude"),
                                      QStringLiteral("FlightMapZoom")};
    QVariantList previousValues;
    const QGeoCoordinate previousPosition = QGroundControlQmlGlobal::flightMapPosition();
    const double previousZoom = QGroundControlQmlGlobal::flightMapZoom();
    {
        QSettings settings;
        settings.beginGroup(settingsGroup);
        for (const QString& key : settingsKeys) {
            previousValues.append(settings.value(key));
        }
        settings.setValue(settingsKeys[0], savedCenter.latitude());
        settings.setValue(settingsKeys[1], savedCenter.longitude());
        settings.setValue(settingsKeys[2], kSavedZoom);
    }
    // The loaded view is also cached in memory, where later tests would inherit it
    const auto savedViewGuard =
        qScopeGuard([&settingsGroup, &settingsKeys, &previousValues, previousPosition, previousZoom] {
            QGroundControlQmlGlobal::setFlightMapViewForTest(previousPosition, previousZoom);
            QSettings settings;
            settings.beginGroup(settingsGroup);
            for (qsizetype i = 0; i < settingsKeys.size(); i++) {
                if (previousValues[i].isValid()) {
                    settings.setValue(settingsKeys[i], previousValues[i]);
                } else {
                    settings.remove(settingsKeys[i]);
                }
            }
        });

    startUI();
    if (QTest::currentTestFailed())
        return;

    // Non-strict: on the software backend the warnings only fire when a frame
    // renders the View3D (timing-dependent here); on RHI backends nothing is
    // ignored, so a fallback regression would fail the test.
    const std::optional<bool> rhiBased = expectSoftwareBackendWarnings(/*strict*/ false);
    QVERIFY2(rhiBased.has_value(), "No renderer interface on the main window");

    // The engine setting was snapshotted at startup: the GeoMap adapter is the
    // Fly View map and the QtLocation map is not instantiated
    QVERIFY2(findVisibleItem(_rootItem, QStringLiteral("flyViewGeoMapAdapter"), kItemAppearTimeoutMs),
             "GeoMap adapter not visible");
    QVERIFY2(!findVisibleItem(_rootItem, QStringLiteral("flyViewMap"), 0),
             "QtLocation map instantiated although the GeoMap engine is enabled");

    // The 3D viewport instantiates and the patch repeater populates from the
    // SurfaceModel regardless of render backend
    QQuickItem* const viewport = findVisibleItem(_rootItem, QStringLiteral("geoMapViewport"), kItemAppearTimeoutMs);
    QVERIFY2(viewport, "GeoMap 3D viewport not visible");
    QTRY_VERIFY_WITH_TIMEOUT(!viewport->findChildren<QObject*>(QStringLiteral("geoMapPatchDelegate")).isEmpty(),
                             kSettleTimeoutMs);

    // Debug chrome stays hidden until the setting is enabled (not reboot-required)
    QVERIFY2(!findVisibleItem(_rootItem, QStringLiteral("flyViewGeoMapDebugOverlay"), 0),
             "Debug overlay visible although the debug UI setting is off");
    debugUIFact->setRawValue(true);
    QVERIFY2(findVisibleItem(_rootItem, QStringLiteral("flyViewGeoMapDebugOverlay")), "Debug overlay not visible");

    // Tile imagery flows end-to-end: the model is wired to the flight map
    // provider setting and every patch receives an image (the test tile
    // generator serves placeholders on cache miss, so no network is involved)
    auto* const patchModel = viewport->parentItem()->findChild<SurfacePatchModel*>(QStringLiteral("geoMapPatchModel"));
    QVERIFY2(patchModel, "SurfacePatchModel not found");
    QVERIFY2(!patchModel->mapType().isEmpty(), "Patch model not wired to a map provider");
    const auto allPatchesImaged = [patchModel] {
        if (patchModel->rowCount() == 0) {
            return false;
        }
        for (int row = 0; row < patchModel->rowCount(); row++) {
            if (!patchModel->data(patchModel->index(row), SurfacePatchModel::HasTileImageRole).toBool()) {
                return false;
            }
        }
        return true;
    };
    QTRY_VERIFY_WITH_TIMEOUT(allPatchesImaged(), kSettleTimeoutMs);

    auto* const cam = viewport->parentItem()->findChild<GeoMapCamera*>(QStringLiteral("geoMapCamera"));
    QVERIFY2(cam, "GeoMapCamera not found");

    // Startup pose restored from the seeded saved view
    QCOMPARE_LT(cam->center().distanceTo(savedCenter), 1.0);
    const qreal savedZoomDistance = cam->distanceForZoomLevel(kSavedZoom);
    QCOMPARE_LT(qAbs(cam->distance() - savedZoomDistance), savedZoomDistance * 1e-6);

    // Scene camera node tracks the GeoMapCamera startup pose (overhead at the
    // saved map zoom distance, identity rotation, origin anchored at the camera center)
    QObject* const sceneCamera = viewport->findChild<QObject*>(QStringLiteral("geoMapSceneCamera"));
    QVERIFY2(sceneCamera, "Scene camera node not found");
    const QVector3D camPos = sceneCamera->property("position").value<QVector3D>();
    QCOMPARE_LT(qAbs(camPos.x()), 1.0f);
    QCOMPARE_LT(qAbs(camPos.y()), 1.0f);
    // Relative slack: float scene coordinates at a whole-earth distance
    QCOMPARE_LT(qAbs(camPos.z() - cam->distance()), 1.0 + (cam->distance() * 1e-6));
    const QQuaternion camRot = sceneCamera->property("rotation").value<QQuaternion>();
    QVERIFY(qFuzzyCompare(camRot, QQuaternion()));

    stopUI();
}

void FlyViewGeoUITest::_testFlyViewEngineSwap()
{
    Fact* const geoEngineFact = SettingsManager::instance()->flyViewSettings()->useGeoMapEngine();
    const QVariant savedEnabled = geoEngineFact->rawValue();
    const auto guard = qScopeGuard([geoEngineFact, savedEnabled] { geoEngineFact->setRawValue(savedEnabled); });
    geoEngineFact->setRawValue(false);

    startUI();
    if (QTest::currentTestFailed())
        return;

    // Default engine: QtLocation map, GeoMap adapter not instantiated
    QVERIFY2(findVisibleItem(_rootItem, QStringLiteral("flyViewMap")), "QtLocation fly view map not visible");
    QVERIFY2(!findVisibleItem(_rootItem, QStringLiteral("flyViewGeoMapAdapter"), 0),
             "GeoMap adapter visible although the engine setting is disabled");

    // Non-strict: see _testViewSwitchWhenEnabled
    const std::optional<bool> rhiBased = expectSoftwareBackendWarnings(/*strict*/ false);
    QVERIFY2(rhiBased.has_value(), "No renderer interface on the main window");

    // The setting is qgcRebootRequired: toggling it at runtime must announce
    // the restart and must NOT swap the engine until the app restarts
    expectAppMessage(QRegularExpression(QStringLiteral("Restart application for changes to take effect")));
    geoEngineFact->setRawValue(true);
    verifyExpectedLogMessage();
    QVERIFY2(findVisibleItem(_rootItem, QStringLiteral("flyViewMap")),
             "QtLocation map disappeared after toggling the reboot-required engine setting");
    QVERIFY2(!findVisibleItem(_rootItem, QStringLiteral("flyViewGeoMapAdapter"), 0),
             "GeoMap adapter instantiated by a runtime toggle of the reboot-required engine setting");

    stopUI();
}

// Exercises every camera gesture against the real view: left-drag pan,
// right-drag orbit, Shift+left-drag orbit (with pivot ring), Ctrl+left-drag
// first-person look, wheel zoom, and synthesized multi-touch (pinch zoom,
// two-finger twist).
void FlyViewGeoUITest::_testCameraGestures()
{
    Fact* const geoEngineFact = SettingsManager::instance()->flyViewSettings()->useGeoMapEngine();
    const QVariant savedEnabled = geoEngineFact->rawValue();
    const auto guard = qScopeGuard([geoEngineFact, savedEnabled] { geoEngineFact->setRawValue(savedEnabled); });
    ignoreLogMessage("API.QGCApplication.AppMessage", QtDebugMsg,
                     QRegularExpression(QStringLiteral("Restart application for changes to take effect")));
    geoEngineFact->setRawValue(true);

    startUI();
    if (QTest::currentTestFailed())
        return;

    const std::optional<bool> rhiBased = expectSoftwareBackendWarnings(/*strict*/ false);
    QVERIFY2(rhiBased.has_value(), "No renderer interface on the main window");

    QQuickItem* const viewport = findVisibleItem(_rootItem, QStringLiteral("geoMapViewport"), kItemAppearTimeoutMs);
    QVERIFY2(viewport, "GeoMap 3D viewport not visible");

    // Non-visual QObject sibling of the viewport: search from the scene root
    // item (the window-level QObject tree does not reach into the Loader item)
    auto* const cam = viewport->parentItem()->findChild<GeoMapCamera*>(QStringLiteral("geoMapCamera"));
    QVERIFY2(cam, "GeoMapCamera not found");

    const qreal w = viewport->width();
    const qreal h = viewport->height();
    QVERIFY2((w > 300) && (h > 300), "GeoMap viewport too small for gesture synthesis");

    // Gesture positions are viewport-local; QTest wants window coordinates
    const auto toWin = [viewport](qreal x, qreal y) { return viewport->mapToScene(QPointF(x, y)).toPoint(); };
    const QPoint center = toWin(w / 2, h / 2);

    // Unlock tilt gestures once up front: the mode change starts the animated
    // 2D->3D transition, so wait for it to settle before posing the camera
    cam->setMode(GeoMapCamera::Mode::Mode3D);
    QTRY_COMPARE_WITH_TIMEOUT(cam->tilt(), GeoMapCamera::kDefault3DTilt, kSettleTimeoutMs);

    // Known pose before every gesture: mercator origin, heading 0, tilt 30,
    // distance 1500 (tilted so orbit/tilt deltas are observable both ways)
    const auto resetPose = [cam] { cam->lookAt(QGeoCoordinate(0, 0), 0, 30, 1500); };

    const auto mouseDrag = [this](Qt::MouseButton button, const QPoint& from, const QPoint& delta) {
        QTest::mousePress(_window, button, Qt::NoModifier, from);
        constexpr int steps = 10;
        for (int i = 1; i <= steps; i++) {
            QTest::mouseMove(_window, from + ((delta * i) / steps));
        }
        QTest::mouseRelease(_window, button, Qt::NoModifier, from + delta);
    };

    // Left drag: pan only — center moves, orientation and distance unchanged
    resetPose();
    const QGeoCoordinate centerBefore = cam->center();
    mouseDrag(Qt::LeftButton, center, QPoint(60, 40));
    QTRY_VERIFY_WITH_TIMEOUT(cam->center().distanceTo(centerBefore) > 1.0, kSettleTimeoutMs);
    QCOMPARE(cam->heading(), 0.0);
    QCOMPARE(cam->tilt(), 30.0);
    QCOMPARE(cam->distance(), 1500.0);

    // Right drag right by a quarter of the width: orbit 90 deg heading
    resetPose();
    mouseDrag(Qt::RightButton, center, QPoint(qRound(w / 4), 0));
    QTRY_VERIFY_WITH_TIMEOUT(qAbs(cam->heading() - 90.0) < 0.5, kSettleTimeoutMs);
    QVERIFY(qAbs(cam->tilt() - 30.0) < 0.5);
    QCOMPARE(cam->distance(), 1500.0);

    // Right drag up a quarter of the height: tilt +45
    resetPose();
    mouseDrag(Qt::RightButton, center, QPoint(0, qRound(-h / 4)));
    QTRY_VERIFY_WITH_TIMEOUT(qAbs(cam->tilt() - 75.0) < 0.5, kSettleTimeoutMs);

    // Shift+left drag: same orbit — pivot ring visible at the press point
    // while dragging, pressed ground point pinned to its screen position,
    // ring gone on release
    resetPose();
    {
        const QPointF pressLocal(w / 2, h * 0.6);
        const QPoint pressPos = toWin(pressLocal.x(), pressLocal.y());
        const auto anchorBefore = cam->screenToGround(pressLocal);
        QVERIFY(anchorBefore.has_value());

        QTest::mousePress(_window, Qt::LeftButton, Qt::ShiftModifier, pressPos);
        const QPoint delta(qRound(w / 4), qRound(-h / 8));
        for (int i = 1; i <= 10; i++) {
            // QTest::mouseMove drops keyboard modifiers, which would deactivate
            // the modifier-gated handler: send the move with Shift held
            const QPoint pos = pressPos + ((delta * i) / 10);
            QMouseEvent move(QEvent::MouseMove, pos, _window->mapToGlobal(pos), Qt::NoButton, Qt::LeftButton,
                             Qt::ShiftModifier);
            QGuiApplication::sendEvent(_window, &move);
        }

        QQuickItem* const pivotRing = findVisibleItem(_rootItem, QStringLiteral("geoMapOrbitPivotIndicator"));
        QVERIFY2(pivotRing, "Pivot ring not visible during Shift+left drag");
        const QPointF ringCenter = pivotRing->mapToScene(QPointF(pivotRing->width() / 2, pivotRing->height() / 2));
        QCOMPARE_LT((ringCenter - QPointF(pressPos)).manhattanLength(), 3.0);

        QTest::mouseRelease(_window, Qt::LeftButton, Qt::ShiftModifier, pressPos + delta);
        QTRY_VERIFY_WITH_TIMEOUT(qAbs(cam->heading() - 90.0) < 0.5, kSettleTimeoutMs);
        QVERIFY(qAbs(cam->tilt() - 52.5) < 0.5);
        QCOMPARE(cam->distance(), 1500.0);

        // The clicked ground point never left its press screen position
        const auto anchorAfter = cam->screenToGround(pressLocal);
        QVERIFY(anchorAfter.has_value());
        QCOMPARE_LT((*anchorAfter - *anchorBefore).manhattanLength(), 2.0);

        QVERIFY2(!findVisibleItem(_rootItem, QStringLiteral("geoMapOrbitPivotIndicator"), 0),
                 "Pivot ring still visible after release");
    }

    // Ctrl+left drag: first-person look — the camera stays fixed while
    // heading/tilt follow the drag (center and distance re-solve). QTest
    // synthesizes Qt modifiers directly (no macOS native Ctrl-click
    // right-button swap), so this exercises lookHandler on every platform.
    resetPose();
    {
        const QPointF camGroundBefore = cam->cameraGroundPosition();
        const float camZBefore = cam->cameraPosition().z();

        QTest::mousePress(_window, Qt::LeftButton, Qt::ControlModifier, center);
        const QPoint delta(qRound(w / 4), qRound(h / 8));
        for (int i = 1; i <= 10; i++) {
            // QTest::mouseMove drops keyboard modifiers (see Shift+left drag above)
            const QPoint pos = center + ((delta * i) / 10);
            QMouseEvent move(QEvent::MouseMove, pos, _window->mapToGlobal(pos), Qt::NoButton, Qt::LeftButton,
                             Qt::ControlModifier);
            QGuiApplication::sendEvent(_window, &move);
        }

        // Look has no ground pivot: the orbit ring must not appear
        QVERIFY2(!findVisibleItem(_rootItem, QStringLiteral("geoMapOrbitPivotIndicator"), 0),
                 "Pivot ring visible during Ctrl+left look drag");

        QTest::mouseRelease(_window, Qt::LeftButton, Qt::ControlModifier, center + delta);

        // Quarter width = 90 deg heading; drag down an eighth = look down 22.5 deg
        QTRY_VERIFY_WITH_TIMEOUT(qAbs(cam->heading() - 90.0) < 0.5, kSettleTimeoutMs);
        QVERIFY(qAbs(cam->tilt() - 7.5) < 0.5);

        // Camera position unchanged; distance re-solved along the new view axis
        QCOMPARE_LT((cam->cameraGroundPosition() - camGroundBefore).manhattanLength(), 1.0);
        QCOMPARE_LT(qAbs(cam->cameraPosition().z() - camZBefore), 1.0f);
        const qreal expectedDistance = (1500.0 * std::cos(qDegreesToRadians(30.0))) / std::cos(qDegreesToRadians(7.5));
        QVERIFY(qAbs(cam->distance() - expectedDistance) < 1.0);
    }

    // Wheel up: zoom in; wheel down: zoom out
    resetPose();
    QTest::wheelEvent(_window, center, QPoint(0, 120));
    QTRY_VERIFY_WITH_TIMEOUT(cam->distance() < 1500.0, kSettleTimeoutMs);
    resetPose();
    QTest::wheelEvent(_window, center, QPoint(0, -120));
    QTRY_VERIFY_WITH_TIMEOUT(cam->distance() > 1500.0, kSettleTimeoutMs);

    QPointingDevice* const touchDevice = QTest::createTouchDevice();
    // A real window merges touch moves until the next frame's sync; render two
    // frames per move so a sync is sure to follow the commit
    const auto commitTouchMove = [this](QTest::QTouchEventSequence& touch) {
        touch.commit();
        for (int frame = 0; frame < 2; frame++) {
            QSignalSpy frameSpy(_window, &QQuickWindow::frameSwapped);
            _window->update();
            QVERIFY_SIGNAL_WAIT(frameSpy, TestTimeout::shortMs());
        }
    };

    // Pinch spread: zoom in
    resetPose();
    {
        QTest::QTouchEventSequence touch = QTest::touchEvent(_window, touchDevice);
        touch.press(0, center + QPoint(-50, 0)).press(1, center + QPoint(50, 0)).commit();
        for (int i = 1; i <= 10; i++) {
            commitTouchMove(
                touch.move(0, center + QPoint(-50 - (i * 10), 0)).move(1, center + QPoint(50 + (i * 10), 0)));
            if (QTest::currentTestFailed()) {
                return;
            }
        }
        touch.release(0, center + QPoint(-150, 0)).release(1, center + QPoint(150, 0)).commit();
    }
    QTRY_VERIFY_WITH_TIMEOUT(cam->distance() < 1500.0, kSettleTimeoutMs);

    // Two-finger twist 90 deg visually counterclockwise (y-down screen): the
    // world follows the fingers, so heading decreases (mod 360). The
    // PinchHandler consumes the rotation applied before its activation
    // threshold, so assert a band rather than the exact angle.
    resetPose();
    {
        constexpr int r = 100;
        QTest::QTouchEventSequence touch = QTest::touchEvent(_window, touchDevice);
        touch.press(0, center + QPoint(-r, 0)).press(1, center + QPoint(r, 0)).commit();
        for (int i = 1; i <= 18; i++) {
            const qreal a = -(i * 5) * M_PI / 180.0;  // decreasing angle = visual CCW with y down
            const QPoint d(qRound(r * qCos(a)), qRound(r * qSin(a)));
            commitTouchMove(touch.move(0, center - d).move(1, center + d));
            if (QTest::currentTestFailed()) {
                return;
            }
        }
        touch.release(0, center + QPoint(0, r)).release(1, center + QPoint(0, -r)).commit();
    }
    QTRY_VERIFY_WITH_TIMEOUT(cam->heading() < 315.0, kSettleTimeoutMs);
    QVERIFY2(cam->heading() > 265.0,
             qPrintable(QStringLiteral("twist overshot the finger rotation: %1").arg(cam->heading())));

    stopUI();
}

// 2D/3D mode switch: the mode flips immediately and locks tilt gestures in 2D;
// tilt and terrain displacement animate to the mode's target; the compass
// control animates heading back to north-up
void FlyViewGeoUITest::_testModeToggleAndCompass()
{
    Fact* const geoEngineFact = SettingsManager::instance()->flyViewSettings()->useGeoMapEngine();
    const QVariant savedEnabled = geoEngineFact->rawValue();
    const auto guard = qScopeGuard([geoEngineFact, savedEnabled] { geoEngineFact->setRawValue(savedEnabled); });
    ignoreLogMessage("API.QGCApplication.AppMessage", QtDebugMsg,
                     QRegularExpression(QStringLiteral("Restart application for changes to take effect")));
    geoEngineFact->setRawValue(true);

    startUI();
    if (QTest::currentTestFailed())
        return;

    const std::optional<bool> rhiBased = expectSoftwareBackendWarnings(/*strict*/ false);
    QVERIFY2(rhiBased.has_value(), "No renderer interface on the main window");

    QQuickItem* const viewport = findVisibleItem(_rootItem, QStringLiteral("geoMapViewport"), kItemAppearTimeoutMs);
    QVERIFY2(viewport, "GeoMap 3D viewport not visible");

    auto* const cam = viewport->parentItem()->findChild<GeoMapCamera*>(QStringLiteral("geoMapCamera"));
    QVERIFY2(cam, "GeoMapCamera not found");
    QQuickItem* const sceneRoot = viewport->parentItem();

    // Startup: 2D mode, flat terrain, toggle offers 3D
    QCOMPARE(cam->mode(), GeoMapCamera::Mode::Mode2D);
    QVERIFY(cam->isTopDown());
    QCOMPARE(sceneRoot->property("terrainScale").toDouble(), 0.0);
    QQuickItem* const modeButton = findVisibleItem(_rootItem, QStringLiteral("flyViewGeoMapModeButton"));
    QVERIFY2(modeButton, "Mode toggle button not visible");
    QCOMPARE(modeButton->property("text").toString(), QStringLiteral("3D"));

    // Toggle to 3D: mode flips immediately, tilt and terrain animate up
    QVERIFY(clickButton(QStringLiteral("flyViewGeoMapModeButton")));
    QCOMPARE(cam->mode(), GeoMapCamera::Mode::Mode3D);
    QCOMPARE(modeButton->property("text").toString(), QStringLiteral("2D"));
    QTRY_COMPARE_WITH_TIMEOUT(cam->tilt(), GeoMapCamera::kDefault3DTilt, kSettleTimeoutMs);
    QTRY_COMPARE_WITH_TIMEOUT(sceneRoot->property("terrainScale").toDouble(), 1.0, kSettleTimeoutMs);
    QVERIFY(!cam->isTopDown());

    // Toggle back to 2D: tilt and terrain animate down
    QVERIFY(clickButton(QStringLiteral("flyViewGeoMapModeButton")));
    QCOMPARE(cam->mode(), GeoMapCamera::Mode::Mode2D);
    QTRY_COMPARE_WITH_TIMEOUT(cam->tilt(), 0.0, kSettleTimeoutMs);
    QTRY_COMPARE_WITH_TIMEOUT(sceneRoot->property("terrainScale").toDouble(), 0.0, kSettleTimeoutMs);
    QVERIFY(cam->isTopDown());

    // Tilt lock: a vertical right-drag in 2D must not pitch the map
    const QPoint center = viewport->mapToScene(QPointF(viewport->width() / 2, viewport->height() / 2)).toPoint();
    QTest::mousePress(_window, Qt::RightButton, Qt::NoModifier, center);
    for (int i = 1; i <= 5; i++) {
        QTest::mouseMove(_window, center + QPoint(0, -i * 20));
    }
    QTest::mouseRelease(_window, Qt::RightButton, Qt::NoModifier, center + QPoint(0, -100));
    QCOMPARE(cam->tilt(), 0.0);

    // Compass reset from a heading past 180: wraps the short way back to north
    cam->setHeading(350);
    QVERIFY(clickButton(QStringLiteral("flyViewGeoMapCompassButton")));
    QTRY_COMPARE_WITH_TIMEOUT(cam->heading(), 0.0, kSettleTimeoutMs);

    stopUI();
}

// The debugHills dev override (settable from QML/C++ during development, no UI
// affordance) swaps the terrain height source for analytic sin-hills: toggling
// on delivers non-flat heights, toggling off flattens again. The test scene
// sits outside the synthetic terrain regions, so the terrain source itself
// delivers all-zero heights here (see TerrariumTileFetcherTest for real
// elevations).
void FlyViewGeoUITest::_testDebugHillsToggle()
{
    Fact* const geoEngineFact = SettingsManager::instance()->flyViewSettings()->useGeoMapEngine();
    const QVariant savedEnabled = geoEngineFact->rawValue();
    const auto guard = qScopeGuard([geoEngineFact, savedEnabled] { geoEngineFact->setRawValue(savedEnabled); });
    ignoreLogMessage("API.QGCApplication.AppMessage", QtDebugMsg,
                     QRegularExpression(QStringLiteral("Restart application for changes to take effect")));
    geoEngineFact->setRawValue(true);

    startUI();
    if (QTest::currentTestFailed())
        return;

    const std::optional<bool> rhiBased = expectSoftwareBackendWarnings(/*strict*/ false);
    QVERIFY2(rhiBased.has_value(), "No renderer interface on the main window");

    QQuickItem* const viewport = findVisibleItem(_rootItem, QStringLiteral("geoMapViewport"), kItemAppearTimeoutMs);
    QVERIFY2(viewport, "GeoMap 3D viewport not visible");

    auto* const patchModel = viewport->parentItem()->findChild<SurfacePatchModel*>(QStringLiteral("geoMapPatchModel"));
    QVERIFY2(patchModel, "SurfacePatchModel not found");

    const auto anyNonZeroHeight = [patchModel] {
        for (int row = 0; row < patchModel->rowCount(); row++) {
            const auto heights =
                patchModel->data(patchModel->index(row), SurfacePatchModel::HeightsRole).value<QList<float>>();
            for (float h : heights) {
                if (h > 0.0f) {
                    return true;
                }
            }
        }
        return false;
    };

    // Terrain is the default: flat zero heights outside the synthetic regions
    QVERIFY(!patchModel->debugHills());
    QVERIFY(patchModel->terrain());
    QTRY_COMPARE_WITH_TIMEOUT(patchModel->pendingCount(), 0, kSettleTimeoutMs);
    QVERIFY(!anyNonZeroHeight());

    // Toggle hills on: non-flat heights
    patchModel->setDebugHills(true);
    QVERIFY(patchModel->debugHills());
    QTRY_VERIFY_WITH_TIMEOUT(anyNonZeroHeight(), kSettleTimeoutMs);

    // Toggle back off: terrain source flattens again
    patchModel->setDebugHills(false);
    QVERIFY(!patchModel->debugHills());
    QTRY_COMPARE_WITH_TIMEOUT(patchModel->pendingCount(), 0, kSettleTimeoutMs);
    QVERIFY(!anyNonZeroHeight());

    stopUI();
}

void FlyViewGeoUITest::_testCircleEditHandles_data()
{
    QTest::addColumn<bool>("mode3D");
    QTest::addColumn<qreal>("heading");
    QTest::addColumn<qreal>("distance");
    QTest::addColumn<double>("altitude");  // NaN: ground-clamped circle

    QTest::addRow("2D ground-clamped") << false << 0.0 << 600.0 << qQNaN();
    QTest::addRow("3D ground-clamped") << true << 30.0 << 600.0 << qQNaN();
    QTest::addRow("3D absolute altitude") << true << 30.0 << 600.0 << 100.0;
}

// Orbit circle editing: a dragged handle stays under the cursor on the surface
// the circle is drawn on (terrain, or the horizontal plane at its altitude), at
// any camera tilt. The rotation arrows flip direction, and none of it pans the
// map or reaches the map-click popup. Handles hide when not interactive.
void FlyViewGeoUITest::_testCircleEditHandles()
{
    QFETCH(bool, mode3D);
    QFETCH(qreal, heading);
    QFETCH(qreal, distance);
    QFETCH(double, altitude);

    Fact* const geoEngineFact = SettingsManager::instance()->flyViewSettings()->useGeoMapEngine();
    const QVariant savedEnabled = geoEngineFact->rawValue();
    const auto guard = qScopeGuard([geoEngineFact, savedEnabled] { geoEngineFact->setRawValue(savedEnabled); });
    ignoreLogMessage("API.QGCApplication.AppMessage", QtDebugMsg,
                     QRegularExpression(QStringLiteral("Restart application for changes to take effect")));
    geoEngineFact->setRawValue(true);

    startUI();
    if (QTest::currentTestFailed())
        return;

    const std::optional<bool> rhiBased = expectSoftwareBackendWarnings(/*strict*/ false);
    QVERIFY2(rhiBased.has_value(), "No renderer interface on the main window");

    QQuickItem* const viewport = findVisibleItem(_rootItem, QStringLiteral("geoMapViewport"), kItemAppearTimeoutMs);
    QVERIFY2(viewport, "GeoMap 3D viewport not visible");
    QQuickItem* const geoMap = viewport->parentItem();
    auto* const cam = geoMap->findChild<GeoMapCamera*>(QStringLiteral("geoMapCamera"));
    QVERIFY2(cam, "GeoMapCamera not found");

    auto* const orbitVisuals = geoMap->findChild<QQuickItem*>(QStringLiteral("flyViewGeoOrbitCircle"));
    QVERIFY2(orbitVisuals, "Orbit circle visuals not found");
    auto* const orbitCircle = qobject_cast<QGCMapCircle*>(orbitVisuals->property("mapCircle").value<QObject*>());
    QVERIFY2(orbitCircle, "Orbit QGCMapCircle not found");

    // Let the animated 2D->3D transition settle before posing the camera
    const qreal tilt = mode3D ? GeoMapCamera::kDefault3DTilt : 0.0;
    if (mode3D) {
        cam->setMode(GeoMapCamera::Mode::Mode3D);
        QTRY_COMPARE_WITH_TIMEOUT(cam->tilt(), GeoMapCamera::kDefault3DTilt, kSettleTimeoutMs);
        QTRY_COMPARE_WITH_TIMEOUT(geoMap->property("terrainScale").toDouble(), 1.0, kSettleTimeoutMs);
    }

    // Zoomed in so the 30 m default radius separates the center and radius handles
    cam->lookAt(kMapCenter, heading, tilt, distance);
    QVERIFY(QMetaObject::invokeMethod(orbitVisuals, "show", Q_ARG(QVariant, QVariant::fromValue(kMapCenter))));
    if (!std::isnan(altitude)) {
        orbitCircle->setCenter(QGeoCoordinate(kMapCenter.latitude(), kMapCenter.longitude(), altitude));
    }
    QVERIFY(orbitVisuals->isVisible());

    // Center the view on the circle as drawn (a floating circle renders high on
    // screen) so its handles stay clear of the Fly View toolbar at any window size
    QQuickItem* const centerHandle = findVisibleItem(_rootItem, QStringLiteral("geoMapCircleCenterHandle"));
    QVERIFY2(centerHandle, "Center drag handle not visible");
    const QPointF viewCenter(viewport->width() / 2, viewport->height() / 2);
    const double circleZ = centerHandle->property("scenePosition").value<QVector3D>().z();
    const QGeoCoordinate cameraCenter =
        cam->centerForCoordinateAtScreenPoint(orbitCircle->center(), viewCenter, circleZ);
    cam->setCenter(cameraCenter);

    QSignalSpy mapClickedSpy(geoMap, SIGNAL(mapClicked(QVariant)));
    QVERIFY(mapClickedSpy.isValid());

    const auto handleCenter = [viewport](QQuickItem* item) {
        return viewport->mapFromItem(item, QPointF(item->width() / 2, item->height() / 2));
    };
    const auto toWin = [viewport](const QPointF& viewportPos) { return viewport->mapToScene(viewportPos).toPoint(); };
    const auto mouseDrag = [this, &toWin](const QPointF& from, const QPointF& to) {
        QTest::mousePress(_window, Qt::LeftButton, Qt::NoModifier, toWin(from));
        constexpr int steps = 10;
        for (int i = 1; i <= steps; i++) {
            QTest::mouseMove(_window, toWin(from + (((to - from) * i) / steps)));
        }
        QTest::mouseRelease(_window, Qt::LeftButton, Qt::NoModifier, toWin(to));
    };

    // Radius handle: drag it to where a 50 m ring's handle would be drawn
    QQuickItem* const radiusHandle = findVisibleItem(_rootItem, QStringLiteral("geoMapCircleRadiusHandle"));
    QVERIFY2(radiusHandle, "Radius drag handle not visible");
    constexpr double kTargetRadius = 50.0;
    const double handleZ = radiusHandle->property("scenePosition").value<QVector3D>().z();
    const auto radiusTarget = radiusHandleScreenPos(cam, orbitCircle->center(), kTargetRadius, handleZ);
    const auto radiusTolerance = radiusToleranceMeters(cam, orbitCircle->center(), kTargetRadius, handleZ);
    QVERIFY(radiusTarget.has_value());
    QVERIFY(radiusTolerance.has_value());
    mouseDrag(handleCenter(radiusHandle), *radiusTarget);
    QCOMPARE_LT(qAbs(orbitCircle->radius()->rawValue().toDouble() - kTargetRadius), *radiusTolerance);
    QCOMPARE_LT((handleCenter(radiusHandle) - *radiusTarget).manhattanLength(), kPixelTolerance);

    // Center handle: follows the cursor, keeping the circle's altitude
    const QPointF centerTarget = handleCenter(centerHandle) + QPointF(-40, 30);
    mouseDrag(handleCenter(centerHandle), centerTarget);
    QCOMPARE_LT((handleCenter(centerHandle) - centerTarget).manhattanLength(), kPixelTolerance);
    if (std::isnan(altitude)) {
        QVERIFY(std::isnan(orbitCircle->center().altitude()));
    } else {
        QCOMPARE(orbitCircle->center().altitude(), altitude);
    }

    // A floating center is anchored to the terrain by a drop line and ground
    // shadow, like the non-interactive orbit center marker
    const bool expectGroundDrop = mode3D && !std::isnan(altitude);
    auto* const dropShadow = centerHandle->findChild<QQuickItem*>(QStringLiteral("geoMapDragHandleDropShadow"));
    QVERIFY2(dropShadow, "Center handle drop shadow not found");
    QCOMPARE(dropShadow->isVisible(), expectGroundDrop);
    QCOMPARE(dropShadow->property("dropLength").toDouble() > 0.0, expectGroundDrop);
    QVERIFY2(centerHandle->property("node3D").value<QObject*>(), "Center handle has no drop line node");
    QVERIFY2(!radiusHandle->property("node3D").value<QObject*>(), "Radius handle has a drop line node");

    // Rotation arrow click flips direction
    QVERIFY(orbitCircle->clockwiseRotation());
    QQuickItem* const arrow = findVisibleItem(_rootItem, QStringLiteral("geoMapCircleRotationIndicator"));
    QVERIFY2(arrow, "Rotation indicator not visible");
    QTest::mouseClick(_window, Qt::LeftButton, Qt::NoModifier, toWin(handleCenter(arrow)));
    QTRY_VERIFY_WITH_TIMEOUT(!orbitCircle->clockwiseRotation(), kSettleTimeoutMs);

    QCOMPARE(mapClickedSpy.count(), 0);
    QCOMPARE_LT(cam->center().distanceTo(cameraCenter), 0.01);
    QCOMPARE(cam->heading(), heading);

    // Not interactive: no handles, and the arrows no longer flip direction
    orbitCircle->setInteractive(false);
    QVERIFY(!findVisibleItem(_rootItem, QStringLiteral("geoMapCircleRadiusHandle"), 0));
    QVERIFY(!findVisibleItem(_rootItem, QStringLiteral("geoMapCircleCenterHandle"), 0));
    QTest::mouseClick(_window, Qt::LeftButton, Qt::NoModifier, toWin(handleCenter(arrow)));
    QVERIFY(!orbitCircle->clockwiseRotation());

    stopUI();
}

// Change Loiter Radius on a forward-flight (ArduPilot fixed-wing) goto: the
// loiter circle offers only the radius handle while editing, and the radius the
// guided action sends follows the drag, reverting on cancel and sticking on confirm.
void FlyViewGeoUITest::_testLoiterRadiusEdit()
{
    if (!apmFirmwareSupported()) {
        QSKIP("ArduPilot support not registered in this build");
    }

    Fact* const geoEngineFact = SettingsManager::instance()->flyViewSettings()->useGeoMapEngine();
    const QVariant savedEnabled = geoEngineFact->rawValue();
    const auto guard = qScopeGuard([geoEngineFact, savedEnabled] { geoEngineFact->setRawValue(savedEnabled); });
    ignoreLogMessage("API.QGCApplication.AppMessage", QtDebugMsg,
                     QRegularExpression(QStringLiteral("Restart application for changes to take effect")));
    geoEngineFact->setRawValue(true);

    runWithMockLink(
        [] { return MockLink::startAPMArduPlaneMockLink(); },
        [this](QPointer<MockLink>, Vehicle* vehicle) {
            const std::optional<bool> rhiBased = expectSoftwareBackendWarnings(/*strict*/ false);
            QVERIFY2(rhiBased.has_value(), "No renderer interface on the main window");
            QVERIFY(vehicle->fixedWing());

            QQuickItem* const viewport =
                findVisibleItem(_rootItem, QStringLiteral("geoMapViewport"), kItemAppearTimeoutMs);
            QVERIFY2(viewport, "GeoMap 3D viewport not visible");
            QQuickItem* const geoMap = viewport->parentItem();
            auto* const cam = geoMap->findChild<GeoMapCamera*>(QStringLiteral("geoMapCamera"));
            QVERIFY2(cam, "GeoMapCamera not found");
            auto* const gotoItem = geoMap->findChild<QQuickItem*>(QStringLiteral("flyViewGeoGotoLocation"));
            QVERIFY2(gotoItem, "Goto location marker not found");
            auto* const loiterVisuals = geoMap->findChild<QQuickItem*>(QStringLiteral("flyViewGeoFwdFlightGotoCircle"));
            QVERIFY2(loiterVisuals, "Loiter circle visuals not found");
            auto* const loiterRadius = loiterVisuals->property("radius").value<Fact*>();
            QVERIFY2(loiterRadius, "Loiter radius Fact not found");

            const double defaultRadius = SettingsManager::instance()
                                             ->flyViewSettings()
                                             ->forwardFlightGoToLocationLoiterRad()
                                             ->rawValue()
                                             .toDouble();
            // The map one-shot centers on the first vehicle position and follows the vehicle back into view:
            // wait out the one-shot and edit next to the vehicle so the camera stays put under the drags
            auto* const positionTracker = geoMap->findChild<MapPositionTracker*>();
            QVERIFY2(positionTracker, "MapPositionTracker not found");
            QTRY_VERIFY_WITH_TIMEOUT(positionTracker->firstVehiclePositionReceived(), kItemAppearTimeoutMs);
            const QGeoCoordinate loiterCenter = vehicle->coordinate().atDistanceAndAzimuth(100, 0);
            cam->lookAt(loiterCenter, 0, 0, 600);
            QVERIFY(QMetaObject::invokeMethod(gotoItem, "show", Q_ARG(QVariant, QVariant::fromValue(loiterCenter))));
            QTRY_VERIFY_WITH_TIMEOUT(loiterVisuals->isVisible(), kSettleTimeoutMs);
            QCOMPARE(loiterRadius->rawValue().toDouble(), defaultRadius);

            // Not editing yet: the ring shows without handles
            QVERIFY(!findVisibleItem(_rootItem, QStringLiteral("geoMapCircleRadiusHandle"), 0));
            QVERIFY(!findVisibleItem(_rootItem, QStringLiteral("geoMapCircleCenterHandle"), 0));

            const auto handleCenter = [viewport](QQuickItem* item) {
                return viewport->mapFromItem(item, QPointF(item->width() / 2, item->height() / 2));
            };
            const auto toWin = [viewport](const QPointF& viewportPos) {
                return viewport->mapToScene(viewportPos).toPoint();
            };
            double radiusTolerance = 0.0;
            // Drags the radius handle to where a ring of targetRadius draws its handle
            const auto dragRadiusTo = [this, cam, &handleCenter, &toWin, &radiusTolerance](
                                          QQuickItem* handle, const QGeoCoordinate& center, double targetRadius) {
                const double handleZ = handle->property("scenePosition").value<QVector3D>().z();
                const auto target = radiusHandleScreenPos(cam, center, targetRadius, handleZ);
                const auto tolerance = radiusToleranceMeters(cam, center, targetRadius, handleZ);
                QVERIFY(target.has_value());
                QVERIFY(tolerance.has_value());
                radiusTolerance = *tolerance;
                const QPointF from = handleCenter(handle);
                QTest::mousePress(_window, Qt::LeftButton, Qt::NoModifier, toWin(from));
                constexpr int steps = 10;
                for (int i = 1; i <= steps; i++) {
                    QTest::mouseMove(_window, toWin(from + (((*target - from) * i) / steps)));
                }
                QTest::mouseRelease(_window, Qt::LeftButton, Qt::NoModifier, toWin(*target));
            };
            const double editedRadius = defaultRadius + 50.0;

            // Cancel: the drag is discarded
            QVERIFY(QMetaObject::invokeMethod(loiterVisuals, "startLoiterRadiusEdit"));
            QQuickItem* const radiusHandle = findVisibleItem(_rootItem, QStringLiteral("geoMapCircleRadiusHandle"));
            QVERIFY2(radiusHandle, "Radius drag handle not visible while editing");
            QVERIFY(radiusHandle->property("labelVisible").toBool());
            QVERIFY2(!findVisibleItem(_rootItem, QStringLiteral("geoMapCircleCenterHandle"), 0),
                     "Loiter center must not be draggable");
            dragRadiusTo(radiusHandle, gotoItem->property("coordinate").value<QGeoCoordinate>(), editedRadius);
            QCOMPARE_LT(qAbs(loiterRadius->rawValue().toDouble() - editedRadius), radiusTolerance);
            QVERIFY(QMetaObject::invokeMethod(loiterVisuals, "actionCancelled"));
            QCOMPARE(loiterRadius->rawValue().toDouble(), defaultRadius);
            QVERIFY(!findVisibleItem(_rootItem, QStringLiteral("geoMapCircleRadiusHandle"), 0));

            // Confirm: the dragged radius is kept for the guided command
            QVERIFY(QMetaObject::invokeMethod(loiterVisuals, "startLoiterRadiusEdit"));
            QVERIFY(findVisibleItem(_rootItem, QStringLiteral("geoMapCircleRadiusHandle")));
            dragRadiusTo(radiusHandle, gotoItem->property("coordinate").value<QGeoCoordinate>(), editedRadius);
            QVERIFY(QMetaObject::invokeMethod(loiterVisuals, "actionConfirmed"));
            QCOMPARE_LT(qAbs(loiterRadius->rawValue().toDouble() - editedRadius), radiusTolerance);
            QVERIFY(!findVisibleItem(_rootItem, QStringLiteral("geoMapCircleRadiusHandle"), 0));
        });
}

// The Fly View map position and zoom are shared with Plan view in both
// directions (FlyViewMap parity via QGroundControl.flightMapPosition/Zoom)
void FlyViewGeoUITest::_testMapViewSharedWithPlanView()
{
    Fact* const geoEngineFact = SettingsManager::instance()->flyViewSettings()->useGeoMapEngine();
    const QVariant savedEnabled = geoEngineFact->rawValue();
    const auto guard = qScopeGuard([geoEngineFact, savedEnabled] { geoEngineFact->setRawValue(savedEnabled); });
    ignoreLogMessage("API.QGCApplication.AppMessage", QtDebugMsg,
                     QRegularExpression(QStringLiteral("Restart application for changes to take effect")));
    geoEngineFact->setRawValue(true);

    startUI();
    if (QTest::currentTestFailed())
        return;

    const std::optional<bool> rhiBased = expectSoftwareBackendWarnings(/*strict*/ false);
    QVERIFY2(rhiBased.has_value(), "No renderer interface on the main window");

    QQuickItem* const viewport = findVisibleItem(_rootItem, QStringLiteral("geoMapViewport"), kItemAppearTimeoutMs);
    QVERIFY2(viewport, "GeoMap 3D viewport not visible");
    auto* const cam = viewport->parentItem()->findChild<GeoMapCamera*>(QStringLiteral("geoMapCamera"));
    QVERIFY2(cam, "GeoMapCamera not found");

    // Fly -> Plan: the Plan map opens where the Fly map was left. Earlier tests
    // leave the shared position at kMapCenter, so use a center only this test sets.
    const QGeoCoordinate flyCenter(35.6812, 139.7671);
    constexpr double kFlyZoom = 15.0;
    cam->lookAt(flyCenter, 0, 0, cam->distanceForZoomLevel(kFlyZoom));

    QVERIFY2(clickToolSelectDropdownButton(QStringLiteral("toolbar_viewPlan")), "Failed to navigate to Plan view");
    QQuickItem* const planMap = findVisibleItem(_rootItem, QStringLiteral("planView_map"), kItemAppearTimeoutMs);
    QVERIFY2(planMap, "planView_map not visible");
    QTRY_COMPARE_LT_WITH_TIMEOUT(planMap->property("center").value<QGeoCoordinate>().distanceTo(flyCenter), 1.0,
                                 kSettleTimeoutMs);
    QCOMPARE_LT(qAbs(planMap->property("zoomLevel").toDouble() - kFlyZoom), 0.01);

    // Plan -> Fly: the Fly map picks up where the Plan map was left
    const QGeoCoordinate planCenter(47.397742, 8.545594);
    constexpr double kPlanZoom = 13.0;
    planMap->setProperty("center", QVariant::fromValue(planCenter));
    planMap->setProperty("zoomLevel", kPlanZoom);

    QVERIFY2(clickToolSelectDropdownButton(QStringLiteral("toolbar_viewFly")), "Failed to navigate to Fly view");
    QVERIFY2(findVisibleItem(_rootItem, QStringLiteral("geoMapViewport"), kItemAppearTimeoutMs),
             "GeoMap 3D viewport not visible after returning to Fly view");
    QTRY_COMPARE_LT_WITH_TIMEOUT(cam->center().distanceTo(planCenter), 1.0, kSettleTimeoutMs);
    QCOMPARE_LT(qAbs(cam->zoomLevelForDistance(cam->distance()) - kPlanZoom), 0.01);

    stopUI();
}

// A window resize keeps the displayed zoom level (FlyViewMap parity), so the zoom
// shared with Plan view still matches what the Fly View shows
void FlyViewGeoUITest::_testResizeKeepsSharedZoom()
{
    Fact* const geoEngineFact = SettingsManager::instance()->flyViewSettings()->useGeoMapEngine();
    const QVariant savedEnabled = geoEngineFact->rawValue();
    const auto guard = qScopeGuard([geoEngineFact, savedEnabled] { geoEngineFact->setRawValue(savedEnabled); });
    ignoreLogMessage("API.QGCApplication.AppMessage", QtDebugMsg,
                     QRegularExpression(QStringLiteral("Restart application for changes to take effect")));
    geoEngineFact->setRawValue(true);

    startUI();
    if (QTest::currentTestFailed())
        return;

    const std::optional<bool> rhiBased = expectSoftwareBackendWarnings(/*strict*/ false);
    QVERIFY2(rhiBased.has_value(), "No renderer interface on the main window");

    QQuickItem* const viewport = findVisibleItem(_rootItem, QStringLiteral("geoMapViewport"), kItemAppearTimeoutMs);
    QVERIFY2(viewport, "GeoMap 3D viewport not visible");
    auto* const cam = viewport->parentItem()->findChild<GeoMapCamera*>(QStringLiteral("geoMapCamera"));
    QVERIFY2(cam, "GeoMapCamera not found");

    constexpr double kZoom = 14.0;
    cam->lookAt(kMapCenter, 0, 0, cam->distanceForZoomLevel(kZoom));
    QCOMPARE_LT(qAbs(QGroundControlQmlGlobal::flightMapZoom() - kZoom), 0.01);

    // Width, not height: in landscape the rendered scale depends on viewport width only
    const QSizeF viewportBefore = cam->viewportSize();
    _window->resize((_window->width() * 2) / 3, _window->height());
    QTRY_VERIFY_WITH_TIMEOUT(cam->viewportSize().width() < viewportBefore.width(), kSettleTimeoutMs);

    QTRY_COMPARE_LT_WITH_TIMEOUT(qAbs(cam->zoomLevelForDistance(cam->distance()) - kZoom), 0.01, kSettleTimeoutMs);
    QCOMPARE_LT(qAbs(QGroundControlQmlGlobal::flightMapZoom() - kZoom), 0.01);

    stopUI();
}

// The Fly map left in PiP picks up the zoom set in Plan view when it is expanded
// again, and the PiP zoom is never shared
void FlyViewGeoUITest::_testPipExitAppliesSharedZoom()
{
    Fact* const geoEngineFact = SettingsManager::instance()->flyViewSettings()->useGeoMapEngine();
    const QVariant savedEnabled = geoEngineFact->rawValue();
    const auto guard = qScopeGuard([geoEngineFact, savedEnabled] { geoEngineFact->setRawValue(savedEnabled); });
    ignoreLogMessage("API.QGCApplication.AppMessage", QtDebugMsg,
                     QRegularExpression(QStringLiteral("Restart application for changes to take effect")));
    geoEngineFact->setRawValue(true);
    // A configured video stream is what makes the Fly View offer PiP
    Fact* const videoSource = SettingsManager::instance()->videoSettings()->videoSource();
    const QVariant savedVideoSource = videoSource->rawValue();
    const auto videoGuard =
        qScopeGuard([videoSource, savedVideoSource] { videoSource->setRawValue(savedVideoSource); });
    videoSource->setRawValue(QString::fromLatin1(VideoSettings::videoSourceUDPH264));

    startUI();
    if (QTest::currentTestFailed())
        return;

    const std::optional<bool> rhiBased = expectSoftwareBackendWarnings(/*strict*/ false);
    QVERIFY2(rhiBased.has_value(), "No renderer interface on the main window");

    QQuickItem* const adapter =
        findVisibleItem(_rootItem, QStringLiteral("flyViewGeoMapAdapter"), kItemAppearTimeoutMs);
    QVERIFY2(adapter, "GeoMap adapter not visible");
    QQuickItem* const viewport = findVisibleItem(_rootItem, QStringLiteral("geoMapViewport"), kItemAppearTimeoutMs);
    QVERIFY2(viewport, "GeoMap 3D viewport not visible");
    auto* const cam = viewport->parentItem()->findChild<GeoMapCamera*>(QStringLiteral("geoMapCamera"));
    QVERIFY2(cam, "GeoMapCamera not found");

    constexpr double kFlyZoom = 15.0;
    cam->lookAt(kMapCenter, 0, 0, cam->distanceForZoomLevel(kFlyZoom));

    const QString pipName = QStringLiteral("flyViewPipView");
    QQuickItem* const pipView = findVisibleItem(_rootItem, pipName, kItemAppearTimeoutMs);
    QVERIFY2(pipView, "PiP view not visible");
    QVERIFY(_clickItemAt(pipView, 0.5, 0.5, pipName));
    QTRY_VERIFY_WITH_TIMEOUT(adapter->property("pipMode").toBool(), kSettleTimeoutMs);
    QCOMPARE_LT(qAbs(QGroundControlQmlGlobal::flightMapZoom() - kFlyZoom), 0.01);

    QVERIFY2(clickToolSelectDropdownButton(QStringLiteral("toolbar_viewPlan")), "Failed to navigate to Plan view");
    QQuickItem* const planMap = findVisibleItem(_rootItem, QStringLiteral("planView_map"), kItemAppearTimeoutMs);
    QVERIFY2(planMap, "planView_map not visible");
    constexpr double kPlanZoom = 13.0;
    planMap->setProperty("zoomLevel", kPlanZoom);

    QVERIFY2(clickToolSelectDropdownButton(QStringLiteral("toolbar_viewFly")), "Failed to navigate to Fly view");
    QVERIFY2(findVisibleItem(_rootItem, pipName, kItemAppearTimeoutMs),
             "PiP view not visible after returning to Fly view");
    QVERIFY(adapter->property("pipMode").toBool());
    QVERIFY(_clickItemAt(pipView, 0.5, 0.5, pipName));
    QTRY_VERIFY_WITH_TIMEOUT(!adapter->property("pipMode").toBool(), kSettleTimeoutMs);

    QTRY_COMPARE_LT_WITH_TIMEOUT(qAbs(cam->zoomLevelForDistance(cam->distance()) - kPlanZoom), 0.01, kSettleTimeoutMs);
    QCOMPARE_LT(qAbs(QGroundControlQmlGlobal::flightMapZoom() - kPlanZoom), 0.01);

    stopUI();
}

void FlyViewGeoUITest::_testZoomToMissionFromVehicle_data()
{
    QTest::addColumn<bool>("inPip");

    QTest::addRow("full view") << false;
    // The fit must survive the swap back to the full view
    QTest::addRow("pip") << true;
}

// A mission downloaded from the vehicle zooms the map to show it (FlyViewMap parity)
void FlyViewGeoUITest::_testZoomToMissionFromVehicle()
{
    QFETCH(bool, inPip);

    Fact* const geoEngineFact = SettingsManager::instance()->flyViewSettings()->useGeoMapEngine();
    const QVariant savedEnabled = geoEngineFact->rawValue();
    const auto guard = qScopeGuard([geoEngineFact, savedEnabled] { geoEngineFact->setRawValue(savedEnabled); });
    ignoreLogMessage("API.QGCApplication.AppMessage", QtDebugMsg,
                     QRegularExpression(QStringLiteral("Restart application for changes to take effect")));
    geoEngineFact->setRawValue(true);
    // A configured video stream is what makes the Fly View offer PiP
    Fact* const videoSource = SettingsManager::instance()->videoSettings()->videoSource();
    const QVariant savedVideoSource = videoSource->rawValue();
    const auto videoGuard =
        qScopeGuard([videoSource, savedVideoSource] { videoSource->setRawValue(savedVideoSource); });
    if (inPip) {
        videoSource->setRawValue(QString::fromLatin1(VideoSettings::videoSourceUDPH264));
    }

    runWithMockLink(
        [] { return MockLink::startPX4MockLinkWithMission(); },
        [this, inPip](QPointer<MockLink>, Vehicle* vehicle) {
            const std::optional<bool> rhiBased = expectSoftwareBackendWarnings(/*strict*/ false);
            QVERIFY2(rhiBased.has_value(), "No renderer interface on the main window");

            QQuickItem* const adapter =
                findVisibleItem(_rootItem, QStringLiteral("flyViewGeoMapAdapter"), kItemAppearTimeoutMs);
            QVERIFY2(adapter, "GeoMap adapter not visible");
            auto* const planController = adapter->property("planMasterController").value<PlanMasterController*>();
            QVERIFY2(planController, "Fly View PlanMasterController not found");
            QQuickItem* const viewport =
                findVisibleItem(_rootItem, QStringLiteral("geoMapViewport"), kItemAppearTimeoutMs);
            QVERIFY2(viewport, "GeoMap 3D viewport not visible");
            QQuickItem* const geoMap = viewport->parentItem();
            auto* const cam = geoMap->findChild<GeoMapCamera*>(QStringLiteral("geoMapCamera"));
            QVERIFY2(cam, "GeoMapCamera not found");
            auto* const positionTracker = geoMap->findChild<MapPositionTracker*>();
            QVERIFY2(positionTracker, "MapPositionTracker not found");

            // Past the one-shot vehicle centering (it would also set a zoom), then zoom far
            // out: only the mission fit can bring the camera back in
            QTRY_VERIFY_WITH_TIMEOUT(positionTracker->firstVehiclePositionReceived(), kItemAppearTimeoutMs);
            constexpr double kFarZoom = 5.0;
            cam->lookAt(vehicle->coordinate(), 0, 0, cam->distanceForZoomLevel(kFarZoom));

            const QString pipName = QStringLiteral("flyViewPipView");
            QQuickItem* const pipView = inPip ? findVisibleItem(_rootItem, pipName, kItemAppearTimeoutMs) : nullptr;
            if (inPip) {
                QVERIFY2(pipView, "PiP view not visible");
                QVERIFY(_clickItemAt(pipView, 0.5, 0.5, pipName));
                QTRY_VERIFY_WITH_TIMEOUT(adapter->property("pipMode").toBool(), kSettleTimeoutMs);
            }

            QSignalSpy newItemsSpy(planController->missionController(), &MissionController::newItemsFromVehicle);
            vehicle->missionManager()->loadFromVehicle();
            QVERIFY(newItemsSpy.wait(kItemAppearTimeoutMs));

            if (inPip) {
                QVERIFY(_clickItemAt(pipView, 0.5, 0.5, pipName));
                QTRY_VERIFY_WITH_TIMEOUT(!adapter->property("pipMode").toBool(), kSettleTimeoutMs);
            }

            // The mission fills the view: fitted well past the far zoom, every item on screen
            QTRY_COMPARE_GT_WITH_TIMEOUT(cam->zoomLevelForDistance(cam->distance()), kFarZoom + 5.0, kSettleTimeoutMs);
            const QRectF screen(QPointF(0, 0), cam->viewportSize());
            QmlObjectListModel* const visualItems = planController->missionController()->visualItems();
            int coordinateItems = 0;
            for (int i = 1; i < visualItems->count(); i++) {
                const auto* const item = visualItems->value<VisualMissionItem*>(i);
                if (!item->specifiesCoordinate() || item->isStandaloneCoordinate()) {
                    continue;
                }
                coordinateItems++;
                const auto projected = cam->worldToScreen(TileMath::geoToWorld(item->coordinate()));
                QVERIFY(projected.has_value());
                QVERIFY2(screen.contains(*projected), qPrintable(QStringLiteral("Mission item %1 off screen").arg(i)));
            }
            QCOMPARE_GT(coordinateItems, 1);
        });
}
