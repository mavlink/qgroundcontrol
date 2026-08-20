#include "SurfaceAnalysisTest.h"

#include <cmath>
#include <limits>

#include "SurfaceAnalysis.h"
#include "SurfaceModel.h"
#include "TileMath.h"

namespace {

constexpr int kGridSize = 4;
constexpr int kVertexCount = (kGridSize + 1) * (kGridSize + 1);

/// Ready patch with a constant height everywhere
SurfaceModel::Patch readyPatch(const TileMath::TileKey& key, float height)
{
    return SurfaceModel::Patch{key, QList<float>(kVertexCount, height), true, false};
}

/// Pending patch: heights still loading, renders flat unless covered
SurfaceModel::Patch pendingPatch(const TileMath::TileKey& key, bool covered = false)
{
    return SurfaceModel::Patch{key, {}, false, covered};
}

/// Degraded patch: retries exhausted, ready with empty heights (flat fallback)
SurfaceModel::Patch degradedPatch(const TileMath::TileKey& key)
{
    return SurfaceModel::Patch{key, {}, true, false};
}

QRectF worldRect(const TileMath::TileKey& key)
{
    const double span = TileMath::tileSpanAtZoom(key.zoom);
    const QPointF minCorner = TileMath::tileMinCorner(key);
    return QRectF(minCorner.x(), minCorner.y(), span, span);
}

// Adjacent same-zoom pair at full terrain scale (zoom >= 14)
constexpr TileMath::TileKey kWestKey{100, 100, 15};
constexpr TileMath::TileKey kEastKey{101, 100, 15};

// Seam-only analysis: no ground samples skips the coverage-hole scan, no
// height scale skips the camera check
const SurfaceAnalysis::ViewState kSeamsOnly;

constexpr int kSampleGrid = 8;

/// View state whose ground samples cover the rect with a cell-center grid
SurfaceAnalysis::ViewState samplesOver(const QRectF& rect)
{
    SurfaceAnalysis::ViewState view;
    for (int row = 0; row < kSampleGrid; row++) {
        for (int col = 0; col < kSampleGrid; col++) {
            view.groundSamples.append(QPointF(rect.left() + (rect.width() * (col + 0.5) / kSampleGrid),
                                              rect.top() + (rect.height() * (row + 0.5) / kSampleGrid)));
        }
    }
    return view;
}

}  // namespace

void SurfaceAnalysisTest::_noSeamsOnMatchingEdges()
{
    const QList<SurfaceModel::Patch> patches{readyPatch(kWestKey, 25.0f), readyPatch(kEastKey, 25.0f)};

    const SurfaceAnalysis::Report report = SurfaceAnalysis::analyze(patches, kGridSize, kSeamsOnly);
    QVERIFY(report.seams.isEmpty());
    QCOMPARE(report.rendered, 2);
    QCOMPARE(report.pending, 0);
    QCOMPARE(report.degraded, 0);
}

void SurfaceAnalysisTest::_pendingFlatSeam()
{
    const QList<SurfaceModel::Patch> patches{readyPatch(kWestKey, 50.0f), pendingPatch(kEastKey)};

    const SurfaceAnalysis::Report report = SurfaceAnalysis::analyze(patches, kGridSize, kSeamsOnly);
    QCOMPARE(report.seams.count(), 1);
    QCOMPARE(report.pending, 1);

    const SurfaceAnalysis::Seam& seam = report.seams.first();
    QCOMPARE(seam.cause, SurfaceAnalysis::SeamCause::PendingFlat);
    QCOMPARE(seam.maxStep, 50.0);
    QVERIFY(seam.worstAt.isValid());
}

void SurfaceAnalysisTest::_degradedFlatSeam()
{
    const QList<SurfaceModel::Patch> patches{readyPatch(kWestKey, 50.0f), degradedPatch(kEastKey)};

    const SurfaceAnalysis::Report report = SurfaceAnalysis::analyze(patches, kGridSize, kSeamsOnly);
    QCOMPARE(report.seams.count(), 1);
    QCOMPARE(report.degraded, 1);
    QCOMPARE(report.seams.first().cause, SurfaceAnalysis::SeamCause::DegradedFlat);
}

void SurfaceAnalysisTest::_coveredPatchIgnored()
{
    // A covered pending patch does not render, so it cannot form a seam
    const QList<SurfaceModel::Patch> patches{readyPatch(kWestKey, 50.0f), pendingPatch(kEastKey, true)};

    const SurfaceAnalysis::Report report = SurfaceAnalysis::analyze(patches, kGridSize, kSeamsOnly);
    QVERIFY(report.seams.isEmpty());
    QCOMPARE(report.rendered, 1);
}

void SurfaceAnalysisTest::_belowThresholdIgnored()
{
    const QList<SurfaceModel::Patch> patches{readyPatch(kWestKey, 0.0f), readyPatch(kEastKey, 0.5f)};

    const SurfaceAnalysis::Report report = SurfaceAnalysis::analyze(patches, kGridSize, kSeamsOnly);
    QVERIFY(report.seams.isEmpty());
}

void SurfaceAnalysisTest::_sameZoomMismatch()
{
    const QList<SurfaceModel::Patch> patches{readyPatch(kWestKey, 0.0f), readyPatch(kEastKey, 50.0f)};

    const SurfaceAnalysis::Report report = SurfaceAnalysis::analyze(patches, kGridSize, kSeamsOnly);
    QCOMPARE(report.seams.count(), 1);

    const SurfaceAnalysis::Seam& seam = report.seams.first();
    QCOMPARE(seam.cause, SurfaceAnalysis::SeamCause::SameZoomMismatch);
    QCOMPARE(seam.maxStep, 50.0);
    QCOMPARE(seam.patch, kWestKey);
    QCOMPARE(seam.neighbor, kEastKey);
    QCOMPARE(seam.edge, QChar('E'));
}

void SurfaceAnalysisTest::_sameZoomSeamReportedOnce()
{
    // The shared boundary must not be reported from both sides
    const QList<SurfaceModel::Patch> patches{readyPatch(kEastKey, 50.0f), readyPatch(kWestKey, 0.0f)};

    const SurfaceAnalysis::Report report = SurfaceAnalysis::analyze(patches, kGridSize, kSeamsOnly);
    QCOMPARE(report.seams.count(), 1);
}

void SurfaceAnalysisTest::_lodTJunction()
{
    // Coarse (1,1,14) east edge borders fine (4,2,15) west edge; both at full
    // height scale, so the only cause left is the LOD boundary itself
    const TileMath::TileKey coarseKey{1, 1, 14};
    const TileMath::TileKey fineKey{4, 2, 15};

    // Coarse heights vary by row (row * 10): the fine edge spans the coarse
    // edge's northern half, so interpolated coarse heights run 0..20
    QList<float> coarseHeights;
    coarseHeights.reserve(kVertexCount);
    for (int row = 0; row <= kGridSize; row++) {
        for (int col = 0; col <= kGridSize; col++) {
            coarseHeights.append(row * 10.0f);
        }
    }

    const QList<SurfaceModel::Patch> patches{
        SurfaceModel::Patch{coarseKey, coarseHeights, true, false},
        readyPatch(fineKey, 0.0f),
    };

    const SurfaceAnalysis::Report report = SurfaceAnalysis::analyze(patches, kGridSize, kSeamsOnly);
    QCOMPARE(report.seams.count(), 1);

    const SurfaceAnalysis::Seam& seam = report.seams.first();
    QCOMPARE(seam.cause, SurfaceAnalysis::SeamCause::LodTJunction);
    QCOMPARE(seam.patch, fineKey);
    QCOMPARE(seam.neighbor, coarseKey);
    QCOMPARE(seam.edge, QChar('W'));
    QVERIFY(std::abs(seam.maxStep - 20.0) < 1e-6);
}

void SurfaceAnalysisTest::_overlappingAncestorNotASeamNeighbor()
{
    // (2,1,14) contains (4,2,15) entirely (a retiring cover): overlap, not a
    // boundary, so no seam despite the height difference
    const TileMath::TileKey coverKey{2, 1, 14};
    const TileMath::TileKey fineKey{4, 2, 15};

    const QList<SurfaceModel::Patch> patches{readyPatch(coverKey, 0.0f), readyPatch(fineKey, 50.0f)};

    const SurfaceAnalysis::Report report = SurfaceAnalysis::analyze(patches, kGridSize, kSeamsOnly);
    QVERIFY(report.seams.isEmpty());
}

void SurfaceAnalysisTest::_noHolesWhenFullyCovered()
{
    const QList<SurfaceModel::Patch> patches{readyPatch(kWestKey, 0.0f)};

    const SurfaceAnalysis::Report report =
        SurfaceAnalysis::analyze(patches, kGridSize, samplesOver(worldRect(kWestKey)));
    QVERIFY(report.holes.isEmpty());
    QCOMPARE(report.totalSamples, kSampleGrid * kSampleGrid);
}

void SurfaceAnalysisTest::_holeNoPatch()
{
    // Visible region spans two tiles but only the west one is resident
    const QRectF visible = worldRect(kWestKey).united(worldRect(kEastKey));
    const QList<SurfaceModel::Patch> patches{readyPatch(kWestKey, 0.0f)};

    const SurfaceAnalysis::Report report = SurfaceAnalysis::analyze(patches, kGridSize, samplesOver(visible));
    QVERIFY(!report.holes.isEmpty());
    for (const SurfaceAnalysis::Hole& hole : report.holes) {
        QCOMPARE(hole.cause, SurfaceAnalysis::HoleCause::NoPatch);
        QVERIFY(hole.at.isValid());
    }
}

void SurfaceAnalysisTest::_holeSuppressedCovered()
{
    // The east tile is resident but suppressed as covered while nothing
    // actually renders over it: every sample there is a hole
    const QList<SurfaceModel::Patch> patches{readyPatch(kWestKey, 0.0f), pendingPatch(kEastKey, true)};

    const SurfaceAnalysis::Report report =
        SurfaceAnalysis::analyze(patches, kGridSize, samplesOver(worldRect(kEastKey)));
    QVERIFY(!report.holes.isEmpty());
    for (const SurfaceAnalysis::Hole& hole : report.holes) {
        QCOMPARE(hole.cause, SurfaceAnalysis::HoleCause::SuppressedCovered);
        QCOMPARE(hole.patch, kEastKey);
    }
}

void SurfaceAnalysisTest::_holeCheckSkippedWithoutSamples()
{
    const QList<SurfaceModel::Patch> patches{readyPatch(kWestKey, 0.0f)};

    const SurfaceAnalysis::Report report = SurfaceAnalysis::analyze(patches, kGridSize, kSeamsOnly);
    QCOMPARE(report.totalSamples, 0);
    QVERIFY(report.holes.isEmpty());
}

void SurfaceAnalysisTest::_holeElevatedTerrainCulled()
{
    // The eye sits over an unrendered tile at 100 units up; the only rendered
    // patch ahead shows 200 m terrain. The sightline to its flat-ground hit
    // passes below that terrain height while still over the unrendered tile:
    // the screen shows a hole there even though the hit point itself is covered
    const QList<SurfaceModel::Patch> patches{readyPatch(kEastKey, 200.0f)};

    SurfaceAnalysis::ViewState view;
    view.cameraGround = worldRect(kWestKey).center();
    view.cameraHeight = 100.0;
    view.heightScale = 1.0;
    view.groundSamples.append(worldRect(kEastKey).center());

    const SurfaceAnalysis::Report report = SurfaceAnalysis::analyze(patches, kGridSize, view);
    QCOMPARE(report.holes.count(), 1);
    QCOMPARE(report.holes.first().cause, SurfaceAnalysis::HoleCause::ElevatedTerrainCulled);
    QVERIFY(report.holes.first().at.isValid());
    QVERIFY(report.text().contains(
        SurfaceAnalysis::holeCauseDescription(SurfaceAnalysis::HoleCause::ElevatedTerrainCulled)));
}

void SurfaceAnalysisTest::_elevatedTerrainRenderedNoHole()
{
    // Same geometry, but the tile under the eye renders too: the sightline
    // meets rendered terrain, so tall terrain in view is not a hole
    const QList<SurfaceModel::Patch> patches{readyPatch(kWestKey, 200.0f), readyPatch(kEastKey, 200.0f)};

    SurfaceAnalysis::ViewState view;
    view.cameraGround = worldRect(kWestKey).center();
    view.cameraHeight = 100.0;
    view.heightScale = 1.0;
    view.groundSamples.append(worldRect(kEastKey).center());

    const SurfaceAnalysis::Report report = SurfaceAnalysis::analyze(patches, kGridSize, view);
    QVERIFY(report.holes.isEmpty());
}

void SurfaceAnalysisTest::_cameraBelowSurface()
{
    // Camera sits 80 units above the ground plane but the rendered surface
    // there is 100 m * 1.5 scale = 150 units: the view clips into the mesh
    const QList<SurfaceModel::Patch> patches{readyPatch(kWestKey, 100.0f)};

    SurfaceAnalysis::ViewState view;
    view.cameraGround = worldRect(kWestKey).center();
    view.cameraHeight = 80.0;
    view.heightScale = 1.5;

    const SurfaceAnalysis::Report report = SurfaceAnalysis::analyze(patches, kGridSize, view);
    QVERIFY(report.cameraChecked);
    QVERIFY(report.cameraBelowSurface);
    QCOMPARE(report.surfaceAtCamera, 150.0);
    QCOMPARE(report.cameraHeight, 80.0);
    QVERIFY(report.text().contains("CAMERA CLIPS INTO TERRAIN"));
}

void SurfaceAnalysisTest::_cameraAboveSurface()
{
    const QList<SurfaceModel::Patch> patches{readyPatch(kWestKey, 100.0f)};

    SurfaceAnalysis::ViewState view;
    view.cameraGround = worldRect(kWestKey).center();
    view.cameraHeight = 200.0;
    view.heightScale = 1.0;
    view.groundSamples.append(worldRect(kWestKey).center());

    const SurfaceAnalysis::Report report = SurfaceAnalysis::analyze(patches, kGridSize, view);
    QVERIFY(report.cameraChecked);
    QVERIFY(!report.cameraBelowSurface);
    QCOMPARE(report.surfaceAtCamera, 100.0);
    QVERIFY(!report.text().contains("CAMERA CLIPS INTO TERRAIN"));
}

void SurfaceAnalysisTest::_cameraClipsNearbyTerrain()
{
    // Surface directly under the camera is low, but a nearby visible sample
    // (well within cameraHeight range) rises above the camera eye height
    const QList<SurfaceModel::Patch> patches{readyPatch(kWestKey, 10.0f), readyPatch(kEastKey, 200.0f)};

    const QRectF westRect = worldRect(kWestKey);
    SurfaceAnalysis::ViewState view;
    view.cameraGround = QPointF(westRect.right() - 10.0, westRect.center().y());
    view.cameraHeight = 150.0;
    view.heightScale = 1.0;
    view.groundSamples.append(QPointF(westRect.right() + 10.0, westRect.center().y()));

    const SurfaceAnalysis::Report report = SurfaceAnalysis::analyze(patches, kGridSize, view);
    QVERIFY(report.cameraChecked);
    QVERIFY(report.cameraBelowSurface);
    QCOMPARE(report.surfaceAtCamera, 10.0);
    QCOMPARE(report.maxSurfaceNearCamera, 200.0);
    QVERIFY(report.text().contains("CAMERA CLIPS INTO TERRAIN"));
}

void SurfaceAnalysisTest::_noSurfaceUnderCamera()
{
    // Camera ground point sits outside every resident patch (happens at high
    // tilt: nearest visible ground is well in front of the camera): the
    // report must say so instead of claiming a surface height of 0
    const QList<SurfaceModel::Patch> patches{readyPatch(kWestKey, 100.0f)};

    SurfaceAnalysis::ViewState view;
    view.cameraGround = worldRect(kEastKey).center();
    view.cameraHeight = 200.0;
    view.heightScale = 1.0;

    const SurfaceAnalysis::Report report = SurfaceAnalysis::analyze(patches, kGridSize, view);
    QVERIFY(report.cameraChecked);
    QVERIFY(!report.surfaceUnderCamera);
    QVERIFY(!report.cameraBelowSurface);
    QVERIFY(report.text().contains("no rendered terrain under the camera"));
}

void SurfaceAnalysisTest::_cameraCheckSkippedWithoutScale()
{
    const QList<SurfaceModel::Patch> patches{readyPatch(kWestKey, 100.0f)};

    const SurfaceAnalysis::Report report = SurfaceAnalysis::analyze(patches, kGridSize, kSeamsOnly);
    QVERIFY(!report.cameraChecked);
    QVERIFY(!report.cameraBelowSurface);
}

void SurfaceAnalysisTest::_cameraCheckSkippedWithoutHeight()
{
    // heightScale set but cameraHeight left at its default of 0: the camera
    // fields are invalid, so the check must not run (and must not report a
    // false clip against terrain above the 0-height eye)
    const QList<SurfaceModel::Patch> patches{readyPatch(kWestKey, 100.0f)};

    SurfaceAnalysis::ViewState view;
    view.cameraGround = worldRect(kWestKey).center();
    view.heightScale = 1.0;

    const SurfaceAnalysis::Report report = SurfaceAnalysis::analyze(patches, kGridSize, view);
    QVERIFY(!report.cameraChecked);
    QVERIFY(!report.cameraBelowSurface);
    QVERIFY(!report.text().contains("CAMERA CLIPS INTO TERRAIN"));
}

void SurfaceAnalysisTest::_nonFiniteHeights()
{
    QList<float> heights(kVertexCount, 10.0f);
    heights[3] = std::numeric_limits<float>::quiet_NaN();
    heights[7] = std::numeric_limits<float>::infinity();
    const QList<SurfaceModel::Patch> patches{SurfaceModel::Patch{kWestKey, heights, true, false}};

    const SurfaceAnalysis::Report report = SurfaceAnalysis::analyze(patches, kGridSize, kSeamsOnly);
    QCOMPARE(report.badHeights.count(), 1);
    QCOMPARE(report.badHeights.first().key, kWestKey);
    QCOMPARE(report.badHeights.first().count, 2);
}

void SurfaceAnalysisTest::_reportText()
{
    // One seam plus one coverage hole: both must show up in the text
    const QRectF visible =
        worldRect(kWestKey).united(worldRect(kEastKey)).adjusted(0, 0, worldRect(kEastKey).width(), 0);
    const QList<SurfaceModel::Patch> patches{readyPatch(kWestKey, 0.0f), readyPatch(kEastKey, 50.0f)};

    const SurfaceAnalysis::Report report = SurfaceAnalysis::analyze(patches, kGridSize, samplesOver(visible));
    const QString text = report.text();

    QVERIFY(!text.isEmpty());
    QVERIFY(text.contains(SurfaceAnalysis::seamCauseDescription(SurfaceAnalysis::SeamCause::SameZoomMismatch)));
    QVERIFY(text.contains(SurfaceAnalysis::holeCauseDescription(SurfaceAnalysis::HoleCause::NoPatch)));
    QVERIFY(text.contains("50.0"));

    QVERIFY(!SurfaceAnalysis::seamCauseDescription(SurfaceAnalysis::SeamCause::PendingFlat).isEmpty());
    QVERIFY(SurfaceAnalysis::seamCauseDescription(SurfaceAnalysis::SeamCause::PendingFlat) !=
            SurfaceAnalysis::seamCauseDescription(SurfaceAnalysis::SeamCause::DegradedFlat));
}

UT_REGISTER_TEST_LIGHTWEIGHT(SurfaceAnalysisTest, TestLabel::Unit)
