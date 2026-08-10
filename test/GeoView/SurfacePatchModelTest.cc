#include "SurfacePatchModelTest.h"

#include <QtTest/QSignalSpy>
#include <cmath>

#include "GeoScene.h"
#include "GeoViewCamera.h"
#include "SurfaceModel.h"
#include "SurfacePatchModel.h"
#include "TileMath.h"

namespace {

const QGeoCoordinate kCenter(47.3977419, 8.5455938);
constexpr QSizeF kViewport(800, 600);

void setupCamera(GeoViewCamera& camera, const QGeoCoordinate& center = kCenter)
{
    camera.setViewportSize(kViewport);
    camera.lookAt(center, 0, 0, 2000);
}

void attach(SurfacePatchModel& model, GeoScene& scene, GeoViewCamera& camera)
{
    scene.setCamera(&camera);
    model.setScene(&scene);
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
    GeoViewCamera camera;
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
    GeoViewCamera camera;
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
    GeoViewCamera camera;
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
    QCOMPARE(resetSpy.count(), 0);
    QCOMPARE_GT(insertSpy.count() + removeSpy.count(), 0);
    QCOMPARE(model.rowCount(), model.patchCount());
}

void SurfacePatchModelTest::_reanchorsOnLargeMove()
{
    GeoViewCamera camera;
    GeoScene scene;
    SurfacePatchModel model;
    setupCamera(camera);
    attach(model, scene, camera);

    QSignalSpy originSpy(&scene, &GeoScene::sceneOriginChanged);

    // Move far beyond kReanchorDistance: origin follows the camera
    const QGeoCoordinate faraway(48.85, 2.35);  // Paris, ~490km from Zurich
    camera.setCenter(faraway);
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
    GeoViewCamera zurichCamera;
    GeoScene scene;
    SurfacePatchModel model;
    setupCamera(zurichCamera);
    attach(model, scene, zurichCamera);

    // Swap to a static camera on another continent: the origin must follow even
    // though the new camera's center never changes after the swap
    GeoViewCamera sydneyCamera;
    const QGeoCoordinate sydney(-33.8688, 151.2093);
    setupCamera(sydneyCamera, sydney);
    scene.setCamera(&sydneyCamera);

    const QPointF expectedOrigin = TileMath::geoToWorld(sydney);
    QCOMPARE_LT(std::hypot(scene.sceneOrigin().x() - expectedOrigin.x(), scene.sceneOrigin().y() - expectedOrigin.y()),
                1.0);
    // The model rebuilt against the new camera
    QCOMPARE_GT(model.rowCount(), 0);
}

void SurfacePatchModelTest::_debugHillsSwitchResets()
{
    GeoViewCamera camera;
    GeoScene scene;
    SurfacePatchModel model;
    setupCamera(camera);
    attach(model, scene, camera);
    QTRY_COMPARE_WITH_TIMEOUT(model.pendingCount(), 0, 5000);

    QSignalSpy resetSpy(&model, &QAbstractItemModel::modelReset);
    model.setDebugHills(true);
    QCOMPARE_GT(resetSpy.count(), 0);
    QCOMPARE_GT(model.rowCount(), 0);
    QTRY_COMPARE_WITH_TIMEOUT(model.pendingCount(), 0, 5000);

    // Debug hills deliver non-flat heights
    bool nonZeroSeen = false;
    for (int row = 0; (row < model.rowCount()) && !nonZeroSeen; row++) {
        const auto heights = model.data(model.index(row), SurfacePatchModel::HeightsRole).value<QList<float>>();
        for (float h : heights) {
            if (h > 0.0f) {
                nonZeroSeen = true;
                break;
            }
        }
    }
    QVERIFY(nonZeroSeen);
}

void SurfacePatchModelTest::_pendingRowsCoveredDuringLodChurn()
{
    GeoViewCamera camera;
    GeoScene scene;
    SurfacePatchModel model;
    camera.setViewportSize(kViewport);
    camera.lookAt(kCenter, 0, 0, GeoViewCamera::kMaxDistance);
    attach(model, scene, camera);
    QTRY_COMPARE_WITH_TIMEOUT(model.pendingCount(), 0, 5000);

    // Refine: pending replacements report covered so the delegate hides their
    // empty flat mesh instead of z-fighting the retained retiring cover
    camera.lookAt(kCenter, 0, 0, 2000);
    QCOMPARE_GT(model.pendingCount(), 0);
    int coveredRows = 0;
    for (int row = 0; row < model.rowCount(); row++) {
        const QModelIndex idx = model.index(row);
        const bool ready = model.data(idx, SurfacePatchModel::ReadyRole).toBool();
        const bool covered = model.data(idx, SurfacePatchModel::CoveredRole).toBool();
        QCOMPARE(covered, !ready);
        if (covered) {
            coveredRows++;
        }
    }
    QCOMPARE_GT(coveredRows, 0);

    QTRY_COMPARE_WITH_TIMEOUT(model.pendingCount(), 0, 5000);
    for (int row = 0; row < model.rowCount(); row++) {
        QVERIFY(!model.data(model.index(row), SurfacePatchModel::CoveredRole).toBool());
    }
}

UT_REGISTER_TEST_LIGHTWEIGHT(SurfacePatchModelTest, TestLabel::Unit)
