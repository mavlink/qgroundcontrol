#include "GeoSceneTest.h"

#include <QtCore/QPointF>
#include <QtCore/QVariant>
#include <QtCore/qnumeric.h>
#include <QtTest/QSignalSpy>

#include <cmath>

#include "GeoMapCamera.h"
#include "GeoScene.h"
#include "TileMath.h"

namespace {

const QGeoCoordinate kCenter(47.3977419, 8.5455938);
constexpr QSizeF kViewport(800, 600);

void setupCamera(GeoMapCamera& camera, const QGeoCoordinate& center = kCenter)
{
    camera.setViewportSize(kViewport);
    camera.lookAt(center, 0, 0, 2000);
}

double originDistance(const GeoScene& scene, const QPointF& expected)
{
    return std::hypot(scene.sceneOrigin().x() - expected.x(), scene.sceneOrigin().y() - expected.y());
}

}  // namespace

void GeoSceneTest::_originAnchorsOnCameraSet()
{
    // Unanchored scene: well-defined defaults at the mercator origin
    GeoScene scene;
    QCOMPARE(scene.sceneOrigin(), QPointF(0, 0));
    QCOMPARE(scene.verticalScale(), 1.0);
    QCOMPARE_LT(scene.scenePositionFor(QGeoCoordinate(0, 0)).length(), 1.0f);

    GeoMapCamera camera;
    setupCamera(camera);
    scene.setCamera(&camera);

    QCOMPARE_LT(originDistance(scene, TileMath::geoToWorld(kCenter)), 1.0);
}

void GeoSceneTest::_reanchorsEuclidean()
{
    GeoMapCamera camera;
    setupCamera(camera);
    GeoScene scene;
    scene.setCamera(&camera);
    const QPointF startOrigin = scene.sceneOrigin();

    QSignalSpy originSpy(&scene, &GeoScene::sceneOriginChanged);

    // Small move within the threshold: origin holds
    camera.setCenter(TileMath::worldToGeo(startOrigin + QPointF(10000, 10000)));
    QCOMPARE(originSpy.count(), 0);

    // Diagonal move under the threshold per axis but over it in Euclidean
    // distance (40k, 40k) -> ~56.6km: must re-anchor
    const QPointF diagonalWorld = startOrigin + QPointF(40000, 40000);
    camera.setCenter(TileMath::worldToGeo(diagonalWorld));
    QCOMPARE(originSpy.count(), 1);
    QCOMPARE_LT(originDistance(scene, diagonalWorld), 1.0);
}

void GeoSceneTest::_cameraSwapAnchorsFresh()
{
    GeoMapCamera zurichCamera;
    setupCamera(zurichCamera);
    GeoScene scene;
    scene.setCamera(&zurichCamera);

    // Swap to a static camera on another continent: the origin must follow even
    // though the new camera's center never changes after the swap
    GeoMapCamera sydneyCamera;
    const QGeoCoordinate sydney(-33.8688, 151.2093);
    setupCamera(sydneyCamera, sydney);
    scene.setCamera(&sydneyCamera);

    QCOMPARE_LT(originDistance(scene, TileMath::geoToWorld(sydney)), 1.0);
}

void GeoSceneTest::_verticalScaleTracksOrigin()
{
    GeoMapCamera camera;
    setupCamera(camera);
    GeoScene scene;
    scene.setCamera(&camera);

    // Anchored at Zurich: 1/cos(47.3977...)
    const double zurichScale = TileMath::mercatorScale(kCenter.latitude());
    QCOMPARE_GT(zurichScale, 1.4);
    QCOMPARE_LT(qAbs(scene.verticalScale() - zurichScale), 1e-6);

    // Re-anchoring updates the factor to the new origin latitude
    const QGeoCoordinate reykjavik(64.15, -21.95);
    camera.setCenter(reykjavik);
    QCOMPARE_LT(qAbs(scene.verticalScale() - TileMath::mercatorScale(reykjavik.latitude())), 1e-3);
}

void GeoSceneTest::_scenePositionFor()
{
    GeoMapCamera camera;
    setupCamera(camera);
    GeoScene scene;
    scene.setCamera(&camera);

    // The anchor coordinate lands at the scene origin
    const QVector3D atOrigin = scene.scenePositionFor(kCenter);
    QCOMPARE_LT(atOrigin.length(), 1.0f);

    // A known world offset is preserved in scene coordinates
    const QPointF offsetWorld = TileMath::geoToWorld(kCenter) + QPointF(1000, -500);
    const QVector3D offset = scene.scenePositionFor(TileMath::worldToGeo(offsetWorld));
    QCOMPARE_LT(qAbs(offset.x() - 1000.0f), 0.01f);
    QCOMPARE_LT(qAbs(offset.y() - (-500.0f)), 0.01f);

    // Altitude (true meters) is scaled into mercator scene units
    QGeoCoordinate withAltitude = kCenter;
    withAltitude.setAltitude(100.0);
    const QVector3D elevated = scene.scenePositionFor(withAltitude);
    QCOMPARE_LT(qAbs(elevated.z() - (100.0 * scene.verticalScale())), 0.01);
}

void GeoSceneTest::_screenPositionFor()
{
    GeoScene scene;
    // No camera: invalid
    QVERIFY(!scene.screenPositionFor(kCenter).isValid());

    GeoMapCamera camera;
    setupCamera(camera);
    scene.setCamera(&camera);

    // Top-down over the anchor: the anchor projects to the viewport center
    const QVariant center = scene.screenPositionFor(kCenter);
    QVERIFY(center.isValid());
    const QPointF centerPos = center.toPointF();
    QCOMPARE_LT(qAbs(centerPos.x() - (kViewport.width() / 2.0)), 0.5);
    QCOMPARE_LT(qAbs(centerPos.y() - (kViewport.height() / 2.0)), 0.5);

    // A point above the camera is behind the view plane: invalid. Altitude
    // participates at the rendered height, so it only displaces in 3D
    // (terrainScale 1); the startup value 0 flattens it to the ground plane.
    QGeoCoordinate aboveCamera = kCenter;
    aboveCamera.setAltitude(5000.0);  // camera sits at 2000 scene units top-down
    QVERIFY(scene.screenPositionFor(aboveCamera).isValid());
    scene.setTerrainScale(1.0);
    QVERIFY(!scene.screenPositionFor(aboveCamera).isValid());
}

void GeoSceneTest::_centerElevationInMeters()
{
    GeoMapCamera camera;
    camera.setViewportSize(kViewport);
    camera.lookAt(kCenter, 0, 45, 2000);  // tilted so pivot elevation affects the solve
    GeoScene scene;
    scene.setCamera(&camera);
    scene.setTerrainScale(1.0);

    QGeoCoordinate target = kCenter.atDistanceAndAzimuth(300, 45);
    target.setAltitude(150.0);
    const QPointF screenPos(200, 150);
    const double elevationMeters = 80.0;

    // The scene API takes true meters and scales to rendered height
    // internally: must match the camera-level solve fed scene units
    const QGeoCoordinate sceneSolve = scene.centerForCoordinateAtScreenPoint(target, screenPos, elevationMeters);
    const double renderedScale = scene.verticalScale() * scene.terrainScale();
    const QGeoCoordinate cameraSolve = camera.centerForCoordinateAtScreenPoint(
        target, screenPos, target.altitude() * renderedScale, elevationMeters * renderedScale);
    QVERIFY(sceneSolve.isValid());
    QCOMPARE_LT(sceneSolve.distanceTo(cameraSolve), 0.01);
}

void GeoSceneTest::_solveRoundTrip()
{
    GeoMapCamera camera;
    camera.setViewportSize(kViewport);
    camera.lookAt(kCenter, 30, 45, 2000);  // rotated and tilted: exercises all solve axes
    GeoScene scene;
    scene.setCamera(&camera);

    QGeoCoordinate target = kCenter.atDistanceAndAzimuth(400, 60);
    const QPointF screenPos(600, 200);

    // centerForCoordinateAtScreenPoint must invert screenPositionFor for every
    // altitude-handling branch: terrainScale 0 (2D, altitude flattened) vs 1
    // (3D), and finite vs NaN altitude (treated as 0)
    for (const qreal terrainScale : {0.0, 1.0}) {
        scene.setTerrainScale(terrainScale);
        for (const bool withAltitude : {true, false}) {
            target.setAltitude(withAltitude ? 150.0 : qQNaN());

            const QGeoCoordinate center = scene.centerForCoordinateAtScreenPoint(target, screenPos);
            QVERIFY(center.isValid());
            camera.setCenter(center);

            const QVariant projected = scene.screenPositionFor(target);
            QVERIFY(projected.isValid());
            const QPointF pos = projected.toPointF();
            QCOMPARE_LT(std::hypot(pos.x() - screenPos.x(), pos.y() - screenPos.y()), 0.5);
        }
    }
}

UT_REGISTER_TEST_LIGHTWEIGHT(GeoSceneTest, TestLabel::Unit)
