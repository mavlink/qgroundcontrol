#include "QGCCachedFileDownloadTest.h"

#include <QtCore/QDateTime>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QPointer>
#include <QtCore/QRegularExpression>
#include <QtCore/QTemporaryDir>
#include <QtNetwork/QNetworkCacheMetaData>
#include <QtNetwork/QNetworkDiskCache>
#include <QtNetwork/QNetworkRequest>
#include <QtTest/QSignalSpy>

#include "LocalHttpTestServer.h"
#include "QGCCachedFileDownload.h"
#include "QGCFileDownload.h"

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

void QGCCachedFileDownloadTest::_testCachedFileDownloadEmptyUrl()
{
    QTemporaryDir tempDir;
    QGCCachedFileDownload downloader(tempDir.path(), this);
    // Empty URL should fail
    expectLogMessage("Utilities.QGCCachedFileDownload", QtWarningMsg, QRegularExpression("Empty URL"));
    QVERIFY(!downloader.download("", 3600));
    verifyExpectedLogMessage();
    QVERIFY(!downloader.errorString().isEmpty());
}

void QGCCachedFileDownloadTest::_testCachedFileDownloadNoCacheDir()
{
    QGCCachedFileDownload downloader(this);
    // No cache directory should fail
    expectLogMessage("Utilities.QGCCachedFileDownload", QtWarningMsg, QRegularExpression("Cache directory not set"));
    QVERIFY(!downloader.download(":/unittest/manifest.json.gz", 3600));
    verifyExpectedLogMessage();
    QVERIFY(!downloader.errorString().isEmpty());
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

void QGCCachedFileDownloadTest::_testCachedFileDownloadMaximumSize()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    QGCCachedFileDownload downloader(tempDir.path(), this);
    QSignalSpy finishedSpy(&downloader, &QGCCachedFileDownload::finished);

    expectLogMessage("Utilities.QGCFileDownload", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Download exceeds maximum size")));
    QVERIFY(downloader.download(QStringLiteral(":/unittest/manifest.json.gz"), 0, 1));
    if (finishedSpy.isEmpty()) {
        QVERIFY_SIGNAL_WAIT(finishedSpy, TestTimeout::mediumMs());
    }
    verifyExpectedLogMessage();
    QCOMPARE(finishedSpy.size(), 1);
    QVERIFY(!finishedSpy.first().at(0).toBool());
    QVERIFY(finishedSpy.first().at(2).toString().contains(QStringLiteral("maximum size")));

    finishedSpy.clear();
    downloader.fileDownloader()->setAutoDecompress(true);
    downloader.fileDownloader()->setOutputPath(tempDir.filePath(QStringLiteral("limited.json.gz")));
    expectLogMessage("Utilities.QGClibarchive", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Size limit exceeded")));
    expectLogMessage("Utilities.QGCFileDownload", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Decompression failed")));
    QVERIFY(downloader.downloadPreferCache(QStringLiteral(":/unittest/manifest.json.gz"), 0, 1));
    QVERIFY_SIGNAL_WAIT(finishedSpy, TestTimeout::mediumMs());
    verifyExpectedLogMessage();
    verifyExpectedLogMessage();
    QCOMPARE(finishedSpy.size(), 1);
    QVERIFY(!finishedSpy.first().at(0).toBool());
    QVERIFY(finishedSpy.first().at(2).toString().contains(QStringLiteral("maximum size")));
    QVERIFY(!QFileInfo::exists(tempDir.filePath(QStringLiteral("limited.json"))));
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
    QVERIFY(downloader.downloadPreferCache(":/unittest/arducopter.apj"));
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
    QVERIFY(downloader.downloadNoCache(":/unittest/arducopter.apj"));
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
    ignoreLogMessage("Utilities.QGCCachedFileDownload", QtWarningMsg,
                     QRegularExpression("Download already in progress"));
    // Second download should be rejected while first is running
    QVERIFY(!downloader.download(":/unittest/arducopter.apj", 3600));
    QVERIFY(!downloader.downloadPreferCache(":/unittest/arducopter.apj"));
    QVERIFY(!downloader.downloadNoCache(":/unittest/arducopter.apj"));
    // Wait for first download to complete
    QSignalSpy spy(&downloader, &QGCCachedFileDownload::finished);
    QVERIFY_SIGNAL_WAIT(spy, TestTimeout::mediumMs());
}

void QGCCachedFileDownloadTest::_testCachedFileDownloadStartupReentrancyRejected()
{
    QTemporaryDir tempDir;
    QGCCachedFileDownload downloader(tempDir.path(), this);
    QSignalSpy finishedSpy(&downloader, &QGCCachedFileDownload::finished);
    bool nestedStartAttempted = false;
    bool nestedStartSucceeded = true;
    connect(&downloader, &QGCCachedFileDownload::urlChanged, this, [&](const QUrl&) {
        if (!nestedStartAttempted) {
            nestedStartAttempted = true;
            nestedStartSucceeded = downloader.download(QStringLiteral(":/unittest/arducopter.apj"), 3600);
        }
    });

    expectLogMessage("Utilities.QGCCachedFileDownload", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Download already in progress")));
    QVERIFY(downloader.download(QStringLiteral(":/unittest/manifest.json.gz"), 3600));
    QVERIFY_SIGNAL_WAIT(finishedSpy, TestTimeout::mediumMs());
    verifyExpectedLogMessage();

    QVERIFY(nestedStartAttempted);
    QVERIFY(!nestedStartSucceeded);
    QCOMPARE(finishedSpy.size(), 1);
    QVERIFY(finishedSpy.first().at(0).toBool());
}

void QGCCachedFileDownloadTest::_testCachedFileDownloadRejectedStartNoFinishedSignal()
{
    QTemporaryDir tempDir;
    QGCCachedFileDownload downloader(tempDir.path(), this);
    QSignalSpy finishedSpy(&downloader, &QGCCachedFileDownload::finished);

    expectLogMessage("Utilities.QGCCachedFileDownload", QtWarningMsg, QRegularExpression("Empty URL"));
    QVERIFY(!downloader.download(QString(), 3600));
    verifyExpectedLogMessage();
    QCOMPARE(finishedSpy.count(), 0);
    QVERIFY(!downloader.errorString().isEmpty());
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

void QGCCachedFileDownloadTest::_testCachedFileDownloadDeferredStartFailureCompletes()
{
    QTemporaryDir tempDir;
    QGCCachedFileDownload downloader(tempDir.path(), this);
    QSignalSpy finishedSpy(&downloader, &QGCCachedFileDownload::finished);
    bool deferredStartAccepted = false;
    connect(&downloader, &QGCCachedFileDownload::finished, this,
            [&](bool success, const QString&, const QString&, bool) {
                if (success && (finishedSpy.size() == 1)) {
                    deferredStartAccepted = downloader.download(QString(), 3600);
                }
            });

    expectLogMessage("Utilities.QGCCachedFileDownload", QtWarningMsg, QRegularExpression(QStringLiteral("Empty URL")));
    QVERIFY(downloader.download(QStringLiteral(":/unittest/manifest.json.gz"), 3600));
    QVERIFY_SIGNAL_COUNT_WAIT(finishedSpy, 2, TestTimeout::mediumMs());
    verifyExpectedLogMessage();

    QVERIFY(deferredStartAccepted);
    QVERIFY(finishedSpy.at(0).at(0).toBool());
    QVERIFY(!finishedSpy.at(1).at(0).toBool());
    QVERIFY(finishedSpy.at(1).at(2).toString().contains(QStringLiteral("Empty URL")));
    QVERIFY(!downloader.isRunning());
}

void QGCCachedFileDownloadTest::_testCachedFileDownloadDirectConnectionDeletion()
{
    {
        QTemporaryDir tempDir;
        auto* const downloader = new QGCCachedFileDownload(tempDir.path());
        QPointer<QGCFileDownload> ownedFileDownload = downloader->findChild<QGCFileDownload*>();
        QVERIFY(ownedFileDownload);
        delete downloader;
        QVERIFY(!ownedFileDownload);
    }

    {
        QTemporaryDir tempDir;
        QPointer<QGCCachedFileDownload> downloader = new QGCCachedFileDownload(tempDir.path());
        connect(downloader, &QGCCachedFileDownload::urlChanged, downloader,
                [downloader](const QUrl&) { delete downloader.data(); });
        QVERIFY(!downloader->download(QStringLiteral(":/unittest/manifest.json.gz"), 3600));
        QVERIFY(!downloader);
    }

    {
        QTemporaryDir tempDir;
        QPointer<QGCCachedFileDownload> downloader = new QGCCachedFileDownload(tempDir.path());
        QSignalSpy finishedSpy(downloader, &QGCCachedFileDownload::finished);
        connect(downloader, &QGCCachedFileDownload::finished, downloader,
                [downloader](bool, const QString&, const QString&, bool) { delete downloader.data(); });
        QVERIFY(downloader->download(QStringLiteral(":/unittest/manifest.json.gz"), 3600));
        QVERIFY_SIGNAL_WAIT(finishedSpy, TestTimeout::mediumMs());
        QVERIFY(!downloader);
        QCOMPARE(finishedSpy.size(), 1);
    }

    {
        QTemporaryDir tempDir;
        QPointer<QGCCachedFileDownload> downloader = new QGCCachedFileDownload(tempDir.path());
        QPointer<QGCFileDownload> fileDownloader = downloader->fileDownloader();
        connect(downloader, &QGCCachedFileDownload::progressChanged, downloader,
                [downloader](qreal) { delete downloader.data(); }, Qt::DirectConnection);

        fileDownloader->downloadProgress(1, 2);

        QVERIFY(!downloader);
        QVERIFY(!fileDownloader);
    }
}

void QGCCachedFileDownloadTest::_testCachedFileDownloadCancelFromRunningChanged()
{
    QTemporaryDir tempDir;
    QGCCachedFileDownload downloader(tempDir.path(), this);
    QSignalSpy finishedSpy(&downloader, &QGCCachedFileDownload::finished);
    bool cancelRequested = false;
    connect(&downloader, &QGCCachedFileDownload::runningChanged, this, [&](bool running) {
        if (running && !cancelRequested) {
            cancelRequested = true;
            downloader.cancel();
        }
    });

    QVERIFY(downloader.download(QStringLiteral(":/unittest/manifest.json.gz"), 3600));
    QCOMPARE(finishedSpy.size(), 1);
    QVERIFY(cancelRequested);
    QVERIFY(!finishedSpy.first().at(0).toBool());
    QVERIFY(finishedSpy.first().at(2).toString().contains(QStringLiteral("cancelled"), Qt::CaseInsensitive));
    QVERIFY(!downloader.isRunning());
}

void QGCCachedFileDownloadTest::_testCachedFileDownloadReentrantStartFromRunningChanged()
{
    QTemporaryDir tempDir;
    QGCCachedFileDownload downloader(tempDir.path(), this);
    QSignalSpy finishedSpy(&downloader, &QGCCachedFileDownload::finished);
    bool restartAttempted = false;
    bool restartAccepted = false;
    connect(&downloader, &QGCCachedFileDownload::runningChanged, this, [&](bool running) {
        if (!running && !restartAttempted) {
            restartAttempted = true;
            restartAccepted = downloader.download(QStringLiteral(":/unittest/arducopter.apj"), 3600);
        }
    });

    QVERIFY(downloader.download(QStringLiteral(":/unittest/manifest.json.gz"), 3600));
    QVERIFY_SIGNAL_COUNT_WAIT(finishedSpy, 2, TestTimeout::mediumMs());

    QVERIFY(restartAttempted);
    QVERIFY(restartAccepted);
    QCOMPARE(finishedSpy.size(), 2);
    QVERIFY(finishedSpy.at(0).at(0).toBool());
    QCOMPARE(QFileInfo(finishedSpy.at(0).at(1).toString()).fileName(), QStringLiteral("manifest.json.gz"));
    QVERIFY(finishedSpy.at(1).at(0).toBool());
    QCOMPARE(QFileInfo(finishedSpy.at(1).at(1).toString()).fileName(), QStringLiteral("arducopter.apj"));
    QVERIFY(!downloader.isRunning());
}

void QGCCachedFileDownloadTest::_testCachedFileDownloadFallbackStartError()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    QGCCachedFileDownload downloader(tempDir.filePath(QStringLiteral("cache")), this);

    const QByteArray payload("cached-fallback-data");
    TestFixtures::LocalHttpTestServer server;
    QVERIFY2(server.listen(), "Could not start local test HTTP server");
    const QString url = server.url(QStringLiteral("/fallback-start-error.txt"));
    server.installHttpResponder(payload, 200, "text/plain", 3600);

    QSignalSpy populateSpy(&downloader, &QGCCachedFileDownload::finished);
    QVERIFY(downloader.download(url, 3600));
    QVERIFY_SIGNAL_WAIT(populateSpy, TestTimeout::mediumMs());
    QVERIFY(populateSpy.first().at(0).toBool());
    QFile::remove(populateSpy.first().at(1).toString());

    QNetworkCacheMetaData metadata = downloader.diskCache()->metaData(QUrl::fromUserInput(url));
    QVERIFY(metadata.isValid());
    QNetworkCacheMetaData::AttributesMap attributes = metadata.attributes();
    attributes.insert(QNetworkRequest::Attribute::User, QDateTime::currentDateTimeUtc().addSecs(-3600));
    metadata.setAttributes(attributes);
    downloader.diskCache()->updateMetaData(metadata);
    server.close();

    bool fallbackFailurePrepared = false;
    connect(downloader.fileDownloader(), &QGCFileDownload::errorStringChanged, this, [&](const QString& error) {
        if (!error.isEmpty() && !fallbackFailurePrepared) {
            fallbackFailurePrepared = true;
            downloader.fileDownloader()->setOutputPath(tempDir.path());
        }
    });

    QSignalSpy finishedSpy(&downloader, &QGCCachedFileDownload::finished);
    expectLogMessage("Utilities.QGCFileDownload", QtWarningMsg, QRegularExpression(QStringLiteral("Download error")));
    QVERIFY(downloader.download(url, 1));
    QVERIFY_SIGNAL_WAIT(finishedSpy, TestTimeout::mediumMs());
    verifyExpectedLogMessage();

    QVERIFY(fallbackFailurePrepared);
    QCOMPARE(finishedSpy.size(), 1);
    QVERIFY(!finishedSpy.first().at(0).toBool());
    const QString error = finishedSpy.first().at(2).toString();
    QVERIFY(error.contains(QStringLiteral("cache fallback failed")));
    QVERIFY(error.contains(QStringLiteral("Cannot open output file")));
}

void QGCCachedFileDownloadTest::_testCachedFileDownloadFallbackCancellationAndDeletion()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    QGCCachedFileDownload cancelDownloader(tempDir.filePath(QStringLiteral("cancel-cache")), this);
    QPointer<QGCCachedFileDownload> deleteDownloader =
        new QGCCachedFileDownload(tempDir.filePath(QStringLiteral("delete-cache")));

    const QByteArray payload("cached-fallback-data");
    TestFixtures::LocalHttpTestServer server;
    QVERIFY2(server.listen(), "Could not start local test HTTP server");
    const QString url = server.url(QStringLiteral("/fallback-lifecycle.txt"));
    server.installHttpResponder(payload, 200, "text/plain", 3600);

    QSignalSpy cancelPopulateSpy(&cancelDownloader, &QGCCachedFileDownload::finished);
    QVERIFY(cancelDownloader.download(url, 3600));
    QVERIFY_SIGNAL_WAIT(cancelPopulateSpy, TestTimeout::mediumMs());
    QVERIFY(cancelPopulateSpy.first().at(0).toBool());
    QFile::remove(cancelPopulateSpy.first().at(1).toString());

    QSignalSpy deletePopulateSpy(deleteDownloader, &QGCCachedFileDownload::finished);
    QVERIFY(deleteDownloader->download(url, 3600));
    QVERIFY_SIGNAL_WAIT(deletePopulateSpy, TestTimeout::mediumMs());
    QVERIFY(deletePopulateSpy.first().at(0).toBool());
    QFile::remove(deletePopulateSpy.first().at(1).toString());

    const auto expireCacheEntry = [&url](QGCCachedFileDownload* downloader) {
        QNetworkCacheMetaData metadata = downloader->diskCache()->metaData(QUrl::fromUserInput(url));
        if (!metadata.isValid()) {
            return false;
        }
        QNetworkCacheMetaData::AttributesMap attributes = metadata.attributes();
        attributes.insert(QNetworkRequest::Attribute::User, QDateTime::currentDateTimeUtc().addSecs(-3600));
        metadata.setAttributes(attributes);
        downloader->diskCache()->updateMetaData(metadata);
        return true;
    };
    QVERIFY(expireCacheEntry(&cancelDownloader));
    QVERIFY(expireCacheEntry(deleteDownloader));
    server.close();

    bool cancelFallbackPrepared = false;
    connect(cancelDownloader.fileDownloader(), &QGCFileDownload::errorStringChanged, this, [&](const QString& error) {
        if (!error.isEmpty() && !cancelFallbackPrepared) {
            cancelFallbackPrepared = true;
            cancelDownloader.fileDownloader()->setOutputPath(tempDir.path());
        }
    });
    bool canceledFromFallback = false;
    connect(&cancelDownloader, &QGCCachedFileDownload::errorStringChanged, this, [&](const QString& error) {
        if (cancelFallbackPrepared && !error.isEmpty() && !canceledFromFallback) {
            canceledFromFallback = true;
            cancelDownloader.cancel();
        }
    });
    QSignalSpy cancelFinishedSpy(&cancelDownloader, &QGCCachedFileDownload::finished);
    expectLogMessage("Utilities.QGCFileDownload", QtWarningMsg, QRegularExpression(QStringLiteral("Download error")));
    QVERIFY(cancelDownloader.download(url, 1));
    QVERIFY_SIGNAL_WAIT(cancelFinishedSpy, TestTimeout::mediumMs());
    verifyExpectedLogMessage();
    QVERIFY(cancelFallbackPrepared);
    QVERIFY(canceledFromFallback);
    QCOMPARE(cancelFinishedSpy.size(), 1);
    QVERIFY(!cancelFinishedSpy.first().at(0).toBool());
    QVERIFY(cancelFinishedSpy.first().at(2).toString().contains(QStringLiteral("cancel"), Qt::CaseInsensitive));
    QVERIFY(!cancelDownloader.isRunning());

    bool deleteFallbackPrepared = false;
    connect(deleteDownloader->fileDownloader(), &QGCFileDownload::errorStringChanged, this,
            [deleteDownloader, &deleteFallbackPrepared, &tempDir](const QString& error) {
                if (deleteDownloader && !error.isEmpty() && !deleteFallbackPrepared) {
                    deleteFallbackPrepared = true;
                    deleteDownloader->fileDownloader()->setOutputPath(tempDir.path());
                }
            });
    bool deletedFromFallback = false;
    connect(
        deleteDownloader, &QGCCachedFileDownload::errorStringChanged, this,
        [deleteDownloader, &deleteFallbackPrepared, &deletedFromFallback](const QString& error) {
            if (deleteDownloader && deleteFallbackPrepared && !error.isEmpty() && !deletedFromFallback) {
                deletedFromFallback = true;
                delete deleteDownloader.data();
            }
        },
        Qt::DirectConnection);
    expectLogMessage("Utilities.QGCFileDownload", QtWarningMsg, QRegularExpression(QStringLiteral("Download error")));
    QVERIFY(deleteDownloader->download(url, 1));
    QTRY_VERIFY_WITH_TIMEOUT(!deleteDownloader, TestTimeout::mediumMs());
    verifyExpectedLogMessage();
    QVERIFY(deleteFallbackPrepared);
    QVERIFY(deletedFromFallback);
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
