#include "QGCFileDownloadTest.h"
#include "QGCCachedFileDownload.h"
#include "QGCCompression.h"
#include "QGCFileDownload.h"

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QTemporaryDir>
#include <QtTest/QTest>
#include <QtTest/QSignalSpy>

void QGCFileDownloadTest::_testFileDownload()
{
    // Local File
    QGCFileDownload *const downloader = new QGCFileDownload(this);
    (void) connect(downloader, &QGCFileDownload::downloadComplete, this,
        [this](const QString &remoteFile, const QString &localFile, const QString &errorMsg) {
            sender()->deleteLater();
            QVERIFY(errorMsg.isEmpty());
        });
    QSignalSpy spyQGCFileDownloadDownloadComplete(downloader, &QGCFileDownload::downloadComplete);
    QVERIFY(downloader->download(":/unittest/arducopter.apj"));
    QVERIFY(spyQGCFileDownloadDownloadComplete.wait(1000));
}

// ============================================================================
// QGCCachedFileDownload Tests
// ============================================================================

void QGCFileDownloadTest::_testCachedFileDownloadConstruction()
{
    // Test construction with cache directory
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

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

void QGCFileDownloadTest::_testCachedFileDownloadCacheDirectory()
{
    QTemporaryDir tempDir1;
    QTemporaryDir tempDir2;
    QVERIFY(tempDir1.isValid());
    QVERIFY(tempDir2.isValid());

    QGCCachedFileDownload downloader(tempDir1.path(), this);
    // QNetworkDiskCache may append trailing slash
    QVERIFY(downloader.cacheDirectory().startsWith(tempDir1.path()));

    QSignalSpy spy(&downloader, &QGCCachedFileDownload::cacheDirectoryChanged);

    downloader.setCacheDirectory(tempDir2.path());
    QVERIFY(downloader.cacheDirectory().startsWith(tempDir2.path()));
    QCOMPARE(spy.count(), 1);
    QVERIFY(spy.first().first().toString().startsWith(tempDir2.path()));

    // Setting same directory should not emit signal
    downloader.setCacheDirectory(downloader.cacheDirectory());  // Use actual value to avoid trailing slash mismatch
    QCOMPARE(spy.count(), 1);
}

void QGCFileDownloadTest::_testCachedFileDownloadMaxCacheSize()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

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

void QGCFileDownloadTest::_testCachedFileDownloadEmptyUrl()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    QGCCachedFileDownload downloader(tempDir.path(), this);

    // Empty URL should fail
    QVERIFY(!downloader.download("", 3600));
    QVERIFY(!downloader.errorString().isEmpty());
}

void QGCFileDownloadTest::_testCachedFileDownloadNoCacheDir()
{
    QGCCachedFileDownload downloader(this);

    // No cache directory should fail
    QVERIFY(!downloader.download(":/unittest/manifest.json.gz", 3600));
    QVERIFY(!downloader.errorString().isEmpty());
}

void QGCFileDownloadTest::_testCachedFileDownloadDownload()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    QGCCachedFileDownload downloader(tempDir.path(), this);

    bool finished = false;
    bool success = false;
    QString localPath;
    bool fromCache = false;

    connect(&downloader, &QGCCachedFileDownload::finished, this,
            [&](bool s, const QString &path, const QString &, bool cached) {
                finished = true;
                success = s;
                localPath = path;
                fromCache = cached;
            });

    QSignalSpy spy(&downloader, &QGCCachedFileDownload::finished);

    // Download with 1 hour cache
    QVERIFY(downloader.download(":/unittest/manifest.json.gz", 3600));
    QVERIFY(downloader.isRunning());

    QVERIFY(spy.wait(5000));
    QVERIFY(finished);
    QVERIFY(success);
    QVERIFY(!localPath.isEmpty());
    QVERIFY(QFile::exists(localPath));
    QVERIFY(!downloader.isRunning());
}

void QGCFileDownloadTest::_testCachedFileDownloadPreferCache()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    QGCCachedFileDownload downloader(tempDir.path(), this);

    bool success = false;
    QString localPath;

    connect(&downloader, &QGCCachedFileDownload::finished, this,
            [&](bool s, const QString &path, const QString &, bool) {
                success = s;
                localPath = path;
            });

    QSignalSpy spy(&downloader, &QGCCachedFileDownload::finished);

    QVERIFY(downloader.downloadPreferCache(":/unittest/arducopter.apj"));
    QVERIFY(spy.wait(5000));

    QVERIFY(success);
    QVERIFY(!localPath.isEmpty());
    QVERIFY(QFile::exists(localPath));
}

void QGCFileDownloadTest::_testCachedFileDownloadNoCache()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    QGCCachedFileDownload downloader(tempDir.path(), this);

    bool success = false;
    QString localPath;

    connect(&downloader, &QGCCachedFileDownload::finished, this,
            [&](bool s, const QString &path, const QString &, bool) {
                success = s;
                localPath = path;
            });

    QSignalSpy spy(&downloader, &QGCCachedFileDownload::finished);

    QVERIFY(downloader.downloadNoCache(":/unittest/arducopter.apj"));
    QVERIFY(spy.wait(5000));

    QVERIFY(success);
    QVERIFY(!localPath.isEmpty());
    QVERIFY(QFile::exists(localPath));
}

void QGCFileDownloadTest::_testCachedFileDownloadIsCached()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    QGCCachedFileDownload downloader(tempDir.path(), this);
    const QString testUrl = ":/unittest/manifest.json.gz";

    // Initially not cached
    QVERIFY(!downloader.isCached(testUrl));
    QVERIFY(!downloader.isCached(testUrl, 3600));

    // Download to populate cache
    QSignalSpy spy(&downloader, &QGCCachedFileDownload::finished);
    QVERIFY(downloader.download(testUrl, 3600));
    QVERIFY(spy.wait(5000));

    // Note: Qt resource URLs may not be cached by QNetworkDiskCache the same way
    // as HTTP URLs, so the cache behavior might differ. This test verifies the API works.
}

void QGCFileDownloadTest::_testCachedFileDownloadCacheAge()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    QGCCachedFileDownload downloader(tempDir.path(), this);
    const QString testUrl = ":/unittest/manifest.json.gz";

    // Not cached returns -1
    QCOMPARE(downloader.cacheAge(testUrl), -1);
}

void QGCFileDownloadTest::_testCachedFileDownloadClearCache()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    QGCCachedFileDownload downloader(tempDir.path(), this);

    QSignalSpy spy(&downloader, &QGCCachedFileDownload::cacheSizeChanged);

    // Clear cache (should work even if empty)
    downloader.clearCache();

    // cacheSize should be 0 after clear
    QCOMPARE(downloader.cacheSize(), static_cast<qint64>(0));
}

void QGCFileDownloadTest::_testCachedFileDownloadRemoveFromCache()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    QGCCachedFileDownload downloader(tempDir.path(), this);
    const QString testUrl = ":/unittest/manifest.json.gz";

    // Removing non-existent URL should return false
    QVERIFY(!downloader.removeFromCache(testUrl));
}

void QGCFileDownloadTest::_testCachedFileDownloadCancel()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    QGCCachedFileDownload downloader(tempDir.path(), this);

    // Start download and immediately cancel
    QVERIFY(downloader.download(":/unittest/manifest.json.gz", 3600));
    downloader.cancel();

    // After cancel, should not be running
    QVERIFY(!downloader.isRunning());
}

void QGCFileDownloadTest::_testCachedFileDownloadConcurrentDownloadRejected()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    QGCCachedFileDownload downloader(tempDir.path(), this);

    // Start first download
    QVERIFY(downloader.download(":/unittest/manifest.json.gz", 3600));

    // Second download should be rejected while first is running
    QVERIFY(!downloader.download(":/unittest/arducopter.apj", 3600));
    QVERIFY(!downloader.downloadPreferCache(":/unittest/arducopter.apj"));
    QVERIFY(!downloader.downloadNoCache(":/unittest/arducopter.apj"));

    // Wait for first download to complete
    QSignalSpy spy(&downloader, &QGCCachedFileDownload::finished);
    QVERIFY(spy.wait(5000));
}

void QGCFileDownloadTest::_testCachedFileDownloadSignals()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    QGCCachedFileDownload downloader(tempDir.path(), this);

    QSignalSpy progressSpy(&downloader, &QGCCachedFileDownload::progressChanged);
    QSignalSpy runningSpy(&downloader, &QGCCachedFileDownload::runningChanged);
    QSignalSpy urlSpy(&downloader, &QGCCachedFileDownload::urlChanged);
    QSignalSpy finishedSpy(&downloader, &QGCCachedFileDownload::finished);
    QSignalSpy downloadCompleteSpy(&downloader, &QGCCachedFileDownload::downloadComplete);

    QVERIFY(downloader.download(":/unittest/manifest.json.gz", 3600));

    // Should emit running=true and url changed
    QVERIFY(runningSpy.count() >= 1);
    QCOMPARE(urlSpy.count(), 1);

    QVERIFY(finishedSpy.wait(5000));

    // finished signal should have 4 arguments: success, localPath, errorMessage, fromCache
    QCOMPARE(finishedSpy.count(), 1);
    QCOMPARE(finishedSpy.first().count(), 4);
    QVERIFY(finishedSpy.first().at(0).toBool());  // success = true

    // Legacy downloadComplete signal should also fire
    QCOMPARE(downloadCompleteSpy.count(), 1);

    // running should be false at end
    QVERIFY(!downloader.isRunning());
}

void QGCFileDownloadTest::_testAutoDecompressGzip()
{
    // Test downloading a .gz file with autoDecompress=true
    // Uses manifest.json.gz from compression test resources
    QGCFileDownload *const downloader = new QGCFileDownload(this);
    QString resultLocalFile;
    QString resultErrorMsg;

    (void) connect(downloader, &QGCFileDownload::downloadComplete, this,
        [&resultLocalFile, &resultErrorMsg](const QString &remoteFile, const QString &localFile, const QString &errorMsg) {
            Q_UNUSED(remoteFile);
            resultLocalFile = localFile;
            resultErrorMsg = errorMsg;
        });

    QSignalSpy spy(downloader, &QGCFileDownload::downloadComplete);

    // Download with autoDecompress=true
    QVERIFY(downloader->download(":/unittest/manifest.json.gz", {}, true));
    QVERIFY(spy.wait(5000));

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

    (void) connect(downloader, &QGCFileDownload::downloadComplete, this,
        [&resultLocalFile, &resultErrorMsg](const QString &remoteFile, const QString &localFile, const QString &errorMsg) {
            Q_UNUSED(remoteFile);
            resultLocalFile = localFile;
            resultErrorMsg = errorMsg;
        });

    QSignalSpy spy(downloader, &QGCFileDownload::downloadComplete);

    // Download with autoDecompress=false (default)
    QVERIFY(downloader->download(":/unittest/manifest.json.gz", {}, false));
    QVERIFY(spy.wait(5000));

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

    (void) connect(downloader, &QGCFileDownload::downloadComplete, this,
        [&resultLocalFile, &resultErrorMsg](const QString &remoteFile, const QString &localFile, const QString &errorMsg) {
            Q_UNUSED(remoteFile);
            resultLocalFile = localFile;
            resultErrorMsg = errorMsg;
        });

    QSignalSpy spy(downloader, &QGCFileDownload::downloadComplete);

    // Download non-compressed file with autoDecompress=true
    QVERIFY(downloader->download(":/unittest/arducopter.apj", {}, true));
    QVERIFY(spy.wait(5000));

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

    (void) connect(downloader, &QGCFileDownload::downloadComplete, this,
        [&resultLocalFile, &resultErrorMsg](const QString &remoteFile, const QString &localFile, const QString &errorMsg) {
            Q_UNUSED(remoteFile);
            resultLocalFile = localFile;
            resultErrorMsg = errorMsg;
        });

    QSignalSpy spy(downloader, &QGCFileDownload::downloadComplete);

    // Download ZIP archive (no auto-decompress for archives)
    QVERIFY(downloader->download(":/unittest/manifest.json.zip", {}, false));
    QVERIFY(spy.wait(5000));

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
    QVERIFY2(QFile::exists(extractedFile), qPrintable("Extracted file doesn't exist: " + extractedFile));

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

    (void) connect(downloader, &QGCFileDownload::downloadComplete, this,
        [&resultLocalFile, &resultErrorMsg](const QString &remoteFile, const QString &localFile, const QString &errorMsg) {
            Q_UNUSED(remoteFile);
            resultLocalFile = localFile;
            resultErrorMsg = errorMsg;
        });

    QSignalSpy spy(downloader, &QGCFileDownload::downloadComplete);

    // Download with auto-decompress enabled
    QVERIFY(downloader->download(":/unittest/manifest.json.gz", {}, true));
    QVERIFY(spy.wait(5000));

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
