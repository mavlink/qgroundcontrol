#include "GeoMapCameraTest.h"

#include <QtCore/QtMath>
#include <QtGui/QQuaternion>
#include <QtGui/QVector3D>
#include <QtTest/QSignalSpy>

#include <cmath>

#include "GeoMapCamera.h"
#include "TileMath.h"

namespace {

constexpr QSizeF kViewport(800, 600);
const QGeoCoordinate kCenter(47.3977419, 8.5455938);

// World-meter tolerance for anchor invariants (double-precision math throughout)
constexpr double kWorldEpsilon = 1e-3;

void setupCamera(GeoMapCamera& camera, qreal tilt = 0.0, qreal heading = 0.0)
{
    camera.setViewportSize(kViewport);
    // Gesture tests exercise the full pose freedom, so run unlocked
    camera.setMode(GeoMapCamera::Mode::Mode3D);
    camera.lookAt(kCenter, heading, tilt, GeoMapCamera::kDefaultDistance);
}

double groundDistance(const QPointF& a, const QPointF& b)
{
    return std::hypot(a.x() - b.x(), a.y() - b.y());
}

}  // namespace

void GeoMapCameraTest::_defaults()
{
    const GeoMapCamera camera;
    QCOMPARE(camera.heading(), 0.0);
    QCOMPARE(camera.tilt(), 0.0);
    QCOMPARE(camera.distance(), GeoMapCamera::kDefaultDistance);
    QCOMPARE(camera.fieldOfView(), GeoMapCamera::kDefaultFieldOfView);
    QVERIFY(camera.isTopDown());
    QCOMPARE(camera.mode(), GeoMapCamera::Mode::Mode2D);
    QVERIFY(!camera.isPositioned());
}

void GeoMapCameraTest::_positionedOnExplicitCenter()
{
    GeoMapCamera camera;
    QSignalSpy centerSpy(&camera, &GeoMapCamera::centerChanged);

    // Setting the center to the default location still counts as positioning
    // and must announce it, so consumers gated on isPositioned wake up
    camera.setCenter(QGeoCoordinate(0, 0));
    QVERIFY(camera.isPositioned());
    QCOMPARE(centerSpy.count(), 1);
}

void GeoMapCameraTest::_clamps()
{
    GeoMapCamera camera;

    camera.setTilt(-10);
    QCOMPARE(camera.tilt(), GeoMapCamera::kMinTilt);
    camera.setTilt(100);
    QCOMPARE(camera.tilt(), GeoMapCamera::kMaxTilt);

    camera.setDistance(0);
    QCOMPARE(camera.distance(), GeoMapCamera::kMinDistance);
    camera.setDistance(1e12);
    QCOMPARE(camera.distance(), GeoMapCamera::kMaxDistance);

    camera.setHeading(370);
    QCOMPARE(camera.heading(), 10.0);
    camera.setHeading(-10);
    QCOMPARE(camera.heading(), 350.0);
    // Exactly 360 wraps to 0 (the compass reset animation ends at 360)
    camera.setHeading(360);
    QCOMPARE(camera.heading(), 0.0);
}

void GeoMapCameraTest::_fieldOfView()
{
    GeoMapCamera camera;
    QSignalSpy fovSpy(&camera, &GeoMapCamera::fieldOfViewChanged);

    camera.setFieldOfView(90);
    QCOMPARE(camera.fieldOfView(), 90.0);
    QCOMPARE(fovSpy.count(), 1);

    camera.setFieldOfView(90);
    QCOMPARE(fovSpy.count(), 1);  // no-op write does not re-signal

    camera.setFieldOfView(5);
    QCOMPARE(camera.fieldOfView(), 10.0);
    camera.setFieldOfView(200);
    QCOMPARE(camera.fieldOfView(), 120.0);
}

void GeoMapCameraTest::_reset()
{
    GeoMapCamera camera;
    setupCamera(camera, 40, 120);
    camera.setDistance(50000);

    camera.reset();

    QCOMPARE(camera.heading(), 0.0);
    QCOMPARE(camera.tilt(), GeoMapCamera::kMinTilt);
    QCOMPARE(camera.distance(), GeoMapCamera::kDefaultDistance);
    // Reset restores the default pose but keeps the location
    QCOMPARE_LT(camera.center().distanceTo(kCenter), 1.0);
}

void GeoMapCameraTest::_cameraPositionConvention()
{
    GeoMapCamera camera;
    camera.setCenter(QGeoCoordinate(0, 0));
    camera.setDistance(1000);

    // Top-down: directly above the center
    camera.setTilt(0);
    QVector3D pos = camera.cameraPosition();
    QCOMPARE_LT(qAbs(pos.x()), 1e-3f);
    QCOMPARE_LT(qAbs(pos.y()), 1e-3f);
    QCOMPARE_LT(qAbs(pos.z() - 1000.0f), 1e-3f);

    // Tilt 30 (sin != cos, catches sin/cos transpositions), heading 0:
    // camera moves south and down-range
    camera.setTilt(30);
    pos = camera.cameraPosition();
    const float expectedSin = static_cast<float>(1000.0 * std::sin(qDegreesToRadians(30.0)));
    const float expectedCos = static_cast<float>(1000.0 * std::cos(qDegreesToRadians(30.0)));
    QCOMPARE_LT(qAbs(pos.x()), 1e-3f);
    QCOMPARE_LT(qAbs(pos.y() + expectedSin), 1e-3f);
    QCOMPARE_LT(qAbs(pos.z() - expectedCos), 1e-3f);

    // Heading 90 rotates the camera offset into +x
    camera.setHeading(90);
    pos = camera.cameraPosition();
    QCOMPARE_LT(qAbs(pos.x() - expectedSin), 1e-3f);
    QCOMPARE_LT(qAbs(pos.y()), 1e-3f);

    // Double-precision ground projection follows the same convention
    const QPointF ground = camera.cameraGroundPosition();
    QCOMPARE_LT(qAbs(ground.x() - expectedSin), 1e-6);
    QCOMPARE_LT(qAbs(ground.y()), 1e-6);
}

void GeoMapCameraTest::_screenToGroundTopDown()
{
    GeoMapCamera camera;
    setupCamera(camera);

    // Screen center hits the camera center on the ground
    const auto center = camera.screenToGround(QPointF(kViewport.width() / 2, kViewport.height() / 2));
    QVERIFY(center.has_value());
    QCOMPARE_LT(groundDistance(*center, TileMath::geoToWorld(kCenter)), kWorldEpsilon);

    // Up-left on screen is north-west on the ground in top-down at heading 0
    const auto upLeft = camera.screenToGround(QPointF(100, 100));
    QVERIFY(upLeft.has_value());
    QCOMPARE_LT(upLeft->x(), center->x());
    QCOMPARE_GT(upLeft->y(), center->y());
}

void GeoMapCameraTest::_screenToGroundHorizonMiss()
{
    GeoMapCamera camera;
    setupCamera(camera, GeoMapCamera::kMaxTilt);

    // At max tilt the ray through the top of the screen points above the horizon
    QVERIFY(!camera.screenToGround(QPointF(kViewport.width() / 2, 0)).has_value());
    // The bottom of the screen still hits the ground
    QVERIFY(camera.screenToGround(QPointF(kViewport.width() / 2, kViewport.height())).has_value());
}

void GeoMapCameraTest::_screenToGroundNoViewport()
{
    GeoMapCamera camera;
    QVERIFY(!camera.screenToGround(QPointF(100, 100)).has_value());
    QCOMPARE(camera.sceneUnitsPerPixel(), 0.0);

    // Gestures without a viewport must not crash or move the camera
    const QGeoCoordinate before = camera.center();
    camera.beginPan(QPointF(1, 1));
    camera.panTo(QPointF(50, 50));
    camera.zoomBy(0.5, QPointF(1, 1));
    QCOMPARE_LT(qAbs(camera.center().latitude() - before.latitude()), 1e-12);
}

void GeoMapCameraTest::_worldToScreen()
{
    // No viewport: the only unconditional failure mode
    {
        const GeoMapCamera camera;
        QVERIFY(!camera.worldToScreen(QPointF(0, 0)).has_value());
    }

    GeoMapCamera camera;
    setupCamera(camera);
    const QPointF centerWorld = TileMath::geoToWorld(kCenter);
    const QPointF screenCenter(kViewport.width() / 2.0, kViewport.height() / 2.0);

    // Top-down: the look-at center projects to the viewport center
    const auto projected = camera.worldToScreen(centerWorld);
    QVERIFY(projected.has_value());
    QCOMPARE_LT(groundDistance(*projected, screenCenter), 0.01);

    // A point directly above the camera is behind the view plane
    QVERIFY(!camera.worldToScreen(centerWorld, camera.distance() * 2).has_value());

    // Inverse of screenToGround at arbitrary screen points and tilts
    for (qreal tilt : {0.0, 30.0, 55.0}) {
        camera.lookAt(kCenter, 42, tilt, 3000);
        for (const QPointF& screenPos :
             {QPointF(200, 150), QPointF(650, 480), QPointF(kViewport.width() / 2.0, kViewport.height() / 2.0)}) {
            const auto ground = camera.screenToGround(screenPos);
            QVERIFY(ground.has_value());
            const auto roundTrip = camera.worldToScreen(*ground);
            QVERIFY(roundTrip.has_value());
            QCOMPARE_LT(groundDistance(*roundTrip, screenPos), 0.01);
        }
    }
}

void GeoMapCameraTest::_groundPointCapped()
{
    // No viewport: the only failure mode
    {
        const GeoMapCamera camera;
        QVERIFY(!camera.groundPointCapped(QPointF(100, 100), 1000.0).has_value());
    }

    GeoMapCamera camera;
    setupCamera(camera, GeoMapCamera::kMaxTilt);
    // Double precision (cameraPosition() is float and loses meters at world scale)
    const QPointF cameraGround = camera.cameraGroundPosition();
    // At tilt 85 (fov 60, 600px viewport) the horizon is at y~254: the screen bottom
    // hits at ~190m, y=280 hits at ~2.7km. maxRange=2km separates the cases robustly.
    const double maxRange = 2000.0;

    // Hit within range: agrees exactly with screenToGround
    const QPointF bottomCenter(kViewport.width() / 2, kViewport.height());
    const auto hit = camera.screenToGround(bottomCenter);
    const auto capped = camera.groundPointCapped(bottomCenter, maxRange);
    QVERIFY(hit.has_value());
    QVERIFY(capped.has_value());
    QCOMPARE_LT(groundDistance(*capped, *hit), kWorldEpsilon);

    // Near-horizon hit beyond maxRange: pulled back to exactly maxRange from the camera ground position
    const QPointF nearHorizon(kViewport.width() / 2, 280);
    const auto farHit = camera.screenToGround(nearHorizon);
    QVERIFY(farHit.has_value());
    QCOMPARE_GT(groundDistance(*farHit, cameraGround), maxRange);
    const auto farCapped = camera.groundPointCapped(nearHorizon, maxRange);
    QVERIFY(farCapped.has_value());
    QCOMPARE_LT(qAbs(groundDistance(*farCapped, cameraGround) - maxRange), kWorldEpsilon);

    // Horizon miss: capped at maxRange along the ray's horizontal direction (north at heading 0)
    const QPointF topCenter(kViewport.width() / 2, 0);
    QVERIFY(!camera.screenToGround(topCenter).has_value());
    const auto missCapped = camera.groundPointCapped(topCenter, maxRange);
    QVERIFY(missCapped.has_value());
    QCOMPARE_LT(qAbs(groundDistance(*missCapped, cameraGround) - maxRange), kWorldEpsilon);
    QCOMPARE_GT(missCapped->y(), cameraGround.y());  // points north
    QCOMPARE_LT(qAbs(missCapped->x() - cameraGround.x()), kWorldEpsilon);
}

void GeoMapCameraTest::_sceneUnitsPerPixel()
{
    GeoMapCamera camera;
    setupCamera(camera);

    // Top-down: vertical fov spans 2*d*tan(fov/2) world meters over the viewport height
    const double expected =
        (2.0 * camera.distance() * std::tan(qDegreesToRadians(camera.fieldOfView()) / 2.0)) / kViewport.height();
    QCOMPARE_LT(qAbs(camera.sceneUnitsPerPixel() - expected), expected * 0.01);
}

void GeoMapCameraTest::_panAnchorInvariant_data()
{
    QTest::addColumn<double>("tilt");
    QTest::newRow("top-down") << 0.0;
    QTest::newRow("tilted 45") << 45.0;
}

void GeoMapCameraTest::_panAnchorInvariant()
{
    QFETCH(double, tilt);

    GeoMapCamera camera;
    setupCamera(camera, tilt);

    const QPointF pressPos(200, 400);
    const QPointF dragPos(450, 320);

    const auto anchor = camera.screenToGround(pressPos);
    QVERIFY(anchor.has_value());

    camera.beginPan(pressPos);
    camera.panTo(dragPos);

    // The ground point picked at press is now under the drag position
    const auto current = camera.screenToGround(dragPos);
    QVERIFY(current.has_value());
    QCOMPARE_LT(groundDistance(*current, *anchor), kWorldEpsilon);

    // Orientation unchanged by a pan
    QCOMPARE(camera.tilt(), tilt);
    QCOMPARE(camera.heading(), 0.0);
}

void GeoMapCameraTest::_zoomAnchorInvariant()
{
    GeoMapCamera camera;
    setupCamera(camera);

    const QPointF zoomPos(600, 150);
    const auto anchor = camera.screenToGround(zoomPos);
    QVERIFY(anchor.has_value());

    const qreal startDistance = camera.distance();
    camera.zoomBy(0.5, zoomPos);
    QCOMPARE(camera.distance(), startDistance * 0.5);

    const auto current = camera.screenToGround(zoomPos);
    QVERIFY(current.has_value());
    QCOMPARE_LT(groundDistance(*current, *anchor), kWorldEpsilon);
}

void GeoMapCameraTest::_rotateAnchorInvariant()
{
    GeoMapCamera camera;
    setupCamera(camera, 30.0);

    const QPointF twistPos(500, 400);
    const auto anchor = camera.screenToGround(twistPos);
    QVERIFY(anchor.has_value());

    camera.rotateBy(35.0, twistPos);
    QCOMPARE(camera.heading(), 35.0);

    const auto current = camera.screenToGround(twistPos);
    QVERIFY(current.has_value());
    QCOMPARE_LT(groundDistance(*current, *anchor), kWorldEpsilon);
}

void GeoMapCameraTest::_tiltAnchorInvariant()
{
    GeoMapCamera camera;
    setupCamera(camera, 20.0);

    const QPointF swipePos(400, 350);
    const auto anchor = camera.screenToGround(swipePos);
    QVERIFY(anchor.has_value());

    camera.tiltBy(25.0, swipePos);
    QCOMPARE(camera.tilt(), 45.0);

    const auto current = camera.screenToGround(swipePos);
    QVERIFY(current.has_value());
    QCOMPARE_LT(groundDistance(*current, *anchor), kWorldEpsilon);
}

void GeoMapCameraTest::_orbitDragRatios()
{
    GeoMapCamera camera;
    setupCamera(camera, 30.0);

    const QPointF start(kViewport.width() / 2, kViewport.height() / 2);
    camera.beginOrbit(start);

    // Half viewport width = 180 deg heading
    camera.orbitTo(start + QPointF(kViewport.width() / 2, 0));
    QCOMPARE_LT(qAbs(camera.heading() - 180.0), 1e-9);
    QCOMPARE(camera.tilt(), 30.0);

    // Additionally drag up a quarter of the height: tilt +45 (clamped at kMaxTilt)
    camera.orbitTo(start + QPointF(kViewport.width() / 2, -kViewport.height() / 4));
    QCOMPARE_LT(qAbs(camera.tilt() - 75.0), 1e-9);

    // Anchor stays pinned to its screen position from gesture start
    const auto anchorNow = camera.screenToGround(start);
    QVERIFY(anchorNow.has_value());
    const auto expected = TileMath::geoToWorld(kCenter);
    QCOMPARE_LT(groundDistance(*anchorNow, expected), kWorldEpsilon);
}

void GeoMapCameraTest::_modeSwitch()
{
    GeoMapCamera camera;
    camera.setViewportSize(kViewport);
    camera.lookAt(kCenter, 0, 0, GeoMapCamera::kDefaultDistance);

    QCOMPARE(camera.mode(), GeoMapCamera::Mode::Mode2D);
    QVERIFY(camera.isTopDown());
    camera.goTo3D();
    QCOMPARE(camera.mode(), GeoMapCamera::Mode::Mode3D);
    QVERIFY(!camera.isTopDown());
    QCOMPARE(camera.tilt(), GeoMapCamera::kDefault3DTilt);
    camera.goTo2D();
    QCOMPARE(camera.mode(), GeoMapCamera::Mode::Mode2D);
    QVERIFY(camera.isTopDown());
}

void GeoMapCameraTest::_tiltLockedIn2D()
{
    GeoMapCamera camera;
    camera.setViewportSize(kViewport);
    camera.lookAt(kCenter, 0, 0, GeoMapCamera::kDefaultDistance);
    QCOMPARE(camera.mode(), GeoMapCamera::Mode::Mode2D);

    const QPointF center(kViewport.width() / 2, kViewport.height() / 2);

    // Tilt gestures are inert in 2D
    camera.tiltBy(25.0, center);
    QCOMPARE(camera.tilt(), 0.0);

    // Orbit still rotates heading but cannot pitch the map
    camera.beginOrbit(center);
    camera.orbitTo(center + QPointF(kViewport.width() / 4, -kViewport.height() / 4));
    QCOMPARE_LT(qAbs(camera.heading() - 90.0), 0.5);
    QCOMPARE(camera.tilt(), 0.0);

    // setTilt stays un-gated: the QML mode-transition animation drives it
    camera.setTilt(30);
    QCOMPARE(camera.tilt(), 30.0);

    // Unlocked in 3D
    camera.setTilt(0);
    camera.setMode(GeoMapCamera::Mode::Mode3D);
    camera.tiltBy(25.0, center);
    QCOMPARE(camera.tilt(), 25.0);
}

void GeoMapCameraTest::_scenePose()
{
    GeoMapCamera camera;
    camera.setViewportSize(kViewport);
    camera.lookAt(kCenter, 0, 0, 1000);
    camera.setSceneOrigin(TileMath::geoToWorld(kCenter));

    // Top-down with origin at center: directly overhead, identity rotation
    QVector3D pos = camera.scenePosition();
    QCOMPARE_LT(qAbs(pos.x()), 1e-3f);
    QCOMPARE_LT(qAbs(pos.y()), 1e-3f);
    QCOMPARE_LT(qAbs(pos.z() - 1000.0f), 1e-3f);
    QVERIFY(qFuzzyCompare(camera.sceneRotation(), QQuaternion()));

    // The rotation must map the camera's -z look direction onto the pose's view
    // direction: at tilt 30 (sin != cos, catches sin/cos transpositions) heading 0
    // the camera looks north-and-down
    camera.setTilt(30);
    const QVector3D viewDir = camera.sceneRotation().rotatedVector(QVector3D(0, 0, -1));
    const float expectedSin = static_cast<float>(std::sin(qDegreesToRadians(30.0)));
    const float expectedCos = static_cast<float>(std::cos(qDegreesToRadians(30.0)));
    QCOMPARE_LT(qAbs(viewDir.x()), 1e-5f);
    QCOMPARE_LT(qAbs(viewDir.y() - expectedSin), 1e-5f);
    QCOMPARE_LT(qAbs(viewDir.z() + expectedCos), 1e-5f);

    // Position matches the world pose convention relative to the origin
    pos = camera.scenePosition();
    QCOMPARE_LT(qAbs(pos.x()), 1e-3f);
    QCOMPARE_LT(qAbs(pos.y() + (1000.0f * expectedSin)), 1e-3f);
    QCOMPARE_LT(qAbs(pos.z() - (1000.0f * expectedCos)), 1e-3f);

    // Shifting the origin shifts the scene position by the opposite amount
    QSignalSpy poseSpy(&camera, &GeoMapCamera::scenePoseChanged);
    camera.setSceneOrigin(camera.sceneOrigin() + QPointF(500, -250));
    QCOMPARE(poseSpy.count(), 1);
    const QVector3D shifted = camera.scenePosition();
    QCOMPARE_LT(qAbs(shifted.x() - (pos.x() - 500.0f)), 1e-3f);
    QCOMPARE_LT(qAbs(shifted.y() - (pos.y() + 250.0f)), 1e-3f);

    // Every pose component change signals the scene pose
    camera.setHeading(90);
    camera.setDistance(2000);
    camera.setCenter(QGeoCoordinate(48.0, 9.0));
    QCOMPARE(poseSpy.count(), 4);
}

UT_REGISTER_TEST_LIGHTWEIGHT(GeoMapCameraTest, TestLabel::Unit)
