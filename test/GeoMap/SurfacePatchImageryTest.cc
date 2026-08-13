#include "SurfacePatchImageryTest.h"

#include <QtCore/QRegularExpression>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QtTest/QSignalSpy>

#include "GeoMapCamera.h"
#include "GeoScene.h"
#include "QGCMapUrlEngine.h"
#include "SurfacePatchModel.h"
#include "UnitTestTileGenerator.h"

namespace {

const QGeoCoordinate kCenter(47.3977419, 8.5455938);
constexpr QSizeF kViewport(800, 600);

// The unit-test tile generator serves a placeholder image on every cache miss
QString mapType()
{
    const QStringList elevationTypes = UrlFactory::getElevationProviderTypes();
    for (const QString& type : UrlFactory::getProviderTypes()) {
        if (!elevationTypes.contains(type)) {
            return type;
        }
    }
    return QString();
}

void attach(SurfacePatchModel& model, GeoScene& scene, GeoMapCamera& camera)
{
    camera.setViewportSize(kViewport);
    camera.lookAt(kCenter, 0, 0, 2000);
    scene.setCamera(&camera);
    model.setScene(&scene);
    model.drainUpdates();
}

int rowsWithImage(const SurfacePatchModel& model)
{
    int count = 0;
    for (int row = 0; row < model.rowCount(); row++) {
        if (!model.data(model.index(row), SurfacePatchModel::TileImageRole).value<QImage>().isNull()) {
            count++;
        }
    }
    return count;
}

int rowsReportingImage(const SurfacePatchModel& model)
{
    int count = 0;
    for (int row = 0; row < model.rowCount(); row++) {
        if (model.data(model.index(row), SurfacePatchModel::HasTileImageRole).toBool()) {
            count++;
        }
    }
    return count;
}

/// Reply that fails asynchronously without touching the network
class FailingReply : public QNetworkReply
{
public:
    explicit FailingReply(const QNetworkRequest& request, QObject* parent) : QNetworkReply(parent)
    {
        setRequest(request);
        setOperation(QNetworkAccessManager::GetOperation);
        open(ReadOnly);
        QMetaObject::invokeMethod(
            this,
            [this] {
                setError(ContentNotFoundError, QStringLiteral("canned failure"));
                emit errorOccurred(ContentNotFoundError);
                setFinished(true);
                emit finished();
            },
            Qt::QueuedConnection);
    }

    void abort() final {}

protected:
    qint64 readData(char*, qint64) final { return -1; }
};

/// Every network fetch fails: models a provider/connectivity outage
class FailingNam : public QNetworkAccessManager
{
public:
    using QNetworkAccessManager::QNetworkAccessManager;

    int requestCount = 0;

protected:
    QNetworkReply* createRequest(Operation, const QNetworkRequest& request, QIODevice*) final
    {
        requestCount++;
        return new FailingReply(request, this);
    }
};

}  // namespace

void SurfacePatchImageryTest::_imagesArriveForAllPatches()
{
    const QString type = mapType();
    QVERIFY2(!type.isEmpty(), "no non-elevation map provider registered");

    GeoMapCamera camera;
    GeoScene scene;
    SurfacePatchModel model;
    attach(model, scene, camera);
    model.setMapType(type);

    QCOMPARE_GT(model.rowCount(), 0);
    QTRY_COMPARE_WITH_TIMEOUT(rowsWithImage(model), model.rowCount(), 5000);
}

void SurfacePatchImageryTest::_emptyMapTypeDisablesImagery()
{
    const QString type = mapType();
    QVERIFY2(!type.isEmpty(), "no non-elevation map provider registered");

    GeoMapCamera camera;
    GeoScene scene;
    SurfacePatchModel model;
    attach(model, scene, camera);
    model.setMapType(type);
    QTRY_COMPARE_WITH_TIMEOUT(rowsWithImage(model), model.rowCount(), 5000);

    // Clearing the map type drops all imagery and stops requesting
    model.setMapType(QString());
    QCOMPARE(rowsWithImage(model), 0);

    // Patch churn with imagery disabled stays image-free
    camera.setCenter(QGeoCoordinate(47.42, 8.58));
    model.drainUpdates();
    QCOMPARE(rowsWithImage(model), 0);
}

void SurfacePatchImageryTest::_mapTypeSwitchRefetches()
{
    const QStringList elevationTypes = UrlFactory::getElevationProviderTypes();
    QStringList types;
    for (const QString& type : UrlFactory::getProviderTypes()) {
        if (!elevationTypes.contains(type)) {
            types.append(type);
        }
        if (types.count() == 2) {
            break;
        }
    }
    QVERIFY2(types.count() == 2, "need two non-elevation map providers");

    GeoMapCamera camera;
    GeoScene scene;
    SurfacePatchModel model;
    attach(model, scene, camera);
    model.setMapType(types.first());
    QTRY_COMPARE_WITH_TIMEOUT(rowsWithImage(model), model.rowCount(), 5000);

    // Provider switch clears delivered images, then refetches for the new provider
    model.setMapType(types.last());
    QTRY_COMPARE_WITH_TIMEOUT(rowsWithImage(model), model.rowCount(), 5000);
    QCOMPARE(model.mapType(), types.last());
}

void SurfacePatchImageryTest::_fallbackCoversLodChanges()
{
    const QString type = mapType();
    QVERIFY2(!type.isEmpty(), "no non-elevation map provider registered");

    GeoMapCamera camera;
    GeoScene scene;
    SurfacePatchModel model;
    attach(model, scene, camera);
    model.setMapType(type);
    QTRY_COMPARE_WITH_TIMEOUT(rowsWithImage(model), model.rowCount(), 5000);

    // Zoom in (LOD refines): every new patch lies inside previously imaged
    // ground and must immediately drape an ancestor fallback -- no flash
    camera.setDistance(500);
    model.drainUpdates();
    QCOMPARE_GT(model.rowCount(), 0);
    QCOMPARE(rowsReportingImage(model), model.rowCount());
    QCOMPARE(rowsWithImage(model), model.rowCount());

    // Zoom back out to the original LOD: the retired originals revive from
    // the cache without any new fetch
    camera.setDistance(2000);
    model.drainUpdates();
    QCOMPARE_GT(model.rowCount(), 0);
    QCOMPARE(rowsReportingImage(model), model.rowCount());

    // Coarsen further: patches over previously seen ground composite their
    // cached children. Edge patches cover never-seen ground and legitimately
    // have no fallback source, so only partial coverage is guaranteed.
    camera.setDistance(4000);
    model.drainUpdates();
    QCOMPARE_GT(model.rowCount(), 0);
    QCOMPARE_GT(rowsReportingImage(model), 0);
    QCOMPARE(rowsWithImage(model), rowsReportingImage(model));
}

void SurfacePatchImageryTest::_imagesRecoverAfterGestureChurn()
{
    const QString type = mapType();
    QVERIFY2(!type.isEmpty(), "no non-elevation map provider registered");

    GeoMapCamera camera;
    GeoScene scene;
    SurfacePatchModel model;
    attach(model, scene, camera);
    model.setMapType(type);
    QTRY_COMPARE_WITH_TIMEOUT(rowsWithImage(model), model.rowCount(), 5000);

    // Drag-like camera motion: per-frame poses with the event loop pumped
    // between frames, so async image deliveries and cancellations interleave
    // with patch churn mid-gesture (a settled-pose test never sees this).
    // Path mirrors the field report: tilt down to max oblique, zoom out,
    // rotate, pan, zoom back in.
    struct Pose
    {
        QGeoCoordinate center;
        qreal heading;
        qreal tilt;
        qreal distance;
    };

    const QGeoCoordinate kPanned(kCenter.latitude() + 0.05, kCenter.longitude() + 0.05);
    const QList<Pose> waypoints = {
        {kCenter, 0, 0, 2000},
        {kCenter, 0, GeoMapCamera::kMaxTilt, 2000},
        {kCenter, 0, GeoMapCamera::kMaxTilt, 200000},
        {kPanned, 180, GeoMapCamera::kMaxTilt, 200000},
        {kPanned, 180, 70, 500},
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
            QCoreApplication::processEvents();  // one frame: deliveries land mid-gesture
        }
        // Camera stops: every patch must end up imaged. A row whose
        // hasTileImage never turns true renders the dark loading fallback
        // forever - the visible "hole" from the field report.
        model.drainUpdates();
        QTRY_COMPARE_WITH_TIMEOUT(rowsReportingImage(model), model.rowCount(), 10000);
        QTRY_COMPARE_WITH_TIMEOUT(rowsWithImage(model), model.rowCount(), 10000);
    }
}

void SurfacePatchImageryTest::_imagesRecoverAfterTransientOutage()
{
    const QString type = mapType();
    QVERIFY2(!type.isEmpty(), "no non-elevation map provider registered");

    GeoMapCamera camera;
    GeoScene scene;
    FailingNam nam;  // must outlive the model: its TileImageSource borrows the manager
    SurfacePatchModel model;
    model.setTileImageNetworkManager(&nam);
    attach(model, scene, camera);
    model.setMapType(type);
    QTRY_COMPARE_WITH_TIMEOUT(rowsWithImage(model), model.rowCount(), 5000);

    // Transient imagery outage: cache lookups miss and the network fallback
    // fails while a pan brings in new patches (the field scenario: gesture
    // during flaky connectivity)
    UnitTestTileGenerator::setForcedMissCount(10000);
    const auto guard = qScopeGuard([] { UnitTestTileGenerator::setForcedMissCount(0); });
    // Fetch failures must be visible without logging configuration; repeats
    // within the throttle window stay at debug
    expectLogMessage("GeoMap.TileImageSource", QtWarningMsg, QRegularExpression(QStringLiteral("failed")));
    camera.setCenter(QGeoCoordinate(kCenter.latitude() + 0.02, kCenter.longitude() + 0.02));
    model.drainUpdates();
    QCOMPARE_GT(model.rowCount(), 0);
    QTRY_COMPARE_WITH_TIMEOUT(model.pendingImageCount(), 0, 10000);  // all outage-era requests resolved
    QCOMPARE_GT(nam.requestCount, 0);
    QCOMPARE_LT(rowsWithImage(model), model.rowCount());             // the outage left imageless patches
    verifyExpectedLogMessage();
    // Outage ends with the camera at rest: every resident patch must still
    // end up imaged. A row stuck without image (or fallback) renders the dark
    // loading fallback forever - the visible "hole" from the field report.
    UnitTestTileGenerator::setForcedMissCount(0);
    QTRY_COMPARE_WITH_TIMEOUT(rowsReportingImage(model), model.rowCount(), 10000);
    QTRY_COMPARE_WITH_TIMEOUT(rowsWithImage(model), model.rowCount(), 10000);
}

UT_REGISTER_TEST(SurfacePatchImageryTest, TestLabel::Integration)
