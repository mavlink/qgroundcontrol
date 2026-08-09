#include "SurfaceModelTest.h"

#include <QtTest/QSignalSpy>
#include <algorithm>

#include "GeoViewCamera.h"
#include "HeightSource.h"
#include "SurfaceModel.h"
#include "TileMath.h"

namespace {

const QGeoCoordinate kCenter(47.3977419, 8.5455938);
constexpr QSizeF kViewport(800, 600);

/// Fails every request asynchronously, mimicking an unreachable terrain backend
class FailingHeightSource : public HeightSource
{
public:
    using HeightSource::HeightSource;

    int requestPatchHeights(const TileMath::TileKey& key, int gridSize) override
    {
        Q_UNUSED(key);
        Q_UNUSED(gridSize);
        const int requestId = _nextRequestId();
        QMetaObject::invokeMethod(
            this, [this, requestId] { emit patchHeightsFailed(requestId); }, Qt::QueuedConnection);
        return requestId;
    }

    void cancelRequest(int requestId) override { Q_UNUSED(requestId); }
};

int maxZoomOf(const QList<SurfaceModel::Patch>& patches)
{
    int maxZoom = 0;
    for (const SurfaceModel::Patch& patch : patches) {
        maxZoom = std::max(maxZoom, patch.key.zoom);
    }
    return maxZoom;
}

}  // namespace

void SurfaceModelTest::_noViewportNoPatches()
{
    GeoViewCamera camera;
    FlatHeightSource source;
    SurfaceModel model(&camera, &source);

    model.update();
    QCOMPARE(model.patchCount(), 0);
}

void SurfaceModelTest::_coarseWhenFar()
{
    GeoViewCamera camera;
    FlatHeightSource source;
    SurfaceModel model(&camera, &source);

    camera.setViewportSize(kViewport);
    camera.lookAt(kCenter, 0, 0, GeoViewCamera::kMaxDistance);

    QCOMPARE_GT(model.patchCount(), 0);
    // From max distance the whole world fits the view: only low zoom levels
    QCOMPARE_LE(maxZoomOf(model.patches()), 4);
}

void SurfaceModelTest::_refinesWhenNear()
{
    GeoViewCamera camera;
    FlatHeightSource source;
    SurfaceModel model(&camera, &source);

    camera.setViewportSize(kViewport);
    camera.lookAt(kCenter, 0, 0, GeoViewCamera::kMaxDistance);
    const int farMaxZoom = maxZoomOf(model.patches());

    camera.lookAt(kCenter, 0, 0, 2000);
    const int nearMaxZoom = maxZoomOf(model.patches());

    QCOMPARE_GT(nearMaxZoom, farMaxZoom);
    QCOMPARE_GT(model.patchCount(), 0);
}

void SurfaceModelTest::_patchCountBounded()
{
    GeoViewCamera camera;
    FlatHeightSource source;
    SurfaceModel model(&camera, &source);

    camera.setViewportSize(kViewport);
    for (double distance : {50.0, 2000.0, 100000.0, GeoViewCamera::kMaxDistance}) {
        for (double tilt : {0.0, 45.0, GeoViewCamera::kMaxTilt}) {
            camera.lookAt(kCenter, 30, tilt, distance);
            QCOMPARE_LE(model.patchCount(), SurfaceModel::kMaxPatches);
        }
    }
}

void SurfaceModelTest::_budgetExhaustedKeepsCoverage()
{
    GeoViewCamera camera;
    FlatHeightSource source;
    SurfaceModel model(&camera, &source);

    // A huge viewport shrinks meters-per-pixel so refinement demand far
    // exceeds the patch budget
    camera.setViewportSize(QSizeF(8000, 6000));
    camera.lookAt(kCenter, 0, 45, 2000);

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

void SurfaceModelTest::_heightsArrive()
{
    GeoViewCamera camera;
    FlatHeightSource source;
    SurfaceModel model(&camera, &source);
    QSignalSpy readySpy(&model, &SurfaceModel::patchReady);

    camera.setViewportSize(kViewport);
    camera.lookAt(kCenter, 0, 0, 2000);

    QCOMPARE_GT(model.patchCount(), 0);
    QTRY_COMPARE_WITH_TIMEOUT(model.pendingCount(), 0, 5000);
    QCOMPARE(readySpy.count(), model.patchCount());

    const int expectedHeights = (SurfaceModel::kGridSize + 1) * (SurfaceModel::kGridSize + 1);
    for (const SurfaceModel::Patch& patch : model.patches()) {
        QVERIFY(patch.ready);
        QCOMPARE(patch.heights.count(), expectedHeights);
    }
}

void SurfaceModelTest::_diffOnMove()
{
    GeoViewCamera camera;
    FlatHeightSource source;
    SurfaceModel model(&camera, &source);

    camera.setViewportSize(kViewport);
    camera.lookAt(kCenter, 0, 0, 2000);
    QTRY_COMPARE_WITH_TIMEOUT(model.pendingCount(), 0, 5000);

    QSignalSpy addedSpy(&model, &SurfaceModel::patchAdded);
    QSignalSpy removedSpy(&model, &SurfaceModel::patchRemoved);

    // Move to the other side of the planet: full set replacement
    camera.setCenter(QGeoCoordinate(-33.8688, 151.2093));
    QCOMPARE_GT(addedSpy.count(), 0);
    QCOMPARE_GT(removedSpy.count(), 0);

    QTRY_COMPARE_WITH_TIMEOUT(model.pendingCount(), 0, 5000);
    for (const SurfaceModel::Patch& patch : model.patches()) {
        QVERIFY(patch.ready);
    }
}

void SurfaceModelTest::_noChurnOnIdenticalUpdate()
{
    GeoViewCamera camera;
    FlatHeightSource source;
    SurfaceModel model(&camera, &source);

    camera.setViewportSize(kViewport);
    camera.lookAt(kCenter, 0, 0, 2000);

    QSignalSpy addedSpy(&model, &SurfaceModel::patchAdded);
    QSignalSpy removedSpy(&model, &SurfaceModel::patchRemoved);
    model.update();
    QCOMPARE(addedSpy.count(), 0);
    QCOMPARE(removedSpy.count(), 0);
}

void SurfaceModelTest::_cullsInvisibleRegion()
{
    GeoViewCamera camera;
    FlatHeightSource source;
    SurfaceModel model(&camera, &source);

    camera.setViewportSize(kViewport);
    camera.lookAt(kCenter, 0, 0, 2000);

    // At 2km altitude over Zurich the antipodean hemisphere must not be resident
    const QPointF sydney = TileMath::geoToWorld(QGeoCoordinate(-33.8688, 151.2093));
    for (const SurfaceModel::Patch& patch : model.patches()) {
        const double span = TileMath::tileSpanAtZoom(patch.key.zoom);
        const QPointF minCorner = TileMath::tileMinCorner(patch.key);
        const QRectF rect(minCorner.x(), minCorner.y(), span, span);
        QVERIFY(!rect.contains(sydney) || (patch.key.zoom == 0));
    }
}

void SurfaceModelTest::_failedHeightsDegradeToFlat()
{
    GeoViewCamera camera;
    FailingHeightSource source;
    SurfaceModel model(&camera, &source);

    camera.setViewportSize(kViewport);
    camera.lookAt(kCenter, 0, 0, 2000);

    QCOMPARE_GT(model.patchCount(), 0);
    QTRY_COMPARE_WITH_TIMEOUT(model.pendingCount(), 0, 5000);
    for (const SurfaceModel::Patch& patch : model.patches()) {
        QVERIFY(patch.ready);
        QVERIFY(patch.heights.isEmpty());  // empty = flat fallback
    }
}

UT_REGISTER_TEST_LIGHTWEIGHT(SurfaceModelTest, TestLabel::Unit)
