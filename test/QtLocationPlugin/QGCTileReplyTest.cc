#include "QGCTileReplyTest.h"

#include <cstring>

#include <QtCore/QBuffer>
#include <QtCore/QFile>
#include <QtCore/QPointer>
#include <QtCore/QScopeGuard>
#include <QtCore/QTimer>
#include <QtGui/QImage>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QtTest/QSignalSpy>

#include "MapProvider.h"
#include "QGCMapUrlEngine.h"
#include "QGCTileReply.h"
#include "UnitTestTileGenerator.h"

namespace {

/// Finishes with a canned body, error and HTTP status; a held reply stays open until finishNow()
class TileReplyCannedReply : public QNetworkReply
{
public:
    TileReplyCannedReply(const QNetworkRequest& request, const QByteArray& body, NetworkError cannedError,
                         int httpStatus, bool hold, QObject* parent)
        : QNetworkReply(parent)
        , _body(body)
        , _cannedError(cannedError)
    {
        setRequest(request);
        setOperation(QNetworkAccessManager::GetOperation);
        if (httpStatus > 0) {
            setAttribute(QNetworkRequest::HttpStatusCodeAttribute, httpStatus);
        }
        (void) open(ReadOnly);
        if (!hold) {
            QTimer::singleShot(0, this, &TileReplyCannedReply::finishNow);
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

class TileReplyMockNam : public QNetworkAccessManager
{
public:
    using QNetworkAccessManager::QNetworkAccessManager;

    QByteArray body;
    QNetworkReply::NetworkError error = QNetworkReply::NoError;
    int httpStatus = 0;
    bool holdReplies = false;
    int requestCount = 0;
    QList<TileReplyCannedReply*> replies;

protected:
    QNetworkReply* createRequest(Operation, const QNetworkRequest& request, QIODevice*) final
    {
        requestCount++;
        auto* const reply = new TileReplyCannedReply(request, body, error, httpStatus, holdReplies, this);
        replies.append(reply);
        return reply;
    }
};

QByteArray tileReplyPngBody()
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

QByteArray tileReplyBingPlaceholder()
{
    QFile file(QStringLiteral(":/res/BingNoTileBytes.dat"));
    return file.open(QFile::ReadOnly) ? file.readAll() : QByteArray();
}

/// Any map provider works: under unit tests a cache miss is served by UnitTestTileGenerator unless a miss is forced
int tileReplyMapId()
{
    const QStringList elevationTypes = UrlFactory::getElevationProviderTypes();
    for (const QString& type : UrlFactory::getProviderTypes()) {
        if (!elevationTypes.contains(type)) {
            return UrlFactory::getQtMapIdFromProviderType(type);
        }
    }
    return -1;
}

constexpr int kTileX = 4302;
constexpr int kTileY = 2867;
constexpr int kTileZoom = 13;

}  // namespace

void QGCTileReplyTest::_cachedTileServedWithoutNetwork()
{
    TileReplyMockNam nam;
    QGCTileReply reply(&nam, tileReplyMapId(), kTileX, kTileY, kTileZoom);
    QSignalSpy finishedSpy(&reply, &QGCTileReply::finished);

    QVERIFY(reply.start());
    QVERIFY(!reply.isFinished());  // never finishes from within start()
    QVERIFY(!reply.start());

    QVERIFY_SIGNAL_COUNT_WAIT(finishedSpy, 1, TestTimeout::mediumMs());
    QVERIFY(reply.isFinished());
    QCOMPARE(reply.error(), QGCTileReply::Error::NoError);
    QVERIFY(reply.isCached());
    QVERIFY(!reply.data().isEmpty());
    QCOMPARE(reply.format(), QStringLiteral("png"));
    QCOMPARE(nam.requestCount, 0);
}

void QGCTileReplyTest::_serverTileStoredInCache()
{
    TileReplyMockNam nam;
    nam.body = tileReplyPngBody();
    QVERIFY(!nam.body.isEmpty());

    // The second forced miss stays unconsumed when the stored tile answers the second lookup
    UnitTestTileGenerator::setForcedMissCount(2);
    const auto guard = qScopeGuard([] { UnitTestTileGenerator::setForcedMissCount(0); });

    QGCTileReply fetched(&nam, tileReplyMapId(), kTileX + 1, kTileY, kTileZoom);
    QSignalSpy fetchedSpy(&fetched, &QGCTileReply::finished);
    QVERIFY(fetched.start());
    QVERIFY_SIGNAL_COUNT_WAIT(fetchedSpy, 1, TestTimeout::mediumMs());
    QCOMPARE(fetched.error(), QGCTileReply::Error::NoError);
    QVERIFY(!fetched.isCached());
    QCOMPARE(fetched.data(), nam.body);
    QCOMPARE(fetched.format(), QStringLiteral("png"));
    QCOMPARE(nam.requestCount, 1);

    QGCTileReply cached(&nam, tileReplyMapId(), kTileX + 1, kTileY, kTileZoom);
    QSignalSpy cachedSpy(&cached, &QGCTileReply::finished);
    QVERIFY(cached.start());
    QVERIFY_SIGNAL_COUNT_WAIT(cachedSpy, 1, TestTimeout::mediumMs());
    QVERIFY(cached.isCached());
    QCOMPARE(cached.data(), nam.body);
    QCOMPARE(nam.requestCount, 1);
}

void QGCTileReplyTest::_serverFailure_data()
{
    QTest::addColumn<QString>("providerType");
    QTest::addColumn<int>("tileX");  // a tile of its own for every row
    QTest::addColumn<QByteArray>("body");
    QTest::addColumn<QNetworkReply::NetworkError>("networkError");
    QTest::addColumn<int>("httpStatus");
    QTest::addColumn<QGCTileReply::Error>("expectedError");

    const QString mapType = UrlFactory::getProviderTypeFromQtMapId(tileReplyMapId());
    QTest::newRow("network error") << mapType << (kTileX + 10) << QByteArray() << QNetworkReply::ContentNotFoundError
                                   << 404 << QGCTileReply::Error::NetworkError;
    QTest::newRow("http error status") << mapType << (kTileX + 11) << tileReplyPngBody() << QNetworkReply::NoError
                                       << 500 << QGCTileReply::Error::NetworkError;
    QTest::newRow("empty body") << mapType << (kTileX + 12) << QByteArray() << QNetworkReply::NoError << 200
                                << QGCTileReply::Error::DataError;
    QTest::newRow("Bing placeholder") << QStringLiteral("Bing Hybrid") << (kTileX + 13) << tileReplyBingPlaceholder()
                                      << QNetworkReply::NoError << 200 << QGCTileReply::Error::NoTileError;
}

void QGCTileReplyTest::_serverFailure()
{
    QFETCH(QString, providerType);
    QFETCH(int, tileX);
    QFETCH(QByteArray, body);
    QFETCH(QNetworkReply::NetworkError, networkError);
    QFETCH(int, httpStatus);
    QFETCH(QGCTileReply::Error, expectedError);

    const int mapId = UrlFactory::getQtMapIdFromProviderType(providerType);

    TileReplyMockNam nam;
    nam.body = body;
    nam.error = networkError;
    nam.httpStatus = httpStatus;

    UnitTestTileGenerator::setForcedMissCount(2);
    const auto guard = qScopeGuard([] { UnitTestTileGenerator::setForcedMissCount(0); });

    QGCTileReply failed(&nam, mapId, tileX, kTileY, kTileZoom);
    QSignalSpy failedSpy(&failed, &QGCTileReply::finished);
    QVERIFY(failed.start());
    QVERIFY_SIGNAL_COUNT_WAIT(failedSpy, 1, TestTimeout::mediumMs());
    QCOMPARE(failed.error(), expectedError);
    QVERIFY(!failed.errorString().isEmpty());
    QVERIFY(failed.data().isEmpty());
    QCOMPARE(nam.requestCount, 1);

    // Nothing was stored: the next request misses the cache again and asks the server
    nam.body = tileReplyPngBody();
    nam.error = QNetworkReply::NoError;
    nam.httpStatus = 200;
    QGCTileReply retried(&nam, mapId, tileX, kTileY, kTileZoom);
    QSignalSpy retriedSpy(&retried, &QGCTileReply::finished);
    QVERIFY(retried.start());
    QVERIFY_SIGNAL_COUNT_WAIT(retriedSpy, 1, TestTimeout::mediumMs());
    QCOMPARE(retried.error(), QGCTileReply::Error::NoError);
    QCOMPARE(nam.requestCount, 2);
}

void QGCTileReplyTest::_abortedReplyStaysSilent()
{
    TileReplyMockNam nam;
    nam.body = tileReplyPngBody();
    nam.holdReplies = true;

    UnitTestTileGenerator::setForcedMissCount(1);
    const auto guard = qScopeGuard([] { UnitTestTileGenerator::setForcedMissCount(0); });

    QGCTileReply reply(&nam, tileReplyMapId(), kTileX + 2, kTileY, kTileZoom);
    QSignalSpy finishedSpy(&reply, &QGCTileReply::finished);
    QVERIFY(reply.start());
    QTRY_COMPARE_WITH_TIMEOUT(nam.requestCount, 1, TestTimeout::mediumMs());

    const QPointer<TileReplyCannedReply> networkReply(nam.replies.at(0));
    reply.abort();
    networkReply->finishNow();  // a late answer of the aborted fetch
    QVERIFY(finishedSpy.isEmpty());
    QVERIFY(!reply.isFinished());

    // The fetch is released without waiting for it to finish
    QTRY_VERIFY_WITH_TIMEOUT(networkReply.isNull(), TestTimeout::mediumMs());
}

void QGCTileReplyTest::_networkManagerDestroyedFirst()
{
    UnitTestTileGenerator::setForcedMissCount(2);
    const auto guard = qScopeGuard([] { UnitTestTileGenerator::setForcedMissCount(0); });

    // Gone before the cache miss: there is nothing to ask the server with
    auto* nam = new TileReplyMockNam;
    QGCTileReply unfetched(nam, tileReplyMapId(), kTileX + 5, kTileY, kTileZoom);
    QSignalSpy unfetchedSpy(&unfetched, &QGCTileReply::finished);
    QVERIFY(unfetched.start());
    delete nam;
    QVERIFY_SIGNAL_COUNT_WAIT(unfetchedSpy, 1, TestTimeout::mediumMs());
    QCOMPARE(unfetched.error(), QGCTileReply::Error::NetworkError);

    // Gone during the fetch: the manager deletes the network reply it owns, which the tile reply must not touch
    nam = new TileReplyMockNam;
    nam->holdReplies = true;
    auto* const fetching = new QGCTileReply(nam, tileReplyMapId(), kTileX + 6, kTileY, kTileZoom);
    QVERIFY(fetching->start());
    QTRY_COMPARE_WITH_TIMEOUT(nam->requestCount, 1, TestTimeout::mediumMs());
    delete nam;
    delete fetching;
}

UT_REGISTER_TEST(QGCTileReplyTest, TestLabel::Unit)
