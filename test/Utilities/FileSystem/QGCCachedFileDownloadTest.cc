#include "QGCCachedFileDownloadTest.h"

#include <QtCore/QFile>
#include <QtNetwork/QHostAddress>
#include <QtNetwork/QNetworkCacheMetaData>
#include <QtNetwork/QNetworkDiskCache>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>
#include <QtTest/QSignalSpy>

#include "QGCCachedFileDownload.h"
#include "QGCFileDownload.h"

namespace {
bool listenOnLocalTestAddress(QTcpServer& server)
{
    return server.listen(QHostAddress::LocalHost, 0) || server.listen(QHostAddress::LocalHostIPv6, 0) ||
           server.listen(QHostAddress::Any, 0);
}
}  // namespace

// ============================================================================
// QGCCachedFileDownload Tests
// ============================================================================
void QGCCachedFileDownloadTest::_testCachedFileDownloadConstruction()
{
    // Test construction with cache directory
    QGCCachedFileDownload downloader1(tempDirPath(), this);
    // QNetworkDiskCache may append trailing slash
    QVERIFY(downloader1.cacheDirectory().startsWith(tempDirPath()));
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
    const QString cacheDir1 = createSubDir(QStringLiteral("cache-dir-1"));
    const QString cacheDir2 = createSubDir(QStringLiteral("cache-dir-2"));
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
    QGCCachedFileDownload downloader(tempDirPath(), this);
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
    QGCCachedFileDownload downloader(tempDirPath(), this);
    // Empty URL should fail
    QVERIFY(!downloader.download("", 3600));
    QVERIFY(!downloader.errorString().isEmpty());
}

void QGCCachedFileDownloadTest::_testCachedFileDownloadNoCacheDir()
{
    QGCCachedFileDownload downloader(this);
    // No cache directory should fail
    QVERIFY(!downloader.download(":/unittest/manifest.json.gz", 3600));
    QVERIFY(!downloader.errorString().isEmpty());
}

void QGCCachedFileDownloadTest::_testCachedFileDownloadDownload()
{
    QGCCachedFileDownload downloader(tempDirPath(), this);
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
    QGCCachedFileDownload downloader(tempDirPath(), this);

    const QByteArray payload("cached-data");
    QTcpServer server;
    if (!listenOnLocalTestAddress(server)) {
        const QString reason = QStringLiteral("Could not start local test HTTP server: %1").arg(server.errorString());
        QSKIP(qPrintable(reason));
    }
    const QString url = QStringLiteral("http://127.0.0.1:%1/synthetic-cache-hit.txt").arg(server.serverPort());

    (void)connect(&server, &QTcpServer::newConnection, &server, [&server, payload]() {
        while (server.hasPendingConnections()) {
            QTcpSocket* const socket = server.nextPendingConnection();
            (void)connect(socket, &QTcpSocket::readyRead, socket, [socket, payload]() {
                (void)socket->readAll();
                const QByteArray response = QByteArrayLiteral(
                                                "HTTP/1.1 200 OK\r\n"
                                                "Content-Type: text/plain\r\n"
                                                "Cache-Control: max-age=3600\r\n"
                                                "Connection: close\r\n"
                                                "Content-Length: ") +
                                            QByteArray::number(payload.size()) + QByteArrayLiteral("\r\n\r\n") +
                                            payload;
                (void)socket->write(response);
                (void)socket->flush();
                socket->disconnectFromHost();
            });
            (void)connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
        }
    });

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
    QGCCachedFileDownload downloader(tempDirPath(), this);

    const QByteArray payload("cached-path-data");
    QTcpServer server;
    if (!listenOnLocalTestAddress(server)) {
        const QString reason = QStringLiteral("Could not start local test HTTP server: %1").arg(server.errorString());
        QSKIP(qPrintable(reason));
    }
    const QString url = QStringLiteral("http://127.0.0.1:%1/cached-path.txt").arg(server.serverPort());
    QVERIFY(downloader.cachedPath(url).isEmpty());

    (void)connect(&server, &QTcpServer::newConnection, &server, [&server, payload]() {
        while (server.hasPendingConnections()) {
            QTcpSocket* const socket = server.nextPendingConnection();
            (void)connect(socket, &QTcpSocket::readyRead, socket, [socket, payload]() {
                (void)socket->readAll();
                const QByteArray response = QByteArrayLiteral(
                                                "HTTP/1.1 200 OK\r\n"
                                                "Content-Type: text/plain\r\n"
                                                "Cache-Control: max-age=3600\r\n"
                                                "Connection: close\r\n"
                                                "Content-Length: ") +
                                            QByteArray::number(payload.size()) + QByteArrayLiteral("\r\n\r\n") +
                                            payload;
                (void)socket->write(response);
                (void)socket->flush();
                socket->disconnectFromHost();
            });
            (void)connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
        }
    });

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
    QGCCachedFileDownload downloader(tempDirPath(), this);
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
    QGCCachedFileDownload downloader(tempDirPath(), this);
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
    QGCCachedFileDownload downloader(tempDirPath(), this);
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
    QGCCachedFileDownload downloader(tempDirPath(), this);

    const QByteArray payload("cached-invalid-timestamp");
    QTcpServer server;
    if (!listenOnLocalTestAddress(server)) {
        const QString reason = QStringLiteral("Could not start local test HTTP server: %1").arg(server.errorString());
        QSKIP(qPrintable(reason));
    }
    const QString url = QStringLiteral("http://127.0.0.1:%1/cached-invalid-timestamp.txt").arg(server.serverPort());

    (void)connect(&server, &QTcpServer::newConnection, &server, [&server, payload]() {
        while (server.hasPendingConnections()) {
            QTcpSocket* const socket = server.nextPendingConnection();
            (void)connect(socket, &QTcpSocket::readyRead, socket, [socket, payload]() {
                (void)socket->readAll();
                const QByteArray response = QByteArrayLiteral(
                                                "HTTP/1.1 200 OK\r\n"
                                                "Content-Type: text/plain\r\n"
                                                "Cache-Control: max-age=3600\r\n"
                                                "Connection: close\r\n"
                                                "Content-Length: ") +
                                            QByteArray::number(payload.size()) + QByteArrayLiteral("\r\n\r\n") +
                                            payload;
                (void)socket->write(response);
                (void)socket->flush();
                socket->disconnectFromHost();
            });
            (void)connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
        }
    });

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
    QGCCachedFileDownload downloader(tempDirPath(), this);
    const QString testUrl = ":/unittest/manifest.json.gz";
    // Not cached returns -1
    QCOMPARE(downloader.cacheAge(testUrl), -1);
}

void QGCCachedFileDownloadTest::_testCachedFileDownloadClearCache()
{
    QGCCachedFileDownload downloader(tempDirPath(), this);
    QSignalSpy spy(&downloader, &QGCCachedFileDownload::cacheSizeChanged);
    // Clear cache (should work even if empty)
    downloader.clearCache();
    // cacheSize should be 0 after clear
    QCOMPARE(downloader.cacheSize(), static_cast<qint64>(0));
}

void QGCCachedFileDownloadTest::_testCachedFileDownloadRemoveFromCache()
{
    QGCCachedFileDownload downloader(tempDirPath(), this);
    const QString testUrl = ":/unittest/manifest.json.gz";
    // Removing non-existent URL should return false
    QVERIFY(!downloader.removeFromCache(testUrl));
}

void QGCCachedFileDownloadTest::_testCachedFileDownloadCancel()
{
    QGCCachedFileDownload downloader(tempDirPath(), this);
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
    QGCCachedFileDownload downloader(tempDirPath(), this);
    // Start first download
    QVERIFY(downloader.download(":/unittest/manifest.json.gz", 3600));
    // Second download should be rejected while first is running
    QVERIFY(!downloader.download(":/unittest/arducopter.apj", 3600));
    QVERIFY(!downloader.downloadPreferCache(":/unittest/arducopter.apj"));
    QVERIFY(!downloader.downloadNoCache(":/unittest/arducopter.apj"));
    // Wait for first download to complete
    QSignalSpy spy(&downloader, &QGCCachedFileDownload::finished);
    QVERIFY_SIGNAL_WAIT(spy, TestTimeout::mediumMs());
}

void QGCCachedFileDownloadTest::_testCachedFileDownloadRejectedStartNoFinishedSignal()
{
    QGCCachedFileDownload downloader(tempDirPath(), this);
    QSignalSpy finishedSpy(&downloader, &QGCCachedFileDownload::finished);

    QVERIFY(!downloader.download(QString(), 3600));
    QCOMPARE(finishedSpy.count(), 0);
    QVERIFY(!downloader.errorString().isEmpty());
}

void QGCCachedFileDownloadTest::_testCachedFileDownloadReentrantStartFromCompletion()
{
    QGCCachedFileDownload downloader(tempDirPath(), this);

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
    QGCCachedFileDownload downloader(tempDirPath(), this);
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
