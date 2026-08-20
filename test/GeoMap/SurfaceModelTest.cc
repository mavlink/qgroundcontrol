#include "SurfaceModelTest.h"

#include <QtCore/QByteArray>
#include <QtCore/QCoreApplication>
#include <QtCore/QHash>
#include <QtCore/QLatin1StringView>
#include <QtCore/QSet>
#include <QtTest/QSignalSpy>

#include <algorithm>
#include <cmath>
#include <utility>

#include "ElevationTilePyramid.h"
#include "GeoMapCamera.h"
#include "HeightField.h"
#include "HeightSource.h"
#include "PatchGeometry.h"
#include "SurfaceModel.h"
#include "TileMath.h"

namespace {

const QGeoCoordinate kCenter(47.3977419, 8.5455938);
constexpr QSizeF kViewport(800, 600);

/// Constant-height decoded tile grid
ElevationTilePyramid::Grid constantGrid(float height, int size = 8)
{
    ElevationTilePyramid::Grid grid;
    grid.width = size;
    grid.height = size;
    grid.heights = QList<float>(qsizetype(size) * size, height);
    return grid;
}

QRectF tileRect(const TileMath::TileKey& key)
{
    const double span = TileMath::tileSpanAtZoom(key.zoom);
    return QRectF(TileMath::tileMinCorner(key), QSizeF(span, span));
}

/// Inflated-rect region contact, matching the model's re-mesh predicate:
/// patch edge vertices exactly on the region boundary sample the changed
/// data, but QRectF::intersects is false for edge-only contact
bool touchesRegion(const TileMath::TileKey& key, const QRectF& region)
{
    const QRectF rect = tileRect(key);
    const double margin = rect.width() * 1e-6;
    return rect.marginsAdded(QMarginsF(margin, margin, margin, margin)).intersects(region);
}

/// Flat source that records coverage requests (the field stays empty)
class RecordingCoverageSource : public FlatHeightSource
{
public:
    using FlatHeightSource::FlatHeightSource;

    bool requestTile(const TileMath::TileKey& key) override
    {
        requested.append(key);
        return false;
    }

    QList<TileMath::TileKey> requested;
};

/// Multi-octave analytic terrain rough at every patch scale, so any
/// unconstrained LOD edge shows a real height mismatch
class RoughHeightSource : public ProceduralHeightSource
{
public:
    using ProceduralHeightSource::ProceduralHeightSource;

    /// Lipschitz bound on the octave sum: |∂f/∂x| ≤ Σ amplitude/xScale =
    /// 300/20000 + 150/1900 + 80/280 + 40/61 ≈ 1.04, |∂f/∂y| ≈ 1.01,
    /// |∇f| ≤ √(1.04² + 1.01²) < 1.45
    static constexpr double kMaxGradient = 1.45;

    /// Total possible height swing: 2 × Σ octave amplitudes
    static constexpr double kHeightRange = 2.0 * (300.0 + 150.0 + 80.0 + 40.0);

protected:
    float heightAtWorld(const QPointF& world) const override
    {
        const double x = world.x();
        const double y = world.y();
        return static_cast<float>((300.0 * std::sin(x / 20000.0) * std::cos(y / 26000.0)) +
                                  (150.0 * std::sin(x / 1900.0) * std::cos(y / 1300.0)) +
                                  (80.0 * std::sin((x / 280.0) + 1.0) * std::cos(y / 240.0)) +
                                  (40.0 * std::sin(x / 61.0) * std::cos(y / 73.0)));
    }
};

constexpr int kFloatsPerVertex = 8;  // position 3, normal 3, uv 2

/// z of grid vertex (row, col) in a built PatchGeometry mesh (grid vertices
/// precede skirt vertices, row-major from the north-west corner)
double meshZ(const QByteArray& vertexData, int row, int col)
{
    const int vpe = SurfaceModel::kGridSize + 1;
    const float* vertex =
        reinterpret_cast<const float*>(vertexData.constData()) + (qsizetype((row * vpe) + col) * kFloatsPerVertex);
    return double(vertex[2]);
}

/// Rendered height along a mesh edge at fractional grid coordinate t: the
/// mesh interpolates linearly between adjacent edge vertices. edge is
/// 'N','S','W','E'.
double meshEdgeHeight(const QByteArray& vertexData, QChar edge, double t)
{
    constexpr int kGrid = SurfaceModel::kGridSize;
    const auto vertex = [&](int idx) {
        switch (edge.unicode()) {
            case u'N':
                return meshZ(vertexData, 0, idx);
            case u'S':
                return meshZ(vertexData, kGrid, idx);
            case u'W':
                return meshZ(vertexData, idx, 0);
            default:
                return meshZ(vertexData, idx, kGrid);
        }
    };
    t = std::clamp(t, 0.0, double(kGrid));
    const int i0 = std::min(int(std::floor(t)), kGrid - 1);
    const double frac = t - i0;
    return (vertex(i0) * (1.0 - frac)) + (vertex(i0 + 1) * frac);
}

/// Largest rendered mismatch between two patch meshes along the world
/// interval [lo, hi] of their shared edge line (horizontal: runs east-west)
double segmentMaxStep(const QByteArray& meshA, QChar edgeA, const QRectF& rectA, const QByteArray& meshB, QChar edgeB,
                      const QRectF& rectB, bool horizontal, double lo, double hi)
{
    constexpr int kGrid = SurfaceModel::kGridSize;
    constexpr int kSamples = 65;  // denser than any vertex spacing involved
    double maxStep = 0.0;
    for (int i = 0; i <= kSamples; i++) {
        const double pos = lo + ((hi - lo) * i / kSamples);
        // Fractional grid coordinate of pos on each patch's edge. Slippy y
        // grows south while world y grows north: row/col index t runs from
        // the NW corner, so along x t follows +x, along y t follows -y.
        const double tA = horizontal ? ((pos - rectA.left()) / rectA.width()) * kGrid
                                     : ((rectA.bottom() - pos) / rectA.height()) * kGrid;
        const double tB = horizontal ? ((pos - rectB.left()) / rectB.width()) * kGrid
                                     : ((rectB.bottom() - pos) / rectB.height()) * kGrid;
        maxStep = std::max(maxStep, std::abs(meshEdgeHeight(meshA, edgeA, tA) - meshEdgeHeight(meshB, edgeB, tB)));
    }
    return maxStep;
}

int maxZoomOf(const QList<SurfaceModel::Patch>& patches)
{
    int maxZoom = 0;
    for (const SurfaceModel::Patch& patch : patches) {
        maxZoom = std::max(maxZoom, patch.key.zoom);
    }
    return maxZoom;
}

/// Zoom of the finest resident patch containing the world point, or -1
int finestResidentZoomAt(const QList<SurfaceModel::Patch>& patches, const QPointF& world)
{
    int zoom = -1;
    for (const SurfaceModel::Patch& patch : patches) {
        if (tileRect(patch.key).contains(world)) {
            zoom = std::max(zoom, patch.key.zoom);
        }
    }
    return zoom;
}

/// Screen-grid coverage check independent of the model's own visibility
/// estimate: every pick-ray ground hit within the coverage contract range
/// must be inside a resident patch. Returns a failure description or empty.
QString coverageHole(const SurfaceModel& model, const GeoMapCamera& camera)
{
    constexpr int kCols = 13;
    constexpr int kRows = 9;
    // Slightly inside the full contract: the range cap itself is exact only
    // along sampled rays
    const double demandRange = SurfaceModel::kMaxRangeMultiplier * camera.distance() * 0.9;
    const QPointF cameraGround = camera.cameraGroundPosition();
    const QList<SurfaceModel::Patch> patches = model.patches();
    for (int row = 0; row < kRows; row++) {
        for (int col = 0; col < kCols; col++) {
            const QPointF screenPos((camera.viewportSize().width() * col) / (kCols - 1),
                                    (camera.viewportSize().height() * row) / (kRows - 1));
            const auto ground = camera.screenToGround(screenPos);
            if (!ground) {
                continue;  // sky
            }
            const double half = TileMath::worldSize() / 2.0;
            if ((std::abs(ground->x()) > half) || (std::abs(ground->y()) > half)) {
                continue;  // off-world: no tile exists beyond the mercator square
            }
            const double range = std::hypot(ground->x() - cameraGround.x(), ground->y() - cameraGround.y());
            if (range > demandRange) {
                continue;  // beyond the coverage contract
            }
            if (finestResidentZoomAt(patches, *ground) < 0) {
                return QStringLiteral("hole at screen (%1,%2) world (%3,%4) range %5 (tilt %6 dist %7 heading %8)")
                    .arg(screenPos.x())
                    .arg(screenPos.y())
                    .arg(ground->x())
                    .arg(ground->y())
                    .arg(range)
                    .arg(camera.tilt())
                    .arg(camera.distance())
                    .arg(camera.heading());
            }
        }
    }
    return {};
}

}  // namespace

void SurfaceModelTest::_patchRendersEstimateImmediately()
{
    GeoMapCamera camera;
    FlatHeightSource source;
    HeightField field;
    QVERIFY(field.insertTile(TileMath::TileKey{0, 0, 0}, constantGrid(100.0f)));
    SurfaceModel model(&camera, &source, &field);

    camera.setViewportSize(kViewport);
    camera.lookAt(kCenter, 0, 0, 2000);
    model.drainUpdates();

    // No event-loop wait: every patch must carry the field estimate the
    // moment it is added (never flat-zero-with-hole)
    QCOMPARE_GT(model.patchCount(), 0);
    const int expectedHeights = (SurfaceModel::kGridSize + 1) * (SurfaceModel::kGridSize + 1);
    for (const SurfaceModel::Patch& patch : model.patches()) {
        QVERIFY(patch.ready);
        QCOMPARE(patch.heights.count(), expectedHeights);
        QCOMPARE(patch.heights.first(), 100.0f);
    }
}

void SurfaceModelTest::_patchesAreViewsOfField()
{
    GeoMapCamera camera;
    FlatHeightSource source;
    HeightField field;
    ElevationTilePyramid::Grid grid;
    grid.width = 8;
    grid.height = 8;
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            grid.heights.append(float((row * 31) + (col * 7)));
        }
    }
    QVERIFY(field.insertTile(TileMath::TileKey{0, 0, 0}, std::move(grid)));
    SurfaceModel model(&camera, &source, &field);

    camera.setViewportSize(kViewport);
    camera.lookAt(kCenter, 0, 0, 2000);
    model.drainUpdates();

    // Patches are views: their heights are exactly the field's samples, so
    // any two patches asking about the same world position always agree
    QCOMPARE_GT(model.patchCount(), 0);
    for (const SurfaceModel::Patch& patch : model.patches()) {
        QCOMPARE(patch.heights, field.samplePatch(patch.key, SurfaceModel::kGridSize));
    }
}

void SurfaceModelTest::_reMeshOnDataArrival()
{
    GeoMapCamera camera;
    FlatHeightSource source;
    HeightField field;
    SurfaceModel model(&camera, &source, &field);

    camera.setViewportSize(kViewport);
    camera.lookAt(kCenter, 0, 0, 2000);
    model.drainUpdates();
    QCOMPARE_GT(model.patchCount(), 0);
    for (const SurfaceModel::Patch& patch : model.patches()) {
        QCOMPARE(patch.heights.first(), 0.0f);  // empty field: flat estimate
    }

    QSignalSpy meshSpy(&model, &SurfaceModel::patchMeshChanged);
    QVERIFY(field.insertTile(TileMath::TileKey{0, 0, 0}, constantGrid(100.0f)));

    // Data arrival re-meshes synchronously: the patches lift, no stale flats
    QCOMPARE_GT(meshSpy.count(), 0);
    for (const SurfaceModel::Patch& patch : model.patches()) {
        QCOMPARE(patch.heights.first(), 100.0f);
    }
}

void SurfaceModelTest::_reMeshScopeGate()
{
    GeoMapCamera camera;
    FlatHeightSource source;
    HeightField field;
    SurfaceModel model(&camera, &source, &field);

    camera.setViewportSize(kViewport);
    camera.lookAt(kCenter, 0, 45, 2000);
    model.drainUpdates();
    QCOMPARE_GT(model.patchCount(), 8);

    // A fine tile at the camera position changes a small region only
    const int fineZoom = maxZoomOf(model.patches());
    const TileMath::TileKey fineKey = TileMath::tileForWorld(TileMath::geoToWorld(kCenter), fineZoom);
    const QRectF region = tileRect(fineKey);

    QSignalSpy meshSpy(&model, &SurfaceModel::patchMeshChanged);
    QVERIFY(field.insertTile(fineKey, constantGrid(50.0f)));

    // Gate: exactly the patches touching the changed region re-mesh, once each
    QSet<TileMath::TileKey> remeshed;
    for (const QList<QVariant>& args : meshSpy) {
        remeshed.insert(args.first().value<TileMath::TileKey>());
    }
    int touching = 0;
    for (const SurfaceModel::Patch& patch : model.patches()) {
        if (touchesRegion(patch.key, region)) {
            QVERIFY2(remeshed.contains(patch.key), "patch touching the changed region was not re-meshed");
            touching++;
        } else {
            QVERIFY2(!remeshed.contains(patch.key), "patch outside the changed region was re-meshed");
        }
    }
    QCOMPARE_GT(touching, 0);
    QCOMPARE_LT(touching, model.patchCount());
    QCOMPARE(meshSpy.count(), touching);
}

void SurfaceModelTest::_requestsTileCoverage()
{
    GeoMapCamera camera;
    RecordingCoverageSource source;
    HeightField field;
    source.setHeightField(&field);
    SurfaceModel model(&camera, &source, &field);

    camera.setViewportSize(kViewport);
    camera.lookAt(kCenter, 0, 0, 2000);
    model.drainUpdates();

    QCOMPARE_GT(model.patchCount(), 0);
    for (const SurfaceModel::Patch& patch : model.patches()) {
        QVERIFY2(source.requested.contains(patch.key), "no tile coverage requested for resident patch");
    }
}

void SurfaceModelTest::_noViewportNoPatches()
{
    GeoMapCamera camera;
    FlatHeightSource source;
    HeightField field;
    SurfaceModel model(&camera, &source, &field);

    model.update();
    QCOMPARE(model.patchCount(), 0);
}

void SurfaceModelTest::_unpositionedCameraNoPatches()
{
    GeoMapCamera camera;
    FlatHeightSource source;
    HeightField field;
    SurfaceModel model(&camera, &source, &field);

    // A sized viewport alone must not build patches: the camera still holds
    // the null-island default pose (doomed terrain fetches would follow)
    camera.setViewportSize(kViewport);
    model.drainUpdates();
    QCOMPARE(model.patchCount(), 0);

    camera.setCenter(kCenter);
    model.drainUpdates();
    QCOMPARE_GT(model.patchCount(), 0);
}

void SurfaceModelTest::_coarseWhenFar()
{
    GeoMapCamera camera;
    FlatHeightSource source;
    HeightField field;
    SurfaceModel model(&camera, &source, &field);

    camera.setViewportSize(kViewport);
    camera.lookAt(kCenter, 0, 0, GeoMapCamera::kMaxDistance);
    model.drainUpdates();

    QCOMPARE_GT(model.patchCount(), 0);
    // From max distance the whole world fits the view: only low zoom levels
    QCOMPARE_LE(maxZoomOf(model.patches()), 4);
}

void SurfaceModelTest::_refinesWhenNear()
{
    GeoMapCamera camera;
    FlatHeightSource source;
    HeightField field;
    SurfaceModel model(&camera, &source, &field);

    camera.setViewportSize(kViewport);
    camera.lookAt(kCenter, 0, 0, GeoMapCamera::kMaxDistance);
    model.drainUpdates();
    const int farMaxZoom = maxZoomOf(model.patches());

    camera.lookAt(kCenter, 0, 0, 2000);
    model.drainUpdates();
    const int nearMaxZoom = maxZoomOf(model.patches());

    QCOMPARE_GT(nearMaxZoom, farMaxZoom);
    QCOMPARE_GT(model.patchCount(), 0);
}

void SurfaceModelTest::_patchCountBounded()
{
    GeoMapCamera camera;
    FlatHeightSource source;
    HeightField field;
    SurfaceModel model(&camera, &source, &field);

    camera.setViewportSize(kViewport);
    for (double distance : {50.0, 2000.0, 100000.0, GeoMapCamera::kMaxDistance}) {
        for (double tilt : {0.0, 45.0, GeoMapCamera::kMaxTilt}) {
            camera.lookAt(kCenter, 30, tilt, distance);
            model.drainUpdates();
            QCOMPARE_LE(model.patchCount(), SurfaceModel::kMaxPatches);
        }
    }
}

void SurfaceModelTest::_budgetExhaustedKeepsCoverage()
{
    GeoMapCamera camera;
    FlatHeightSource source;
    HeightField field;
    SurfaceModel model(&camera, &source, &field);

    // A huge viewport shrinks meters-per-pixel so refinement demand far
    // exceeds the patch budget
    camera.setViewportSize(QSizeF(8000, 6000));
    camera.lookAt(kCenter, 0, 45, 2000);
    model.drainUpdates();

    QCOMPARE_LE(model.patchCount(), SurfaceModel::kMaxPatches);
    // A split replaces one patch with up to four children, so an exhausted
    // budget lands within 3 of the cap. Anything lower means demand never
    // hit the cap and this test exercises nothing.
    QCOMPARE_GE(model.patchCount(), SurfaceModel::kMaxPatches - 3);

    // Coverage-preserving cap: every visible ground point must fall inside a
    // resident patch (holes were possible before the budget-aware traversal)
    const QList<SurfaceModel::Patch> patches = model.patches();
    const double maxRange = camera.distance() * SurfaceModel::kMaxRangeMultiplier;
    const double half = TileMath::worldSize() / 2.0;
    constexpr int sampleGrid = 9;
    for (int row = 0; row < sampleGrid; row++) {
        for (int col = 0; col < sampleGrid; col++) {
            const QPointF screenPos((camera.viewportSize().width() * col) / (sampleGrid - 1),
                                    (camera.viewportSize().height() * row) / (sampleGrid - 1));
            const auto ground = camera.groundPointCapped(screenPos, maxRange);
            if (!ground || (qAbs(ground->x()) > half) || (qAbs(ground->y()) > half)) {
                continue;  // horizon miss or capped outside the mercator world
            }
            bool covered = false;
            for (const SurfaceModel::Patch& patch : patches) {
                const double span = TileMath::tileSpanAtZoom(patch.key.zoom);
                const QPointF minCorner = TileMath::tileMinCorner(patch.key);
                if (QRectF(minCorner.x(), minCorner.y(), span, span).contains(*ground)) {
                    covered = true;
                    break;
                }
            }
            QVERIFY2(
                covered,
                qPrintable(
                    QStringLiteral("uncovered ground point at screen (%1, %2)").arg(screenPos.x()).arg(screenPos.y())));
        }
    }
}

void SurfaceModelTest::_diffOnMove()
{
    GeoMapCamera camera;
    FlatHeightSource source;
    HeightField field;
    SurfaceModel model(&camera, &source, &field);

    camera.setViewportSize(kViewport);
    camera.lookAt(kCenter, 0, 0, 2000);
    model.drainUpdates();

    QSignalSpy addedSpy(&model, &SurfaceModel::patchAdded);
    QSignalSpy removedSpy(&model, &SurfaceModel::patchRemoved);

    // Move to the other side of the planet: full set replacement
    camera.setCenter(QGeoCoordinate(-33.8688, 151.2093));
    model.drainUpdates();
    QCOMPARE_GT(addedSpy.count(), 0);
    QCOMPARE_GT(removedSpy.count(), 0);
}

void SurfaceModelTest::_noChurnOnIdenticalUpdate()
{
    GeoMapCamera camera;
    FlatHeightSource source;
    HeightField field;
    SurfaceModel model(&camera, &source, &field);

    camera.setViewportSize(kViewport);
    camera.lookAt(kCenter, 0, 0, 2000);
    model.drainUpdates();

    QSignalSpy addedSpy(&model, &SurfaceModel::patchAdded);
    QSignalSpy removedSpy(&model, &SurfaceModel::patchRemoved);
    model.update();
    QCOMPARE(addedSpy.count(), 0);
    QCOMPARE(removedSpy.count(), 0);
}

void SurfaceModelTest::_cullsInvisibleRegion()
{
    GeoMapCamera camera;
    FlatHeightSource source;
    HeightField field;
    SurfaceModel model(&camera, &source, &field);

    camera.setViewportSize(kViewport);
    camera.lookAt(kCenter, 0, 0, 2000);
    model.drainUpdates();

    // At 2km altitude over Zurich the antipodean hemisphere must not be resident
    const QPointF sydney = TileMath::geoToWorld(QGeoCoordinate(-33.8688, 151.2093));
    for (const SurfaceModel::Patch& patch : model.patches()) {
        const double span = TileMath::tileSpanAtZoom(patch.key.zoom);
        const QPointF minCorner = TileMath::tileMinCorner(patch.key);
        const QRectF rect(minCorner.x(), minCorner.y(), span, span);
        QVERIFY(!rect.contains(sydney) || (patch.key.zoom == 0));
    }
}

void SurfaceModelTest::_coverageMaintainedDuringLodChurn()
{
    GeoMapCamera camera;
    FlatHeightSource source;
    HeightField field;
    SurfaceModel model(&camera, &source, &field);

    camera.setViewportSize(kViewport);
    camera.lookAt(kCenter, 0, 0, GeoMapCamera::kMaxDistance);
    model.drainUpdates();
    QCOMPARE_GT(model.patchCount(), 0);

    // Refine toward the ground one capped pass at a time: after every single
    // pass the resident set must still cover the visible region — a coarse
    // patch may only go once its replacements are resident
    camera.lookAt(kCenter, 0, 0, 2000);
    const double maxRange = camera.distance() * SurfaceModel::kMaxRangeMultiplier;
    const double half = TileMath::worldSize() / 2.0;
    constexpr int sampleGrid = 5;
    int passes = 0;
    do {
        model.update();
        passes++;
        const QList<SurfaceModel::Patch> patches = model.patches();
        for (int row = 0; row < sampleGrid; row++) {
            for (int col = 0; col < sampleGrid; col++) {
                const QPointF screenPos((camera.viewportSize().width() * col) / (sampleGrid - 1),
                                        (camera.viewportSize().height() * row) / (sampleGrid - 1));
                const auto ground = camera.groundPointCapped(screenPos, maxRange);
                if (!ground || (qAbs(ground->x()) > half) || (qAbs(ground->y()) > half)) {
                    continue;
                }
                bool covered = false;
                for (const SurfaceModel::Patch& patch : patches) {
                    if (tileRect(patch.key).contains(*ground)) {
                        covered = true;
                        break;
                    }
                }
                QVERIFY2(covered, qPrintable(QStringLiteral("hole after pass %1 at screen (%2, %3)")
                                                 .arg(passes)
                                                 .arg(screenPos.x())
                                                 .arg(screenPos.y())));
            }
        }
    } while (!model.updateSettled() && (passes < 400));
    QVERIFY(model.updateSettled());
}

void SurfaceModelTest::_renderedEdgeContractAcrossLodBoundaries()
{
    // Layered edge contract, replacing the retired 0.5 m
    // C0-exactness assertion, which promised more
    // than sample-center tiles can deliver. Across every shared patch edge of
    // the real rendered meshes:
    // (a) neighbors resolved from the same backing tile match bit-exactly;
    // (T) a stitched fine edge lies on its coarse neighbor's rendered
    //     polyline to float-lerp precision;
    // (b) same-level neighbors backed by different tiles mismatch at most by
    //     the source's variation over each side's sampling reach — the
    //     residual that skirts hide (contract (c), the skirt sizing step).
    // Corner vertices can be captured by a perpendicular edge's constraint
    // (N,S,W,E precedence, see PatchGeometry::_heightAt), pulling the
    // outermost mesh segment off this neighbor's polyline; skirts hide those
    // corner nicks, so (a)/(T) apply to the segment interior (one coarse
    // render cell in from each end) and the full segment gets the (b) bound.
    GeoMapCamera camera;
    RoughHeightSource source;
    HeightField field;
    source.setHeightField(&field);
    SurfaceModel model(&camera, &source, &field);
    camera.setViewportSize(kViewport);

    // Per-side sampling reach of a rendered edge value around its query
    // point: one render cell (mesh/constraint lerp span) plus 1.5 backing
    // texels (bilinear support + the half-texel border clamp of
    // sample-center grids); the source varies by at most kMaxGradient·reach
    const auto reach = [](int renderZoom, int backingZoom) {
        return (TileMath::tileSpanAtZoom(renderZoom) / SurfaceModel::kGridSize) +
               (1.5 * TileMath::tileSpanAtZoom(backingZoom) / ProceduralHeightSource::kSynthGridSize);
    };
    const auto stepBound = [&reach](int renderZoomA, int backingZoomA, int renderZoomB, int backingZoomB) {
        return std::min(
            RoughHeightSource::kMaxGradient * (reach(renderZoomA, backingZoomA) + reach(renderZoomB, backingZoomB)),
            RoughHeightSource::kHeightRange);
    };

    // Oblique poses with a far horizon maximize the LOD spread across the
    // resident set (the screenshot pose family)
    struct Pose
    {
        qreal tilt;
        qreal distance;
    };

    const QList<Pose> poses = {{80, 3000}, {GeoMapCamera::kMaxTilt, 15000}};

    int stitchedPairs = 0;  // (T) pairs seen: proves the stitch contract ran
    for (const Pose& pose : poses) {
        camera.lookAt(kCenter, 0, pose.tilt, pose.distance);

        // Settle: drain model churn, deliver queued tile inserts, and repeat
        // until a full pass changes nothing (deliveries re-mesh and can
        // re-cull, which can request more tiles)
        QSignalSpy regionSpy(&field, &HeightField::regionChanged);
        int guard = 200;
        do {
            regionSpy.clear();
            model.drainUpdates();
            QCoreApplication::processEvents();
        } while (((regionSpy.count() > 0) || !model.updateSettled()) && (--guard > 0));
        QVERIFY(guard > 0);

        const QList<SurfaceModel::Patch> patches = model.patches();
        QCOMPARE_GT(patches.count(), 8);

        // Build each patch's real rendered mesh exactly as GeoMap.qml binds
        // PatchGeometry: model heights + edge deltas
        QHash<TileMath::TileKey, QByteArray> meshes;
        for (const SurfaceModel::Patch& patch : patches) {
            QVERIFY(patch.ready);
            PatchGeometry geometry;
            geometry.setGridSize(SurfaceModel::kGridSize);
            geometry.setSpan(tileRect(patch.key).width());
            geometry.setHeights(patch.heights);
            geometry.setEdgeLodDeltas(model.edgeLodDeltas(patch.key));
            meshes.insert(patch.key, geometry.vertexData());
        }

        for (const SurfaceModel::Patch& a : patches) {
            const QRectF rectA = tileRect(a.key);
            for (const SurfaceModel::Patch& b : patches) {
                if (a.key == b.key) {
                    continue;
                }
                const QRectF rectB = tileRect(b.key);
                for (const QChar edgeA : {u'N', u'S', u'W', u'E'}) {
                    // The world line A's edge lies on; B must sit across it
                    const double edgeLineA = (edgeA == u'N')   ? rectA.bottom()  // north = max y
                                             : (edgeA == u'S') ? rectA.top()
                                             : (edgeA == u'W') ? rectA.left()
                                                               : rectA.right();
                    const double edgeLineB = (edgeA == u'N')   ? rectB.top()
                                             : (edgeA == u'S') ? rectB.bottom()
                                             : (edgeA == u'W') ? rectB.right()
                                                               : rectB.left();
                    const double tolerance = std::min(rectA.width(), rectB.width()) * 1e-9;
                    if (std::abs(edgeLineA - edgeLineB) > tolerance) {
                        continue;
                    }
                    const bool horizontal = (edgeA == u'N') || (edgeA == u'S');
                    const double lo =
                        horizontal ? std::max(rectA.left(), rectB.left()) : std::max(rectA.top(), rectB.top());
                    const double hi =
                        horizontal ? std::min(rectA.right(), rectB.right()) : std::min(rectA.bottom(), rectB.bottom());
                    if (hi <= lo) {
                        continue;
                    }
                    const QChar edgeB = (edgeA == u'N') ? u'S' : (edgeA == u'S') ? u'N' : (edgeA == u'W') ? u'E' : u'W';
                    const auto pairText = [&](double step, const char* what) {
                        return QStringLiteral("%1: %2 m step, z%3 (%4,%5) %6 edge vs z%7 (%8,%9), tilt %10 dist %11")
                            .arg(QLatin1StringView(what))
                            .arg(step, 0, 'f', 3)
                            .arg(a.key.zoom)
                            .arg(a.key.x)
                            .arg(a.key.y)
                            .arg(edgeA)
                            .arg(b.key.zoom)
                            .arg(b.key.x)
                            .arg(b.key.y)
                            .arg(pose.tilt)
                            .arg(pose.distance);
                    };

                    const TileMath::TileKey backA = field.backingKeyFor(a.key);
                    const TileMath::TileKey backB = field.backingKeyFor(b.key);
                    QVERIFY(TileMath::isValidKey(backA) && TileMath::isValidKey(backB));

                    // Full segment, corners included: bounded by the source's
                    // variation over each side's worst effective resolution —
                    // its backing, or the coarsest perpendicular constraint
                    // that may have captured a corner
                    const QList<int> deltasA = model.edgeLodDeltas(a.key);
                    const QList<int> deltasB = model.edgeLodDeltas(b.key);
                    const int zEffA =
                        std::min(backA.zoom, a.key.zoom - *std::max_element(deltasA.cbegin(), deltasA.cend()));
                    const int zEffB =
                        std::min(backB.zoom, b.key.zoom - *std::max_element(deltasB.cbegin(), deltasB.cend()));
                    const double fullStep =
                        segmentMaxStep(meshes[a.key], edgeA, rectA, meshes[b.key], edgeB, rectB, horizontal, lo, hi);
                    QVERIFY2(fullStep <= stepBound(zEffA, zEffA, zEffB, zEffB),
                             qPrintable(pairText(fullStep, "(b) full-segment bound exceeded")));

                    // Interior (clear of corner capture): exact contracts
                    const double cell = std::max(rectA.width(), rectB.width()) / SurfaceModel::kGridSize;
                    const double loIn = lo + cell;
                    const double hiIn = hi - cell;
                    if (hiIn <= loIn) {
                        continue;
                    }
                    const double innerStep = segmentMaxStep(meshes[a.key], edgeA, rectA, meshes[b.key], edgeB, rectB,
                                                            horizontal, loIn, hiIn);
                    if (a.key.zoom != b.key.zoom) {
                        // (T) LOD boundary: the fine edge is constrained to
                        // the coarse neighbor's own rendered polyline, so
                        // only float-lerp rounding remains
                        QVERIFY2(innerStep <= 1e-3, qPrintable(pairText(innerStep, "(T) stitch broken")));
                        stitchedPairs++;
                    } else if (backA == backB) {
                        // (a) same backing tile: dyadic subwindow arithmetic
                        // makes coincident edge samples bit-exact
                        QCOMPARE(innerStep, 0.0);
                    } else {
                        // (b) different backings at the same level: interior
                        // mismatch bounded by the border texel variation
                        QVERIFY2(innerStep <= stepBound(a.key.zoom, backA.zoom, b.key.zoom, backB.zoom),
                                 qPrintable(pairText(innerStep, "(b) interior bound exceeded")));
                    }
                }
            }
        }
    }
    QCOMPARE_GT(stitchedPairs, 0);  // the pose family must exercise LOD boundaries
}

void SurfaceModelTest::_tallTerrainRecullsOnDataArrival()
{
    // Terrain (500 m) rises far above the camera eye (~229 up at tilt 55,
    // distance 400). Ground-plane culling alone would drop the ground under
    // and just behind the bottom screen edge, leaving a hole where that tall
    // terrain should render. When tall tile data arrives the model must
    // schedule a terrain-aware re-cull without any camera movement.
    GeoMapCamera camera;
    FlatHeightSource source;
    HeightField field;
    SurfaceModel model(&camera, &source, &field);

    camera.setViewportSize(kViewport);
    camera.lookAt(kCenter, 0, 55, 400);
    model.drainUpdates();
    QVERIFY(model.updateSettled());

    QVERIFY(field.insertTile(TileMath::TileKey{0, 0, 0}, constantGrid(500.0f)));
    QVERIFY2(!model.updateSettled(), "tall data arrival did not schedule a re-cull");

    QTRY_VERIFY_WITH_TIMEOUT(model.updateSettled(), 5000);
    const QPointF cameraGround = camera.cameraGroundPosition();
    QVERIFY(model.visibleGroundRect().contains(cameraGround));
    bool renderedUnderCamera = false;
    for (const SurfaceModel::Patch& patch : model.patches()) {
        if (tileRect(patch.key).contains(cameraGround)) {
            renderedUnderCamera = true;
            break;
        }
    }
    QVERIFY2(renderedUnderCamera, "no rendered patch spans the camera ground position");
}

void SurfaceModelTest::_cameraGroundTileResidentOverFlatTerrain()
{
    // The terrain ceiling comes only from resident patches, so tall terrain
    // living solely in never-requested tiles (e.g. a cliff under the camera
    // with flat water everywhere visible) could never be discovered. The
    // visible region must therefore include the camera ground point even
    // when every resident patch is flat.
    GeoMapCamera camera;
    camera.setViewportSize(kViewport);
    camera.lookAt(kCenter, 0, 55, 400);
    const QPointF cameraGround = camera.cameraGroundPosition();

    FlatHeightSource source;
    HeightField field;
    SurfaceModel model(&camera, &source, &field);
    model.drainUpdates();

    QVERIFY(model.visibleGroundRect().contains(cameraGround));

    bool renderedUnderCamera = false;
    for (const SurfaceModel::Patch& patch : model.patches()) {
        if (tileRect(patch.key).contains(cameraGround)) {
            renderedUnderCamera = true;
            break;
        }
    }
    QVERIFY2(renderedUnderCamera, "no rendered patch spans the camera ground position");
}

void SurfaceModelTest::_edgeLodDeltasMatchResidentNeighbors()
{
    GeoMapCamera camera;
    FlatHeightSource source;
    HeightField field;
    SurfaceModel model(&camera, &source, &field);

    // Tilted view: LOD rings guarantee coarser neighbors across ring boundaries
    camera.setViewportSize(kViewport);
    camera.lookAt(kCenter, 0, 45, 2000);
    model.drainUpdates();
    QCOMPARE_GT(model.patchCount(), 8);

    // Semantics: each edge's delta is how many levels coarser the finest
    // resident patch just across that edge renders (same/finer/absent = 0)
    const QList<SurfaceModel::Patch> patches = model.patches();
    int positiveDeltas = 0;
    for (const SurfaceModel::Patch& patch : patches) {
        const QRectF rect = tileRect(patch.key);
        const double eps = rect.width() * 0.01;
        // {N,S,W,E}: world y grows north, slippy y grows south
        const QList<QPointF> acrossPoints = {
            QPointF(rect.center().x(), rect.bottom() + eps),  // north (max y)
            QPointF(rect.center().x(), rect.top() - eps),     // south (min y)
            QPointF(rect.left() - eps, rect.center().y()),    // west
            QPointF(rect.right() + eps, rect.center().y()),   // east
        };
        const QList<int> deltas = model.edgeLodDeltas(patch.key);
        QCOMPARE(deltas.count(), 4);
        for (int edge = 0; edge < 4; edge++) {
            const int neighborZoom = finestResidentZoomAt(patches, acrossPoints[edge]);
            const int expected =
                ((neighborZoom >= 0) && (neighborZoom < patch.key.zoom)) ? (patch.key.zoom - neighborZoom) : 0;
            QVERIFY2(
                deltas[edge] == expected,
                qPrintable(QStringLiteral("edge %1: delta %2, expected %3").arg(edge).arg(deltas[edge]).arg(expected)));
            if (deltas[edge] > 0) {
                positiveDeltas++;
            }
        }
    }
    QCOMPARE_GT(positiveDeltas, 0);  // the view must actually exercise stitching
}

void SurfaceModelTest::_edgeDeltasNotifiedOnNeighborChurn()
{
    GeoMapCamera camera;
    FlatHeightSource source;
    HeightField field;
    SurfaceModel model(&camera, &source, &field);

    camera.setViewportSize(kViewport);
    camera.lookAt(kCenter, 0, 0, GeoMapCamera::kMaxDistance);
    model.drainUpdates();

    // Refinement churn adds/removes neighbors: resident patches whose edge
    // deltas may have changed must be notified so consumers re-pull them
    QSignalSpy deltasSpy(&model, &SurfaceModel::patchEdgeDeltasChanged);
    camera.lookAt(kCenter, 0, 45, 2000);
    model.drainUpdates();

    QCOMPARE_GT(deltasSpy.count(), 0);
    for (const QList<QVariant>& args : deltasSpy) {
        QVERIFY(TileMath::isValidKey(args.first().value<TileMath::TileKey>()));
    }
}

void SurfaceModelTest::_coverageSweepAcrossPoses()
{
    GeoMapCamera camera;
    FlatHeightSource source;
    HeightField field;
    // Real terrain heights change the cull: sweep with data resident too
    QVERIFY(field.insertTile(TileMath::TileKey{0, 0, 0}, constantGrid(400.0f)));
    SurfaceModel model(&camera, &source, &field);
    camera.setViewportSize(kViewport);

    // Sequential poses on purpose: each settle starts from the previous
    // pose's resident set, like interactive use. Oblique tilts stress the
    // AABB visibility estimate the most.
    const QList<qreal> tilts = {0, 30, 55, 70, 80, GeoMapCamera::kMaxTilt};
    const QList<qreal> distances = {200, 2000, 50000, 2000000};
    const QList<qreal> headings = {0, 135};
    for (const qreal heading : headings) {
        for (const qreal distance : distances) {
            for (const qreal tilt : tilts) {
                camera.lookAt(kCenter, heading, tilt, distance);
                model.drainUpdates();
                const QString hole = coverageHole(model, camera);
                QVERIFY2(hole.isEmpty(), qPrintable(hole));
            }
        }
    }
}

void SurfaceModelTest::_coverageAfterInteractiveGesture()
{
    GeoMapCamera camera;
    FlatHeightSource source;
    HeightField field;
    QVERIFY(field.insertTile(TileMath::TileKey{0, 0, 0}, constantGrid(400.0f)));
    SurfaceModel model(&camera, &source, &field);
    camera.setViewportSize(kViewport);

    struct Pose
    {
        QGeoCoordinate center;
        qreal heading;
        qreal tilt;
        qreal distance;
    };

    const QGeoCoordinate kPannedCenter(kCenter.latitude() + 0.05, kCenter.longitude() + 0.05);
    // Drag-like path: tilt down to max oblique, zoom out, rotate, pan, zoom
    // back in. Interpolated per-frame with a single update pass per frame so
    // the camera outruns the add/removal caps, exactly like interactive use.
    const QList<Pose> waypoints = {
        {kCenter, 0, 0, 2000},
        {kCenter, 0, GeoMapCamera::kMaxTilt, 2000},
        {kCenter, 0, GeoMapCamera::kMaxTilt, 200000},
        {kCenter, 180, GeoMapCamera::kMaxTilt, 200000},
        {kPannedCenter, 180, GeoMapCamera::kMaxTilt, 200000},
        {kPannedCenter, 180, 70, 500},
    };
    constexpr int kStepsPerSegment = 30;
    for (int seg = 1; seg < waypoints.count(); seg++) {
        const Pose& from = waypoints[seg - 1];
        const Pose& to = waypoints[seg];
        for (int step = 1; step <= kStepsPerSegment; step++) {
            const qreal t = qreal(step) / kStepsPerSegment;
            const QGeoCoordinate center(
                from.center.latitude() + ((to.center.latitude() - from.center.latitude()) * t),
                from.center.longitude() + ((to.center.longitude() - from.center.longitude()) * t));
            camera.lookAt(center, from.heading + ((to.heading - from.heading) * t),
                          from.tilt + ((to.tilt - from.tilt) * t),
                          from.distance * std::pow(to.distance / from.distance, t));
            model.update();  // one pass per frame: camera moves faster than the model settles
        }
        // Camera stops: the model must clean up to full coverage
        model.drainUpdates();
        QVERIFY(model.updateSettled());
        const QString hole = coverageHole(model, camera);
        QVERIFY2(hole.isEmpty(), qPrintable(QStringLiteral("segment %1: %2").arg(seg).arg(hole)));
    }
}

void SurfaceModelTest::_pinsPatchBackingTiles()
{
    // Tiles that back resident patches (their keys and resolving ancestors)
    // must be pinned against LRU eviction: an evicted backing tile silently
    // coarsens a rendered mesh next to an intact neighbor — a cliff
    GeoMapCamera camera;
    FlatHeightSource source;
    HeightField field;
    SurfaceModel model(&camera, &source, &field);
    camera.setViewportSize(kViewport);
    camera.lookAt(kCenter, 0, 0, 2000);
    model.drainUpdates();
    QCOMPARE_GT(model.patchCount(), 0);

    // Give one resident patch backing data at its own key, then flood with
    // unrelated tiles far past the pyramid cap
    const TileMath::TileKey backingKey = model.patches().first().key;
    QVERIFY(field.insertTile(backingKey, constantGrid(123.0f)));
    for (int i = 0; i < ElevationTilePyramid::kMaxTiles + 8; i++) {
        QVERIFY(field.insertTile(TileMath::TileKey{i, 200, 9}, constantGrid(1.0f)));
    }

    QVERIFY2(field.hasTile(backingKey), "patch-backing tile was evicted");
}

UT_REGISTER_TEST_LIGHTWEIGHT(SurfaceModelTest, TestLabel::Unit)
