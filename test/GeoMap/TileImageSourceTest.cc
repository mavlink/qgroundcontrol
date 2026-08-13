#include "TileImageSourceTest.h"

#include <QtCore/QBuffer>
#include <QtCore/QFile>
#include <QtCore/QScopeGuard>
#include <QtCore/QTimer>
#include <QtGui/QImage>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QtTest/QSignalSpy>

#include <cstring>

#include "QGCMapUrlEngine.h"
#include "QGeoFileTileCacheQGC.h"
#include "TileImageSource.h"
#include "TileMath.h"
#include "UnitTestTileGenerator.h"

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

/// Serves a canned body (or error) asynchronously; held replies stay open
/// until finished manually
class CannedReply : public QNetworkReply
{
public:
    CannedReply(const QNetworkRequest& request, const QByteArray& body, NetworkError cannedError, QObject* parent,
                bool hold)
        : QNetworkReply(parent), _body(body), _cannedError(cannedError)
    {
        setRequest(request);
        setOperation(QNetworkAccessManager::GetOperation);
        (void) open(ReadOnly);
        if (!hold) {
            QTimer::singleShot(0, this, &CannedReply::finishNow);
        }
    }

    void finishNow()
    {
        if (_cannedError != NoError) {
            setError(_cannedError, QStringLiteral("canned error"));
            emit errorOccurred(_cannedError);
        }
        setFinished(true);
        emit finished();
    }

    void abort() final {}

    qint64 bytesAvailable() const final { return (_body.size() - _offset) + QNetworkReply::bytesAvailable(); }

protected:
    qint64 readData(char* data, qint64 maxSize) final
    {
        const qint64 n = qMin<qint64>(maxSize, _body.size() - _offset);
        if (n <= 0) {
            return (_offset >= _body.size()) ? -1 : 0;
        }
        (void) memcpy(data, _body.constData() + _offset, static_cast<size_t>(n));
        _offset += n;
        return n;
    }

private:
    const QByteArray _body;
    const NetworkError _cannedError;
    qint64 _offset = 0;
};

/// Counts requests and serves the configured body/error
class MockNam : public QNetworkAccessManager
{
public:
    using QNetworkAccessManager::QNetworkAccessManager;

    QByteArray body;
    QNetworkReply::NetworkError error = QNetworkReply::NoError;
    int requestCount = 0;
    bool holdReplies = false;     ///< replies stay open until finished manually
    QList<CannedReply*> replies;  ///< creation order, only tracked while holding

protected:
    QNetworkReply* createRequest(Operation, const QNetworkRequest& request, QIODevice*) final
    {
        requestCount++;
        CannedReply* const reply = new CannedReply(request, body, error, this, holdReplies);
        if (holdReplies) {
            replies.append(reply);
        }
        return reply;
    }
};

/// Any decodable image works as a network tile body
QByteArray pngBody()
{
    QImage image(32, 32, QImage::Format_RGB32);
    image.fill(Qt::darkGreen);
    QByteArray data;
    QBuffer buffer(&data);
    if (!buffer.open(QIODevice::WriteOnly) || !image.save(&buffer, "PNG")) {
        return QByteArray();
    }
    return data;
}

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
    // Both the provider lookup and the source itself deliberately warn once at construction
    expectLogMessage("QtLocationPlugin.QGCMapUrlEngine", QtWarningMsg,
                     QRegularExpression(QStringLiteral("type not found: \"No Such Provider\"")));
    expectLogMessage("GeoMap.TileImageSource", QtWarningMsg, QRegularExpression(QStringLiteral("unknown map type")));
    TileImageSource source(QStringLiteral("No Such Provider"));
    verifyExpectedLogMessage();
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

void TileImageSourceTest::_networkFallbackDeliversAndCaches()
{
    const QString type = mapType();
    QVERIFY2(!type.isEmpty(), "no non-elevation map provider registered");
    const TileMath::TileKey key{kKey.x + 10, kKey.y, kKey.zoom};  // unique: write-back caches this tile

    MockNam nam;
    nam.body = pngBody();
    QVERIFY(!nam.body.isEmpty());
    TileImageSource source(type, nullptr, &nam);
    QSignalSpy readySpy(&source, &TileImageSource::tileImageReady);
    QSignalSpy failedSpy(&source, &TileImageSource::tileImageFailed);

    // Two forced generator misses: the first request must fall back to the
    // network; the second must be served by the write-back cached tile (the
    // DB hit precedes the generator, so the second miss stays unconsumed)
    UnitTestTileGenerator::setForcedMissCount(2);
    const auto guard = qScopeGuard([] { UnitTestTileGenerator::setForcedMissCount(0); });

    source.requestTileImage(key);
    QTRY_COMPARE_WITH_TIMEOUT(readySpy.count(), 1, 5000);
    QCOMPARE(nam.requestCount, 1);
    QVERIFY(!readySpy.first().at(1).value<QImage>().isNull());

    source.requestTileImage(key);
    QTRY_COMPARE_WITH_TIMEOUT(readySpy.count(), 2, 5000);
    QCOMPARE(nam.requestCount, 1);  // cached by write-back: no second network fetch
    QVERIFY(failedSpy.isEmpty());
}

void TileImageSourceTest::_networkErrorFails()
{
    const QString type = mapType();
    QVERIFY2(!type.isEmpty(), "no non-elevation map provider registered");
    const TileMath::TileKey key{kKey.x + 11, kKey.y, kKey.zoom};

    MockNam nam;
    nam.error = QNetworkReply::ContentNotFoundError;
    TileImageSource source(type, nullptr, &nam);
    QSignalSpy readySpy(&source, &TileImageSource::tileImageReady);
    QSignalSpy failedSpy(&source, &TileImageSource::tileImageFailed);

    UnitTestTileGenerator::setForcedMissCount(1);
    const auto guard = qScopeGuard([] { UnitTestTileGenerator::setForcedMissCount(0); });

    // Fetch failures must be visible without logging configuration
    expectLogMessage("GeoMap.TileImageSource", QtWarningMsg, QRegularExpression(QStringLiteral("network error")));
    const int requestId = source.requestTileImage(key);
    QTRY_COMPARE_WITH_TIMEOUT(failedSpy.count(), 1, 5000);
    verifyExpectedLogMessage();
    QCOMPARE(failedSpy.first().at(0).toInt(), requestId);
    QCOMPARE(nam.requestCount, 1);
    QVERIFY(readySpy.isEmpty());
}

void TileImageSourceTest::_networkEmptyBodyFails()
{
    const QString type = mapType();
    QVERIFY2(!type.isEmpty(), "no non-elevation map provider registered");
    const TileMath::TileKey key{kKey.x + 12, kKey.y, kKey.zoom};

    MockNam nam;  // NoError with an empty body: HTTP success carrying nothing
    TileImageSource source(type, nullptr, &nam);
    QSignalSpy readySpy(&source, &TileImageSource::tileImageReady);
    QSignalSpy failedSpy(&source, &TileImageSource::tileImageFailed);

    UnitTestTileGenerator::setForcedMissCount(1);
    const auto guard = qScopeGuard([] { UnitTestTileGenerator::setForcedMissCount(0); });

    expectLogMessage("GeoMap.TileImageSource", QtWarningMsg, QRegularExpression(QStringLiteral("empty body")));
    const int requestId = source.requestTileImage(key);
    QTRY_COMPARE_WITH_TIMEOUT(failedSpy.count(), 1, 5000);
    verifyExpectedLogMessage();
    QCOMPARE(failedSpy.first().at(0).toInt(), requestId);
    QVERIFY(readySpy.isEmpty());
}

void TileImageSourceTest::_networkGarbageBodyFailsAndNotCached()
{
    // An HTTP-200 non-image body (e.g. an error page) must fail the request
    // and must NOT be written back to the cache — a cached invalid tile would
    // fail every retry from then on
    const QString type = mapType();
    QVERIFY2(!type.isEmpty(), "no non-elevation map provider registered");
    const TileMath::TileKey key{kKey.x + 13, kKey.y, kKey.zoom};

    MockNam nam;
    nam.body = QByteArrayLiteral("this is not an image");
    TileImageSource source(type, nullptr, &nam);
    QSignalSpy readySpy(&source, &TileImageSource::tileImageReady);
    QSignalSpy failedSpy(&source, &TileImageSource::tileImageFailed);

    UnitTestTileGenerator::setForcedMissCount(2);
    const auto guard = qScopeGuard([] { UnitTestTileGenerator::setForcedMissCount(0); });

    expectLogMessage("GeoMap.TileImageSource", QtWarningMsg, QRegularExpression(QStringLiteral("failed to decode")));
    source.requestTileImage(key);
    QTRY_COMPARE_WITH_TIMEOUT(failedSpy.count(), 1, 5000);
    verifyExpectedLogMessage();
    QCOMPARE(nam.requestCount, 1);

    // Garbage was not cached: the retry misses again and re-fetches, this
    // time getting a valid image
    nam.body = pngBody();
    source.requestTileImage(key);
    QTRY_COMPARE_WITH_TIMEOUT(readySpy.count(), 1, 5000);
    QCOMPARE(nam.requestCount, 2);
}

void TileImageSourceTest::_bingPlaceholderNotDelivered()
{
    // Bing serves a placeholder image instead of an HTTP error where it has
    // no imagery: it decodes fine, but delivering or caching it would drape
    // it onto patches
    QFile file(QStringLiteral(":/res/BingNoTileBytes.dat"));
    QVERIFY(file.open(QFile::ReadOnly));
    const QByteArray placeholder = file.readAll();
    QVERIFY(!placeholder.isEmpty());

    const TileMath::TileKey key{kKey.x + 14, kKey.y, kKey.zoom};
    MockNam nam;
    nam.body = placeholder;
    TileImageSource source(QStringLiteral("Bing Hybrid"), nullptr, &nam);
    QSignalSpy readySpy(&source, &TileImageSource::tileImageReady);
    QSignalSpy failedSpy(&source, &TileImageSource::tileImageFailed);

    UnitTestTileGenerator::setForcedMissCount(1);
    const auto guard = qScopeGuard([] { UnitTestTileGenerator::setForcedMissCount(0); });

    expectLogMessage("GeoMap.TileImageSource", QtWarningMsg, QRegularExpression(QStringLiteral("no-tile placeholder")));
    source.requestTileImage(key);
    QTRY_COMPARE_WITH_TIMEOUT(failedSpy.count(), 1, 5000);
    verifyExpectedLogMessage();
    QCOMPARE(nam.requestCount, 1);
    QVERIFY(readySpy.isEmpty());
}

void TileImageSourceTest::_cachedUnusableTileFallsBackToNetwork()
{
    // Pre-seed the shared cache with an undecodable body: the cache-hit path
    // must treat it as a miss and recover via the network — failing would
    // wedge the tile on the same bytes every paced retry
    const QString type = mapType();
    QVERIFY2(!type.isEmpty(), "no non-elevation map provider registered");
    const TileMath::TileKey key{kKey.x + 15, kKey.y, kKey.zoom};
    // The cache worker runs tasks in order: the store lands before the lookup
    QGeoFileTileCacheQGC::cacheTile(type, key.x, key.y, key.zoom, QByteArrayLiteral("garbage"), QStringLiteral("png"));

    MockNam nam;
    nam.body = pngBody();
    QVERIFY(!nam.body.isEmpty());
    TileImageSource source(type, nullptr, &nam);
    QSignalSpy readySpy(&source, &TileImageSource::tileImageReady);
    QSignalSpy failedSpy(&source, &TileImageSource::tileImageFailed);

    const int requestId = source.requestTileImage(key);
    QTRY_COMPARE_WITH_TIMEOUT(readySpy.count(), 1, 5000);
    QCOMPARE(readySpy.first().at(0).toInt(), requestId);
    QCOMPARE(nam.requestCount, 1);  // cache hit rejected, recovered from the network
    QVERIFY(failedSpy.isEmpty());

    // Write-back is INSERT OR IGNORE, so the bad entry survives: a repeat
    // request pays another network fetch but still delivers
    source.requestTileImage(key);
    QTRY_COMPARE_WITH_TIMEOUT(readySpy.count(), 2, 5000);
    QCOMPARE(nam.requestCount, 2);
    QVERIFY(failedSpy.isEmpty());
}

void TileImageSourceTest::_cancelAbortsNetworkFetch()
{
    const QString type = mapType();
    QVERIFY2(!type.isEmpty(), "no non-elevation map provider registered");
    const TileMath::TileKey key{kKey.x + 16, kKey.y, kKey.zoom};

    MockNam nam;
    nam.body = pngBody();
    nam.holdReplies = true;
    TileImageSource source(type, nullptr, &nam);
    QSignalSpy readySpy(&source, &TileImageSource::tileImageReady);
    QSignalSpy failedSpy(&source, &TileImageSource::tileImageFailed);

    UnitTestTileGenerator::setForcedMissCount(1);
    const auto guard = qScopeGuard([] { UnitTestTileGenerator::setForcedMissCount(0); });

    const int requestId = source.requestTileImage(key);
    QTRY_COMPARE_WITH_TIMEOUT(nam.requestCount, 1, 5000);

    source.cancelRequest(requestId);
    QCOMPARE(source.pendingCount(), 0);
    source.cancelRequest(requestId);  // double cancel is a no-op

    // The aborted reply's late finished emission must stay silent
    nam.replies.at(0)->finishNow();
    QVERIFY_NO_SIGNAL_WAIT(readySpy, TestTimeout::shortMs());
    QVERIFY(failedSpy.isEmpty());
}

UT_REGISTER_TEST(TileImageSourceTest, TestLabel::Integration)
