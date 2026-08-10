#include "TileImageSourceTest.h"

#include <QtTest/QSignalSpy>

#include "QGCMapUrlEngine.h"
#include "TileImageSource.h"
#include "TileMath.h"

namespace {

// Any registered map provider works: the unit-test tile generator serves a
// deterministic placeholder image on every cache miss, so no network is hit
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

const TileMath::TileKey kKey{4302, 2867, 13};  // Zurich area

}  // namespace

void TileImageSourceTest::_tileDelivered()
{
    const QString type = mapType();
    QVERIFY2(!type.isEmpty(), "no non-elevation map provider registered");

    TileImageSource source(type);
    QSignalSpy readySpy(&source, &TileImageSource::tileImageReady);
    QSignalSpy failedSpy(&source, &TileImageSource::tileImageFailed);

    const int requestId = source.requestTileImage(kKey);
    QCOMPARE_GT(requestId, 0);
    QCOMPARE(source.pendingCount(), 1);

    QTRY_COMPARE_WITH_TIMEOUT(readySpy.count(), 1, 5000);
    QCOMPARE(failedSpy.count(), 0);
    QCOMPARE(source.pendingCount(), 0);

    QCOMPARE(readySpy.first().at(0).toInt(), requestId);
    const QImage image = readySpy.first().at(1).value<QImage>();
    QVERIFY(!image.isNull());
    QCOMPARE_GT(image.width(), 0);
}

void TileImageSourceTest::_cancelledRequestSilent()
{
    const QString type = mapType();
    QVERIFY2(!type.isEmpty(), "no non-elevation map provider registered");

    TileImageSource source(type);
    QSignalSpy readySpy(&source, &TileImageSource::tileImageReady);
    QSignalSpy failedSpy(&source, &TileImageSource::tileImageFailed);

    const int cancelledId = source.requestTileImage(kKey);
    source.cancelRequest(cancelledId);
    QCOMPARE(source.pendingCount(), 0);

    // The cache worker runs tasks in order: once this later request delivers,
    // the cancelled one has already run its course without signalling
    const int sentinelId = source.requestTileImage(TileMath::TileKey{kKey.x + 1, kKey.y, kKey.zoom});
    QTRY_COMPARE_WITH_TIMEOUT(readySpy.count(), 1, 5000);
    QCOMPARE(readySpy.first().at(0).toInt(), sentinelId);
    QCOMPARE(failedSpy.count(), 0);
}

void TileImageSourceTest::_multipleRequestsIndependent()
{
    const QString type = mapType();
    QVERIFY2(!type.isEmpty(), "no non-elevation map provider registered");

    TileImageSource source(type);
    QSignalSpy readySpy(&source, &TileImageSource::tileImageReady);

    const int first = source.requestTileImage(kKey);
    const int second = source.requestTileImage(TileMath::TileKey{kKey.x + 1, kKey.y, kKey.zoom});
    QCOMPARE_NE(first, second);

    QTRY_COMPARE_WITH_TIMEOUT(readySpy.count(), 2, 5000);

    QSet<int> deliveredIds;
    for (const QList<QVariant>& args : readySpy) {
        deliveredIds.insert(args.at(0).toInt());
    }
    QCOMPARE(deliveredIds, (QSet<int>{first, second}));
}

void TileImageSourceTest::_unknownProviderFails()
{
    // The provider lookup deliberately warns once at construction
    expectLogMessage("QtLocationPlugin.QGCMapUrlEngine", QtWarningMsg,
                     QRegularExpression(QStringLiteral("type not found: \"No Such Provider\"")));
    TileImageSource source(QStringLiteral("No Such Provider"));
    verifyExpectedLogMessage();

    QSignalSpy readySpy(&source, &TileImageSource::tileImageReady);
    QSignalSpy failedSpy(&source, &TileImageSource::tileImageFailed);

    const int requestId = source.requestTileImage(kKey);
    QCOMPARE_GT(requestId, 0);
    QCOMPARE(source.pendingCount(), 1);  // failure is still delivered asynchronously

    QTRY_COMPARE_WITH_TIMEOUT(failedSpy.count(), 1, 5000);
    QCOMPARE(failedSpy.first().at(0).toInt(), requestId);
    QCOMPARE(readySpy.count(), 0);
    QCOMPARE(source.pendingCount(), 0);
}

UT_REGISTER_TEST(TileImageSourceTest, TestLabel::Integration)
