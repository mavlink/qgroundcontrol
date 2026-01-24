#include "QGCFileDownloadTest.h"
#include "QGCCachedFileDownload.h"
#include "QGCCompression.h"
#include "QGCFileDownload.h"

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtTest/QTest>
#include <QtTest/QSignalSpy>

// ============================================================================
// Setup and Helpers
// ============================================================================

QGCCachedFileDownload* QGCFileDownloadTest::_createCachedDownloader()
{
    return new QGCCachedFileDownload(tempPath(), this);
}

// ============================================================================
// QGCFileDownload Tests
// ============================================================================

void QGCFileDownloadTest::_testFileDownload()
{
    // Local File
    QGCFileDownload *const downloader = new QGCFileDownload(this);
    (void)connect(downloader, &QGCFileDownload::downloadComplete, this,
        [this](const QString &remoteFile, const QString &localFile, const QString &errorMsg) {
            sender()->deleteLater();
            QVERIFY(errorMsg.isEmpty());
        });
    QSignalSpy spyQGCFileDownloadDownloadComplete(downloader, &QGCFileDownload::downloadComplete);
    QVERIFY(downloader->download(":/unittest/arducopter.apj"));
    QVERIFY(spyQGCFileDownloadDownloadComplete.wait(TestHelpers::kShortTimeoutMs));
}

// ============================================================================
// QGCCachedFileDownload Tests
// ============================================================================

void QGCFileDownloadTest::_testCachedFileDownloadConstruction()
{
    // Test construction with cache directory
    QGCCachedFileDownload* downloader1 = _createCachedDownloader();
    // QNetworkDiskCache may append trailing slash
    QVERIFY(downloader1->cacheDirectory().startsWith(tempPath()));
    QVERIFY(!downloader1->isRunning());
    QCOMPARE(downloader1->progress(), 0.0);
    QGC_VERIFY_EMPTY(downloader1->errorString());
    QVERIFY(!downloader1->isFromCache());
    QGC_VERIFY_EMPTY(downloader1->localPath());

    // Test construction without cache directory
    QGCCachedFileDownload downloader2(this);
    QGC_VERIFY_EMPTY(downloader2.cacheDirectory());
}

void QGCFileDownloadTest::_testCachedFileDownloadCacheDirectory()
{
    QTemporaryDir tempDir2;
    QVERIFY(tempDir2.isValid());

    QGCCachedFileDownload* downloader = _createCachedDownloader();
    // QNetworkDiskCache may append trailing slash
    QVERIFY(downloader->cacheDirectory().startsWith(tempPath()));

    QSignalSpy spy(downloader, &QGCCachedFileDownload::cacheDirectoryChanged);
    QGC_VERIFY_SPY_VALID(spy);

    downloader->setCacheDirectory(tempDir2.path());
    QVERIFY(downloader->cacheDirectory().startsWith(tempDir2.path()));
    QCOMPARE(spy.count(), 1);
    QVERIFY(spy.first().first().toString().startsWith(tempDir2.path()));

    // Setting same directory should not emit signal
    downloader->setCacheDirectory(downloader->cacheDirectory());  // Use actual value to avoid trailing slash mismatch
    QCOMPARE(spy.count(), 1);
}

void QGCFileDownloadTest::_testCachedFileDownloadMaxCacheSize()
{
    QGCCachedFileDownload* downloader = _createCachedDownloader();

    QSignalSpy spy(downloader, &QGCCachedFileDownload::maxCacheSizeChanged);

    constexpr qint64 newSize = 100 * 1024 * 1024;  // 100 MB
    downloader->setMaxCacheSize(newSize);
    QCOMPARE(downloader->maxCacheSize(), newSize);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.first().first().toLongLong(), newSize);

    // Setting same size should not emit signal
    downloader->setMaxCacheSize(newSize);
    QCOMPARE(spy.count(), 1);
}

void QGCFileDownloadTest::_testCachedFileDownloadEmptyUrl()
{
    QGCCachedFileDownload* downloader = _createCachedDownloader();

    // Empty URL should fail
    QVERIFY(!downloader->download("", 3600));
    QGC_VERIFY_NOT_EMPTY(downloader->errorString());
}

void QGCFileDownloadTest::_testCachedFileDownloadNoCacheDir()
{
    QGCCachedFileDownload downloader(this);

    // No cache directory should fail
    QVERIFY(!downloader.download(":/unittest/manifest.json.gz", 3600));
    QGC_VERIFY_NOT_EMPTY(downloader.errorString());
}

void QGCFileDownloadTest::_testCachedFileDownloadDownload()
{
    QGCCachedFileDownload* downloader = _createCachedDownloader();

    bool finished = false;
    bool success = false;
    QString localPath;
    bool fromCache = false;

    connect(downloader, &QGCCachedFileDownload::finished, this,
            [&](bool s, const QString &path, const QString &, bool cached) {
                finished = true;
                success = s;
                localPath = path;
                fromCache = cached;
            });

    QSignalSpy spy(downloader, &QGCCachedFileDownload::finished);
    QGC_VERIFY_SPY_VALID(spy);

    // Download with 1 hour cache
    QVERIFY(downloader->download(":/unittest/manifest.json.gz", 3600));
    QVERIFY(downloader->isRunning());

    QVERIFY(spy.wait(TestHelpers::kDefaultTimeoutMs));
    QVERIFY(finished);
    QVERIFY(success);
    QGC_VERIFY_NOT_EMPTY(localPath);
    VERIFY_FILE_EXISTS(localPath);
    QVERIFY(!downloader->isRunning());
}

void QGCFileDownloadTest::_testCachedFileDownloadPreferCache()
{
    QGCCachedFileDownload* downloader = _createCachedDownloader();

    bool success = false;
    QString localPath;

    connect(downloader, &QGCCachedFileDownload::finished, this,
            [&](bool s, const QString &path, const QString &, bool) {
                success = s;
                localPath = path;
            });

    QSignalSpy spy(downloader, &QGCCachedFileDownload::finished);
    QGC_VERIFY_SPY_VALID(spy);

    QVERIFY(downloader->downloadPreferCache(":/unittest/arducopter.apj"));
    QVERIFY(spy.wait(TestHelpers::kDefaultTimeoutMs));

    QVERIFY(success);
    QGC_VERIFY_NOT_EMPTY(localPath);
    VERIFY_FILE_EXISTS(localPath);
}

void QGCFileDownloadTest::_testCachedFileDownloadNoCache()
{
    QGCCachedFileDownload* downloader = _createCachedDownloader();

    bool success = false;
    QString localPath;

    connect(downloader, &QGCCachedFileDownload::finished, this,
            [&](bool s, const QString &path, const QString &, bool) {
                success = s;
                localPath = path;
            });

    QSignalSpy spy(downloader, &QGCCachedFileDownload::finished);
    QGC_VERIFY_SPY_VALID(spy);

    QVERIFY(downloader->downloadNoCache(":/unittest/arducopter.apj"));
    QVERIFY(spy.wait(TestHelpers::kDefaultTimeoutMs));

    QVERIFY(success);
    QGC_VERIFY_NOT_EMPTY(localPath);
    VERIFY_FILE_EXISTS(localPath);
}

void QGCFileDownloadTest::_testCachedFileDownloadIsCached()
{
    QGCCachedFileDownload* downloader = _createCachedDownloader();
    const QString testUrl = ":/unittest/manifest.json.gz";

    // Initially not cached
    QVERIFY(!downloader->isCached(testUrl));
    QVERIFY(!downloader->isCached(testUrl, 3600));

    // Download to populate cache
    QSignalSpy spy(downloader, &QGCCachedFileDownload::finished);
    QVERIFY(downloader->download(testUrl, 3600));
    QVERIFY(spy.wait(TestHelpers::kDefaultTimeoutMs));

    // Note: Qt resource URLs may not be cached by QNetworkDiskCache the same way
    // as HTTP URLs, so the cache behavior might differ. This test verifies the API works.
}

void QGCFileDownloadTest::_testCachedFileDownloadCacheAge()
{
    QGCCachedFileDownload* downloader = _createCachedDownloader();
    const QString testUrl = ":/unittest/manifest.json.gz";

    // Not cached returns -1
    QCOMPARE(downloader->cacheAge(testUrl), -1);
}

void QGCFileDownloadTest::_testCachedFileDownloadClearCache()
{
    QGCCachedFileDownload* downloader = _createCachedDownloader();

    QSignalSpy spy(downloader, &QGCCachedFileDownload::cacheSizeChanged);

    // Clear cache (should work even if empty)
    downloader->clearCache();

    // cacheSize should be 0 after clear
    QCOMPARE(downloader->cacheSize(), static_cast<qint64>(0));
}

void QGCFileDownloadTest::_testCachedFileDownloadRemoveFromCache()
{
    QGCCachedFileDownload* downloader = _createCachedDownloader();
    const QString testUrl = ":/unittest/manifest.json.gz";

    // Removing non-existent URL should return false
    QVERIFY(!downloader->removeFromCache(testUrl));
}

void QGCFileDownloadTest::_testCachedFileDownloadCancel()
{
    QGCCachedFileDownload* downloader = _createCachedDownloader();

    // Start download and immediately cancel
    QVERIFY(downloader->download(":/unittest/manifest.json.gz", 3600));
    downloader->cancel();

    // After cancel, should not be running
    QVERIFY(!downloader->isRunning());
}

void QGCFileDownloadTest::_testCachedFileDownloadConcurrentDownloadRejected()
{
    QGCCachedFileDownload* downloader = _createCachedDownloader();

    // Start first download
    QVERIFY(downloader->download(":/unittest/manifest.json.gz", 3600));

    // Second download should be rejected while first is running
    QVERIFY(!downloader->download(":/unittest/arducopter.apj", 3600));
    QVERIFY(!downloader->downloadPreferCache(":/unittest/arducopter.apj"));
    QVERIFY(!downloader->downloadNoCache(":/unittest/arducopter.apj"));

    // Wait for first download to complete
    QSignalSpy spy(downloader, &QGCCachedFileDownload::finished);
    QVERIFY(spy.wait(TestHelpers::kDefaultTimeoutMs));
}

void QGCFileDownloadTest::_testCachedFileDownloadSignals()
{
    QGCCachedFileDownload* downloader = _createCachedDownloader();

    QSignalSpy progressSpy(downloader, &QGCCachedFileDownload::progressChanged);
    QSignalSpy runningSpy(downloader, &QGCCachedFileDownload::runningChanged);
    QSignalSpy urlSpy(downloader, &QGCCachedFileDownload::urlChanged);
    QSignalSpy finishedSpy(downloader, &QGCCachedFileDownload::finished);
    QSignalSpy downloadCompleteSpy(downloader, &QGCCachedFileDownload::downloadComplete);
    QGC_VERIFY_SPY_VALID(progressSpy);
    QGC_VERIFY_SPY_VALID(runningSpy);
    QGC_VERIFY_SPY_VALID(urlSpy);
    QGC_VERIFY_SPY_VALID(finishedSpy);
    QGC_VERIFY_SPY_VALID(downloadCompleteSpy);

    QVERIFY(downloader->download(":/unittest/manifest.json.gz", 3600));

    // Should emit running=true and url changed
    QCOMPARE_GE(runningSpy.count(), 1);
    QCOMPARE(urlSpy.count(), 1);

    QVERIFY(finishedSpy.wait(TestHelpers::kDefaultTimeoutMs));

    // finished signal should have 4 arguments: success, localPath, errorMessage, fromCache
    QCOMPARE(finishedSpy.count(), 1);
    QCOMPARE(finishedSpy.first().count(), 4);
    QVERIFY(finishedSpy.first().at(0).toBool());  // success = true

    // Legacy downloadComplete signal should also fire
    QCOMPARE(downloadCompleteSpy.count(), 1);

    // running should be false at end
    QVERIFY(!downloader->isRunning());
}

// ============================================================================
// Auto-Decompress Tests
// ============================================================================

void QGCFileDownloadTest::_testAutoDecompressGzip()
{
    // Test downloading a .gz file with autoDecompress=true
    // Uses manifest.json.gz from compression test resources
    QGCFileDownload *const downloader = new QGCFileDownload(this);
    QString resultLocalFile;
    QString resultErrorMsg;

    (void)connect(downloader, &QGCFileDownload::downloadComplete, this,
        [&resultLocalFile, &resultErrorMsg](const QString &remoteFile, const QString &localFile, const QString &errorMsg) {
            Q_UNUSED(remoteFile);
            resultLocalFile = localFile;
            resultErrorMsg = errorMsg;
        });

    QSignalSpy spy(downloader, &QGCFileDownload::downloadComplete);

    // Download with autoDecompress=true
    QVERIFY(downloader->download(":/unittest/manifest.json.gz", {}, true));
    QVERIFY(spy.wait(TestHelpers::kDefaultTimeoutMs));

    downloader->deleteLater();

    // Verify no error
    QVERIFY2(resultErrorMsg.isEmpty(), qPrintable(resultErrorMsg));

    // Verify the output file is decompressed (should be manifest.json, not .gz)
    QVERIFY2(!resultLocalFile.isEmpty(), "Local file path is empty");
    QVERIFY2(!resultLocalFile.endsWith(".gz"), "File should be decompressed (no .gz extension)");
    QVERIFY2(QFile::exists(resultLocalFile), qPrintable("Output file doesn't exist: " + resultLocalFile));

    // Verify content is valid JSON (not compressed data)
    QFile outputFile(resultLocalFile);
    QVERIFY(outputFile.open(QIODevice::ReadOnly));
    const QByteArray content = outputFile.readAll();
    outputFile.close();

    QVERIFY2(!content.isEmpty(), "Decompressed file is empty");
    QVERIFY2(content.contains("\"name\""), "Content doesn't look like valid JSON");

    // Cleanup
    QFile::remove(resultLocalFile);
}

void QGCFileDownloadTest::_testAutoDecompressDisabled()
{
    // Test downloading a .gz file with autoDecompress=false (default)
    // The file should remain compressed
    QGCFileDownload *const downloader = new QGCFileDownload(this);
    QString resultLocalFile;
    QString resultErrorMsg;

    (void)connect(downloader, &QGCFileDownload::downloadComplete, this,
        [&resultLocalFile, &resultErrorMsg](const QString &remoteFile, const QString &localFile, const QString &errorMsg) {
            Q_UNUSED(remoteFile);
            resultLocalFile = localFile;
            resultErrorMsg = errorMsg;
        });

    QSignalSpy spy(downloader, &QGCFileDownload::downloadComplete);

    // Download with autoDecompress=false (default)
    QVERIFY(downloader->download(":/unittest/manifest.json.gz", {}, false));
    QVERIFY(spy.wait(TestHelpers::kDefaultTimeoutMs));

    downloader->deleteLater();

    // Verify no error
    QVERIFY2(resultErrorMsg.isEmpty(), qPrintable(resultErrorMsg));

    // Verify the output file is still compressed (.gz extension)
    QVERIFY2(!resultLocalFile.isEmpty(), "Local file path is empty");
    QVERIFY2(resultLocalFile.endsWith(".gz"), "File should remain compressed with .gz extension");
    QVERIFY2(QFile::exists(resultLocalFile), qPrintable("Output file doesn't exist: " + resultLocalFile));

    // Verify content is compressed (starts with gzip magic bytes 1f 8b)
    QFile outputFile(resultLocalFile);
    QVERIFY(outputFile.open(QIODevice::ReadOnly));
    const QByteArray content = outputFile.readAll();
    outputFile.close();

    QVERIFY2(!content.isEmpty(), "File is empty");
    QVERIFY2(content.size() >= 2, "File too small to be gzip");
    QVERIFY2(static_cast<unsigned char>(content[0]) == 0x1f &&
             static_cast<unsigned char>(content[1]) == 0x8b,
             "File doesn't have gzip magic bytes");

    // Cleanup
    QFile::remove(resultLocalFile);
}

void QGCFileDownloadTest::_testAutoDecompressUncompressedFile()
{
    // Test downloading a non-compressed file with autoDecompress=true
    // Should work normally without any decompression attempt
    QGCFileDownload *const downloader = new QGCFileDownload(this);
    QString resultLocalFile;
    QString resultErrorMsg;

    (void)connect(downloader, &QGCFileDownload::downloadComplete, this,
        [&resultLocalFile, &resultErrorMsg](const QString &remoteFile, const QString &localFile, const QString &errorMsg) {
            Q_UNUSED(remoteFile);
            resultLocalFile = localFile;
            resultErrorMsg = errorMsg;
        });

    QSignalSpy spy(downloader, &QGCFileDownload::downloadComplete);

    // Download non-compressed file with autoDecompress=true
    QVERIFY(downloader->download(":/unittest/arducopter.apj", {}, true));
    QVERIFY(spy.wait(TestHelpers::kDefaultTimeoutMs));

    downloader->deleteLater();

    // Verify no error
    QVERIFY2(resultErrorMsg.isEmpty(), qPrintable(resultErrorMsg));

    // Verify file downloaded successfully
    QVERIFY2(!resultLocalFile.isEmpty(), "Local file path is empty");
    QVERIFY2(QFile::exists(resultLocalFile), qPrintable("Output file doesn't exist: " + resultLocalFile));
    QVERIFY2(QFileInfo(resultLocalFile).size() > 0, "Downloaded file is empty");

    // Cleanup
    QFile::remove(resultLocalFile);
}

// ============================================================================
// Integration Tests (Download + Compression)
// ============================================================================

void QGCFileDownloadTest::_testDownloadAndExtractZip()
{
    // Integration test: Download a ZIP archive and extract it
    // This tests the full workflow of QGCFileDownload + QGCCompression

    QGCFileDownload *const downloader = new QGCFileDownload(this);
    QString resultLocalFile;
    QString resultErrorMsg;

    (void)connect(downloader, &QGCFileDownload::downloadComplete, this,
        [&resultLocalFile, &resultErrorMsg](const QString &remoteFile, const QString &localFile, const QString &errorMsg) {
            Q_UNUSED(remoteFile);
            resultLocalFile = localFile;
            resultErrorMsg = errorMsg;
        });

    QSignalSpy spy(downloader, &QGCFileDownload::downloadComplete);

    // Download ZIP archive (no auto-decompress for archives)
    QVERIFY(downloader->download(":/unittest/manifest.json.zip", {}, false));
    QVERIFY(spy.wait(TestHelpers::kDefaultTimeoutMs));

    downloader->deleteLater();

    // Verify download succeeded
    QVERIFY2(resultErrorMsg.isEmpty(), qPrintable(resultErrorMsg));
    QVERIFY2(QFile::exists(resultLocalFile), qPrintable("Downloaded file doesn't exist: " + resultLocalFile));

    // Create temp directory for extraction
    QTemporaryDir extractDir;
    QVERIFY(extractDir.isValid());

    // Extract the downloaded archive using QGCCompression
    const bool extractResult = QGCCompression::extractArchive(resultLocalFile, extractDir.path());
    QVERIFY2(extractResult, qPrintable("Extraction failed: " + QGCCompression::lastErrorString()));

    // Verify extracted content
    const QString extractedFile = extractDir.path() + "/manifest.json";
    VERIFY_FILE_EXISTS(extractedFile);

    // Verify extracted content is valid JSON
    QFile jsonFile(extractedFile);
    QVERIFY(jsonFile.open(QIODevice::ReadOnly));
    const QByteArray jsonData = jsonFile.readAll();
    jsonFile.close();

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(jsonData, &parseError);
    QVERIFY2(!doc.isNull(), qPrintable("Invalid JSON: " + parseError.errorString()));
    QVERIFY(doc.isObject());
    QVERIFY(doc.object().contains("name"));

    // Cleanup downloaded file
    QFile::remove(resultLocalFile);
}

void QGCFileDownloadTest::_testDownloadDecompressAndParse()
{
    // Integration test: Download a gzipped JSON file, decompress it, and parse
    // This tests the full workflow with auto-decompression

    QGCFileDownload *const downloader = new QGCFileDownload(this);
    QString resultLocalFile;
    QString resultErrorMsg;

    (void)connect(downloader, &QGCFileDownload::downloadComplete, this,
        [&resultLocalFile, &resultErrorMsg](const QString &remoteFile, const QString &localFile, const QString &errorMsg) {
            Q_UNUSED(remoteFile);
            resultLocalFile = localFile;
            resultErrorMsg = errorMsg;
        });

    QSignalSpy spy(downloader, &QGCFileDownload::downloadComplete);

    // Download with auto-decompress enabled
    QVERIFY(downloader->download(":/unittest/manifest.json.gz", {}, true));
    QVERIFY(spy.wait(TestHelpers::kDefaultTimeoutMs));

    downloader->deleteLater();

    // Verify download and decompression succeeded
    QVERIFY2(resultErrorMsg.isEmpty(), qPrintable(resultErrorMsg));
    QVERIFY2(QFile::exists(resultLocalFile), qPrintable("Decompressed file doesn't exist: " + resultLocalFile));

    // Parse the decompressed JSON directly
    QFile jsonFile(resultLocalFile);
    QVERIFY(jsonFile.open(QIODevice::ReadOnly));
    const QByteArray jsonData = jsonFile.readAll();
    jsonFile.close();

    // Verify it's valid JSON
    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(jsonData, &parseError);
    QVERIFY2(!doc.isNull(), qPrintable("Invalid JSON after decompression: " + parseError.errorString()));
    QVERIFY(doc.isObject());

    // Verify expected content
    const QJsonObject obj = doc.object();
    QVERIFY(obj.contains("name"));
    QCOMPARE(obj.value("name").toString(), QStringLiteral("test"));

    // Cleanup
    QFile::remove(resultLocalFile);
}
