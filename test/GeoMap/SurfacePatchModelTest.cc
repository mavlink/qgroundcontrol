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

UT_REGISTER_TEST_LIGHTWEIGHT(SurfacePatchModelTest, TestLabel::Unit)
