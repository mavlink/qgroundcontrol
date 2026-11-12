#include "QGCCachedFileDownloadTest.h"

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

bool startCachedDownload(QGCCachedFileDownload& downloader, int entryPoint, const QString& url)
{
    switch (entryPoint) {
        case CachedDownload:
            return downloader.download(url, 3600);
        case CachedDownloadPreferCache:
            return downloader.downloadPreferCache(url);
        case CachedDownloadNoCache:
            return downloader.downloadNoCache(url);
        default:
            return false;
    }
}

/// Content of the file that a finished() emission reports. The file is removed.
QByteArray takeCachedDownloadContent(const QList<QVariant>& finishedArguments)
{
    const QString path = finishedArguments.at(1).toString();
    const QByteArray content = TestFixtures::readFile(path);
    QFile::remove(path);
    return content;
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

void QGCCachedFileDownloadTest::_testCachedFileDownloadNoCacheNeedsNoCacheDirectory()
{
    QGCCachedFileDownload downloader(this);
    QSignalSpy finishedSpy(&downloader, &QGCCachedFileDownload::finished);

    QVERIFY(downloader.downloadNoCache(":/unittest/arducopter.apj"));
    QVERIFY_SIGNAL_WAIT(finishedSpy, TestTimeout::mediumMs());
    QVERIFY2(finishedSpy.first().at(0).toBool(), qPrintable(finishedSpy.first().at(2).toString()));
    QVERIFY(!takeCachedDownloadContent(finishedSpy.first()).isEmpty());
}

void QGCCachedFileDownloadTest::_testCachedFileDownloadDownload()
{
    QTemporaryDir tempDir;
    QGCCachedFileDownload downloader(tempDir.path(), this);
    bool finished = false;
    bool success = false;
    QString localPath;
    bool fromCache = false;
    connect(&downloader, &QGCCachedFileDownload::finished, this,
            [&](bool s, const QString& path, const QString&, bool cached) {
                finished = true;
                success = s;
                localPath = path;
                fromCache = cached;
            });
    QSignalSpy spy(&downloader, &QGCCachedFileDownload::finished);
    // Download with 1 hour cache
    QVERIFY(downloader.download(":/unittest/manifest.json.gz", 3600));
    QVERIFY(downloader.isRunning());
    QVERIFY_SIGNAL_WAIT(spy, TestTimeout::mediumMs());
    QVERIFY(finished);
    QVERIFY(success);
    QVERIFY(!localPath.isEmpty());
    QVERIFY(QFile::exists(localPath));
    QVERIFY(!downloader.isRunning());
    QCOMPARE(fromCache, downloader.fileDownloader()->lastResultFromCache());
    QCOMPARE(downloader.isFromCache(), downloader.fileDownloader()->lastResultFromCache());
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

    QSignalSpy firstFinishedSpy(&downloader, &QGCCachedFileDownload::finished);
    QVERIFY(downloader.download(url, 3600));
    QVERIFY_SIGNAL_WAIT(firstFinishedSpy, TestTimeout::mediumMs());
    const QList<QVariant> firstArgs = firstFinishedSpy.first();
    QVERIFY(firstArgs.at(0).toBool());

    server.close();
    QSignalSpy finishedSpy(&downloader, &QGCCachedFileDownload::finished);
    QVERIFY(downloader.download(url, 3600));
    QVERIFY_SIGNAL_WAIT(finishedSpy, TestTimeout::mediumMs());
    const QList<QVariant> args = finishedSpy.first();
    QVERIFY(args.at(0).toBool());
    const QString localPath = args.at(1).toString();
    QVERIFY(!localPath.isEmpty());
    QVERIFY(QFile::exists(localPath));
    QVERIFY(args.at(3).toBool());

    QFile file(localPath);
    QVERIFY(file.open(QIODevice::ReadOnly));
    QCOMPARE(file.readAll(), payload);
    file.close();
    QFile::remove(localPath);
}

void QGCCachedFileDownloadTest::_testCachedFileDownloadCacheHitKeepsTimestamp()
{
    QTemporaryDir tempDir;
    QGCCachedFileDownload downloader(tempDir.path(), this);

    TestFixtures::LocalHttpTestServer server;
    QVERIFY2(server.listen(), "Could not start local test HTTP server");
    const QString url = server.url(QStringLiteral("/cache-hit-timestamp.txt"));
    server.installHttpResponder("cached-data", 200, "text/plain", 3600);

    QSignalSpy finishedSpy(&downloader, &QGCCachedFileDownload::finished);
    QVERIFY(downloader.download(url, 3600));
    QVERIFY_SIGNAL_WAIT(finishedSpy, TestTimeout::mediumMs());
    QVERIFY(finishedSpy.first().at(0).toBool());
    QVERIFY(!finishedSpy.first().at(3).toBool());
    QFile::remove(finishedSpy.first().at(1).toString());
    server.close();

    // Old enough to tell from a fresh stamp, young enough for the next download to be a cache hit
    constexpr int ageSec = 600;
    setCachedDownloadTimestamp(downloader.diskCache(), url, QDateTime::currentDateTime().addSecs(-ageSec));

    finishedSpy.clear();
    QVERIFY(downloader.download(url, 3600));
    QVERIFY_SIGNAL_WAIT(finishedSpy, TestTimeout::mediumMs());
    QVERIFY(finishedSpy.first().at(0).toBool());
    QVERIFY(finishedSpy.first().at(3).toBool());
    QFile::remove(finishedSpy.first().at(1).toString());

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
    QSignalSpy finishedSpy(&downloader, &QGCCachedFileDownload::finished);

    const QByteArray cachedBody("cached body");
    const QByteArray serverBody("newer body on the server");
    TestFixtures::LocalHttpTestServer server;
    QVERIFY2(server.listen(), "Could not start local test HTTP server");
    const QString url = server.url(QStringLiteral("/max-age.txt"));
    server.installHttpResponder(cachedBody, 200, "text/plain", 3600);
    QVERIFY(downloader.download(url, maxCacheAgeSec));
    QVERIFY_SIGNAL_WAIT(finishedSpy, TestTimeout::mediumMs());
    QVERIFY(finishedSpy.first().at(0).toBool());
    QCOMPARE(takeCachedDownloadContent(finishedSpy.first()), cachedBody);

    server.installHttpResponder(serverBody, 200, "text/plain", 3600);
    setCachedDownloadTimestamp(downloader.diskCache(), url, QDateTime::currentDateTime().addSecs(-entryAgeSec));
    QCOMPARE(downloader.isCached(url, maxCacheAgeSec), fromCache);

    finishedSpy.clear();
    QVERIFY(downloader.download(url, maxCacheAgeSec));
    QVERIFY_SIGNAL_WAIT(finishedSpy, TestTimeout::mediumMs());
    QVERIFY2(finishedSpy.first().at(0).toBool(), qPrintable(finishedSpy.first().at(2).toString()));
    QCOMPARE(finishedSpy.first().at(3).toBool(), fromCache);
    QCOMPARE(takeCachedDownloadContent(finishedSpy.first()), fromCache ? cachedBody : serverBody);

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

    QSignalSpy finishedSpy(&downloader, &QGCCachedFileDownload::finished);
    QVERIFY(downloader.download(url, 3600));
    QVERIFY_SIGNAL_WAIT(finishedSpy, TestTimeout::mediumMs());
    QVERIFY(finishedSpy.first().at(0).toBool());
    QFile::remove(finishedSpy.first().at(1).toString());
    server.close();

    setCachedDownloadTimestamp(downloader.diskCache(), url, QDateTime::currentDateTime().addSecs(-7200));

    finishedSpy.clear();
    expectLogMessage("Utilities.QGCFileDownload", QtWarningMsg, QRegularExpression("Download error:"));
    QVERIFY(downloader.download(url, 3600));
    QVERIFY_SIGNAL_WAIT(finishedSpy, TestTimeout::mediumMs());
    verifyExpectedLogMessage();

    const QList<QVariant> args = finishedSpy.first();
    QVERIFY2(args.at(0).toBool(), qPrintable(args.at(2).toString()));
    QVERIFY(args.at(3).toBool());
    const QString localPath = args.at(1).toString();
    QFile file(localPath);
    QVERIFY(file.open(QIODevice::ReadOnly));
    QCOMPARE(file.readAll(), payload);
    file.close();
    QFile::remove(localPath);
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
    QSignalSpy finishedSpy(&downloader, &QGCCachedFileDownload::finished);

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
        finishedSpy.clear();
        QVERIFY(downloader.download(cachedUrl, 3600));
        QVERIFY_SIGNAL_WAIT(finishedSpy, TestTimeout::mediumMs());
        QVERIFY(finishedSpy.first().at(0).toBool());
        QCOMPARE(takeCachedDownloadContent(finishedSpy.first()), cachedBody);
    }

    // The refresh of an expired entry fails and the download falls back to the cache
    failingServer.close();
    setCachedDownloadTimestamp(downloader.diskCache(), failingUrl, QDateTime::currentDateTime().addSecs(-7200));
    finishedSpy.clear();
    expectLogMessage("Utilities.QGCFileDownload", QtWarningMsg, QRegularExpression("Download error:"));
    QVERIFY(downloader.download(failingUrl, 3600));
    QVERIFY_SIGNAL_WAIT(finishedSpy, TestTimeout::mediumMs());
    verifyExpectedLogMessage();
    QVERIFY2(finishedSpy.first().at(0).toBool(), qPrintable(finishedSpy.first().at(2).toString()));
    QVERIFY(finishedSpy.first().at(3).toBool());
    QCOMPARE(takeCachedDownloadContent(finishedSpy.first()), cachedBody);

    finishedSpy.clear();
    if (entryPoint == CachedDownloadNoCache) {
        // The next failure is reported as its own, not as the one of the download before
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
    QVERIFY(startCachedDownload(downloader, entryPoint, url));
    QVERIFY_SIGNAL_WAIT(finishedSpy, TestTimeout::mediumMs());
    QVERIFY2(finishedSpy.first().at(0).toBool(), qPrintable(finishedSpy.first().at(2).toString()));
    QVERIFY(!finishedSpy.first().at(3).toBool());
    QCOMPARE(takeCachedDownloadContent(finishedSpy.first()), serverBody);
}

void QGCCachedFileDownloadTest::_testCachedFileDownloadNoCacheDoesNotFallBack()
{
    QTemporaryDir tempDir;
    QGCCachedFileDownload downloader(tempDir.path(), this);

    TestFixtures::LocalHttpTestServer server;
    QVERIFY2(server.listen(), "Could not start local test HTTP server");
    const QString url = server.url(QStringLiteral("/no-cache-fallback.txt"));
    server.installHttpResponder("cached-data", 200, "text/plain", 3600);

    QSignalSpy finishedSpy(&downloader, &QGCCachedFileDownload::finished);
    QVERIFY(downloader.download(url, 3600));
    QVERIFY_SIGNAL_WAIT(finishedSpy, TestTimeout::mediumMs());
    QVERIFY(finishedSpy.first().at(0).toBool());
    QFile::remove(finishedSpy.first().at(1).toString());
    server.close();
    QVERIFY(downloader.isCached(url));

    finishedSpy.clear();
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

    QSignalSpy finishedSpy(&downloader, &QGCCachedFileDownload::finished);
    QVERIFY(downloader.download(url, 3600));
    QVERIFY_SIGNAL_WAIT(finishedSpy, TestTimeout::mediumMs());
    QVERIFY(finishedSpy.first().at(0).toBool());
    QFile::remove(finishedSpy.first().at(1).toString());
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

    finishedSpy.clear();
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

    QSignalSpy finishedSpy(&downloader, &QGCCachedFileDownload::finished);
    QVERIFY(downloader.download(url, 3600));
    QVERIFY_SIGNAL_WAIT(finishedSpy, TestTimeout::mediumMs());
    QVERIFY(finishedSpy.first().at(0).toBool());
    server.close();

    const QString cachedPath = downloader.cachedPath(url);
    QVERIFY(!cachedPath.isEmpty());
    QVERIFY(QFile::exists(cachedPath));

    QFile file(cachedPath);
    QVERIFY(file.open(QIODevice::ReadOnly));
    QCOMPARE(file.readAll(), payload);
    file.close();
    QFile::remove(cachedPath);
}

void QGCCachedFileDownloadTest::_testCachedFileDownloadPreferCache()
{
    QTemporaryDir tempDir;
    QGCCachedFileDownload downloader(tempDir.path(), this);
    bool success = false;
    QString localPath;
    connect(&downloader, &QGCCachedFileDownload::finished, this,
            [&](bool s, const QString& path, const QString&, bool) {
                success = s;
                localPath = path;
            });
    QSignalSpy spy(&downloader, &QGCCachedFileDownload::finished);
    QSignalSpy urlSpy(&downloader, &QGCCachedFileDownload::urlChanged);
    QVERIFY(downloader.downloadPreferCache(":/unittest/arducopter.apj"));
    QCOMPARE(urlSpy.count(), 1);
    QCOMPARE(downloader.url(), QUrl::fromUserInput(":/unittest/arducopter.apj"));
    QVERIFY_SIGNAL_WAIT(spy, TestTimeout::mediumMs());
    QVERIFY(success);
    QVERIFY(!localPath.isEmpty());
    QVERIFY(QFile::exists(localPath));
}

void QGCCachedFileDownloadTest::_testCachedFileDownloadNoCache()
{
    QTemporaryDir tempDir;
    QGCCachedFileDownload downloader(tempDir.path(), this);
    bool success = false;
    QString localPath;
    connect(&downloader, &QGCCachedFileDownload::finished, this,
            [&](bool s, const QString& path, const QString&, bool) {
                success = s;
                localPath = path;
            });
    QSignalSpy spy(&downloader, &QGCCachedFileDownload::finished);
    QSignalSpy urlSpy(&downloader, &QGCCachedFileDownload::urlChanged);
    QVERIFY(downloader.downloadNoCache(":/unittest/arducopter.apj"));
    QCOMPARE(urlSpy.count(), 1);
    QCOMPARE(downloader.url(), QUrl::fromUserInput(":/unittest/arducopter.apj"));
    QVERIFY_SIGNAL_WAIT(spy, TestTimeout::mediumMs());
    QVERIFY(success);
    QVERIFY(!localPath.isEmpty());
    QVERIFY(QFile::exists(localPath));
}

void QGCCachedFileDownloadTest::_testCachedFileDownloadIsCached()
{
    QTemporaryDir tempDir;
    QGCCachedFileDownload downloader(tempDir.path(), this);
    const QString testUrl = ":/unittest/manifest.json.gz";
    // Initially not cached
    QVERIFY(!downloader.isCached(testUrl));
    QVERIFY(!downloader.isCached(testUrl, 3600));
    // Download to populate cache
    QSignalSpy spy(&downloader, &QGCCachedFileDownload::finished);
    QVERIFY(downloader.download(testUrl, 3600));
    QVERIFY_SIGNAL_WAIT(spy, TestTimeout::mediumMs());
    // Note: Qt resource URLs may not be cached by QNetworkDiskCache the same way
    // as HTTP URLs, so the cache behavior might differ. This test verifies the API works.
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

    QSignalSpy finishedSpy(&downloader, &QGCCachedFileDownload::finished);
    QVERIFY(downloader.download(url, 3600));
    QVERIFY_SIGNAL_WAIT(finishedSpy, TestTimeout::mediumMs());
    QVERIFY(finishedSpy.first().at(0).toBool());
    const QString downloadedPath = finishedSpy.first().at(1).toString();
    QVERIFY(!downloadedPath.isEmpty());
    QFile::remove(downloadedPath);

    QNetworkCacheMetaData metadata = downloader.diskCache()->metaData(QUrl::fromUserInput(url));
    QVERIFY(metadata.isValid());
    QNetworkCacheMetaData::AttributesMap attributes = metadata.attributes();
    attributes.remove(QNetworkRequest::Attribute::User);
    metadata.setAttributes(attributes);
    downloader.diskCache()->updateMetaData(metadata);

    QVERIFY(downloader.isCached(url));
    QVERIFY(!downloader.isCached(url, 3600));
}

void QGCCachedFileDownloadTest::_testCachedFileDownloadCacheAge()
{
    QTemporaryDir tempDir;
    QGCCachedFileDownload downloader(tempDir.path(), this);
    const QString testUrl = ":/unittest/manifest.json.gz";
    // Not cached returns -1
    QCOMPARE(downloader.cacheAge(testUrl), -1);
}

void QGCCachedFileDownloadTest::_testCachedFileDownloadClearCache()
{
    QTemporaryDir tempDir;
    QGCCachedFileDownload downloader(tempDir.path(), this);
    QSignalSpy spy(&downloader, &QGCCachedFileDownload::cacheSizeChanged);
    // Clear cache (should work even if empty)
    downloader.clearCache();
    // cacheSize should be 0 after clear
    QCOMPARE(downloader.cacheSize(), static_cast<qint64>(0));
}

void QGCCachedFileDownloadTest::_testCachedFileDownloadRemoveFromCache()
{
    QTemporaryDir tempDir;
    QGCCachedFileDownload downloader(tempDir.path(), this);
    const QString testUrl = ":/unittest/manifest.json.gz";
    // Removing non-existent URL should return false
    QVERIFY(!downloader.removeFromCache(testUrl));
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

void QGCCachedFileDownloadTest::_testCachedFileDownloadSignals()
{
    QTemporaryDir tempDir;
    QGCCachedFileDownload downloader(tempDir.path(), this);
    QSignalSpy progressSpy(&downloader, &QGCCachedFileDownload::progressChanged);
    QSignalSpy runningSpy(&downloader, &QGCCachedFileDownload::runningChanged);
    QSignalSpy urlSpy(&downloader, &QGCCachedFileDownload::urlChanged);
    QSignalSpy finishedSpy(&downloader, &QGCCachedFileDownload::finished);
    QVERIFY(downloader.download(":/unittest/manifest.json.gz", 3600));
    // Should emit running=true and url changed
    QVERIFY_SIGNAL_COUNT_WAIT(runningSpy, 1, TestTimeout::mediumMs());
    QVERIFY_SIGNAL_COUNT_WAIT(urlSpy, 1, TestTimeout::mediumMs());
    QVERIFY_SIGNAL_WAIT(finishedSpy, TestTimeout::mediumMs());
    // finished signal should have 4 arguments: success, localPath, errorMessage, fromCache
    QCOMPARE(finishedSpy.count(), 1);
    QCOMPARE(finishedSpy.first().count(), 4);
    QVERIFY(finishedSpy.first().at(0).toBool());  // success = true
    // running should be false at end
    QVERIFY(!downloader.isRunning());
}

UT_REGISTER_TEST(QGCCachedFileDownloadTest, TestLabel::Integration, TestLabel::Network, TestLabel::Utilities)
