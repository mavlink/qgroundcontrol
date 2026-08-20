#include "GeoMapProjectedPathTest.h"

#include <QtPositioning/QGeoCoordinate>
#include <QtTest/QSignalSpy>

#include <cmath>

#include "GeoMapCamera.h"
#include "GeoMapProjectedPath.h"
#include "GeoScene.h"
#include "HeightField.h"
#include "SurfacePatchModel.h"
#include "TileMath.h"

namespace {

const QGeoCoordinate kCenter(47.3977419, 8.5455938);
constexpr QSizeF kViewport(800, 600);

void setupCamera(GeoMapCamera& camera, qreal tilt = 0)
{
    camera.setViewportSize(kViewport);
    camera.lookAt(kCenter, 0, tilt, 2000);
}

QPointF expectedScreen(const GeoMapCamera& camera, const GeoScene& scene, const QGeoCoordinate& coordinate,
                       double altitude)
{
    const auto screen =
        camera.worldToScreen(TileMath::geoToWorld(coordinate), altitude * scene.verticalScale() * scene.terrainScale());
    return screen.value();
}

double screenError(const QVariant& actual, const QPointF& expected)
{
    const QPointF point = actual.toPointF();
    return std::hypot(point.x() - expected.x(), point.y() - expected.y());
}

QVariantList coordinateList(std::initializer_list<QGeoCoordinate> coordinates)
{
    QVariantList list;
    for (const QGeoCoordinate& coordinate : coordinates) {
        list.append(QVariant::fromValue(coordinate));
    }
    return list;
}

ElevationTilePyramid::Grid uniformGrid(float height)
{
    ElevationTilePyramid::Grid grid;
    grid.width = 4;
    grid.height = 4;
    grid.heights = QList<float>(16, height);
    return grid;
}

}  // namespace

void GeoMapProjectedPathTest::_projectsAbsolute()
{
    GeoMapCamera camera;
    setupCamera(camera, 45);
    GeoScene scene;
    scene.setCamera(&camera);
    scene.setTerrainScale(1.0);

    GeoMapProjectedPath path;
    path.setAltitudeMode(GeoMapItem::AltitudeMode::Absolute);
    path.setScene(&scene);

    // No coordinates: nothing to project
    QVERIFY(!path.projected());
    QVERIFY(path.screenPoints().isEmpty());

    QGeoCoordinate elevated(47.3990, 8.5470, 150.0);
    path.setCoordinates(coordinateList({kCenter, elevated}));

    QVERIFY(path.projected());
    QCOMPARE(path.screenPoints().count(), 2);
    QCOMPARE_LT(screenError(path.screenPoints()[0], expectedScreen(camera, scene, kCenter, 0)), 0.01);
    QCOMPARE_LT(screenError(path.screenPoints()[1], expectedScreen(camera, scene, elevated, 150.0)), 0.01);
}

void GeoMapProjectedPathTest::_clampToGroundTracksTerrain()
{
    GeoMapCamera camera;
    setupCamera(camera, 45);
    GeoScene scene;
    scene.setCamera(&camera);
    scene.setTerrainScale(1.0);
    SurfacePatchModel model;
    model.setScene(&scene);
    model.drainUpdates();

    GeoMapProjectedPath path;
    path.setScene(&scene);
    path.setSurfaceModel(&model);
    // Ignores the coordinate's own altitude in ClampToGround mode
    path.setCoordinates(coordinateList({QGeoCoordinate(kCenter.latitude(), kCenter.longitude(), 500.0)}));

    // No terrain data yet: on the ground plane
    QVERIFY(path.projected());
    QCOMPARE_LT(screenError(path.screenPoints()[0], expectedScreen(camera, scene, kCenter, 0)), 0.01);

    // Terrain arriving must re-seat the points on the surface without a
    // coordinate or camera change
    QSignalSpy pointsSpy(&path, &GeoMapProjectedPath::screenPointsChanged);
    const TileMath::TileKey key = TileMath::tileForWorld(TileMath::geoToWorld(kCenter), 10);
    QVERIFY(model.heightField()->insertTile(key, uniformGrid(100.0f)));
    QCOMPARE(pointsSpy.count(), 1);
    QCOMPARE_LT(screenError(path.screenPoints()[0], expectedScreen(camera, scene, kCenter, 100.0)), 0.01);
}

void GeoMapProjectedPathTest::_failedProjections()
{
    GeoMapCamera camera;
    setupCamera(camera);
    GeoScene scene;
    scene.setCamera(&camera);
    scene.setTerrainScale(1.0);

    GeoMapProjectedPath path;
    path.setAltitudeMode(GeoMapItem::AltitudeMode::Absolute);

    // Coordinates without a scene: not projected
    path.setCoordinates(coordinateList({kCenter}));
    QVERIFY(!path.projected());
    QVERIFY(path.screenPoints().isEmpty());

    // Scene arrives: projected
    QSignalSpy projectedSpy(&path, &GeoMapProjectedPath::projectedChanged);
    path.setScene(&scene);
    QVERIFY(path.projected());
    QCOMPARE(projectedSpy.count(), 1);

    // Invalid coordinate poisons the whole path
    path.setCoordinates(coordinateList({kCenter, QGeoCoordinate()}));
    QVERIFY(!path.projected());
    QCOMPARE(path.screenPoints().count(), 1);  // the valid point still projects

    // Above the camera (distance 2000, top-down): behind the view plane
    path.setCoordinates(coordinateList({QGeoCoordinate(kCenter.latitude(), kCenter.longitude(), 5000.0)}));
    QVERIFY(!path.projected());
    QVERIFY(path.screenPoints().isEmpty());
}

void GeoMapProjectedPathTest::_reprojectsOnCameraPose()
{
    GeoMapCamera camera;
    setupCamera(camera);
    GeoScene scene;
    scene.setCamera(&camera);
    scene.setTerrainScale(1.0);

    GeoMapProjectedPath path;
    path.setScene(&scene);
    path.setCoordinates(coordinateList({kCenter, QGeoCoordinate(47.3990, 8.5470)}));
    QVERIFY(path.projected());

    QSignalSpy pointsSpy(&path, &GeoMapProjectedPath::screenPointsChanged);
    camera.setCenter(QGeoCoordinate(47.3990, 8.5480));
    QCOMPARE(pointsSpy.count(), 1);
    QCOMPARE_LT(screenError(path.screenPoints()[0], expectedScreen(camera, scene, kCenter, 0)), 0.01);

    // Re-assigning identical coordinates must not re-emit
    path.setCoordinates(path.coordinates());
    QCOMPARE(pointsSpy.count(), 1);
}

void GeoMapProjectedPathTest::_cameraReplacement()
{
    GeoMapCamera cameraA;
    setupCamera(cameraA);
    GeoScene scene;
    scene.setCamera(&cameraA);
    scene.setTerrainScale(1.0);

    GeoMapProjectedPath path;
    path.setScene(&scene);
    path.setCoordinates(coordinateList({kCenter}));
    QVERIFY(path.projected());

    // Swapping the scene's camera reconnects: pose changes on the new camera reproject
    GeoMapCamera cameraB;
    setupCamera(cameraB);
    cameraB.setCenter(QGeoCoordinate(47.3990, 8.5480));
    scene.setCamera(&cameraB);
    QCOMPARE_LT(screenError(path.screenPoints()[0], expectedScreen(cameraB, scene, kCenter, 0)), 0.01);

    QSignalSpy pointsSpy(&path, &GeoMapProjectedPath::screenPointsChanged);
    cameraB.setDistance(3000);
    QCOMPARE(pointsSpy.count(), 1);

    // Old camera no longer drives the path
    cameraA.setDistance(5000);
    QCOMPARE(pointsSpy.count(), 1);
}

void GeoMapProjectedPathTest::_sceneAndModelDestruction()
{
    GeoMapProjectedPath path;
    path.setCoordinates(coordinateList({kCenter}));

    auto* camera = new GeoMapCamera();
    setupCamera(*camera);
    auto* scene = new GeoScene();
    scene->setCamera(camera);
    scene->setTerrainScale(1.0);
    auto* model = new SurfacePatchModel();
    model->setScene(scene);
    model->drainUpdates();

    path.setScene(scene);
    path.setSurfaceModel(model);
    QVERIFY(path.projected());

    // Model destruction detaches cleanly and reprojects on the ground plane
    QSignalSpy modelSpy(&path, &GeoMapProjectedPath::surfaceModelChanged);
    delete model;
    QCOMPARE(modelSpy.count(), 1);
    QVERIFY(!path.surfaceModel());
    QVERIFY(path.projected());

    // Scene destruction (which also destroys the camera parent-child or not)
    QSignalSpy sceneSpy(&path, &GeoMapProjectedPath::sceneChanged);
    delete scene;
    QCOMPARE(sceneSpy.count(), 1);
    QVERIFY(!path.scene());
    QVERIFY(!path.projected());
    QVERIFY(path.screenPoints().isEmpty());

    delete camera;
}

void GeoMapProjectedPathTest::_circleCoordinates()
{
    const QGeoCoordinate center(kCenter.latitude(), kCenter.longitude(), 120.0);
    const double radius = 75.0;
    const int segments = 16;
    const QVariantList circle = GeoMapProjectedPath::circleCoordinates(center, radius, segments);

    // Closed ring: segments points plus the closing copy of the first
    QCOMPARE(circle.count(), segments + 1);
    const QGeoCoordinate first = circle.first().value<QGeoCoordinate>();
    const QGeoCoordinate last = circle.last().value<QGeoCoordinate>();
    QCOMPARE_LT(first.distanceTo(last), 0.01);

    // Every point sits on the circle and preserves the center's altitude
    for (const QVariant& variant : circle) {
        const QGeoCoordinate coordinate = variant.value<QGeoCoordinate>();
        QCOMPARE_LT(qAbs(center.distanceTo(coordinate) - radius), 0.01);
        QCOMPARE(coordinate.altitude(), 120.0);
    }

    // Degenerate inputs produce no ring
    QVERIFY(GeoMapProjectedPath::circleCoordinates(QGeoCoordinate(), radius).isEmpty());
    QVERIFY(GeoMapProjectedPath::circleCoordinates(center, 0.0).isEmpty());
    QVERIFY(GeoMapProjectedPath::circleCoordinates(center, radius, 2).isEmpty());
}

UT_REGISTER_TEST_LIGHTWEIGHT(GeoMapProjectedPathTest, TestLabel::Unit)
