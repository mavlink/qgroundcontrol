#include "QGCFileDownloadTest.h"

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtTest/QSignalSpy>

#include "QGCCompression.h"
#include "QGCFileDownload.h"
#include "QGCFileHelper.h"

void QGCFileDownloadTest::_testFileDownload()
{
    QGCFileDownload downloader(this);
    QSignalSpy finishedSpy(&downloader, &QGCFileDownload::finished);

    QVERIFY(downloader.start(":/unittest/arducopter.apj"));
    QVERIFY(finishedSpy.wait(1000));
    QCOMPARE(finishedSpy.count(), 1);

    const QList<QVariant> args = finishedSpy.first();
    QVERIFY(args.at(0).toBool());
    const QString localPath = args.at(1).toString();
    QVERIFY(!localPath.isEmpty());
    QVERIFY(QFile::exists(localPath));
    QVERIFY(args.at(2).toString().isEmpty());

    QFile::remove(localPath);
}

void QGCFileDownloadTest::_testFileDownloadEmptyUrl()
{
    QGCFileDownload downloader(this);
    QSignalSpy finishedSpy(&downloader, &QGCFileDownload::finished);

    QVERIFY(!downloader.start(QString()));
    QCOMPARE(finishedSpy.count(), 0);
    QVERIFY(!downloader.errorString().isEmpty());
}

void QGCFileDownloadTest::_testFileDownloadConcurrentStartRejected()
{
    QGCFileDownload downloader(this);
    QSignalSpy finishedSpy(&downloader, &QGCFileDownload::finished);

    QVERIFY(downloader.start(":/unittest/manifest.json.gz"));
    QVERIFY(downloader.isRunning());
    QVERIFY(!downloader.start(":/unittest/arducopter.apj"));

    QVERIFY(finishedSpy.wait(5000));
    QCOMPARE(finishedSpy.count(), 1);
    QVERIFY(finishedSpy.first().at(0).toBool());
}

void QGCFileDownloadTest::_testFileDownloadHashVerificationFailure()
{
    QGCFileDownload downloader(this);
    QSignalSpy finishedSpy(&downloader, &QGCFileDownload::finished);
    downloader.setExpectedHash(QStringLiteral("0000000000000000000000000000000000000000000000000000000000000000"));

    QVERIFY(downloader.start(":/unittest/arducopter.apj"));
    QVERIFY(finishedSpy.wait(5000));
    QCOMPARE(finishedSpy.count(), 1);

    const QList<QVariant> args = finishedSpy.first();
    QVERIFY(!args.at(0).toBool());              // success
    QVERIFY(args.at(1).toString().isEmpty());   // localPath
    QVERIFY(args.at(2).toString().contains(QStringLiteral("Hash verification failed")));
    QVERIFY(!downloader.errorString().isEmpty());
}

void QGCFileDownloadTest::_testFileDownloadHashVerificationSuccess()
{
    QGCFileDownload downloader(this);
    QSignalSpy finishedSpy(&downloader, &QGCFileDownload::finished);

    const QString expectedHash = QGCFileHelper::computeFileHash(":/unittest/arducopter.apj");
    QVERIFY(!expectedHash.isEmpty());
    downloader.setExpectedHash(expectedHash);

    QVERIFY(downloader.start(":/unittest/arducopter.apj"));
    QVERIFY(finishedSpy.wait(5000));
    QCOMPARE(finishedSpy.count(), 1);

    const QList<QVariant> args = finishedSpy.first();
    QVERIFY(args.at(0).toBool());
    const QString localPath = args.at(1).toString();
    QVERIFY(!localPath.isEmpty());
    QVERIFY(QFile::exists(localPath));
    QVERIFY(args.at(2).toString().isEmpty());
    QVERIFY(downloader.errorString().isEmpty());

    QFile::remove(localPath);
}

void QGCFileDownloadTest::_testFileDownloadCustomOutputPath()
{
    QGCFileDownload downloader(this);
    QSignalSpy finishedSpy(&downloader, &QGCFileDownload::finished);

    const QString customOutput = QDir::temp().filePath(QStringLiteral("qgc_custom_download.apj"));
    QFile::remove(customOutput);
    downloader.setOutputPath(customOutput);

    QVERIFY(downloader.start(":/unittest/arducopter.apj"));
    QVERIFY(finishedSpy.wait(5000));
    QCOMPARE(finishedSpy.count(), 1);

    const QList<QVariant> args = finishedSpy.first();
    QVERIFY(args.at(0).toBool());
    QCOMPARE(args.at(1).toString(), customOutput);
    QVERIFY(QFile::exists(customOutput));

    QFile::remove(customOutput);
}

void QGCFileDownloadTest::_testFileDownloadCancelSingleCompletion()
{
    QGCFileDownload downloader(this);
    QSignalSpy finishedSpy(&downloader, &QGCFileDownload::finished);

    downloader.setAutoDecompress(true);
    QVERIFY(downloader.start(":/unittest/manifest.json.gz"));
    downloader.cancel();

    QTRY_VERIFY_WITH_TIMEOUT(finishedSpy.count() >= 1, 5000);
    QTest::qWait(200);
    QCOMPARE(finishedSpy.count(), 1);
    QVERIFY(!downloader.isRunning());

    const QList<QVariant> args = finishedSpy.first();
    QVERIFY(!args.at(0).toBool());
}
void QGCFileDownloadTest::_testAutoDecompressGzip()
{
    // Test downloading a .gz file with autoDecompress=true
    // Uses manifest.json.gz from compression test resources
    QGCFileDownload* const downloader = new QGCFileDownload(this);
    QString resultLocalFile;
    QString resultErrorMsg;
    (void)connect(downloader, &QGCFileDownload::finished, this,
                  [&resultLocalFile, &resultErrorMsg](bool success, const QString& localFile,
                                                      const QString& errorMsg) {
                      QVERIFY(success);
                      resultLocalFile = localFile;
                      resultErrorMsg = errorMsg;
                  });
    QSignalSpy spy(downloader, &QGCFileDownload::finished);
    // Download with autoDecompress=true
    downloader->setAutoDecompress(true);
    QVERIFY(downloader->start(":/unittest/manifest.json.gz"));
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
    QGCFileDownload* const downloader = new QGCFileDownload(this);
    QString resultLocalFile;
    QString resultErrorMsg;
    (void)connect(downloader, &QGCFileDownload::finished, this,
                  [&resultLocalFile, &resultErrorMsg](bool success, const QString& localFile,
                                                      const QString& errorMsg) {
                      QVERIFY(success);
                      resultLocalFile = localFile;
                      resultErrorMsg = errorMsg;
                  });
    QSignalSpy spy(downloader, &QGCFileDownload::finished);
    // Download with autoDecompress=false (default)
    downloader->setAutoDecompress(false);
    QVERIFY(downloader->start(":/unittest/manifest.json.gz"));
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
    QVERIFY2(static_cast<unsigned char>(content[0]) == 0x1f && static_cast<unsigned char>(content[1]) == 0x8b,
             "File doesn't have gzip magic bytes");
    // Cleanup
    QFile::remove(resultLocalFile);
}

void QGCFileDownloadTest::_testAutoDecompressUncompressedFile()
{
    // Test downloading a non-compressed file with autoDecompress=true
    // Should work normally without any decompression attempt
    QGCFileDownload* const downloader = new QGCFileDownload(this);
    QString resultLocalFile;
    QString resultErrorMsg;
    (void)connect(downloader, &QGCFileDownload::finished, this,
                  [&resultLocalFile, &resultErrorMsg](bool success, const QString& localFile,
                                                      const QString& errorMsg) {
                      QVERIFY(success);
                      resultLocalFile = localFile;
                      resultErrorMsg = errorMsg;
                  });
    QSignalSpy spy(downloader, &QGCFileDownload::finished);
    // Download non-compressed file with autoDecompress=true
    downloader->setAutoDecompress(true);
    QVERIFY(downloader->start(":/unittest/arducopter.apj"));
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
    QGCFileDownload* const downloader = new QGCFileDownload(this);
    QString resultLocalFile;
    QString resultErrorMsg;
    (void)connect(downloader, &QGCFileDownload::finished, this,
                  [&resultLocalFile, &resultErrorMsg](bool success, const QString& localFile,
                                                      const QString& errorMsg) {
                      QVERIFY(success);
                      resultLocalFile = localFile;
                      resultErrorMsg = errorMsg;
                  });
    QSignalSpy spy(downloader, &QGCFileDownload::finished);
    // Download ZIP archive (no auto-decompress for archives)
    downloader->setAutoDecompress(false);
    QVERIFY(downloader->start(":/unittest/manifest.json.zip"));
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
    QGCFileDownload* const downloader = new QGCFileDownload(this);
    QString resultLocalFile;
    QString resultErrorMsg;
    (void)connect(downloader, &QGCFileDownload::finished, this,
                  [&resultLocalFile, &resultErrorMsg](bool success, const QString& localFile,
                                                      const QString& errorMsg) {
                      QVERIFY(success);
                      resultLocalFile = localFile;
                      resultErrorMsg = errorMsg;
                  });
    QSignalSpy spy(downloader, &QGCFileDownload::finished);
    // Download with auto-decompress enabled
    downloader->setAutoDecompress(true);
    QVERIFY(downloader->start(":/unittest/manifest.json.gz"));
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

UT_REGISTER_TEST(QGCFileDownloadTest, TestLabel::Integration, TestLabel::Network, TestLabel::Utilities)
