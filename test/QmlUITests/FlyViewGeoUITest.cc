#include "FlyViewGeoUITest.h"

#include <QtCore/QList>
#include <QtCore/QScopeGuard>
#include <QtCore/QtMath>
#include <QtGui/QGuiApplication>
#include <QtGui/QMouseEvent>
#include <QtGui/QPointingDevice>
#include <QtGui/QQuaternion>
#include <QtGui/QVector3D>
#include <QtPositioning/QGeoCoordinate>
#include <QtQuick/QQuickItem>
#include <QtQuick/QQuickWindow>
#include <QtTest/QTest>

#include <cmath>
#include <optional>

#include "Fact.h"
#include "GeoMapCamera.h"
#include "GeoViewSettings.h"
#include "SettingsManager.h"
#include "SurfacePatchModel.h"

UT_REGISTER_TEST(FlyViewGeoUITest, TestLabel::Integration)

void FlyViewGeoUITest::_testHiddenWhenDisabled()
{
    startUI();
    if (QTest::currentTestFailed())
        return;

    // Default: preview setting disabled, GeoView entry must not appear in the
    // dropdown. GeoView's View3D is Loader-gated on visibility, so no Quick 3D
    // initialization (or software-backend warnings) can occur in this test.
    QVERIFY(clickButton(QStringLiteral("toolbar_qgcLogo")));
    QVERIFY2(findVisibleItem(_rootItem, QStringLiteral("toolbar_viewFly")), "View dropdown did not open");
    QVERIFY2(!findVisibleItem(_rootItem, QStringLiteral("toolbar_viewGeo"), 0),
             "GeoView button visible although preview setting is disabled");
    QVERIFY2(!findVisibleItem(_rootItem, QStringLiteral("mainView_geo"), 0),
             "GeoView main panel visible although preview setting is disabled");

    stopUI();
}

void FlyViewGeoUITest::_testViewSwitchWhenEnabled()
{
    Fact* const enabled = SettingsManager::instance()->geoViewSettings()->enabled();
    const QVariant savedEnabled = enabled->rawValue();
    const auto guard = qScopeGuard([enabled, savedEnabled] { enabled->setRawValue(savedEnabled); });
    enabled->setRawValue(true);

    startUI();
    if (QTest::currentTestFailed())
        return;

    // Non-strict: on the software backend the warnings only fire when a frame
    // renders the View3D (timing-dependent here); on RHI backends nothing is
    // ignored, so a fallback regression would fail the test.
    const std::optional<bool> rhiBased = expectSoftwareBackendWarnings(/*strict*/ false);
    QVERIFY2(rhiBased.has_value(), "No renderer interface on the main window");

    // GeoView entry visible and switches the main panel
    QVERIFY(clickToolSelectDropdownButton(QStringLiteral("toolbar_viewGeo")));
    QVERIFY2(findVisibleItem(_rootItem, QStringLiteral("mainView_geo")), "GeoView main panel not visible");
    QVERIFY2(!findVisibleItem(_rootItem, QStringLiteral("mainView_fly"), 0), "FlyView still visible");
    QVERIFY2(!findVisibleItem(_rootItem, QStringLiteral("mainView_plan"), 0), "PlanView still visible");

    // The 3D viewport instantiates and the patch repeater populates from the
    // SurfaceModel regardless of render backend
    QQuickItem* const viewport = findVisibleItem(_rootItem, QStringLiteral("geoMapViewport"), 10000);
    QVERIFY2(viewport, "GeoView 3D viewport not visible");
    QTRY_VERIFY_WITH_TIMEOUT(!viewport->findChildren<QObject*>(QStringLiteral("geoMapPatchDelegate")).isEmpty(), 5000);
    QVERIFY2(findVisibleItem(_rootItem, QStringLiteral("flyViewGeoDebugOverlay")), "Debug overlay not visible");

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
    QTRY_VERIFY_WITH_TIMEOUT(allPatchesImaged(), 5000);

    // Scene camera node tracks the GeoMapCamera debug pose (overhead at 1500m,
    // identity rotation, origin anchored at the camera center)
    QObject* const sceneCamera = viewport->findChild<QObject*>(QStringLiteral("geoMapSceneCamera"));
    QVERIFY2(sceneCamera, "Scene camera node not found");
    const QVector3D camPos = sceneCamera->property("position").value<QVector3D>();
    QCOMPARE_LT(qAbs(camPos.x()), 1.0f);
    QCOMPARE_LT(qAbs(camPos.y()), 1.0f);
    QCOMPARE_LT(qAbs(camPos.z() - 1500.0f), 1.0f);
    const QQuaternion camRot = sceneCamera->property("rotation").value<QQuaternion>();
    QVERIFY(qFuzzyCompare(camRot, QQuaternion()));

    // Switching back to Fly hides GeoView again
    QVERIFY(clickToolSelectDropdownButton(QStringLiteral("toolbar_viewFly")));
    QVERIFY2(findVisibleItem(_rootItem, QStringLiteral("mainView_fly")), "FlyView not visible");
    QVERIFY2(!findVisibleItem(_rootItem, QStringLiteral("mainView_geo"), 0), "GeoView still visible");

    stopUI();
}

// Exercises every camera gesture against the real view: left-drag pan,
// right-drag orbit, Shift+left-drag orbit (with pivot ring), Ctrl+left-drag
// first-person look, wheel zoom, and synthesized multi-touch (pinch zoom,
// two-finger twist).
void FlyViewGeoUITest::_testCameraGestures()
{
    Fact* const enabled = SettingsManager::instance()->geoViewSettings()->enabled();
    const QVariant savedEnabled = enabled->rawValue();
    const auto guard = qScopeGuard([enabled, savedEnabled] { enabled->setRawValue(savedEnabled); });
    enabled->setRawValue(true);

    startUI();
    if (QTest::currentTestFailed())
        return;

    const std::optional<bool> rhiBased = expectSoftwareBackendWarnings(/*strict*/ false);
    QVERIFY2(rhiBased.has_value(), "No renderer interface on the main window");

    QVERIFY(clickToolSelectDropdownButton(QStringLiteral("toolbar_viewGeo")));
    QQuickItem* const viewport = findVisibleItem(_rootItem, QStringLiteral("geoMapViewport"), 10000);
    QVERIFY2(viewport, "GeoView 3D viewport not visible");

    // Non-visual QObject sibling of the viewport: search from the scene root
    // item (the window-level QObject tree does not reach into the Loader item)
    auto* const cam = viewport->parentItem()->findChild<GeoMapCamera*>(QStringLiteral("geoMapCamera"));
    QVERIFY2(cam, "GeoMapCamera not found");

    const qreal w = viewport->width();
    const qreal h = viewport->height();
    QVERIFY2((w > 300) && (h > 300), "GeoView viewport too small for gesture synthesis");

    // Gesture positions are viewport-local; QTest wants window coordinates
    const auto toWin = [viewport](qreal x, qreal y) { return viewport->mapToScene(QPointF(x, y)).toPoint(); };
    const QPoint center = toWin(w / 2, h / 2);

    // Unlock tilt gestures once up front: the mode change starts the animated
    // 2D->3D transition, so wait for it to settle before posing the camera
    cam->setMode(GeoMapCamera::Mode::Mode3D);
    QTRY_COMPARE_WITH_TIMEOUT(cam->tilt(), GeoMapCamera::kDefault3DTilt, 5000);

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
    QTRY_VERIFY_WITH_TIMEOUT(cam->center().distanceTo(centerBefore) > 1.0, 5000);
    QCOMPARE(cam->heading(), 0.0);
    QCOMPARE(cam->tilt(), 30.0);
    QCOMPARE(cam->distance(), 1500.0);

    // Right drag right by a quarter of the width: orbit 90 deg heading
    resetPose();
    mouseDrag(Qt::RightButton, center, QPoint(qRound(w / 4), 0));
    QTRY_VERIFY_WITH_TIMEOUT(qAbs(cam->heading() - 90.0) < 0.5, 5000);
    QVERIFY(qAbs(cam->tilt() - 30.0) < 0.5);
    QCOMPARE(cam->distance(), 1500.0);

    // Right drag up a quarter of the height: tilt +45
    resetPose();
    mouseDrag(Qt::RightButton, center, QPoint(0, qRound(-h / 4)));
    QTRY_VERIFY_WITH_TIMEOUT(qAbs(cam->tilt() - 75.0) < 0.5, 5000);

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
        QTRY_VERIFY_WITH_TIMEOUT(qAbs(cam->heading() - 90.0) < 0.5, 5000);
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
        QTRY_VERIFY_WITH_TIMEOUT(qAbs(cam->heading() - 90.0) < 0.5, 5000);
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
    QTRY_VERIFY_WITH_TIMEOUT(cam->distance() < 1500.0, 5000);
    resetPose();
    QTest::wheelEvent(_window, center, QPoint(0, -120));
    QTRY_VERIFY_WITH_TIMEOUT(cam->distance() > 1500.0, 5000);

    QPointingDevice* const touchDevice = QTest::createTouchDevice();

    // Pinch spread: zoom in
    resetPose();
    {
        QTest::QTouchEventSequence touch = QTest::touchEvent(_window, touchDevice);
        touch.press(0, center + QPoint(-50, 0)).press(1, center + QPoint(50, 0)).commit();
        for (int i = 1; i <= 10; i++) {
            touch.move(0, center + QPoint(-50 - (i * 10), 0)).move(1, center + QPoint(50 + (i * 10), 0)).commit();
        }
        touch.release(0, center + QPoint(-150, 0)).release(1, center + QPoint(150, 0)).commit();
    }
    QTRY_VERIFY_WITH_TIMEOUT(cam->distance() < 1500.0, 5000);

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
            touch.move(0, center - d).move(1, center + d).commit();
        }
        touch.release(0, center + QPoint(0, r)).release(1, center + QPoint(0, -r)).commit();
    }
    QTRY_VERIFY_WITH_TIMEOUT(cam->heading() < 315.0, 5000);
    QVERIFY2(cam->heading() > 265.0,
             qPrintable(QStringLiteral("twist overshot the finger rotation: %1").arg(cam->heading())));

    stopUI();
}

// 2D/3D mode switch: the mode flips immediately and locks tilt gestures in 2D;
// tilt and terrain displacement animate to the mode's target; the compass
// control animates heading back to north-up
void FlyViewGeoUITest::_testModeToggleAndCompass()
{
    Fact* const enabled = SettingsManager::instance()->geoViewSettings()->enabled();
    const QVariant savedEnabled = enabled->rawValue();
    const auto guard = qScopeGuard([enabled, savedEnabled] { enabled->setRawValue(savedEnabled); });
    enabled->setRawValue(true);

    startUI();
    if (QTest::currentTestFailed())
        return;

    const std::optional<bool> rhiBased = expectSoftwareBackendWarnings(/*strict*/ false);
    QVERIFY2(rhiBased.has_value(), "No renderer interface on the main window");

    QVERIFY(clickToolSelectDropdownButton(QStringLiteral("toolbar_viewGeo")));
    QQuickItem* const viewport = findVisibleItem(_rootItem, QStringLiteral("geoMapViewport"), 10000);
    QVERIFY2(viewport, "GeoView 3D viewport not visible");

    auto* const cam = viewport->parentItem()->findChild<GeoMapCamera*>(QStringLiteral("geoMapCamera"));
    QVERIFY2(cam, "GeoMapCamera not found");
    QQuickItem* const sceneRoot = viewport->parentItem();

    // Startup: 2D mode, flat terrain, toggle offers 3D
    QCOMPARE(cam->mode(), GeoMapCamera::Mode::Mode2D);
    QVERIFY(cam->isTopDown());
    QCOMPARE(sceneRoot->property("terrainScale").toDouble(), 0.0);
    QQuickItem* const modeButton = findVisibleItem(_rootItem, QStringLiteral("flyViewGeoModeButton"));
    QVERIFY2(modeButton, "Mode toggle button not visible");
    QCOMPARE(modeButton->property("text").toString(), QStringLiteral("3D"));

    // Toggle to 3D: mode flips immediately, tilt and terrain animate up
    QVERIFY(clickButton(QStringLiteral("flyViewGeoModeButton")));
    QCOMPARE(cam->mode(), GeoMapCamera::Mode::Mode3D);
    QCOMPARE(modeButton->property("text").toString(), QStringLiteral("2D"));
    QTRY_COMPARE_WITH_TIMEOUT(cam->tilt(), GeoMapCamera::kDefault3DTilt, 5000);
    QTRY_COMPARE_WITH_TIMEOUT(sceneRoot->property("terrainScale").toDouble(), 1.0, 5000);
    QVERIFY(!cam->isTopDown());

    // Toggle back to 2D: tilt and terrain animate down
    QVERIFY(clickButton(QStringLiteral("flyViewGeoModeButton")));
    QCOMPARE(cam->mode(), GeoMapCamera::Mode::Mode2D);
    QTRY_COMPARE_WITH_TIMEOUT(cam->tilt(), 0.0, 5000);
    QTRY_COMPARE_WITH_TIMEOUT(sceneRoot->property("terrainScale").toDouble(), 0.0, 5000);
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
    QVERIFY(clickButton(QStringLiteral("flyViewGeoCompassButton")));
    QTRY_COMPARE_WITH_TIMEOUT(cam->heading(), 0.0, 5000);

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
    Fact* const enabled = SettingsManager::instance()->geoViewSettings()->enabled();
    const QVariant savedEnabled = enabled->rawValue();
    const auto guard = qScopeGuard([enabled, savedEnabled] { enabled->setRawValue(savedEnabled); });
    enabled->setRawValue(true);

    startUI();
    if (QTest::currentTestFailed())
        return;

    const std::optional<bool> rhiBased = expectSoftwareBackendWarnings(/*strict*/ false);
    QVERIFY2(rhiBased.has_value(), "No renderer interface on the main window");

    QVERIFY(clickToolSelectDropdownButton(QStringLiteral("toolbar_viewGeo")));
    QQuickItem* const viewport = findVisibleItem(_rootItem, QStringLiteral("geoMapViewport"), 10000);
    QVERIFY2(viewport, "GeoView 3D viewport not visible");

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
    QTRY_COMPARE_WITH_TIMEOUT(patchModel->pendingCount(), 0, 5000);
    QVERIFY(!anyNonZeroHeight());

    // Toggle hills on: non-flat heights
    patchModel->setDebugHills(true);
    QVERIFY(patchModel->debugHills());
    QTRY_VERIFY_WITH_TIMEOUT(anyNonZeroHeight(), 5000);

    // Toggle back off: terrain source flattens again
    patchModel->setDebugHills(false);
    QVERIFY(!patchModel->debugHills());
    QTRY_COMPARE_WITH_TIMEOUT(patchModel->pendingCount(), 0, 5000);
    QVERIFY(!anyNonZeroHeight());

    stopUI();
}
