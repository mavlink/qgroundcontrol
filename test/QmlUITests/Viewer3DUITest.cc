#include "Viewer3DUITest.h"

#include <QtCore/QtMath>
#include <QtGui/QGuiApplication>
#include <QtGui/QMouseEvent>
#include <QtGui/QVector3D>
#include <QtQuick/QQuickItem>
#include <QtQuick/QQuickWindow>
#include <QtQuick/QSGRendererInterface>
#include <QtTest/QTest>

#include "Fact.h"
#include "MockLink.h"
#include "SettingsManager.h"
#include "Vehicle.h"
#include "Viewer3DCameraController.h"
#include "Viewer3DSettings.h"

UT_REGISTER_TEST(Viewer3DUITest, TestLabel::Integration)

void Viewer3DUITest::_test3DViewShowsVehicle()
{
    runWithMockLink(
        [] { return MockLink::startPX4MockLink(); },
        [this](QPointer<MockLink> /*mockLink*/, Vehicle* vehicle) {
            const std::optional<bool> rhiBased = expectSoftwareBackendWarnings();
            QVERIFY2(rhiBased.has_value(), "No renderer interface on the main window");

            SettingsManager::instance()->viewer3DSettings()->enabled()->setRawValue(true);

            QVERIFY2(clickButton(QStringLiteral("flyToolStrip_viewer3DButton")), "Failed to click 3D View button");

            QQuickItem* const view = findVisibleItem(_rootItem, QStringLiteral("viewer3DView"), 15000);
            QVERIFY2(view, "3D view never became visible");

            // Drone model is created per-vehicle once the scene loads
            QObject* droneModel = nullptr;
            QTRY_VERIFY_WITH_TIMEOUT(
                (droneModel = view->findChild<QObject*>(QStringLiteral("viewer3DDroneModel"))) != nullptr, 10000);

            // Drone model must track real vehicle telemetry
            const double vehicleHeading = vehicle->heading()->rawValue().toDouble();
            QTRY_VERIFY_WITH_TIMEOUT(qAbs(droneModel->property("heading").toDouble() - vehicleHeading) < 1.0, 10000);

            const double vehicleAltitude = vehicle->altitudeRelative()->rawValue().toDouble();
            QTRY_VERIFY_WITH_TIMEOUT(qAbs(droneModel->property("pose_z").toDouble() - (vehicleAltitude * 10.0)) < 5.0,
                                     10000);

            // Scale bar appears in the widget layer (same position as Fly View map scale)
            QVERIFY2(findVisibleItem(_rootItem, QStringLiteral("viewer3DScaleBar"), 5000),
                     "3D scale bar never became visible");

            // Middle-drag over the view must orbit the camera (Gazebo mapping)
            QObject* const cameraController = view->property("cameraController").value<QObject*>();
            QVERIFY2(cameraController, "cameraController not exposed on 3D view");

            // Initial camera position: no OSM map is configured, so
            // _resetCameraForContent() falls back to the side view
            // lookAt(origin, heading 0, tilt 75, distance 400). GPS ref
            // updates from MockLink re-trigger the same reset, so the values
            // are stable once settled.
            QTRY_COMPARE_WITH_TIMEOUT(cameraController->property("heading").toDouble(), 0.0, 5000);
            QTRY_COMPARE_WITH_TIMEOUT(cameraController->property("tilt").toDouble(), 75.0, 5000);
            QTRY_COMPARE_WITH_TIMEOUT(cameraController->property("distance").toDouble(), 400.0, 5000);
            QCOMPARE(cameraController->property("orbitCenter").value<QVector3D>(), QVector3D(0, 0, 0));

            const double headingBefore = cameraController->property("heading").toDouble();

            const QPoint dragStart = view->mapToScene(QPointF(view->width() / 2, view->height() / 2)).toPoint();
            QTest::mousePress(_window, Qt::MiddleButton, Qt::NoModifier, dragStart);
            for (int i = 1; i <= 10; i++) {
                QTest::mouseMove(_window, dragStart + QPoint(i * 15, 0));
            }
            QTest::mouseRelease(_window, Qt::MiddleButton, Qt::NoModifier, dragStart + QPoint(150, 0));

            QTRY_VERIFY_WITH_TIMEOUT(qAbs(cameraController->property("heading").toDouble() - headingBefore) > 1.0,
                                     5000);

            // Toggle back to the map view
            QVERIFY2(clickButton(QStringLiteral("flyToolStrip_viewer3DButton")), "Failed to click Fly button");
            QTRY_VERIFY_WITH_TIMEOUT(!view->isVisible(), 5000);

            if (!*rhiBased) {
                // Software backend: the View3D fallback warnings must have
                // been emitted while the 3D view was up.
                verifyExpectedLogMessage();
                verifyExpectedLogMessage();
            }
        });
}

// Exercises every Gazebo-style camera gesture against the real 3D view:
// mouse drags (pan/orbit/zoom), wheel zoom, and synthesized multi-touch
// (pinch zoom, two-finger twist, two-finger tilt, single-finger pan).
void Viewer3DUITest::_test3DViewCameraGestures()
{
    runWithMockLink(
        [] { return MockLink::startPX4MockLink(); },
        [this](QPointer<MockLink> /*mockLink*/, Vehicle* /*vehicle*/) {
            const std::optional<bool> rhiBased = expectSoftwareBackendWarnings();
            QVERIFY2(rhiBased.has_value(), "No renderer interface on the main window");

            SettingsManager::instance()->viewer3DSettings()->enabled()->setRawValue(true);

            QVERIFY2(clickButton(QStringLiteral("flyToolStrip_viewer3DButton")), "Failed to click 3D View button");

            QQuickItem* const view = findVisibleItem(_rootItem, QStringLiteral("viewer3DView"), 15000);
            QVERIFY2(view, "3D view never became visible");

            auto* const cam =
                qobject_cast<Viewer3DCameraController*>(view->property("cameraController").value<QObject*>());
            QVERIFY2(cam, "cameraController not exposed on 3D view");

            // Wait for the initial GPS-ref-driven camera reset to settle so a
            // late reset can't clobber the poses set up by the gestures below
            QTRY_COMPARE_WITH_TIMEOUT(cam->tilt(), 75.0, 5000);
            QTRY_COMPARE_WITH_TIMEOUT(cam->distance(), 400.0, 5000);

            const qreal w = view->width();
            const qreal h = view->height();
            QVERIFY2((w > 300) && (h > 300), "3D view too small for gesture synthesis");

            // Gesture positions are in view-local coordinates; QTest wants
            // window coordinates
            const auto toWin = [view](qreal x, qreal y) { return view->mapToScene(QPointF(x, y)).toPoint(); };
            const QPoint center = toWin(w / 2, h / 2);

            // Known pose before every gesture: origin, heading 0, tilt 60,
            // distance 1000
            const auto resetPose = [cam] { cam->lookAt(QVector3D(0, 0, 0), 0, 60, 1000); };

            const auto mouseDrag = [this](Qt::MouseButton button, const QPoint& from, const QPoint& delta,
                                          Qt::KeyboardModifiers modifiers = Qt::NoModifier) {
                QTest::mousePress(_window, button, modifiers, from);
                constexpr int steps = 10;
                for (int i = 1; i <= steps; i++) {
                    // QTest::mouseMove cannot carry keyboard modifiers, and
                    // DragHandler::acceptedModifiers filters every event (not
                    // just the press), so deliver the moves directly
                    const QPointF pos = from + (delta * i) / steps;
                    QMouseEvent move(QEvent::MouseMove, pos, _window->mapToGlobal(pos), Qt::NoButton, button,
                                     modifiers);
                    QGuiApplication::sendEvent(_window, &move);
                }
                QTest::mouseRelease(_window, button, modifiers, from + delta);
            };

            QPointingDevice* const touchDevice = QTest::createTouchDevice();

            // Left drag: pan without rotating or zooming
            resetPose();
            mouseDrag(Qt::LeftButton, center, QPoint(50, 30));
            QTRY_VERIFY_WITH_TIMEOUT(!qFuzzyIsNull(cam->orbitCenter().x()) || !qFuzzyIsNull(cam->orbitCenter().y()),
                                     5000);
            QCOMPARE(cam->heading(), 0.0);
            QCOMPARE(cam->tilt(), 60.0);
            QCOMPARE(cam->distance(), 1000.0);

            // Middle drag right by a quarter of the view width: orbit 90 deg
            resetPose();
            mouseDrag(Qt::MiddleButton, center, QPoint(qRound(w / 4), 0));
            QTRY_VERIFY_WITH_TIMEOUT(qAbs(cam->heading() - 90.0) < 0.5, 5000);
            QVERIFY(qAbs(cam->tilt() - 60.0) < 0.5);
            QCOMPARE(cam->distance(), 1000.0);

            // Middle drag up half the view height: +90 deg tilt, clamped to 85
            resetPose();
            mouseDrag(Qt::MiddleButton, toWin(w / 2, (h * 3) / 4), QPoint(0, qRound(-h / 2)));
            QTRY_COMPARE_WITH_TIMEOUT(cam->tilt(), 85.0, 5000);

            // Shift+left drag: trackpad-friendly orbit
            resetPose();
            mouseDrag(Qt::LeftButton, center, QPoint(100, 0), Qt::ShiftModifier);
            QTRY_VERIFY_WITH_TIMEOUT(cam->heading() > 1.0, 5000);

            // Right drag up: zoom in without rotating
            resetPose();
            mouseDrag(Qt::RightButton, center, QPoint(0, -100));
            QTRY_VERIFY_WITH_TIMEOUT(cam->distance() < 1000.0, 5000);
            QCOMPARE(cam->heading(), 0.0);
            QVERIFY(qAbs(cam->tilt() - 60.0) < 0.001);

            // Right drag down: zoom out
            resetPose();
            mouseDrag(Qt::RightButton, center, QPoint(0, 100));
            QTRY_VERIFY_WITH_TIMEOUT(cam->distance() > 1000.0, 5000);

            // Wheel up: zoom in; wheel down: zoom out
            resetPose();
            QTest::wheelEvent(_window, center, QPoint(0, 120));
            QTRY_VERIFY_WITH_TIMEOUT(cam->distance() < 1000.0, 5000);
            resetPose();
            QTest::wheelEvent(_window, center, QPoint(0, -120));
            QTRY_VERIFY_WITH_TIMEOUT(cam->distance() > 1000.0, 5000);

            // Pinch spread: zoom in
            resetPose();
            {
                QTest::QTouchEventSequence touch = QTest::touchEvent(_window, touchDevice);
                touch.press(0, center + QPoint(-50, 0)).press(1, center + QPoint(50, 0)).commit();
                for (int i = 1; i <= 10; i++) {
                    touch.move(0, center + QPoint(-50 - (i * 10), 0))
                        .move(1, center + QPoint(50 + (i * 10), 0))
                        .commit();
                }
                touch.release(0, center + QPoint(-150, 0)).release(1, center + QPoint(150, 0)).commit();
            }
            QTRY_VERIFY_WITH_TIMEOUT(cam->distance() < 1000.0, 5000);

            // Single finger drag: pan only; heading/tilt/distance must not
            // change (regression: touch has no buttons, so the button-driven
            // orbit handler grabbed single-finger drags and tilted the view)
            resetPose();
            {
                QTest::QTouchEventSequence touch = QTest::touchEvent(_window, touchDevice);
                touch.press(0, center).commit();
                for (int i = 1; i <= 6; i++) {
                    touch.move(0, center + QPoint(0, i * 10)).commit();
                }
                touch.release(0, center + QPoint(0, 60)).commit();
            }
            QTRY_VERIFY_WITH_TIMEOUT(!qFuzzyIsNull(cam->orbitCenter().x()) || !qFuzzyIsNull(cam->orbitCenter().y()),
                                     5000);
            QCOMPARE(cam->heading(), 0.0);
            QVERIFY2(qAbs(cam->tilt() - 60.0) < 0.001, qPrintable(QStringLiteral("tilt %1").arg(cam->tilt())));
            QVERIFY(qAbs(cam->distance() - 1000.0) < 0.001);

            // Two-finger twist 90 deg visually counterclockwise (y-down
            // screen): the world follows the fingers, so heading decreases.
            // The PinchHandler consumes the rotation applied before its
            // activation threshold is crossed, so assert a band rather than
            // the exact angle.
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
            QTRY_VERIFY_WITH_TIMEOUT(cam->heading() < -45.0, 5000);
            QVERIFY2(cam->heading() >= -91.0,
                     qPrintable(QStringLiteral("twist overshot the finger rotation: %1").arg(cam->heading())));
            QVERIFY(qAbs(cam->tilt() - 60.0) < 2.0);
            QVERIFY(qAbs(cam->distance() - 1000.0) < 10.0);

            // Two-finger vertical swipe: tilt (gain: full height = 180 deg).
            // Swipe up 60 px tilts toward the horizon
            const qreal swipeTiltDegrees = (60.0 / h) * 180.0;
            resetPose();
            {
                QTest::QTouchEventSequence touch = QTest::touchEvent(_window, touchDevice);
                touch.press(0, center + QPoint(-50, 100)).press(1, center + QPoint(50, 100)).commit();
                for (int i = 1; i <= 6; i++) {
                    touch.move(0, center + QPoint(-50, 100 - (i * 10)))
                        .move(1, center + QPoint(50, 100 - (i * 10)))
                        .commit();
                }
                touch.release(0, center + QPoint(-50, 40)).release(1, center + QPoint(50, 40)).commit();
            }
            QTRY_VERIFY2_WITH_TIMEOUT(
                qAbs(cam->tilt() - (60.0 + swipeTiltDegrees)) < 2.0,
                qPrintable(QStringLiteral("tilt %1 expected %2").arg(cam->tilt()).arg(60.0 + swipeTiltDegrees)), 5000);
            QVERIFY(qAbs(cam->heading()) < 2.0);
            QVERIFY(qAbs(cam->distance() - 1000.0) < 10.0);

            // Swipe down 60 px tilts back toward top-down
            resetPose();
            {
                QTest::QTouchEventSequence touch = QTest::touchEvent(_window, touchDevice);
                touch.press(0, center + QPoint(-50, -100)).press(1, center + QPoint(50, -100)).commit();
                for (int i = 1; i <= 6; i++) {
                    touch.move(0, center + QPoint(-50, -100 + (i * 10)))
                        .move(1, center + QPoint(50, -100 + (i * 10)))
                        .commit();
                }
                touch.release(0, center + QPoint(-50, -40)).release(1, center + QPoint(50, -40)).commit();
            }
            QTRY_VERIFY2_WITH_TIMEOUT(
                qAbs(cam->tilt() - (60.0 - swipeTiltDegrees)) < 2.0,
                qPrintable(QStringLiteral("tilt %1 expected %2").arg(cam->tilt()).arg(60.0 - swipeTiltDegrees)), 5000);

            if (!*rhiBased) {
                verifyExpectedLogMessage();
                verifyExpectedLogMessage();
            }
        });
}
