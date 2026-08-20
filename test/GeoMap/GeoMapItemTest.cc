#include "GeoMapItemTest.h"

#include <QtCore/QPointer>
#include <QtQml/QQmlComponent>
#include <QtQml/QQmlEngine>
#include <QtQuick/QQuickItem>
#include <QtTest/QSignalSpy>

#include <cmath>

#include "GeoMapCamera.h"
#include "GeoMapItem.h"
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

double positionError(const GeoMapItem& item, const QPointF& expected)
{
    return std::hypot(item.position().x() - expected.x(), item.position().y() - expected.y());
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

void GeoMapItemTest::_positionTracksProjection()
{
    GeoMapCamera camera;
    setupCamera(camera);
    GeoScene scene;
    scene.setCamera(&camera);

    GeoMapItem item;
    item.setScene(&scene);
    const QGeoCoordinate coordinate(47.3990, 8.5470);
    item.setCoordinate(coordinate);

    QCOMPARE_LT(positionError(item, expectedScreen(camera, scene, coordinate, 0)), 0.01);
}

void GeoMapItemTest::_anchorPointOffset()
{
    GeoMapCamera camera;
    setupCamera(camera);
    GeoScene scene;
    scene.setCamera(&camera);

    GeoMapItem item;
    item.setScene(&scene);
    item.setCoordinate(kCenter);
    item.setAnchorPoint(QPointF(10, 20));

    QCOMPARE_LT(positionError(item, expectedScreen(camera, scene, kCenter, 0) - QPointF(10, 20)), 0.01);
}

void GeoMapItemTest::_notProjectedParksOffScreen()
{
    GeoMapCamera camera;
    setupCamera(camera);
    GeoScene scene;
    scene.setCamera(&camera);
    scene.setTerrainScale(1.0);

    GeoMapItem item;
    item.setAltitudeMode(GeoMapItem::AltitudeMode::Absolute);

    // No coordinate yet: not projected
    item.setScene(&scene);
    QVERIFY(!item.projected());

    item.setCoordinate(kCenter);
    QVERIFY(item.projected());

    // Above the camera (distance 2000, top-down): behind the view plane
    QGeoCoordinate aboveCamera = kCenter;
    aboveCamera.setAltitude(5000.0);
    item.setCoordinate(aboveCamera);
    QVERIFY(!item.projected());
    QCOMPARE_LT(item.position().x(), -kViewport.width());
    QCOMPARE_LT(item.position().y(), -kViewport.height());

    // Back on the ground: shown again
    item.setCoordinate(kCenter);
    QVERIFY(item.projected());
    QCOMPARE_LT(positionError(item, expectedScreen(camera, scene, kCenter, 0)), 0.01);
}

void GeoMapItemTest::_repositionsOnCameraPose()
{
    GeoMapCamera camera;
    setupCamera(camera);
    GeoScene scene;
    scene.setCamera(&camera);

    GeoMapItem item;
    item.setScene(&scene);
    const QGeoCoordinate coordinate(47.3990, 8.5470);
    item.setCoordinate(coordinate);
    const QPointF before = item.position();

    camera.setCenter(QGeoCoordinate(47.3990, 8.5480));

    QCOMPARE_GT(positionError(item, before), 1.0);
    QCOMPARE_LT(positionError(item, expectedScreen(camera, scene, coordinate, 0)), 0.01);
}

void GeoMapItemTest::_terrainScaleScalesAltitude()
{
    GeoMapCamera camera;
    setupCamera(camera, 45);
    GeoScene scene;
    scene.setCamera(&camera);

    GeoMapItem item;
    item.setScene(&scene);
    item.setAltitudeMode(GeoMapItem::AltitudeMode::Absolute);
    QGeoCoordinate elevated = kCenter;
    elevated.setAltitude(100.0);
    item.setCoordinate(elevated);

    // 2D (terrainScale 0): altitude flattens with the terrain
    QCOMPARE_LT(positionError(item, expectedScreen(camera, scene, elevated, 0)), 0.01);

    // 3D: altitude displaces the projection like the rendered surface
    scene.setTerrainScale(1.0);
    const QPointF expected3D = expectedScreen(camera, scene, elevated, 100.0);
    QCOMPARE_GT(positionError(item, expectedScreen(camera, scene, elevated, 0)), 1.0);
    QCOMPARE_LT(positionError(item, expected3D), 0.01);
}

void GeoMapItemTest::_clampToGroundTracksTerrain()
{
    GeoMapCamera camera;
    setupCamera(camera, 45);
    GeoScene scene;
    scene.setCamera(&camera);
    scene.setTerrainScale(1.0);
    SurfacePatchModel model;
    model.setScene(&scene);
    model.drainUpdates();

    GeoMapItem item;
    item.setScene(&scene);
    item.setSurfaceModel(&model);
    item.setCoordinate(kCenter);

    // No terrain data yet: on the ground plane
    QCOMPARE_LT(positionError(item, expectedScreen(camera, scene, kCenter, 0)), 0.01);

    // Terrain arriving must re-seat the item on the surface without a
    // coordinate or camera change
    const TileMath::TileKey key = TileMath::tileForWorld(TileMath::geoToWorld(kCenter), 10);
    QVERIFY(model.heightField()->insertTile(key, uniformGrid(100.0f)));
    QCOMPARE(model.terrainHeightAt(kCenter), 100.0);
    QCOMPARE_LT(positionError(item, expectedScreen(camera, scene, kCenter, 100.0)), 0.01);
}

void GeoMapItemTest::_scenePosition()
{
    GeoMapCamera camera;
    setupCamera(camera, 45);
    GeoScene scene;
    scene.setCamera(&camera);

    GeoMapItem item;
    item.setScene(&scene);
    item.setAltitudeMode(GeoMapItem::AltitudeMode::Absolute);
    QGeoCoordinate elevated = kCenter;
    elevated.setAltitude(100.0);
    item.setCoordinate(elevated);

    // 2D (terrainScale 0): on the flattened surface
    const QPointF world = TileMath::geoToWorld(elevated);
    const QPointF origin = scene.sceneOrigin();
    QVector3D expected(static_cast<float>(world.x() - origin.x()), static_cast<float>(world.y() - origin.y()), 0.0f);
    QCOMPARE_LT((item.scenePosition() - expected).length(), 0.01f);

    // 3D: z follows altitude like the rendered surface
    QSignalSpy spy(&item, &GeoMapItem::scenePositionChanged);
    scene.setTerrainScale(1.0);
    expected.setZ(static_cast<float>(100.0 * scene.verticalScale()));
    QCOMPARE_LT((item.scenePosition() - expected).length(), 0.01f);
    QCOMPARE_GT(spy.count(), 0);
}

void GeoMapItemTest::_scenePositionShiftsOnReanchor()
{
    GeoMapCamera camera;
    setupCamera(camera);
    GeoScene scene;
    scene.setCamera(&camera);

    GeoMapItem item;
    item.setScene(&scene);
    item.setCoordinate(kCenter);
    const QPointF originBefore = scene.sceneOrigin();

    // Move the camera past the re-anchor threshold; the item must follow the
    // new origin so its scene position stays renderable in float precision
    camera.setCenter(QGeoCoordinate(47.9, 9.4));
    QVERIFY(scene.sceneOrigin() != originBefore);

    const QPointF world = TileMath::geoToWorld(kCenter);
    const QPointF origin = scene.sceneOrigin();
    const QVector3D expected(static_cast<float>(world.x() - origin.x()), static_cast<float>(world.y() - origin.y()),
                             0.0f);
    QCOMPARE_LT((item.scenePosition() - expected).length(), 0.01f);
}

void GeoMapItemTest::_delegate3DLifecycle()
{
    GeoMapCamera camera;
    setupCamera(camera, 45);
    GeoScene scene;
    scene.setCamera(&camera);

    QQmlEngine engine;
    QQmlComponent nodeComponent(&engine);
    nodeComponent.setData("import QtQuick3D\nNode {}", QUrl());
    const QScopedPointer<QObject> content3D(nodeComponent.create());
    QVERIFY2(content3D, qPrintable(nodeComponent.errorString()));
    scene.setContent3D(content3D.get());

    GeoMapItem item;
    item.setScene(&scene);
    item.setAltitudeMode(GeoMapItem::AltitudeMode::Absolute);
    QGeoCoordinate elevated = kCenter;
    elevated.setAltitude(100.0);
    item.setCoordinate(elevated);

    QQmlComponent delegate(&engine);
    delegate.setData("import QtQuick3D\nNode {}", QUrl());
    item.setDelegate3D(&delegate);

    const QPointer<QObject> node = item.node3D();
    QVERIFY2(node, qPrintable(delegate.errorString()));
    QCOMPARE(node->parent(), content3D.get());
    QCOMPARE(node->property("position").value<QVector3D>(), item.scenePosition());

    // 2D (terrainScale 0): 3D content hidden, 2D content fully visible
    QCOMPARE(node->property("opacity").toReal(), 0.0);
    QCOMPARE(item.contentOpacity2D(), 1.0);

    // Transitioning: node tracks scenePosition and the opacities crossfade
    scene.setTerrainScale(0.25);
    QCOMPARE(node->property("position").value<QVector3D>(), item.scenePosition());
    QCOMPARE(node->property("opacity").toReal(), 0.25);
    QCOMPARE(item.contentOpacity2D(), 0.75);

    // Crossfade opt-out: node stays opaque regardless of terrainScale
    item.setCrossfade3D(false);
    QCOMPARE(node->property("opacity").toReal(), 1.0);
    QCOMPARE(item.contentOpacity2D(), 1.0);
    scene.setTerrainScale(0.0);
    QCOMPARE(node->property("opacity").toReal(), 1.0);

    // Re-enabling restores the crossfade at the current terrainScale
    item.setCrossfade3D(true);
    QCOMPARE(node->property("opacity").toReal(), 0.0);
    QCOMPARE(item.contentOpacity2D(), 1.0);

    // Clearing the delegate destroys the node and restores 2D opacity
    item.setDelegate3D(nullptr);
    QVERIFY(!item.node3D());
    QTRY_VERIFY(node.isNull());
    QCOMPARE(item.contentOpacity2D(), 1.0);
}

void GeoMapItemTest::_delegate3DWaitsForContainer()
{
    GeoMapCamera camera;
    setupCamera(camera);
    GeoScene scene;
    scene.setCamera(&camera);

    QQmlEngine engine;
    QQmlComponent delegate(&engine);
    delegate.setData("import QtQuick3D\nNode {}", QUrl());

    GeoMapItem item;
    item.setScene(&scene);
    item.setCoordinate(kCenter);
    item.setDelegate3D(&delegate);

    // No content3D container yet: nothing to instantiate into
    QVERIFY(!item.node3D());

    QQmlComponent nodeComponent(&engine);
    nodeComponent.setData("import QtQuick3D\nNode {}", QUrl());
    const QScopedPointer<QObject> content3D(nodeComponent.create());
    QVERIFY2(content3D, qPrintable(nodeComponent.errorString()));
    scene.setContent3D(content3D.get());
    QVERIFY(item.node3D());
}

void GeoMapItemTest::_node3DVisibilityTracksItem()
{
    GeoMapCamera camera;
    setupCamera(camera);
    GeoScene scene;
    scene.setCamera(&camera);
    scene.setTerrainScale(1.0);

    QQmlEngine engine;
    QQmlComponent nodeComponent(&engine);
    nodeComponent.setData("import QtQuick3D\nNode {}", QUrl());
    const QScopedPointer<QObject> content3D(nodeComponent.create());
    QVERIFY2(content3D, qPrintable(nodeComponent.errorString()));
    scene.setContent3D(content3D.get());

    // Effective visibility needs a visible parent chain (as in real usage)
    QQuickItem parentItem;
    GeoMapItem item;
    item.setParentItem(&parentItem);
    item.setScene(&scene);
    item.setCoordinate(kCenter);

    QQmlComponent delegate(&engine);
    delegate.setData("import QtQuick3D\nNode {}", QUrl());
    item.setDelegate3D(&delegate);

    QObject* const node = item.node3D();
    QVERIFY2(node, qPrintable(delegate.errorString()));
    QVERIFY(node->property("visible").toBool());

    // Hiding the 2D item must hide the 3D node too
    item.setVisible(false);
    QVERIFY(!node->property("visible").toBool());
    item.setVisible(true);
    QVERIFY(node->property("visible").toBool());

    // Invalid coordinate: no phantom node at the scene origin
    item.setCoordinate(QGeoCoordinate());
    QVERIFY(!node->property("visible").toBool());
    item.setCoordinate(kCenter);
    QVERIFY(node->property("visible").toBool());
}

UT_REGISTER_TEST_LIGHTWEIGHT(GeoMapItemTest, TestLabel::Unit)
