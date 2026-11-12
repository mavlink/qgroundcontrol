#include "SurfacePatchImageryTest.h"

#include <QtCore/QRegularExpression>
#include <QtGui/QColor>
#include <QtNetwork/QNetworkReply>
#include <QtTest/QSignalSpy>

#include "AppSettings.h"
#include "Fixtures/RAIIFixtures.h"
#include "GeoMapCamera.h"
#include "GeoScene.h"
#include "MapsSettings.h"
#include "SettingsManager.h"
#include "SurfacePatchModel.h"
#include "TileReplyNetworkMock.h"
#include "UnitTestTileGenerator.h"

namespace {

const QGeoCoordinate kCenter(47.3977419, 8.5455938);
constexpr QSizeF kViewport(800, 600);
constexpr int kRetryBaseMs = 50;  ///< first delay of the paced retry: the production one is seconds

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

int rowsWithImageOfColor(const SurfacePatchModel& model, const QColor& color)
{
    int count = 0;
    for (int row = 0; row < model.rowCount(); row++) {
        const QImage image = model.data(model.index(row), SurfacePatchModel::TileImageRole).value<QImage>();
        if (!image.isNull() && (image.pixelColor(0, 0) == color)) {
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

}  // namespace

void SurfacePatchImageryTest::initTestCase()
{
    UnitTest::initTestCase();
    // The tests fetch tiles that the tile cache of the process must not have yet
    QVERIFY(UnitTestTileGenerator::resetTileCache());
}

void SurfacePatchImageryTest::_imagesArriveForAllPatches()
{
    const QString type = UnitTestTileGenerator::mapProviderType();
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
    const QString type = UnitTestTileGenerator::mapProviderType();
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
    const QString type = UnitTestTileGenerator::mapProviderType();
    const QString otherType = UnitTestTileGenerator::mapProviderType(1);
    QVERIFY2(!type.isEmpty() && !otherType.isEmpty(), "need two non-elevation map providers");

    GeoMapCamera camera;
    GeoScene scene;
    SurfacePatchModel model;
    attach(model, scene, camera);
    model.setMapType(type);
    QTRY_COMPARE_WITH_TIMEOUT(rowsWithImage(model), model.rowCount(), 5000);

    // Provider switch clears delivered images, then refetches for the new provider
    model.setMapType(otherType);
    QTRY_COMPARE_WITH_TIMEOUT(rowsWithImage(model), model.rowCount(), 5000);
    QCOMPARE(model.mapType(), otherType);
}

void SurfacePatchImageryTest::_sourceChangeRefetches()
{
    Fact* const customUrl = SettingsManager::instance()->appSettings()->customURL();
    TestFixtures::SettingsFixture saved;
    saved.setFactValue(customUrl, QStringLiteral("https://a.example/{z}/{x}/{y}.png"));

    // Every tile comes from the network, which answers in the colour of the source
    const UnitTestTileGenerator::ForcedMisses forcedMisses(UnitTestTileGenerator::ForcedMisses::kEveryLookup);
    TileReplyMockNam nam;  // must outlive the model: its TileImageSource borrows the manager
    nam.body = tileReplyPngBody(Qt::red);

    GeoMapCamera camera;
    GeoScene scene;
    SurfacePatchModel model;
    model.setTileImageNetworkManager(&nam);
    attach(model, scene, camera);
    model.setMapType(QStringLiteral("CustomURL Custom"));
    QCOMPARE_GT(model.rowCount(), 0);
    QTRY_COMPARE_WITH_TIMEOUT(rowsWithImageOfColor(model, Qt::red), model.rowCount(), TestTimeout::mediumMs());

    // The user sets another URL while the map is open: the images it shows are those of the previous source
    nam.body = tileReplyPngBody(Qt::blue);
    customUrl->setRawValue(QStringLiteral("https://b.example/{z}/{x}/{y}.png"));
    QTRY_COMPARE_WITH_TIMEOUT(rowsWithImageOfColor(model, Qt::blue), model.rowCount(), TestTimeout::mediumMs());
}

void SurfacePatchImageryTest::_fallbackCoversLodChanges()
{
    const QString type = UnitTestTileGenerator::mapProviderType();
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

void SurfacePatchImageryTest::_retiredImagesFollowMemoryCacheSetting_data()
{
    QTest::addColumn<int>("megabytes");
    QTest::addColumn<int>("tileEdgePixels");
    QTest::addColumn<int>("expectedRetired");

    // The store gets half of the setting, and an image of 256 pixels takes 256 KB
    QTest::newRow("halfOfTheSetting") << 20 << 256 << 40;
    QTest::newRow("minimum") << 1 << 256 << 32;
    // The setting is a budget in bytes: an image of 512 pixels takes four times as much
    QTest::newRow("largerImages") << 20 << 512 << 10;
}

void SurfacePatchImageryTest::_retiredImagesFollowMemoryCacheSetting()
{
    QFETCH(int, megabytes);
    QFETCH(int, tileEdgePixels);
    QFETCH(int, expectedRetired);

    TestFixtures::SettingsFixture saved;
    ignoreLogMessage("API.QGCApplication.AppMessage", QtDebugMsg,
                     QRegularExpression(QStringLiteral("Restart application for changes to take effect")));
    saved.setFactValue(SettingsManager::instance()->mapsSettings()->maxCacheMemorySize(), megabytes);

    // Every tile comes from the network, as an image of the size of the row with 4 bytes a pixel. A source of its
    // own for each size keeps these tiles apart in the tile cache of the process.
    saved.setFactValue(SettingsManager::instance()->appSettings()->customURL(),
                       QStringLiteral("https://tiles%1.example/{z}/{x}/{y}.png").arg(tileEdgePixels));
    const UnitTestTileGenerator::ForcedMisses forcedMisses(UnitTestTileGenerator::ForcedMisses::kEveryLookup);
    TileReplyMockNam nam;  // must outlive the model: its TileImageSource borrows the manager
    nam.body = tileReplyPngBody(Qt::darkGreen, tileEdgePixels);

    GeoMapCamera camera;
    GeoScene scene;
    SurfacePatchModel model;
    model.setTileImageNetworkManager(&nam);
    attach(model, scene, camera);
    model.setMapType(QStringLiteral("CustomURL Custom"));
    // A view towards the horizon shows more patches than any row keeps images for
    camera.setTilt(GeoMapCamera::kMaxTilt);
    model.drainUpdates();
    const int imagedPatches = model.rowCount();
    QCOMPARE_GT(imagedPatches, expectedRetired);
    QTRY_COMPARE_WITH_TIMEOUT(model.pendingImageCount(), 0, TestTimeout::mediumMs());
    QCOMPARE(rowsWithImage(model), imagedPatches);

    // Leaving retires every image. Coming back, only the patches whose image was dropped ask for it again
    camera.setCenter(QGeoCoordinate(kCenter.latitude() + 5.0, kCenter.longitude() + 5.0));
    model.drainUpdates();
    camera.setCenter(kCenter);
    model.drainUpdates();
    QCOMPARE(model.rowCount(), imagedPatches);
    QCOMPARE(model.pendingImageCount(), imagedPatches - expectedRetired);
}

void SurfacePatchImageryTest::_imagesRecoverAfterGestureChurn()
{
    const QString type = UnitTestTileGenerator::mapProviderType();
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
    const QString type = UnitTestTileGenerator::mapProviderType();
    QVERIFY2(!type.isEmpty(), "no non-elevation map provider registered");

    GeoMapCamera camera;
    GeoScene scene;
    TileReplyMockNam nam;  // must outlive the model: its TileImageSource borrows the manager
    nam.error = QNetworkReply::ConnectionRefusedError;
    SurfacePatchModel model;
    model.setTileImageNetworkManager(&nam);
    model.setImageRetryBaseIntervalMs(kRetryBaseMs);
    attach(model, scene, camera);
    model.setMapType(type);
    QTRY_COMPARE_WITH_TIMEOUT(rowsWithImage(model), model.rowCount(), TestTimeout::mediumMs());

    // Transient imagery outage: cache lookups miss and the network fallback
    // fails while a pan brings in new patches (the field scenario: gesture
    // during flaky connectivity)
    UnitTestTileGenerator::ForcedMisses outage(UnitTestTileGenerator::ForcedMisses::kEveryLookup);
    // Fetch failures must be visible without logging configuration; repeats
    // within the throttle window stay at debug
    expectLogMessage("GeoMap.TileImageSource", QtWarningMsg, QRegularExpression(QStringLiteral("failed")));
    camera.setCenter(QGeoCoordinate(kCenter.latitude() + 0.02, kCenter.longitude() + 0.02));
    model.drainUpdates();
    QCOMPARE_GT(model.rowCount(), 0);
    // The images are requested again, and with the outage lasting every round puts the next one off for longer
    QTRY_VERIFY_WITH_TIMEOUT(model.imageRetryIntervalMs() > kRetryBaseMs, TestTimeout::mediumMs());
    QCOMPARE_GT(nam.requestCount, 0);
    QCOMPARE_LT(rowsWithImage(model), model.rowCount());  // the outage left imageless patches
    verifyExpectedLogMessage();
    // Outage ends with the camera at rest: every resident patch must still
    // end up imaged. A row stuck without image (or fallback) renders the dark
    // loading fallback forever - the visible "hole" from the field report.
    outage.setCount(0);
    QTRY_COMPARE_WITH_TIMEOUT(rowsReportingImage(model), model.rowCount(), TestTimeout::longMs());
    QTRY_COMPARE_WITH_TIMEOUT(rowsWithImage(model), model.rowCount(), TestTimeout::longMs());
    // The images that arrive end the spacing out
    QTRY_COMPARE_WITH_TIMEOUT(model.imageRetryIntervalMs(), kRetryBaseMs, TestTimeout::longMs());
}

void SurfacePatchImageryTest::_missingImagesNotRequestedAgain()
{
    const QString type = UnitTestTileGenerator::mapProviderType();
    const QString otherType = UnitTestTileGenerator::mapProviderType(1);
    QVERIFY2(!type.isEmpty() && !otherType.isEmpty(), "need two non-elevation map providers");

    // Every tile comes from the network, which answers when the test says so
    const UnitTestTileGenerator::ForcedMisses forcedMisses(UnitTestTileGenerator::ForcedMisses::kEveryLookup);
    TileReplyMockNam nam;  // must outlive the model: its TileImageSource borrows the manager
    nam.holdReplies = true;

    GeoMapCamera camera;
    GeoScene scene;
    SurfacePatchModel model;
    model.setTileImageNetworkManager(&nam);
    model.setImageRetryBaseIntervalMs(kRetryBaseMs);
    attach(model, scene, camera);
    model.setMapType(type);
    const int patches = model.rowCount();
    QCOMPARE_GT(patches, 1);
    QTRY_COMPARE_WITH_TIMEOUT(nam.requestCount, patches, TestTimeout::mediumMs());

    // One tile fails. The provider has none of the others, which is no failure: it answers HTTP 404
    const QUrl failedUrl = nam.requests.constFirst().url();
    expectLogMessage("GeoMap.TileImageSource", QtWarningMsg, QRegularExpression(QStringLiteral("failed")));
    nam.replies.at(0)->failNow(QNetworkReply::ConnectionRefusedError);
    verifyExpectedLogMessage();
    for (int i = 1; i < patches; i++) {
        nam.replies.at(i)->failNow(QNetworkReply::ContentNotFoundError, 404);
    }
    QCOMPARE(model.pendingImageCount(), 0);
    QCOMPARE(rowsWithImage(model), 0);

    // The paced rounds ask again for the tile that failed, and for no other. A round makes its requests at once
    // and the cache answers in request order, so what the first round asks for is here before the request of the
    // second round.
    QTRY_COMPARE_WITH_TIMEOUT(nam.requestCount, patches + 1, TestTimeout::mediumMs());
    QCOMPARE(nam.requests.constLast().url(), failedUrl);
    nam.replies.constLast()->failNow(QNetworkReply::ConnectionRefusedError);
    QTRY_COMPARE_WITH_TIMEOUT(nam.requestCount, patches + 2, TestTimeout::mediumMs());
    QCOMPARE(nam.requests.constLast().url(), failedUrl);

    // Another map type starts afresh: its provider can have the tiles that this one lacks
    model.setMapType(otherType);
    QTRY_COMPARE_WITH_TIMEOUT(nam.requestCount, (2 * patches) + 2, TestTimeout::mediumMs());
}

UT_REGISTER_TEST(SurfacePatchImageryTest, TestLabel::Integration)
