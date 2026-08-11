#include "GeoMapPerfUITest.h"

#include <QtCore/QEventLoop>
#include <QtCore/QPoint>
#include <QtCore/QScopeGuard>
#include <QtCore/QTextStream>
#include <QtCore/QTimer>
#include <QtPositioning/QGeoCoordinate>
#include <QtQuick/QQuickItem>
#include <QtQuick/QQuickWindow>
#include <QtTest/QTest>
#include <optional>

#include "Fact.h"
#include "GeoMapCamera.h"
#include "GeoViewSettings.h"
#include "SettingsManager.h"
#include "SurfacePatchModel.h"

UT_REGISTER_TEST_STANDALONE(GeoMapPerfUITest, TestLabel::Integration)

// Fixed benchmark pose: same land area with terrain relief the view starts on,
// so imagery and elevation paths are exercised identically run to run
namespace {
const QGeoCoordinate kBenchCenter(47.6329078, -122.0876875);
}

void GeoMapPerfUITest::_benchmark()
{
    Fact* const enabled = SettingsManager::instance()->geoViewSettings()->enabled();
    const QVariant savedEnabled = enabled->rawValue();
    const auto guard = qScopeGuard([enabled, savedEnabled] { enabled->setRawValue(savedEnabled); });
    enabled->setRawValue(true);

    startUI();
    if (QTest::currentTestFailed())
        return;

    const std::optional<bool> rhiBased = expectSoftwareBackendWarnings(/*strict*/ false);
    QVERIFY2(rhiBased.has_value(), "No renderer interface on the main window");

    QVERIFY(clickToolSelectDropdownButton(QStringLiteral("toolbar_viewGeo")));
    QQuickItem* const viewport = findVisibleItem(_rootItem, QStringLiteral("geoMapViewport"), 10000);
    QVERIFY2(viewport, "GeoMap 3D viewport not visible");

    auto* const cam = viewport->parentItem()->findChild<GeoMapCamera*>(QStringLiteral("geoMapCamera"));
    QVERIFY2(cam, "GeoMapCamera not found");
    auto* const patchModel = viewport->parentItem()->findChild<SurfacePatchModel*>(QStringLiteral("geoMapPatchModel"));
    QVERIFY2(patchModel, "SurfacePatchModel not found");

    const qreal w = viewport->width();
    const qreal h = viewport->height();
    QVERIFY2((w > 300) && (h > 300), "viewport too small for gesture synthesis");
    const auto toWin = [viewport](qreal x, qreal y) { return viewport->mapToScene(QPointF(x, y)).toPoint(); };
    const QPoint center = toWin(w / 2, h / 2);

    // Wall-clock pacing between synthesized input events: this reproduces
    // human gesture speed so per-second capture rows resolve the phases. It
    // is deliberate benchmark pacing, not a flaky synchronization wait.
    const auto pace = [](int ms) {
        QEventLoop loop;
        QTimer::singleShot(ms, &loop, &QEventLoop::quit);
        loop.exec();
    };

    const auto mouseDrag = [this, &pace](const QPoint& from, const QPoint& delta, int steps) {
        QTest::mousePress(_window, Qt::LeftButton, Qt::NoModifier, from);
        for (int i = 1; i <= steps; i++) {
            QTest::mouseMove(_window, from + ((delta * i) / steps));
            pace(40);
        }
        QTest::mouseRelease(_window, Qt::LeftButton, Qt::NoModifier, from + delta);
    };
    const auto wheelSteps = [this, &pace, center](int stepDelta, int count) {
        for (int i = 0; i < count; i++) {
            QTest::wheelEvent(_window, center, QPoint(0, stepDelta));
            pace(200);
        }
    };
    const auto settle = [patchModel] { QTRY_COMPARE_WITH_TIMEOUT(patchModel->pendingCount(), 0, 30000); };

    // Deterministic start pose: 3D mode at street zoom over the bench area.
    // debugHills replaces the terrain pipeline with analytic heights: the
    // unit-test sandbox has no terrain data (fetches fail and degrade to
    // flat), and hills give repeatable non-flat geometry everywhere.
    patchModel->setDebugHills(true);
    cam->setMode(GeoMapCamera::Mode::Mode3D);
    QTRY_COMPARE_WITH_TIMEOUT(cam->tilt(), GeoMapCamera::kDefault3DTilt, 5000);
    cam->lookAt(kBenchCenter, 0, GeoMapCamera::kDefault3DTilt, 1500);
    settle();

    patchModel->startCapture();

    // Phase A: street-zoom pan - 8 alternating drags (churn at high LOD)
    for (int i = 0; i < 8; i++) {
        const QPoint delta((i % 2) ? QPoint(-220, -80) : QPoint(220, 80));
        mouseDrag(center, delta, 20);
    }
    settle();

    // Phase B: zoom out toward whole-earth (LOD coarsening burst)
    wheelSteps(-120, 25);
    settle();

    // Phase C: pan zoomed out (cheap-churn control)
    for (int i = 0; i < 6; i++) {
        const QPoint delta((i % 2) ? QPoint(-220, 0) : QPoint(220, 0));
        mouseDrag(center, delta, 20);
    }
    settle();

    // Phase D: zoom back in to street level (refinement burst)
    wheelSteps(120, 25);
    settle();

    // Phase E: high-tilt orbit churn - manual capture data showed near-horizon
    // tilts at street zoom blowing the patch budget into retiring overshoot and
    // mass delegate destruction (400+ ms passes when removals were uncapped).
    // lookAt poses jump between tilts/headings to force full-set replacement.
    for (int i = 0; i < 6; i++) {
        cam->lookAt(kBenchCenter, (i * 120) % 360, (i % 2) ? 70.0 : 40.0, 955);
        pace(800);
    }
    settle();

    // Phase F: sustained no-settle churn - continuous zoom oscillation at high
    // tilt with SLOW heights. Height latency pins pending patches, so retiring
    // covers cannot sweep and the resident set transiently exceeds the patch
    // budget (accepted; see SurfaceModel::kMaxPatches) - this phase measures
    // the per-pass costs under that load. debugHills answers instantly and
    // cannot reproduce it, so this phase runs the real terrain source, whose
    // sandbox fetches retry on multi-second backoff.
    patchModel->setDebugHills(false);
    cam->lookAt(kBenchCenter, 0, 60.0, 1500);
    for (int i = 0; i < 12; i++) {
        wheelSteps((i % 2) ? 120 : -120, 8);
    }
    // Instant heights again: the switch rebuilds the model, so the tail of the
    // capture settles quickly instead of waiting out terrain retry backoffs
    patchModel->setDebugHills(true);
    settle();

    const QString capturePath = patchModel->stopCapture();
    QVERIFY2(!capturePath.isEmpty(), "capture file was not written");
    // stdout, not the Qt log: the UI test harness fails on unexpected log messages
    QTextStream(stdout) << "GeoMap benchmark capture: " << capturePath << Qt::endl;

    stopUI();
}
