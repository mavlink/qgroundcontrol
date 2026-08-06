/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "Viewer3DCameraControllerTest.h"

#include <QtCore/QtMath>
#include <QtGui/QVector3D>
#include <QtTest/QSignalSpy>
#include <cmath>

#include "Viewer3DCameraController.h"

namespace {

constexpr QSizeF kViewport(800.0, 600.0);
const QPointF kCenterPixel(400.0, 300.0);

bool vectorsClose(const QVector3D& actual, const QVector3D& expected, float epsilon = 0.5f)
{
    return (std::fabs(actual.x() - expected.x()) < epsilon) && (std::fabs(actual.y() - expected.y()) < epsilon) &&
           (std::fabs(actual.z() - expected.z()) < epsilon);
}

bool scalarsClose(qreal actual, qreal expected, qreal epsilon = 1e-6)
{
    return std::fabs(actual - expected) < epsilon;
}

}  // namespace

void Viewer3DCameraControllerTest::_testDefaults()
{
    Viewer3DCameraController controller;

    QCOMPARE(controller.orbitCenter(), QVector3D(0, 0, 0));
    QCOMPARE(controller.heading(), 0.0);
    QCOMPARE(controller.tilt(), 0.0);
    QCOMPARE(controller.distance(), Viewer3DCameraController::kDefaultDistance);
    QCOMPARE(controller.fieldOfView(), Viewer3DCameraController::kDefaultFieldOfView);
    QVERIFY(!controller.viewportSize().isValid() || controller.viewportSize().isEmpty());
}

void Viewer3DCameraControllerTest::_testReset()
{
    Viewer3DCameraController controller;

    controller.lookAt(QVector3D(100, 200, 0), 45.0, 60.0, 3000.0);
    controller.reset();

    QCOMPARE(controller.orbitCenter(), QVector3D(0, 0, 0));
    QCOMPARE(controller.heading(), 0.0);
    QCOMPARE(controller.tilt(), 0.0);
    QCOMPARE(controller.distance(), Viewer3DCameraController::kDefaultDistance);
}

void Viewer3DCameraControllerTest::_testLookAt()
{
    Viewer3DCameraController controller;

    QSignalSpy centerSpy(&controller, &Viewer3DCameraController::orbitCenterChanged);
    QSignalSpy headingSpy(&controller, &Viewer3DCameraController::headingChanged);
    QSignalSpy tiltSpy(&controller, &Viewer3DCameraController::tiltChanged);
    QSignalSpy distanceSpy(&controller, &Viewer3DCameraController::distanceChanged);

    controller.lookAt(QVector3D(10, 20, 0), 45.0, 60.0, 2000.0);

    QCOMPARE(controller.orbitCenter(), QVector3D(10, 20, 0));
    QCOMPARE(controller.heading(), 45.0);
    QCOMPARE(controller.tilt(), 60.0);
    QCOMPARE(controller.distance(), 2000.0);
    QCOMPARE(centerSpy.count(), 1);
    QCOMPARE(headingSpy.count(), 1);
    QCOMPARE(tiltSpy.count(), 1);
    QCOMPARE(distanceSpy.count(), 1);
}

void Viewer3DCameraControllerTest::_testLookAtClamps()
{
    Viewer3DCameraController controller;

    controller.lookAt(QVector3D(), 0.0, 100.0, 10.0);
    QCOMPARE(controller.tilt(), Viewer3DCameraController::kMaxTilt);
    QCOMPARE(controller.distance(), Viewer3DCameraController::kMinDistance);

    controller.lookAt(QVector3D(), 0.0, -10.0, 1e9);
    QCOMPARE(controller.tilt(), Viewer3DCameraController::kMinTilt);
    QCOMPARE(controller.distance(), Viewer3DCameraController::kMaxDistance);
}

void Viewer3DCameraControllerTest::_testTiltSetterClamp()
{
    Viewer3DCameraController controller;

    controller.setTilt(-10.0);
    QCOMPARE(controller.tilt(), Viewer3DCameraController::kMinTilt);

    controller.setTilt(100.0);
    QCOMPARE(controller.tilt(), Viewer3DCameraController::kMaxTilt);
}

void Viewer3DCameraControllerTest::_testDistanceSetterClamp()
{
    Viewer3DCameraController controller;

    controller.setDistance(1.0);
    QCOMPARE(controller.distance(), Viewer3DCameraController::kMinDistance);

    controller.setDistance(1e9);
    QCOMPARE(controller.distance(), Viewer3DCameraController::kMaxDistance);
}

void Viewer3DCameraControllerTest::_testCameraPositionTopDown()
{
    Viewer3DCameraController controller;

    controller.lookAt(QVector3D(0, 0, 0), 0.0, 0.0, 1000.0);
    QVERIFY(vectorsClose(controller.cameraPosition(), QVector3D(0, 0, 1000)));
}

void Viewer3DCameraControllerTest::_testCameraPositionTilted()
{
    Viewer3DCameraController controller;

    // P = C + d * (sin(t)sin(h), -sin(t)cos(h), cos(t))
    controller.lookAt(QVector3D(0, 0, 0), 0.0, 60.0, 1000.0);
    QVERIFY(vectorsClose(controller.cameraPosition(), QVector3D(0, -866.025f, 500)));
}

void Viewer3DCameraControllerTest::_testCameraPositionHeading()
{
    Viewer3DCameraController controller;

    controller.lookAt(QVector3D(0, 0, 0), 90.0, 60.0, 1000.0);
    QVERIFY(vectorsClose(controller.cameraPosition(), QVector3D(866.025f, 0, 500)));
}

void Viewer3DCameraControllerTest::_testScreenToGroundNoViewport()
{
    Viewer3DCameraController controller;

    QVERIFY(!controller.screenToGround(kCenterPixel).has_value());
}

void Viewer3DCameraControllerTest::_testScreenToGroundCenter()
{
    Viewer3DCameraController controller;
    controller.setViewportSize(kViewport);

    // Center pixel ray passes through the orbit center, which lies on the ground plane
    controller.lookAt(QVector3D(100, -50, 0), 30.0, 55.0, 1200.0);

    const auto ground = controller.screenToGround(kCenterPixel);
    QVERIFY(ground.has_value());
    QVERIFY(vectorsClose(*ground, QVector3D(100, -50, 0)));
}

void Viewer3DCameraControllerTest::_testScreenToGroundOffCenter()
{
    Viewer3DCameraController controller;
    controller.setViewportSize(kViewport);

    // Top-down from 1000: right edge center row lands at x = h * tan(fov/2) * aspect
    controller.lookAt(QVector3D(0, 0, 0), 0.0, 0.0, 1000.0);

    const auto ground = controller.screenToGround(QPointF(800, 300));
    QVERIFY(ground.has_value());
    const float expectedX = 1000.0f * std::tan(qDegreesToRadians(30.0f)) * (800.0f / 600.0f);
    QVERIFY(vectorsClose(*ground, QVector3D(expectedX, 0, 0), 1.0f));
}

void Viewer3DCameraControllerTest::_testScreenToGroundMiss()
{
    Viewer3DCameraController controller;
    controller.setViewportSize(kViewport);

    // Near-horizontal view: ray through the top of the screen points above the horizon
    controller.lookAt(QVector3D(0, 0, 0), 0.0, 85.0, 1000.0);

    QVERIFY(!controller.screenToGround(QPointF(400, 0)).has_value());
}

void Viewer3DCameraControllerTest::_testPanKeepsAnchorUnderCursor()
{
    Viewer3DCameraController controller;
    controller.setViewportSize(kViewport);
    controller.lookAt(QVector3D(0, 0, 0), 20.0, 60.0, 1000.0);

    const QPointF startPos(400, 300);
    const QPointF endPos(450, 330);

    const auto anchor = controller.screenToGround(startPos);
    QVERIFY(anchor.has_value());

    controller.beginPan(startPos);
    controller.panTo(endPos);

    const auto groundUnderCursor = controller.screenToGround(endPos);
    QVERIFY(groundUnderCursor.has_value());
    QVERIFY(vectorsClose(*groundUnderCursor, *anchor));
}

void Viewer3DCameraControllerTest::_testPanPreservesPose()
{
    Viewer3DCameraController controller;
    controller.setViewportSize(kViewport);
    controller.lookAt(QVector3D(0, 0, 0), 20.0, 60.0, 1000.0);

    controller.beginPan(QPointF(400, 300));
    controller.panTo(QPointF(500, 350));

    QCOMPARE(controller.heading(), 20.0);
    QCOMPARE(controller.tilt(), 60.0);
    QCOMPARE(controller.distance(), 1000.0);
    QVERIFY(!vectorsClose(controller.orbitCenter(), QVector3D(0, 0, 0), 1.0f));
}

void Viewer3DCameraControllerTest::_testOrbitYawAngle()
{
    Viewer3DCameraController controller;
    controller.setViewportSize(kViewport);
    controller.lookAt(QVector3D(0, 0, 0), 0.0, 30.0, 1000.0);

    // Full viewport width = 360 deg: quarter width = 90 deg
    controller.beginOrbit(kCenterPixel);
    controller.orbitTo(QPointF(600, 300));

    QVERIFY(scalarsClose(controller.heading(), 90.0));
    QVERIFY(scalarsClose(controller.tilt(), 30.0));
    QVERIFY(scalarsClose(controller.distance(), 1000.0));
    // Anchor was the orbit center itself: it must not move
    QVERIFY(vectorsClose(controller.orbitCenter(), QVector3D(0, 0, 0)));
}

void Viewer3DCameraControllerTest::_testOrbitTiltAngle()
{
    Viewer3DCameraController controller;
    controller.setViewportSize(kViewport);
    controller.lookAt(QVector3D(0, 0, 0), 0.0, 30.0, 1000.0);

    // Full viewport height = 180 deg: quarter height drag up = +45 deg tilt
    controller.beginOrbit(kCenterPixel);
    controller.orbitTo(QPointF(400, 150));

    QVERIFY(scalarsClose(controller.tilt(), 75.0));
    QVERIFY(scalarsClose(controller.heading(), 0.0));
    QVERIFY(scalarsClose(controller.distance(), 1000.0));
}

void Viewer3DCameraControllerTest::_testOrbitTiltClamped()
{
    Viewer3DCameraController controller;
    controller.setViewportSize(kViewport);
    controller.lookAt(QVector3D(0, 0, 0), 0.0, 30.0, 1000.0);

    // Half viewport height drag up = +90 deg requested: clamps at kMaxTilt
    controller.beginOrbit(kCenterPixel);
    controller.orbitTo(QPointF(400, 0));

    QCOMPARE(controller.tilt(), Viewer3DCameraController::kMaxTilt);
}

void Viewer3DCameraControllerTest::_testOrbitOffCenterAnchor()
{
    Viewer3DCameraController controller;
    controller.setViewportSize(kViewport);
    controller.lookAt(QVector3D(0, 0, 0), 0.0, 45.0, 1000.0);

    const QPointF startPos(500, 400);
    const auto anchor = controller.screenToGround(startPos);
    QVERIFY(anchor.has_value());

    controller.beginOrbit(startPos);
    controller.orbitTo(QPointF(560, 360));

    // The picked ground point must stay at its original screen position
    const auto groundAtStart = controller.screenToGround(startPos);
    QVERIFY(groundAtStart.has_value());
    QVERIFY(vectorsClose(*groundAtStart, *anchor, 1.0f));

    QVERIFY(scalarsClose(controller.heading(), 60.0 / 800.0 * 360.0));
    QVERIFY(scalarsClose(controller.tilt(), 45.0 + (40.0 / 600.0 * 180.0)));
}

void Viewer3DCameraControllerTest::_testOrbitPreservesDistance()
{
    Viewer3DCameraController controller;
    controller.setViewportSize(kViewport);
    controller.lookAt(QVector3D(0, 0, 0), 0.0, 45.0, 1000.0);

    controller.beginOrbit(QPointF(500, 400));
    controller.orbitTo(QPointF(560, 360));

    QVERIFY(scalarsClose(controller.distance(), 1000.0));
}

void Viewer3DCameraControllerTest::_testRotateByChangesHeading()
{
    Viewer3DCameraController controller;
    controller.setViewportSize(kViewport);
    controller.lookAt(QVector3D(0, 0, 0), 0.0, 30.0, 1000.0);

    // Twist about the screen center: anchor is the orbit center, so only heading changes
    controller.rotateBy(45.0, kCenterPixel);

    QVERIFY(scalarsClose(controller.heading(), 45.0));
    QVERIFY(scalarsClose(controller.tilt(), 30.0));
    QVERIFY(scalarsClose(controller.distance(), 1000.0));
    QVERIFY(vectorsClose(controller.orbitCenter(), QVector3D(0, 0, 0)));
}

void Viewer3DCameraControllerTest::_testRotateByKeepsAnchorUnderTouch()
{
    Viewer3DCameraController controller;
    controller.setViewportSize(kViewport);
    controller.lookAt(QVector3D(0, 0, 0), 0.0, 45.0, 1000.0);

    const QPointF touchPos(500, 400);
    const auto anchor = controller.screenToGround(touchPos);
    QVERIFY(anchor.has_value());

    controller.rotateBy(30.0, touchPos);

    // The ground point under the gesture centroid must stay at that screen position
    const auto groundAfter = controller.screenToGround(touchPos);
    QVERIFY(groundAfter.has_value());
    QVERIFY(vectorsClose(*groundAfter, *anchor, 1.0f));
    QVERIFY(scalarsClose(controller.heading(), 30.0));
    QVERIFY(scalarsClose(controller.distance(), 1000.0));
}

void Viewer3DCameraControllerTest::_testTiltByChangesTilt()
{
    Viewer3DCameraController controller;
    controller.setViewportSize(kViewport);
    controller.lookAt(QVector3D(0, 0, 0), 0.0, 30.0, 1000.0);

    controller.tiltBy(20.0, kCenterPixel);

    QVERIFY(scalarsClose(controller.tilt(), 50.0));
    QVERIFY(scalarsClose(controller.heading(), 0.0));
    QVERIFY(scalarsClose(controller.distance(), 1000.0));
    QVERIFY(vectorsClose(controller.orbitCenter(), QVector3D(0, 0, 0)));
}

void Viewer3DCameraControllerTest::_testTiltByClamped()
{
    Viewer3DCameraController controller;
    controller.setViewportSize(kViewport);
    controller.lookAt(QVector3D(0, 0, 0), 0.0, 30.0, 1000.0);

    controller.tiltBy(1000.0, kCenterPixel);
    QCOMPARE(controller.tilt(), Viewer3DCameraController::kMaxTilt);

    controller.tiltBy(-1000.0, kCenterPixel);
    QCOMPARE(controller.tilt(), Viewer3DCameraController::kMinTilt);
}

void Viewer3DCameraControllerTest::_testTiltByKeepsAnchorUnderTouch()
{
    Viewer3DCameraController controller;
    controller.setViewportSize(kViewport);
    controller.lookAt(QVector3D(0, 0, 0), 0.0, 45.0, 1000.0);

    const QPointF touchPos(500, 400);
    const auto anchor = controller.screenToGround(touchPos);
    QVERIFY(anchor.has_value());

    controller.tiltBy(15.0, touchPos);

    const auto groundAfter = controller.screenToGround(touchPos);
    QVERIFY(groundAfter.has_value());
    QVERIFY(vectorsClose(*groundAfter, *anchor, 1.0f));
    QVERIFY(scalarsClose(controller.tilt(), 60.0));
    QVERIFY(scalarsClose(controller.distance(), 1000.0));
}

void Viewer3DCameraControllerTest::_testZoomChangesDistance()
{
    Viewer3DCameraController controller;
    controller.setViewportSize(kViewport);
    controller.lookAt(QVector3D(0, 0, 0), 0.0, 60.0, 1000.0);

    controller.zoom(120.0, kCenterPixel);
    QVERIFY(controller.distance() < 1000.0);

    controller.lookAt(QVector3D(0, 0, 0), 0.0, 60.0, 1000.0);
    controller.zoom(-120.0, kCenterPixel);
    QVERIFY(controller.distance() > 1000.0);
}

void Viewer3DCameraControllerTest::_testZoomRoundTrip()
{
    Viewer3DCameraController controller;
    controller.setViewportSize(kViewport);
    controller.lookAt(QVector3D(0, 0, 0), 0.0, 60.0, 1000.0);

    controller.zoom(120.0, kCenterPixel);
    controller.zoom(-120.0, kCenterPixel);

    QVERIFY(scalarsClose(controller.distance(), 1000.0, 0.5));
    QVERIFY(vectorsClose(controller.orbitCenter(), QVector3D(0, 0, 0)));
}

void Viewer3DCameraControllerTest::_testZoomByFactor()
{
    Viewer3DCameraController controller;
    controller.setViewportSize(kViewport);
    controller.lookAt(QVector3D(0, 0, 0), 0.0, 60.0, 1000.0);

    const QPointF cursorPos(550, 380);
    const auto anchor = controller.screenToGround(cursorPos);
    QVERIFY(anchor.has_value());

    controller.zoomBy(0.5, cursorPos);

    QVERIFY(scalarsClose(controller.distance(), 500.0, 0.5));
    const auto groundUnderCursor = controller.screenToGround(cursorPos);
    QVERIFY(groundUnderCursor.has_value());
    QVERIFY(vectorsClose(*groundUnderCursor, *anchor, 1.0f));
}

void Viewer3DCameraControllerTest::_testZoomTowardCursorKeepsAnchor()
{
    Viewer3DCameraController controller;
    controller.setViewportSize(kViewport);
    controller.lookAt(QVector3D(0, 0, 0), 0.0, 60.0, 1000.0);

    const QPointF cursorPos(550, 380);
    const auto anchor = controller.screenToGround(cursorPos);
    QVERIFY(anchor.has_value());

    controller.zoom(240.0, cursorPos);

    const auto groundUnderCursor = controller.screenToGround(cursorPos);
    QVERIFY(groundUnderCursor.has_value());
    QVERIFY(vectorsClose(*groundUnderCursor, *anchor, 1.0f));
}

void Viewer3DCameraControllerTest::_testZoomClamp()
{
    Viewer3DCameraController controller;
    controller.setViewportSize(kViewport);
    controller.lookAt(QVector3D(0, 0, 0), 0.0, 60.0, 1000.0);

    controller.zoom(1e6, kCenterPixel);
    QCOMPARE(controller.distance(), Viewer3DCameraController::kMinDistance);

    controller.lookAt(QVector3D(0, 0, 0), 0.0, 60.0, 1000.0);
    controller.zoom(-1e6, kCenterPixel);
    QCOMPARE(controller.distance(), Viewer3DCameraController::kMaxDistance);
}

void Viewer3DCameraControllerTest::_testZoomAboveHorizonFallback()
{
    Viewer3DCameraController controller;
    controller.setViewportSize(kViewport);
    controller.lookAt(QVector3D(0, 0, 0), 0.0, 85.0, 1000.0);

    // Cursor ray misses the ground: zoom still works (about the orbit center)
    controller.zoom(120.0, QPointF(400, 0));

    QVERIFY(controller.distance() < 1000.0);
    QVERIFY(vectorsClose(controller.orbitCenter(), QVector3D(0, 0, 0)));
}

void Viewer3DCameraControllerTest::_testSceneUnitsPerPixel()
{
    Viewer3DCameraController controller;

    QCOMPARE(controller.sceneUnitsPerPixel(), 0.0);

    controller.setViewportSize(kViewport);
    controller.lookAt(QVector3D(0, 0, 0), 0.0, 60.0, 1000.0);

    const qreal expected = 2.0 * 1000.0 * std::tan(qDegreesToRadians(30.0)) / 600.0;
    QVERIFY(scalarsClose(controller.sceneUnitsPerPixel(), expected, 1e-3));

    controller.setDistance(2000.0);
    QVERIFY(scalarsClose(controller.sceneUnitsPerPixel(), 2.0 * expected, 1e-3));
}

void Viewer3DCameraControllerTest::_testGesturesWithoutViewportNoop()
{
    Viewer3DCameraController controller;
    controller.lookAt(QVector3D(10, 20, 0), 30.0, 40.0, 1000.0);

    controller.beginPan(kCenterPixel);
    controller.panTo(QPointF(500, 350));
    controller.beginOrbit(kCenterPixel);
    controller.orbitTo(QPointF(500, 350));
    controller.zoom(120.0, kCenterPixel);

    QCOMPARE(controller.orbitCenter(), QVector3D(10, 20, 0));
    QCOMPARE(controller.heading(), 30.0);
    QCOMPARE(controller.tilt(), 40.0);
    QCOMPARE(controller.distance(), 1000.0);
}

UT_REGISTER_TEST(Viewer3DCameraControllerTest, TestLabel::Unit)
