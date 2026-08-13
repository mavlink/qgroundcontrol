#include "GeoSceneTest.h"

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

    // A point above the camera is behind the view plane: invalid
    QGeoCoordinate aboveCamera = kCenter;
    aboveCamera.setAltitude(5000.0);  // camera sits at 2000 scene units top-down
    QVERIFY(!scene.screenPositionFor(aboveCamera).isValid());
}

UT_REGISTER_TEST_LIGHTWEIGHT(GeoSceneTest, TestLabel::Unit)
