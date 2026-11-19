#include "QGCCachedFileDownloadTest.h"

#include <memory>

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QRegularExpression>
#include <QtCore/QTemporaryDir>
#include <QtNetwork/QNetworkCacheMetaData>
#include <QtNetwork/QNetworkDiskCache>
#include <QtNetwork/QNetworkRequest>
#include <QtTest/QSignalSpy>

#include "FileFixtures.h"
#include "LocalHttpTestServer.h"
#include "QGCCachedFileDownload.h"
#include "QGCFileDownload.h"

namespace {

void setCachedDownloadTimestamp(QNetworkDiskCache* cache, const QString& url, const QDateTime& timestamp)
{
    QNetworkCacheMetaData metadata = cache->metaData(QUrl::fromUserInput(url));
    QNetworkCacheMetaData::AttributesMap attributes = metadata.attributes();
    attributes.insert(QNetworkRequest::Attribute::User, timestamp);
    metadata.setAttributes(attributes);
    cache->updateMetaData(metadata);
}

enum CachedDownloadEntryPoint
{
    CachedDownload,
    CachedDownloadPreferCache,
    CachedDownloadNoCache,
};

bool startCachedDownload(QGCCachedFileDownload& downloader, int entryPoint, const QString& url,
                         int maxCacheAgeSec = 3600)
{
    switch (entryPoint) {
        case CachedDownload:
            return downloader.download(url, maxCacheAgeSec);
        case CachedDownloadPreferCache:
            return downloader.downloadPreferCache(url);
        case CachedDownloadNoCache:
            return downloader.downloadNoCache(url);
        default:
            return false;
    }
}

/// Runs one download to its end and returns the arguments of its finished(). A download that does not start, or
/// does not finish in time, is returned as one that failed with an error that says so: where a failure is what a
/// test expects, it starts and waits itself.
QList<QVariant> runCachedDownload(QGCCachedFileDownload& downloader, const QString& url,
                                  int entryPoint = CachedDownload, int maxCacheAgeSec = 3600)
{
    // A test runs several downloads: a wait that times out names this one
    TEST_CONTEXT(url);
    QSignalSpy finishedSpy(&downloader, &QGCCachedFileDownload::finished);
    if (!startCachedDownload(downloader, entryPoint, url, maxCacheAgeSec)) {
        return {false, QString(), QStringLiteral("The download did not start: ") + downloader.errorString(), false};
    }
    if (!UnitTest::waitForSignal(finishedSpy, TestTimeout::mediumMs(), QStringLiteral("finished"))) {
        return {false, QString(), QStringLiteral("The download did not finish in time"), false};
    }
    return finishedSpy.first();
}

/// Content of the file that a finished() emission reports. The file is removed.
QByteArray takeCachedDownloadContent(const QList<QVariant>& finishedArguments)
{
    const QString path = finishedArguments.at(1).toString();
    const QByteArray content = TestFixtures::readFile(path);
    QFile::remove(path);
    return content;
}

/// Has the cache hold what the server sends for @a url. The downloaded file is removed.
bool primeCachedDownload(QGCCachedFileDownload& downloader, const QString& url)
{
    const QList<QVariant> finished = runCachedDownload(downloader, url);
    return finished.at(0).toBool() && QFile::remove(finished.at(1).toString());
}

}  // namespace

// ============================================================================
// QGCCachedFileDownload Tests
// ============================================================================
void QGCCachedFileDownloadTest::_testCachedFileDownloadConstruction()
{
    QTemporaryDir tempDir;
    // Test construction with cache directory
    QGCCachedFileDownload downloader1(tempDir.path(), this);
    // QNetworkDiskCache may append trailing slash
    QVERIFY(downloader1.cacheDirectory().startsWith(tempDir.path()));
    QVERIFY(!downloader1.isRunning());
    QCOMPARE(downloader1.progress(), 0.0);
    QVERIFY(downloader1.errorString().isEmpty());
    QVERIFY(!downloader1.isFromCache());
    QVERIFY(downloader1.localPath().isEmpty());
    // Test construction without cache directory
    QGCCachedFileDownload downloader2(this);
    QVERIFY(downloader2.cacheDirectory().isEmpty());
}

void QGCCachedFileDownloadTest::_testCachedFileDownloadCacheDirectory()
{
    QTemporaryDir tempDir;
    const QString cacheDir1 = tempDir.path() + "/cache-dir-1";
    QDir().mkpath(cacheDir1);
    const QString cacheDir2 = tempDir.path() + "/cache-dir-2";
    QDir().mkpath(cacheDir2);
    QVERIFY(!cacheDir1.isEmpty());
    QVERIFY(!cacheDir2.isEmpty());
    QGCCachedFileDownload downloader(cacheDir1, this);
    // QNetworkDiskCache may append trailing slash
    QVERIFY(downloader.cacheDirectory().startsWith(cacheDir1));
    QSignalSpy spy(&downloader, &QGCCachedFileDownload::cacheDirectoryChanged);
    downloader.setCacheDirectory(cacheDir2);
    QVERIFY(downloader.cacheDirectory().startsWith(cacheDir2));
    QCOMPARE(spy.count(), 1);
    QVERIFY(spy.first().first().toString().startsWith(cacheDir2));
    // Setting same directory should not emit signal
    downloader.setCacheDirectory(downloader.cacheDirectory());  // Use actual value to avoid trailing slash mismatch
    QCOMPARE(spy.count(), 1);
}

void QGCCachedFileDownloadTest::_testCachedFileDownloadMaxCacheSize()
{
    QTemporaryDir tempDir;
    QGCCachedFileDownload downloader(tempDir.path(), this);
    QSignalSpy spy(&downloader, &QGCCachedFileDownload::maxCacheSizeChanged);
    constexpr qint64 newSize = 100 * 1024 * 1024;  // 100 MB
    downloader.setMaxCacheSize(newSize);
    QCOMPARE(downloader.maxCacheSize(), newSize);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.first().first().toLongLong(), newSize);
    // Setting same size should not emit signal
    downloader.setMaxCacheSize(newSize);
    QCOMPARE(spy.count(), 1);
}

void QGCCachedFileDownloadTest::_testCachedFileDownloadRejectedStart_data()
{
    QTest::addColumn<int>("entryPoint");
    QTest::addColumn<bool>("cacheDirectorySet");
    QTest::addColumn<QString>("url");
    QTest::addColumn<QString>("warning");
    QTest::addColumn<QString>("error");

    const QString emptyUrlWarning = QStringLiteral("Empty URL");
    const QString emptyUrlError = QStringLiteral("Empty URL");
    QTest::newRow("download, empty URL") << int(CachedDownload) << true << QString() << emptyUrlWarning
                                         << emptyUrlError;
    QTest::newRow("downloadPreferCache, empty URL")
        << int(CachedDownloadPreferCache) << true << QString() << emptyUrlWarning << emptyUrlError;
    QTest::newRow("downloadNoCache, empty URL")
        << int(CachedDownloadNoCache) << true << QString() << emptyUrlWarning << emptyUrlError;

    const QString resourceUrl = QStringLiteral(":/unittest/arducopter.apj");
    const QString noCacheDirWarning = QStringLiteral("Cache directory not set");
    const QString noCacheDirError = QStringLiteral("Cache directory not configured");
    QTest::newRow("download, no cache directory")
        << int(CachedDownload) << false << resourceUrl << noCacheDirWarning << noCacheDirError;
    QTest::newRow("downloadPreferCache, no cache directory")
        << int(CachedDownloadPreferCache) << false << resourceUrl << noCacheDirWarning << noCacheDirError;
}

void QGCCachedFileDownloadTest::_testCachedFileDownloadRejectedStart()
{
    QFETCH(int, entryPoint);
    QFETCH(bool, cacheDirectorySet);
    QFETCH(QString, url);
    QFETCH(QString, warning);
    QFETCH(QString, error);

    QTemporaryDir tempDir;
    QGCCachedFileDownload downloader(this);
    if (cacheDirectorySet) {
        downloader.setCacheDirectory(tempDir.path());
    }
    QSignalSpy finishedSpy(&downloader, &QGCCachedFileDownload::finished);
    QSignalSpy urlSpy(&downloader, &QGCCachedFileDownload::urlChanged);

    expectLogMessage("Utilities.QGCCachedFileDownload", QtWarningMsg, QRegularExpression(warning));
    QVERIFY(!startCachedDownload(downloader, entryPoint, url));
    verifyExpectedLogMessage();

    QCOMPARE(downloader.errorString(), error);
    QVERIFY(!downloader.isRunning());
    QCOMPARE(finishedSpy.count(), 0);
    QCOMPARE(urlSpy.count(), 0);
}

void QGCCachedFileDownloadTest::_testCachedFileDownloadDownload_data()
{
    QTest::addColumn<int>("entryPoint");
    QTest::addColumn<bool>("cacheDirectorySet");
    QTest::addColumn<QString>("url");

    const QString firmware = QStringLiteral(":/unittest/arducopter.apj");
    QTest::newRow("download") << int(CachedDownload) << true << QStringLiteral(":/unittest/manifest.json.gz");
    QTest::newRow("downloadPreferCache") << int(CachedDownloadPreferCache) << true << firmware;
    QTest::newRow("downloadNoCache") << int(CachedDownloadNoCache) << true << firmware;
    QTest::newRow("downloadNoCache, no cache directory") << int(CachedDownloadNoCache) << false << firmware;
}

void QGCCachedFileDownloadTest::_testCachedFileDownloadDownload()
{
    QFETCH(int, entryPoint);
    QFETCH(bool, cacheDirectorySet);
    QFETCH(QString, url);

    // Each of the two constructors makes the downloader of its rows
    QTemporaryDir tempDir;
    const std::unique_ptr<QGCCachedFileDownload> owner(
        cacheDirectorySet ? new QGCCachedFileDownload(tempDir.path(), this) : new QGCCachedFileDownload(this));
    QGCCachedFileDownload& downloader = *owner;
    QSignalSpy progressSpy(&downloader, &QGCCachedFileDownload::progressChanged);
    QSignalSpy bytesSpy(&downloader, &QGCCachedFileDownload::downloadProgress);
    QSignalSpy runningSpy(&downloader, &QGCCachedFileDownload::runningChanged);
    QSignalSpy urlSpy(&downloader, &QGCCachedFileDownload::urlChanged);
    QSignalSpy finishedSpy(&downloader, &QGCCachedFileDownload::finished);

    QVERIFY(startCachedDownload(downloader, entryPoint, url));
    QCOMPARE(urlSpy.count(), 1);
    QCOMPARE(downloader.url(), QUrl::fromUserInput(url));
    QCOMPARE(runningSpy.count(), 1);
    QVERIFY(downloader.isRunning());
    QVERIFY_SIGNAL_WAIT(finishedSpy, TestTimeout::mediumMs());

    QCOMPARE(finishedSpy.count(), 1);
    const QList<QVariant> args = finishedSpy.first();
    QCOMPARE(args.count(), 4);
    QVERIFY2(args.at(0).toBool(), qPrintable(args.at(2).toString()));
    QCOMPARE(args.at(3).toBool(), downloader.fileDownloader()->lastResultFromCache());
    QCOMPARE(downloader.isFromCache(), downloader.fileDownloader()->lastResultFromCache());
    QVERIFY(!downloader.isRunning());
    QVERIFY(!takeCachedDownloadContent(args).isEmpty());

    // The progress of the download is passed on: its byte counts, and the fraction they make, which ends at 1
    QVERIFY(!bytesSpy.isEmpty());
    QVERIFY(bytesSpy.constLast().at(0).toLongLong() > 0);
    QCOMPARE(bytesSpy.constLast().at(0), bytesSpy.constLast().at(1));
    QVERIFY(!progressSpy.isEmpty());
    QCOMPARE(progressSpy.constLast().constFirst().toReal(), 1.0);
    QCOMPARE(downloader.progress(), 1.0);
}

void QGCCachedFileDownloadTest::_testCachedFileDownloadSyntheticCacheHit()
{
    QTemporaryDir tempDir;
    QGCCachedFileDownload downloader(tempDir.path(), this);

    const QByteArray payload("cached-data");
    TestFixtures::LocalHttpTestServer server;
    QVERIFY2(server.listen(), "Could not start local test HTTP server");
    const QString url = server.url(QStringLiteral("/synthetic-cache-hit.txt"));
    server.installHttpResponder(payload, 200, "text/plain", 3600);

    QVERIFY(primeCachedDownload(downloader, url));

    server.close();
    const QList<QVariant> args = runCachedDownload(downloader, url);
    QVERIFY2(args.at(0).toBool(), qPrintable(args.at(2).toString()));
    QVERIFY(args.at(3).toBool());
    QCOMPARE(takeCachedDownloadContent(args), payload);
}

void QGCCachedFileDownloadTest::_testCachedFileDownloadCacheHitKeepsTimestamp()
{
    QTemporaryDir tempDir;
    QGCCachedFileDownload downloader(tempDir.path(), this);

    TestFixtures::LocalHttpTestServer server;
    QVERIFY2(server.listen(), "Could not start local test HTTP server");
    const QString url = server.url(QStringLiteral("/cache-hit-timestamp.txt"));
    server.installHttpResponder("cached-data", 200, "text/plain", 3600);

    const QList<QVariant> fromNetwork = runCachedDownload(downloader, url);
    QVERIFY2(fromNetwork.at(0).toBool(), qPrintable(fromNetwork.at(2).toString()));
    QVERIFY(!fromNetwork.at(3).toBool());
    QFile::remove(fromNetwork.at(1).toString());
    server.close();

    // Old enough to tell from a fresh stamp, young enough for the next download to be a cache hit
    constexpr int ageSec = 600;
    setCachedDownloadTimestamp(downloader.diskCache(), url, QDateTime::currentDateTime().addSecs(-ageSec));

    const QList<QVariant> fromCache = runCachedDownload(downloader, url);
    QVERIFY2(fromCache.at(0).toBool(), qPrintable(fromCache.at(2).toString()));
    QVERIFY(fromCache.at(3).toBool());
    QFile::remove(fromCache.at(1).toString());

    QVERIFY(downloader.cacheAge(url) >= ageSec);
}

void QGCCachedFileDownloadTest::_testCachedFileDownloadMaxAgeDecidesCacheOrNetwork_data()
{
    QTest::addColumn<int>("entryAgeSec");
    QTest::addColumn<int>("maxCacheAgeSec");
    QTest::addColumn<bool>("fromCache");

    QTest::newRow("younger than the maximum age") << 600 << 3600 << true;
    QTest::newRow("older than the maximum age") << 7200 << 3600 << false;
    QTest::newRow("no maximum age") << 7200 << 0 << true;
}

void QGCCachedFileDownloadTest::_testCachedFileDownloadMaxAgeDecidesCacheOrNetwork()
{
    QFETCH(int, entryAgeSec);
    QFETCH(int, maxCacheAgeSec);
    QFETCH(bool, fromCache);

    QTemporaryDir tempDir;
    QGCCachedFileDownload downloader(tempDir.path(), this);

    const QByteArray cachedBody("cached body");
    const QByteArray serverBody("newer body on the server");
    TestFixtures::LocalHttpTestServer server;
    QVERIFY2(server.listen(), "Could not start local test HTTP server");
    const QString url = server.url(QStringLiteral("/max-age.txt"));
    server.installHttpResponder(cachedBody, 200, "text/plain", 3600);
    const QList<QVariant> first = runCachedDownload(downloader, url, CachedDownload, maxCacheAgeSec);
    QVERIFY2(first.at(0).toBool(), qPrintable(first.at(2).toString()));
    QCOMPARE(takeCachedDownloadContent(first), cachedBody);

    server.installHttpResponder(serverBody, 200, "text/plain", 3600);
    setCachedDownloadTimestamp(downloader.diskCache(), url, QDateTime::currentDateTime().addSecs(-entryAgeSec));
    QCOMPARE(downloader.isCached(url, maxCacheAgeSec), fromCache);

    const QList<QVariant> second = runCachedDownload(downloader, url, CachedDownload, maxCacheAgeSec);
    QVERIFY2(second.at(0).toBool(), qPrintable(second.at(2).toString()));
    QCOMPARE(second.at(3).toBool(), fromCache);
    QCOMPARE(takeCachedDownloadContent(second), fromCache ? cachedBody : serverBody);

    // Only a download from the network renews the entry
    QCOMPARE(downloader.cacheAge(url) < entryAgeSec, !fromCache);
}

void QGCCachedFileDownloadTest::_testCachedFileDownloadExpiredFallsBackToCache()
{
    QTemporaryDir tempDir;
    QGCCachedFileDownload downloader(tempDir.path(), this);

    // No Cache-Control, so HTTP considers the stored reply stale at once
    const QByteArray payload("stale-cache-data");
    TestFixtures::LocalHttpTestServer server;
    QVERIFY2(server.listen(), "Could not start local test HTTP server");
    const QString url = server.url(QStringLiteral("/expired-fallback.txt"));
    server.installHttpResponder(payload, 200, "text/plain");

    QVERIFY(primeCachedDownload(downloader, url));
    server.close();

    setCachedDownloadTimestamp(downloader.diskCache(), url, QDateTime::currentDateTime().addSecs(-7200));

    expectLogMessage("Utilities.QGCFileDownload", QtWarningMsg, QRegularExpression("Download error:"));
    const QList<QVariant> args = runCachedDownload(downloader, url);
    verifyExpectedLogMessage();

    QVERIFY2(args.at(0).toBool(), qPrintable(args.at(2).toString()));
    QVERIFY(args.at(3).toBool());
    QCOMPARE(takeCachedDownloadContent(args), payload);
}

void QGCCachedFileDownloadTest::_testCachedFileDownloadStartForgetsFailedNetworkAttempt_data()
{
    QTest::addColumn<int>("entryPoint");

    QTest::newRow("download") << int(CachedDownload);
    QTest::newRow("downloadPreferCache") << int(CachedDownloadPreferCache);
    QTest::newRow("downloadNoCache") << int(CachedDownloadNoCache);
}

void QGCCachedFileDownloadTest::_testCachedFileDownloadStartForgetsFailedNetworkAttempt()
{
    QFETCH(int, entryPoint);

    QTemporaryDir tempDir;
    QGCCachedFileDownload downloader(tempDir.path(), this);

    // No Cache-Control, so HTTP considers the stored replies stale at once
    const QByteArray cachedBody("cached body");
    TestFixtures::LocalHttpTestServer failingServer;
    TestFixtures::LocalHttpTestServer server;
    QVERIFY2(failingServer.listen() && server.listen(), "Could not start local test HTTP server");
    const QString failingUrl = failingServer.url(QStringLiteral("/failing.txt"));
    const QString url = server.url(QStringLiteral("/next.txt"));
    failingServer.installHttpResponder(cachedBody, 200, "text/plain");
    server.installHttpResponder(cachedBody, 200, "text/plain");
    for (const QString& cachedUrl : {failingUrl, url}) {
        const QList<QVariant> cached = runCachedDownload(downloader, cachedUrl);
        QVERIFY2(cached.at(0).toBool(), qPrintable(cached.at(2).toString()));
        QCOMPARE(takeCachedDownloadContent(cached), cachedBody);
    }

    // The refresh of an expired entry fails and the download falls back to the cache
    failingServer.close();
    setCachedDownloadTimestamp(downloader.diskCache(), failingUrl, QDateTime::currentDateTime().addSecs(-7200));
    expectLogMessage("Utilities.QGCFileDownload", QtWarningMsg, QRegularExpression("Download error:"));
    const QList<QVariant> fallback = runCachedDownload(downloader, failingUrl);
    verifyExpectedLogMessage();
    QVERIFY2(fallback.at(0).toBool(), qPrintable(fallback.at(2).toString()));
    QVERIFY(fallback.at(3).toBool());
    QCOMPARE(takeCachedDownloadContent(fallback), cachedBody);

    if (entryPoint == CachedDownloadNoCache) {
        // The next failure is reported as its own, not as the one of the download before
        QSignalSpy finishedSpy(&downloader, &QGCCachedFileDownload::finished);
        server.installHttpResponder("not here", 404, "text/plain");
        expectLogMessage("Utilities.QGCFileDownload", QtWarningMsg, QRegularExpression("Download error:"));
        QVERIFY(startCachedDownload(downloader, entryPoint, url));
        QVERIFY_SIGNAL_WAIT(finishedSpy, TestTimeout::mediumMs());
        verifyExpectedLogMessage();
        QVERIFY(!finishedSpy.first().at(0).toBool());
        QVERIFY2(finishedSpy.first().at(2).toString().contains(QStringLiteral("Not Found")),
                 qPrintable(finishedSpy.first().at(2).toString()));
        return;
    }

    // The next download refreshes a stale entry from the network: only the fallback keeps to the cache
    const QByteArray serverBody("newer body on the server");
    server.installHttpResponder(serverBody, 200, "text/plain");
    const QList<QVariant> refreshed = runCachedDownload(downloader, url, entryPoint);
    QVERIFY2(refreshed.at(0).toBool(), qPrintable(refreshed.at(2).toString()));
    QVERIFY(!refreshed.at(3).toBool());
    QCOMPARE(takeCachedDownloadContent(refreshed), serverBody);
}

void QGCCachedFileDownloadTest::_testCachedFileDownloadNoCacheDoesNotFallBack()
{
    QTemporaryDir tempDir;
    QGCCachedFileDownload downloader(tempDir.path(), this);

    TestFixtures::LocalHttpTestServer server;
    QVERIFY2(server.listen(), "Could not start local test HTTP server");
    const QString url = server.url(QStringLiteral("/no-cache-fallback.txt"));
    server.installHttpResponder("cached-data", 200, "text/plain", 3600);

    QVERIFY(primeCachedDownload(downloader, url));
    server.close();
    QVERIFY(downloader.isCached(url));

    QSignalSpy finishedSpy(&downloader, &QGCCachedFileDownload::finished);
    expectLogMessage("Utilities.QGCFileDownload", QtWarningMsg, QRegularExpression("Download error:"));
    QVERIFY(downloader.downloadNoCache(url));
    QVERIFY_SIGNAL_WAIT(finishedSpy, TestTimeout::mediumMs());
    verifyExpectedLogMessage();

    QVERIFY(!finishedSpy.first().at(0).toBool());
    QVERIFY(!finishedSpy.first().at(3).toBool());
}

void QGCCachedFileDownloadTest::_testCachedFileDownloadFailedFallbackReportsNetworkError()
{
    QTemporaryDir tempDir;
    QGCCachedFileDownload downloader(tempDir.path(), this);

    // must-revalidate: the stored reply may not be used without asking the server, so the fallback fails too
    TestFixtures::LocalHttpTestServer server;
    QVERIFY2(server.listen(), "Could not start local test HTTP server");
    const QString url = server.url(QStringLiteral("/must-revalidate.txt"));
    server.installRawResponder(
        "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nConnection: close\r\n"
        "Cache-Control: must-revalidate\r\nContent-Length: 4\r\n\r\ndata");

    QVERIFY(primeCachedDownload(downloader, url));
    server.close();
    QVERIFY(downloader.isCached(url));

    // The error a download reports when only the network is tried
    QTemporaryDir emptyCacheDir;
    QGCCachedFileDownload networkOnlyDownloader(emptyCacheDir.path(), this);
    QSignalSpy networkOnlySpy(&networkOnlyDownloader, &QGCCachedFileDownload::finished);
    expectLogMessage("Utilities.QGCFileDownload", QtWarningMsg, QRegularExpression("Download error:"));
    QVERIFY(networkOnlyDownloader.download(url, 3600));
    QVERIFY_SIGNAL_WAIT(networkOnlySpy, TestTimeout::mediumMs());
    verifyExpectedLogMessage();
    QVERIFY(!networkOnlySpy.first().at(0).toBool());
    const QString networkError = networkOnlySpy.first().at(2).toString();
    QVERIFY(!networkError.isEmpty());

    setCachedDownloadTimestamp(downloader.diskCache(), url, QDateTime::currentDateTime().addSecs(-7200));

    QSignalSpy finishedSpy(&downloader, &QGCCachedFileDownload::finished);
    // One warning for the network attempt and one for the cache
    expectLogMessage("Utilities.QGCFileDownload", QtWarningMsg, QRegularExpression("Download error:"));
    expectLogMessage("Utilities.QGCFileDownload", QtWarningMsg, QRegularExpression("Download error:"));
    QVERIFY(downloader.download(url, 3600));
    QVERIFY_SIGNAL_WAIT(finishedSpy, TestTimeout::mediumMs());
    verifyExpectedLogMessage();
    verifyExpectedLogMessage();

    QVERIFY(!finishedSpy.first().at(0).toBool());
    QCOMPARE(finishedSpy.first().at(2).toString(), networkError);
    QCOMPARE(downloader.errorString(), networkError);
}

void QGCCachedFileDownloadTest::_testCachedFileDownloadCachedPath()
{
    QTemporaryDir tempDir;
    QGCCachedFileDownload downloader(tempDir.path(), this);

    const QByteArray payload("cached-path-data");
    TestFixtures::LocalHttpTestServer server;
    QVERIFY2(server.listen(), "Could not start local test HTTP server");
    const QString url = server.url(QStringLiteral("/cached-path.txt"));
    QVERIFY(downloader.cachedPath(url).isEmpty());
    server.installHttpResponder(payload, 200, "text/plain", 3600);

    QVERIFY(primeCachedDownload(downloader, url));
    server.close();

    const QString cachedPath = downloader.cachedPath(url);
    QCOMPARE(TestFixtures::readFile(cachedPath), payload);
    QFile::remove(cachedPath);
}

void QGCCachedFileDownloadTest::_testCachedFileDownloadEmptyCache()
{
    QTemporaryDir tempDir;
    QGCCachedFileDownload downloader(tempDir.path(), this);
    const QString url = QStringLiteral(":/unittest/manifest.json.gz");

    QVERIFY(!downloader.isCached(url));
    QVERIFY(!downloader.isCached(url, 3600));
    QCOMPARE(downloader.cacheAge(url), -1);
    QVERIFY(!downloader.removeFromCache(url));
    // Clearing a cache that holds nothing works too
    downloader.clearCache();
    QCOMPARE(downloader.cacheSize(), static_cast<qint64>(0));
}

void QGCCachedFileDownloadTest::_testCachedFileDownloadIsCachedInvalidTimestamp()
{
    QTemporaryDir tempDir;
    QGCCachedFileDownload downloader(tempDir.path(), this);

    const QByteArray payload("cached-invalid-timestamp");
    TestFixtures::LocalHttpTestServer server;
    QVERIFY2(server.listen(), "Could not start local test HTTP server");
    const QString url = server.url(QStringLiteral("/cached-invalid-timestamp.txt"));
    server.installHttpResponder(payload, 200, "text/plain", 3600);

    QVERIFY(primeCachedDownload(downloader, url));

    QNetworkCacheMetaData metadata = downloader.diskCache()->metaData(QUrl::fromUserInput(url));
    QVERIFY(metadata.isValid());
    QNetworkCacheMetaData::AttributesMap attributes = metadata.attributes();
    attributes.remove(QNetworkRequest::Attribute::User);
    metadata.setAttributes(attributes);
    downloader.diskCache()->updateMetaData(metadata);

    QVERIFY(downloader.isCached(url));
    QVERIFY(!downloader.isCached(url, 3600));
}

void QGCCachedFileDownloadTest::_testCachedFileDownloadCancel()
{
    QTemporaryDir tempDir;
    QGCCachedFileDownload downloader(tempDir.path(), this);
    QSignalSpy finishedSpy(&downloader, &QGCCachedFileDownload::finished);

    // Start download and immediately cancel
    QVERIFY(downloader.download(":/unittest/manifest.json.gz", 3600));
    downloader.cancel();

    QTRY_COMPARE_WITH_TIMEOUT(finishedSpy.count(), 1, TestTimeout::mediumMs());
    QVERIFY(!downloader.isRunning());

    const QList<QVariant> args = finishedSpy.first();
    QVERIFY(!args.at(0).toBool());  // success
    QVERIFY(args.at(1).toString().isEmpty());
    QVERIFY(args.at(2).toString().contains(QStringLiteral("cancel"), Qt::CaseInsensitive));
    QVERIFY(!args.at(3).toBool());  // fromCache
}

void QGCCachedFileDownloadTest::_testCachedFileDownloadConcurrentDownloadRejected()
{
    QTemporaryDir tempDir;
    QGCCachedFileDownload downloader(tempDir.path(), this);
    // Start first download
    QVERIFY(downloader.download(":/unittest/manifest.json.gz", 3600));
    // Each of the three rejection calls logs "Download already in progress"
    ignoreLogMessage("Utilities.QGCCachedFileDownload", QtWarningMsg, QRegularExpression("Download already in progress"));
    // Second download should be rejected while first is running
    QVERIFY(!downloader.download(":/unittest/arducopter.apj", 3600));
    QVERIFY(!downloader.downloadPreferCache(":/unittest/arducopter.apj"));
    QVERIFY(!downloader.downloadNoCache(":/unittest/arducopter.apj"));
    // Wait for first download to complete
    QSignalSpy spy(&downloader, &QGCCachedFileDownload::finished);
    QVERIFY_SIGNAL_WAIT(spy, TestTimeout::mediumMs());
}

void QGCCachedFileDownloadTest::_testCachedFileDownloadReentrantStartFromCompletion()
{
    QTemporaryDir tempDir;
    QGCCachedFileDownload downloader(tempDir.path(), this);

    QSignalSpy finishedSpy(&downloader, &QGCCachedFileDownload::finished);
    bool secondStartAttempted = false;
    bool secondStartSucceeded = false;
    QString secondStartError;

    connect(&downloader, &QGCCachedFileDownload::finished, this,
            [&](bool success, const QString&, const QString&, bool) {
                if (!secondStartAttempted && success) {
                    secondStartAttempted = true;
                    secondStartSucceeded = downloader.download(":/unittest/arducopter.apj", 3600);
                    secondStartError = downloader.errorString();
                }
            });

    QVERIFY(downloader.download(":/unittest/manifest.json.gz", 3600));

    QVERIFY_SIGNAL_COUNT_WAIT(finishedSpy, 2, TestTimeout::mediumMs());

    QVERIFY(secondStartAttempted);
    QVERIFY2(secondStartSucceeded, qPrintable(secondStartError));
    QCOMPARE(finishedSpy.count(), 2);
    QVERIFY(!downloader.isRunning());
}

UT_REGISTER_TEST(QGCCachedFileDownloadTest, TestLabel::Integration, TestLabel::Network, TestLabel::Utilities)
