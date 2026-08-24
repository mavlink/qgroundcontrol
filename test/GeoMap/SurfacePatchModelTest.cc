#include "SurfacePatchModelTest.h"

#include <QtCore/QPointF>
#include <QtTest/QSignalSpy>

#include <cmath>

#include "GeoMapCamera.h"
#include "GeoScene.h"
#include "HeightField.h"
#include "SurfaceModel.h"
#include "SurfacePatchModel.h"
#include "TileMath.h"

namespace {

const QGeoCoordinate kCenter(47.3977419, 8.5455938);
constexpr QSizeF kViewport(800, 600);

void setupCamera(GeoMapCamera& camera, const QGeoCoordinate& center = kCenter)
{
    camera.setViewportSize(kViewport);
    camera.lookAt(center, 0, 0, 2000);
}

void attach(SurfacePatchModel& model, GeoScene& scene, GeoMapCamera& camera)
{
    scene.setCamera(&camera);
    model.setScene(&scene);
    model.drainUpdates();
}

ElevationTilePyramid::Grid uniformGrid(float height)
{
    ElevationTilePyramid::Grid grid;
    grid.width = 4;
    grid.height = 4;
    grid.heights = QList<float>(16, height);
    return grid;
}

double groundDistance(const QPointF& a, const QPointF& b)
{
    return std::hypot(a.x() - b.x(), a.y() - b.y());
}

}  // namespace

void SurfacePatchModelTest::_emptyWithoutCamera()
{
    const SurfacePatchModel model;
    QCOMPARE(model.rowCount(), 0);
    QCOMPARE(model.patchCount(), 0);
    QCOMPARE(model.pendingCount(), 0);
}

void SurfacePatchModelTest::_populatesFromCamera()
{
    GeoMapCamera camera;
    GeoScene scene;
    SurfacePatchModel model;
    setupCamera(camera);
    attach(model, scene, camera);

    QCOMPARE_GT(model.rowCount(), 0);
    QCOMPARE(model.rowCount(), model.patchCount());
    QTRY_COMPARE_WITH_TIMEOUT(model.pendingCount(), 0, 5000);

    // Scene origin anchors at the camera center
    const QPointF expectedOrigin = TileMath::geoToWorld(kCenter);
    QCOMPARE_LT(std::hypot(scene.sceneOrigin().x() - expectedOrigin.x(), scene.sceneOrigin().y() - expectedOrigin.y()),
                1.0);
}

void SurfacePatchModelTest::_rolesValid()
{
    GeoMapCamera camera;
    GeoScene scene;
    SurfacePatchModel model;
    setupCamera(camera);
    attach(model, scene, camera);
    QTRY_COMPARE_WITH_TIMEOUT(model.pendingCount(), 0, 5000);

    const int expectedHeights = (model.gridSize() + 1) * (model.gridSize() + 1);
    bool nearOriginPatchSeen = false;
    for (int row = 0; row < model.rowCount(); row++) {
        const QModelIndex idx = model.index(row);
        const double span = model.data(idx, SurfacePatchModel::SpanRole).toDouble();
        const int zoom = model.data(idx, SurfacePatchModel::ZoomRole).toInt();
        QCOMPARE_GT(span, 0.0);
        QCOMPARE_GE(zoom, 0);
        QCOMPARE_LE(zoom, TileMath::kMaxZoom);
        QVERIFY(model.data(idx, SurfacePatchModel::ReadyRole).toBool());
        // Flat source delivers zero heights of the full grid size
        const auto heights = model.data(idx, SurfacePatchModel::HeightsRole).value<QList<float>>();
        QCOMPARE(heights.count(), expectedHeights);

        // Positions are scene-relative: the patch containing the camera center
        // must be within its own span of the origin
        const double centerX = model.data(idx, SurfacePatchModel::CenterXRole).toDouble();
        const double centerY = model.data(idx, SurfacePatchModel::CenterYRole).toDouble();
        if ((qAbs(centerX) <= span) && (qAbs(centerY) <= span)) {
            nearOriginPatchSeen = true;
        }
    }
    QVERIFY(nearOriginPatchSeen);
}

void SurfacePatchModelTest::_incrementalUpdatesOnMove()
{
    GeoMapCamera camera;
    GeoScene scene;
    SurfacePatchModel model;
    setupCamera(camera);
    attach(model, scene, camera);
    QTRY_COMPARE_WITH_TIMEOUT(model.pendingCount(), 0, 5000);

    QSignalSpy resetSpy(&model, &QAbstractItemModel::modelReset);
    QSignalSpy insertSpy(&model, &QAbstractItemModel::rowsInserted);
    QSignalSpy removeSpy(&model, &QAbstractItemModel::rowsRemoved);

    // Small pan within the re-anchor threshold: incremental row churn, no reset
    camera.setCenter(QGeoCoordinate(47.42, 8.58));
    model.drainUpdates();
    QCOMPARE(resetSpy.count(), 0);
    QCOMPARE_GT(insertSpy.count() + removeSpy.count(), 0);
    QCOMPARE(model.rowCount(), model.patchCount());
}

void SurfacePatchModelTest::_reanchorsOnLargeMove()
{
    GeoMapCamera camera;
    GeoScene scene;
    SurfacePatchModel model;
    setupCamera(camera);
    attach(model, scene, camera);

    QSignalSpy originSpy(&scene, &GeoScene::sceneOriginChanged);

    // Move far beyond kReanchorDistance: origin follows the camera
    const QGeoCoordinate faraway(48.85, 2.35);  // Paris, ~490km from Zurich
    camera.setCenter(faraway);
    model.drainUpdates();
    QCOMPARE_GT(originSpy.count(), 0);

    const QPointF expectedOrigin = TileMath::geoToWorld(faraway);
    QCOMPARE_LT(std::hypot(scene.sceneOrigin().x() - expectedOrigin.x(), scene.sceneOrigin().y() - expectedOrigin.y()),
                1.0);

    // Scene positions stay small after the re-anchor
    QTRY_COMPARE_WITH_TIMEOUT(model.pendingCount(), 0, 5000);
    for (int row = 0; row < model.rowCount(); row++) {
        const QModelIndex idx = model.index(row);
        const double span = model.data(idx, SurfacePatchModel::SpanRole).toDouble();
        const double centerX = model.data(idx, SurfacePatchModel::CenterXRole).toDouble();
        const double centerY = model.data(idx, SurfacePatchModel::CenterYRole).toDouble();
        // Bounded by the visible region around the camera, not by world scale
        QCOMPARE_LT(qAbs(centerX), GeoScene::kReanchorDistance + span);
        QCOMPARE_LT(qAbs(centerY), GeoScene::kReanchorDistance + span);
    }
}

void SurfacePatchModelTest::_cameraSwapAnchorsFresh()
{
    GeoMapCamera zurichCamera;
    GeoScene scene;
    SurfacePatchModel model;
    setupCamera(zurichCamera);
    attach(model, scene, zurichCamera);

    // Swap to a static camera on another continent: the origin must follow even
    // though the new camera's center never changes after the swap
    GeoMapCamera sydneyCamera;
    const QGeoCoordinate sydney(-33.8688, 151.2093);
    setupCamera(sydneyCamera, sydney);
    scene.setCamera(&sydneyCamera);
    model.drainUpdates();

    const QPointF expectedOrigin = TileMath::geoToWorld(sydney);
    QCOMPARE_LT(std::hypot(scene.sceneOrigin().x() - expectedOrigin.x(), scene.sceneOrigin().y() - expectedOrigin.y()),
                1.0);
    // The model rebuilt against the new camera
    QCOMPARE_GT(model.rowCount(), 0);
}

void SurfacePatchModelTest::_debugHillsSwitchResets()
{
    GeoMapCamera camera;
    GeoScene scene;
    SurfacePatchModel model;
    setupCamera(camera);
    attach(model, scene, camera);
    QTRY_COMPARE_WITH_TIMEOUT(model.pendingCount(), 0, 5000);

    QSignalSpy resetSpy(&model, &QAbstractItemModel::modelReset);
    model.setDebugHills(true);
    model.drainUpdates();
    QCOMPARE_GT(resetSpy.count(), 0);
    QCOMPARE_GT(model.rowCount(), 0);

    // Debug hills deliver non-flat heights once the synthesized tiles land in
    // the field (delivered via the event loop)
    const auto nonZeroSeen = [&model]() {
        for (int row = 0; row < model.rowCount(); row++) {
            const auto heights = model.data(model.index(row), SurfacePatchModel::HeightsRole).value<QList<float>>();
            for (float h : heights) {
                if (h > 0.0f) {
                    return true;
                }
            }
        }
        return false;
    };
    QTRY_VERIFY_WITH_TIMEOUT(nonZeroSeen(), 5000);
}

void SurfacePatchModelTest::_rowsAlwaysMeshedDuringLodChurn()
{
    GeoMapCamera camera;
    GeoScene scene;
    SurfacePatchModel model;
    camera.setViewportSize(kViewport);
    camera.lookAt(kCenter, 0, 0, GeoMapCamera::kMaxDistance);
    attach(model, scene, camera);

    // Refine hard: every row must present a full mesh from the field's
    // estimate - the delegate never sees a pending/hidden row. (Per-pass
    // coverage during churn is guarded at the SurfaceModel level.)
    camera.lookAt(kCenter, 0, 0, 2000);
    model.drainUpdates();
    QCOMPARE_GT(model.rowCount(), 0);
    for (int row = 0; row < model.rowCount(); row++) {
        const QModelIndex idx = model.index(row);
        QVERIFY(model.data(idx, SurfacePatchModel::ReadyRole).toBool());
        QVERIFY(!model.data(idx, SurfacePatchModel::CoveredRole).toBool());
        const auto heights = model.data(idx, SurfacePatchModel::HeightsRole).value<QList<float>>();
        QCOMPARE(heights.count(), (SurfaceModel::kGridSize + 1) * (SurfaceModel::kGridSize + 1));
    }
}

void SurfacePatchModelTest::_edgeLodDeltasRoleStitchesLodRings()
{
    GeoMapCamera camera;
    GeoScene scene;
    SurfacePatchModel model;
    camera.setViewportSize(kViewport);
    // Tilted view: LOD rings guarantee coarser neighbors across ring boundaries
    camera.lookAt(kCenter, 0, 45, 2000);
    attach(model, scene, camera);
    QCOMPARE_GT(model.rowCount(), 8);

    QVERIFY(model.roleNames().value(SurfacePatchModel::EdgeLodDeltasRole) == QByteArray("edgeLodDeltas"));

    // Every row exposes {N,S,W,E}; the mixed-LOD view must exercise stitching
    int positiveDeltas = 0;
    for (int row = 0; row < model.rowCount(); row++) {
        const auto deltas = model.data(model.index(row), SurfacePatchModel::EdgeLodDeltasRole).value<QList<int>>();
        QCOMPARE(deltas.count(), 4);
        for (const int delta : deltas) {
            QCOMPARE_GE(delta, 0);
            if (delta > 0) {
                positiveDeltas++;
            }
        }
    }
    QCOMPARE_GT(positiveDeltas, 0);

    // Neighbor churn refreshes the role so delegates re-stitch
    QSignalSpy dataSpy(&model, &QAbstractItemModel::dataChanged);
    camera.lookAt(kCenter, 0, 45, 1000);
    model.drainUpdates();
    bool deltasRefreshed = false;
    for (const QList<QVariant>& args : dataSpy) {
        const auto roles = args.at(2).value<QList<int>>();
        if (roles.contains(SurfacePatchModel::EdgeLodDeltasRole)) {
            deltasRefreshed = true;
            break;
        }
    }
    QVERIFY2(deltasRefreshed, "no dataChanged carried EdgeLodDeltasRole during LOD churn");
}

void SurfacePatchModelTest::_tileKeyAndHeightFieldExposedToDelegates()
{
    // The tile key roles (tileX, tileY + the existing zoomLevel) must address
    // the patch actually rendered at each row, and the model's field is the
    // live one the row heights were sampled from.
    GeoMapCamera camera;
    GeoScene scene;
    SurfacePatchModel model;
    setupCamera(camera);
    attach(model, scene, camera);
    QCOMPARE_GT(model.rowCount(), 0);

    QVERIFY(model.roleNames().value(SurfacePatchModel::TileXRole) == QByteArray("tileX"));
    QVERIFY(model.roleNames().value(SurfacePatchModel::TileYRole) == QByteArray("tileY"));
    QVERIFY(model.heightField() != nullptr);

    for (int row = 0; row < model.rowCount(); row++) {
        const QModelIndex idx = model.index(row);
        const TileMath::TileKey key{model.data(idx, SurfacePatchModel::TileXRole).toInt(),
                                    model.data(idx, SurfacePatchModel::TileYRole).toInt(),
                                    model.data(idx, SurfacePatchModel::ZoomRole).toInt()};
        QVERIFY(TileMath::isValidKey(key));

        // The key must address the patch actually rendered there: its tile
        // rect (scene-relative) matches the row's center and span
        const double span = TileMath::tileSpanAtZoom(key.zoom);
        QCOMPARE(model.data(idx, SurfacePatchModel::SpanRole).toDouble(), span);
        const QPointF minCorner = TileMath::tileMinCorner(key);
        const QPointF sceneCenter(minCorner.x() + (span / 2.0) - scene.sceneOrigin().x(),
                                  minCorner.y() + (span / 2.0) - scene.sceneOrigin().y());
        QCOMPARE(model.data(idx, SurfacePatchModel::CenterXRole).toDouble(), sceneCenter.x());
        QCOMPARE(model.data(idx, SurfacePatchModel::CenterYRole).toDouble(), sceneCenter.y());
    }

    // The exposed field is the live one: model heights come from it
    QTRY_COMPARE_WITH_TIMEOUT(model.pendingCount(), 0, 5000);
    const QModelIndex first = model.index(0);
    const TileMath::TileKey firstKey{model.data(first, SurfacePatchModel::TileXRole).toInt(),
                                     model.data(first, SurfacePatchModel::TileYRole).toInt(),
                                     model.data(first, SurfacePatchModel::ZoomRole).toInt()};
    const auto heights = model.data(first, SurfacePatchModel::HeightsRole).value<QList<float>>();
    QCOMPARE(model.heightField()->samplePatch(firstKey, model.gridSize()), heights);
}

void SurfacePatchModelTest::_terrainHeightAt()
{
    // No scene/camera: no field, well-defined zero
    SurfacePatchModel model;
    QCOMPARE(model.terrainHeightAt(kCenter), 0.0);

    GeoMapCamera camera;
    GeoScene scene;
    setupCamera(camera);
    attach(model, scene, camera);
    QCOMPARE(model.terrainHeightAt(kCenter), 0.0);

    // Terrain data arriving changes the answer and notifies consumers
    QSignalSpy heightsSpy(&model, &SurfacePatchModel::terrainHeightsChanged);
    const TileMath::TileKey key = TileMath::tileForWorld(TileMath::geoToWorld(kCenter), 10);
    ElevationTilePyramid::Grid grid;
    grid.width = 4;
    grid.height = 4;
    grid.heights = QList<float>(16, 100.0f);
    QVERIFY(model.heightField()->insertTile(key, grid));
    QCOMPARE(model.terrainHeightAt(kCenter), 100.0);
    QCOMPARE_GE(heightsSpy.count(), 1);
}

// The surface-pick tests below never spin the event loop after attach: the
// flat height source delivers its all-zero tiles through queued singleShots,
// so the field holds exactly the tiles each test inserts.

void SurfacePatchModelTest::_surfacePickFlatMatchesPlanePick()
{
    GeoMapCamera camera;
    GeoScene scene;
    SurfacePatchModel model;
    setupCamera(camera);
    camera.setTilt(60);
    camera.setHeading(30);
    attach(model, scene, camera);

    QVERIFY(!model.surfaceCoordinateAtScreenPoint(nullptr, QPointF(400, 300), 1.0).isValid());

    // With no height data the surface is the z=0 plane: the march must agree
    // with the analytic plane pick everywhere on screen
    const QList<QPointF> points{QPointF(400, 300), QPointF(200, 150), QPointF(600, 450)};
    for (const QPointF& p : points) {
        const QGeoCoordinate surface = model.surfaceCoordinateAtScreenPoint(&camera, p, 1.0);
        const QGeoCoordinate plane = camera.coordinateAtScreenPoint(p);
        QVERIFY(surface.isValid());
        QVERIFY(plane.isValid());
        QCOMPARE_LT(groundDistance(TileMath::geoToWorld(surface), TileMath::geoToWorld(plane)), 0.01);
    }
}

void SurfacePatchModelTest::_surfacePickLandsOnPlateau()
{
    // Camera centered on a zoom-12 tile raised to a 500 m plateau
    const TileMath::TileKey key = TileMath::tileForWorld(TileMath::geoToWorld(kCenter), 12);
    const double span = TileMath::tileSpanAtZoom(12);
    const QPointF tileCenter = TileMath::tileMinCorner(key) + QPointF(span / 2.0, span / 2.0);

    GeoMapCamera camera;
    GeoScene scene;
    SurfacePatchModel model;
    setupCamera(camera, TileMath::worldToGeo(tileCenter));
    camera.setTilt(60);
    camera.setDistance(4000);  // camera z stays above the scaled plateau top
    attach(model, scene, camera);
    QVERIFY(model.heightField()->insertTile(key, uniformGrid(500.0f)));

    const QPointF screenPos(400, 300);
    const double zScale = 2.0;  // exercise scale independence (mercator vertical stretch)
    const QGeoCoordinate pick = model.surfaceCoordinateAtScreenPoint(&camera, screenPos, zScale);
    QVERIFY(pick.isValid());
    QCOMPARE_LE(std::abs(model.terrainHeightAt(pick) - 500.0), 1e-6);

    // The invariant of a correct pick: the returned surface point projects
    // back to the clicked pixel
    const auto roundTrip = camera.worldToScreen(TileMath::geoToWorld(pick), 500.0 * zScale);
    QVERIFY(roundTrip.has_value());
    QCOMPARE_LT(std::hypot(roundTrip->x() - screenPos.x(), roundTrip->y() - screenPos.y()), 0.5);

    // And it lands nearer the camera than the ground-plane pick would
    const QGeoCoordinate plane = camera.coordinateAtScreenPoint(screenPos);
    QCOMPARE_LT(groundDistance(camera.cameraGroundPosition(), TileMath::geoToWorld(pick)),
                groundDistance(camera.cameraGroundPosition(), TileMath::geoToWorld(plane)));
}

void SurfacePatchModelTest::_surfacePickOccludesGroundBehindRidge()
{
    const TileMath::TileKey key = TileMath::tileForWorld(TileMath::geoToWorld(kCenter), 12);
    const double span = TileMath::tileSpanAtZoom(12);
    const QPointF tileCenter = TileMath::tileMinCorner(key) + QPointF(span / 2.0, span / 2.0);
    const double tileMaxY = TileMath::tileMinCorner(key).y() + span;

    GeoMapCamera camera;
    GeoScene scene;
    SurfacePatchModel model;
    setupCamera(camera, TileMath::worldToGeo(tileCenter));
    camera.setTilt(70);
    camera.setDistance(3000);
    attach(model, scene, camera);
    QVERIFY(model.heightField()->insertTile(key, uniformGrid(500.0f)));

    // A ground point north of (behind) the plateau, on screen when terrain is
    // ignored: the ray to it passes below the plateau top on the way there
    const QPointF behind(tileCenter.x(), tileMaxY + 1000.0);
    const auto screenPos = camera.worldToScreen(behind, 0.0);
    QVERIFY(screenPos.has_value());

    const QGeoCoordinate pick = model.surfaceCoordinateAtScreenPoint(&camera, *screenPos, 1.0);
    QVERIFY(pick.isValid());
    const QPointF pickWorld = TileMath::geoToWorld(pick);

    // The pick lands on the plateau top, not on the occluded ground behind it
    QCOMPARE_LE(std::abs(model.terrainHeightAt(pick) - 500.0), 1e-6);
    QCOMPARE_LT(pickWorld.y(), tileMaxY);
    QCOMPARE_LT(groundDistance(camera.cameraGroundPosition(), pickWorld),
                groundDistance(camera.cameraGroundPosition(), behind));
}

void SurfacePatchModelTest::_surfacePickSkyInvalid()
{
    GeoMapCamera camera;
    GeoScene scene;
    SurfacePatchModel model;
    setupCamera(camera);
    camera.setTilt(85);
    attach(model, scene, camera);

    // Top-of-screen ray points above the horizon at max tilt: nothing to hit
    QVERIFY(!model.surfaceCoordinateAtScreenPoint(&camera, QPointF(400, 10), 1.0).isValid());

    // A camera without a viewport cannot form a pick ray
    GeoMapCamera bare;
    QVERIFY(!model.surfaceCoordinateAtScreenPoint(&bare, QPointF(400, 300), 1.0).isValid());
}

void SurfacePatchModelTest::_surfacePickZeroZScaleMatchesPlanePick()
{
    // terrainScale animates to 0 in 2D: even with height data loaded the pick
    // must collapse to the analytic z=0 plane pick
    const TileMath::TileKey key = TileMath::tileForWorld(TileMath::geoToWorld(kCenter), 12);
    const double span = TileMath::tileSpanAtZoom(12);
    const QPointF tileCenter = TileMath::tileMinCorner(key) + QPointF(span / 2.0, span / 2.0);

    GeoMapCamera camera;
    GeoScene scene;
    SurfacePatchModel model;
    setupCamera(camera, TileMath::worldToGeo(tileCenter));
    camera.setTilt(45);
    attach(model, scene, camera);
    QVERIFY(model.heightField()->insertTile(key, uniformGrid(500.0f)));

    const QPointF screenPos(400, 300);
    const QGeoCoordinate pick = model.surfaceCoordinateAtScreenPoint(&camera, screenPos, 0.0);
    const QGeoCoordinate plane = camera.coordinateAtScreenPoint(screenPos);
    QVERIFY(pick.isValid());
    QVERIFY(plane.isValid());
    QCOMPARE_LT(groundDistance(TileMath::geoToWorld(pick), TileMath::geoToWorld(plane)), 0.01);
}

UT_REGISTER_TEST_LIGHTWEIGHT(SurfacePatchModelTest, TestLabel::Unit)
