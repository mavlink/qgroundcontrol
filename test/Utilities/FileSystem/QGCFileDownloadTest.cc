#include "QGCFileDownloadTest.h"

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QPointer>
#include <QtCore/QRegularExpression>
#include <QtCore/QTemporaryDir>
#include <QtTest/QSignalSpy>

#include "Compression.h"
#include "CompressionJob.h"
#include "FileFixtures.h"
#include "LocalHttpTestServer.h"
#include "ProcessFixtures.h"
#include "QGCFileDownload.h"
#include "QGCFileHelper.h"

void QGCFileDownloadTest::_testFileDownload()
{
    QGCFileDownload downloader(this);
    QSignalSpy finishedSpy(&downloader, &QGCFileDownload::finished);

    QVERIFY(downloader.start(":/unittest/arducopter.apj"));
    QVERIFY_SIGNAL_WAIT(finishedSpy, TestTimeout::mediumMs());
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

    expectLogMessage("Utilities.QGCFileDownload", QtWarningMsg, QRegularExpression("Empty URL provided"));
    QVERIFY(!downloader.start(QString()));
    verifyExpectedLogMessage();
    QCOMPARE(finishedSpy.count(), 0);
    QVERIFY(!downloader.errorString().isEmpty());
}

void QGCFileDownloadTest::_testFileDownloadConcurrentStartRejected()
{
    QGCFileDownload downloader(this);
    QSignalSpy finishedSpy(&downloader, &QGCFileDownload::finished);

    QVERIFY(downloader.start(":/unittest/manifest.json.gz"));
    QVERIFY(downloader.isRunning());
    expectLogMessage("Utilities.QGCFileDownload", QtWarningMsg, QRegularExpression("Download already in progress"));
    QVERIFY(!downloader.start(":/unittest/arducopter.apj"));
    verifyExpectedLogMessage();

    QVERIFY_SIGNAL_WAIT(finishedSpy, TestTimeout::mediumMs());
    QCOMPARE(finishedSpy.count(), 1);
    QVERIFY(finishedSpy.first().at(0).toBool());
}

void QGCFileDownloadTest::_testFileDownloadHashVerificationFailure()
{
    QGCFileDownload downloader(this);
    QSignalSpy finishedSpy(&downloader, &QGCFileDownload::finished);
    downloader.setExpectedHash(QStringLiteral("0000000000000000000000000000000000000000000000000000000000000000"));

    QVERIFY(downloader.start(":/unittest/arducopter.apj"));
    QVERIFY_SIGNAL_WAIT(finishedSpy, TestTimeout::mediumMs());
    QCOMPARE(finishedSpy.count(), 1);

    const QList<QVariant> args = finishedSpy.first();
    QVERIFY(!args.at(0).toBool());             // success
    QVERIFY(args.at(1).toString().isEmpty());  // localPath
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
    QVERIFY_SIGNAL_WAIT(finishedSpy, TestTimeout::mediumMs());
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
    QTemporaryDir tempDir;
    QGCFileDownload downloader(this);
    QSignalSpy finishedSpy(&downloader, &QGCFileDownload::finished);

    const QString customOutput = tempDir.filePath(QStringLiteral("qgc_custom_download.apj"));
    QFile::remove(customOutput);
    downloader.setOutputPath(customOutput);

    QVERIFY(downloader.start(":/unittest/arducopter.apj"));
    QVERIFY_SIGNAL_WAIT(finishedSpy, TestTimeout::mediumMs());
    QCOMPARE(finishedSpy.count(), 1);

    const QList<QVariant> args = finishedSpy.first();
    QVERIFY(args.at(0).toBool());
    QCOMPARE(args.at(1).toString(), customOutput);
    QVERIFY(QFile::exists(customOutput));

    QFile::remove(customOutput);
}

void QGCFileDownloadTest::_testFileDownloadOutputDirectory()
{
    QTemporaryDir outputDir;
    QVERIFY(outputDir.isValid());
    QGCFileDownload downloader(this);
    QSignalSpy finishedSpy(&downloader, &QGCFileDownload::finished);

    // The download keeps the name of the URL, and its content as it was sent
    downloader.setOutputDirectory(outputDir.path());
    QVERIFY(downloader.start(":/unittest/manifest.json.gz"));
    QVERIFY_SIGNAL_WAIT(finishedSpy, TestTimeout::mediumMs());

    const QList<QVariant> args = finishedSpy.first();
    QVERIFY2(args.at(0).toBool(), qPrintable(args.at(2).toString()));
    QCOMPARE(args.at(1).toString(), outputDir.filePath(QStringLiteral("manifest.json.gz")));
    QCOMPARE(QDir(outputDir.path()).entryList(QDir::Files), QStringList{QStringLiteral("manifest.json.gz")});
    QCOMPARE(TestFixtures::readFile(args.at(1).toString()), TestFixtures::readFile(":/unittest/manifest.json.gz"));
}

void QGCFileDownloadTest::_testFileDownloadNonExistentLocalFile()
{
    QTemporaryDir tempDir;
    QGCFileDownload downloader(this);
    QSignalSpy finishedSpy(&downloader, &QGCFileDownload::finished);

    const QString missingPath = tempDir.filePath(QStringLiteral("does_not_exist.apj"));
    QVERIFY(!QFile::exists(missingPath));

    expectLogMessage("Utilities.QGCFileDownload", QtWarningMsg, QRegularExpression("Download error:"));
    QVERIFY(downloader.start(missingPath));
    QVERIFY_SIGNAL_WAIT(finishedSpy, TestTimeout::mediumMs());
    verifyExpectedLogMessage();
    QCOMPARE(finishedSpy.count(), 1);

    const QList<QVariant> args = finishedSpy.first();
    QVERIFY(!args.at(0).toBool());
    QVERIFY(args.at(1).toString().isEmpty());
    QVERIFY(!args.at(2).toString().isEmpty());
}

void QGCFileDownloadTest::_testFileDownloadOutputPathIsDirectory()
{
    QTemporaryDir tempDir;
    QGCFileDownload downloader(this);
    QSignalSpy finishedSpy(&downloader, &QGCFileDownload::finished);

    downloader.setOutputPath(tempDir.path());
    QVERIFY(!downloader.start(":/unittest/arducopter.apj"));
    QCOMPARE(finishedSpy.count(), 0);
    QVERIFY(!downloader.errorString().isEmpty());
}

void QGCFileDownloadTest::_testFileDownloadCancelSingleCompletion()
{
    QGCFileDownload downloader(this);
    QSignalSpy finishedSpy(&downloader, &QGCFileDownload::finished);

    QVERIFY(downloader.start(":/unittest/manifest.json.gz"));
    downloader.cancel();

    QTRY_COMPARE_WITH_TIMEOUT(finishedSpy.count(), 1, TestTimeout::mediumMs());
    QVERIFY(!downloader.isRunning());

    const QList<QVariant> args = finishedSpy.first();
    QVERIFY(!args.at(0).toBool());
}

void QGCFileDownloadTest::_testFileDownloadSizeLimit_data()
{
    // A raw HTTP response, or empty to download a local file of the body
    QTest::addColumn<QByteArray>("response");
    QTest::addColumn<QByteArray>("body");
    QTest::addColumn<qint64>("maxBytes");

    const QByteArray body = QByteArray("0123456789abcdef").repeated(256);
    const auto withLength = [](const QByteArray& content, const QByteArray& extraHeader = {}) {
        return QByteArray("HTTP/1.1 200 OK\r\nConnection: close\r\n") + extraHeader +
               "Content-Length: " + QByteArray::number(content.size()) + "\r\n\r\n" + content;
    };
    const QByteArray withoutLength = QByteArray("HTTP/1.1 200 OK\r\nConnection: close\r\n\r\n") + body;
    const qint64 size = body.size();

    QTest::newRow("at the limit") << withLength(body) << body << size;
    QTest::newRow("of no announced length, at the limit") << withoutLength << body << size;
    QTest::newRow("larger than the limit") << withLength(body) << body << (size - 1);
    // Refused by the length it announces: the connection closes before more arrives
    QTest::newRow("announced larger than the limit")
        << (QByteArray("HTTP/1.1 200 OK\r\nConnection: close\r\nContent-Length: ") + QByteArray::number(2 * size) +
            "\r\n\r\n" + body)
        << body.repeated(2) << size;
    QTest::newRow("of no announced length, larger than the limit") << withoutLength << body << (size - 1);
    // The body that counts is the one the network stack has decoded
    QTest::newRow("larger than the limit once its content encoding is decoded")
        << withLength(TestFixtures::gzipMember(body), "Content-Encoding: gzip\r\n") << body << (size - 1);
    QTest::newRow("local file larger than the limit") << QByteArray() << body << (size - 1);
}

// A download is held to its size limit, by the length it announces and by what it receives. One that is larger fails,
// and no part of it is kept
void QGCFileDownloadTest::_testFileDownloadSizeLimit()
{
    QFETCH(QByteArray, response);
    QFETCH(QByteArray, body);
    QFETCH(qint64, maxBytes);
    const bool refused = (maxBytes < body.size());

    QTemporaryDir outputDir;
    QVERIFY(outputDir.isValid());
    QString url;
    TestFixtures::LocalHttpTestServer server;
    if (response.isEmpty()) {
        url = outputDir.filePath(QStringLiteral("source/data.bin"));
        QVERIFY(QDir().mkpath(QFileInfo(url).absolutePath()));
        QVERIFY(TestFixtures::writeFile(url, body));
    } else {
        QVERIFY(server.listen());
        server.installRawResponder(response);
        url = server.url(QStringLiteral("/data.bin"));
    }

    QGCFileDownload downloader(this);
    downloader.setOutputDirectory(outputDir.filePath(QStringLiteral("downloads")));
    downloader.setMaxBytes(maxBytes);
    QSignalSpy finishedSpy(&downloader, &QGCFileDownload::finished);
    if (refused) {
        expectLogMessage("Utilities.QGCFileDownload", QtWarningMsg, QRegularExpression("File is larger than"));
    }
    QVERIFY(downloader.start(url));
    QVERIFY_SIGNAL_WAIT(finishedSpy, TestTimeout::mediumMs());
    QCOMPARE(finishedSpy.count(), 1);
    const QList<QVariant> args = finishedSpy.first();

    const QDir downloads(outputDir.filePath(QStringLiteral("downloads")));
    if (refused) {
        verifyExpectedLogMessage();
        QVERIFY(!args.at(0).toBool());
        QVERIFY(args.at(1).toString().isEmpty());
        QVERIFY2(args.at(2).toString().startsWith(QStringLiteral("File is larger than")),
                 qPrintable(args.at(2).toString()));
        QCOMPARE(downloads.entryList(QDir::Files), QStringList());
    } else {
        QVERIFY2(args.at(0).toBool(), qPrintable(args.at(2).toString()));
        QCOMPARE(TestFixtures::readFile(args.at(1).toString()), body);
    }
}

void QGCFileDownloadTest::_testAutoDecompress_data()
{
    // The file to download: a resource, or empty for a local file "content.json.gz" of the content
    QTest::addColumn<QString>("source");
    QTest::addColumn<QByteArray>("content");
    // Name of the file that finished() gives, the only one left in the output directory
    QTest::addColumn<QString>("resultName");

    QByteArray manifest;
    QVERIFY(Compression::readFile(QStringLiteral(":/unittest/manifest.json.gz"), manifest));
    QTest::newRow("compressed") << ":/unittest/manifest.json.gz" << manifest << "manifest.json";
    // As when the HTTP client has taken a content encoding off: the content is kept under the name without the
    // extension
    const QByteArray plain("{\"name\": \"not compressed\"}\n");
    QTest::newRow("only the name compressed") << QString() << plain << "content.json";
    // A file whose name and content are not compressed is not touched
    QTest::newRow("not compressed") << ":/unittest/arducopter.apj"
                                    << TestFixtures::readFile(QStringLiteral(":/unittest/arducopter.apj"))
                                    << "arducopter.apj";
}

// With autoDecompress, a compressed download is replaced by the file it decompresses to
void QGCFileDownloadTest::_testAutoDecompress()
{
    QFETCH(QString, source);
    QFETCH(QByteArray, content);
    QFETCH(QString, resultName);

    QTemporaryDir sourceDir;
    QTemporaryDir outputDir;
    QVERIFY(sourceDir.isValid());
    QVERIFY(outputDir.isValid());
    if (source.isEmpty()) {
        source = sourceDir.filePath(QStringLiteral("content.json.gz"));
        QVERIFY(TestFixtures::writeFile(source, content));
    }

    QGCFileDownload downloader(this);
    QSignalSpy finishedSpy(&downloader, &QGCFileDownload::finished);
    downloader.setAutoDecompress(true);
    downloader.setOutputDirectory(outputDir.path());
    QVERIFY(downloader.start(source));
    QVERIFY_SIGNAL_WAIT(finishedSpy, TestTimeout::mediumMs());

    const QList<QVariant> args = finishedSpy.first();
    QVERIFY2(args.at(0).toBool(), qPrintable(args.at(2).toString()));
    QCOMPARE(args.at(1).toString(), outputDir.filePath(resultName));
    QCOMPARE(downloader.localPath(), outputDir.filePath(resultName));
    QCOMPARE(QDir(outputDir.path()).entryList(QDir::Files | QDir::Hidden), QStringList{resultName});
    QCOMPARE(TestFixtures::readFile(outputDir.filePath(resultName)), content);
}

void QGCFileDownloadTest::_testAutoDecompressFailure_data()
{
    QTest::addColumn<QByteArray>("content");
    QTest::addColumn<qint64>("maxDecompressedBytes");
    // The start of the error, after the path of the download, and what follows
    QTest::addColumn<QString>("reason");

    const QByteArray compressed = TestFixtures::gzipMember(QByteArray(4096, 'x'));
    QTest::newRow("truncated") << compressed.first(compressed.size() / 2)
                               << Compression::kDefaultMaxDecompressedFileBytes << " \\(.*truncated gzip input";
    QTest::newRow("over the limit") << compressed << qint64{1024} << ": size limit exceeded$";
}

// A download that cannot be decompressed, also one over the limit, fails with the reason and leaves no file. The
// limit of the transfer, which the download keeps, does not let a larger file through
void QGCFileDownloadTest::_testAutoDecompressFailure()
{
    QFETCH(QByteArray, content);
    QFETCH(qint64, maxDecompressedBytes);
    QFETCH(QString, reason);

    QTemporaryDir sourceDir;
    QTemporaryDir outputDir;
    QVERIFY(sourceDir.isValid());
    QVERIFY(outputDir.isValid());
    const QString sourcePath = sourceDir.filePath(QStringLiteral("content.gz"));
    QVERIFY(TestFixtures::writeFile(sourcePath, content));

    QGCFileDownload downloader(this);
    QSignalSpy finishedSpy(&downloader, &QGCFileDownload::finished);
    downloader.setAutoDecompress(true);
    downloader.setMaxBytes(content.size());
    downloader.setMaxDecompressedBytes(maxDecompressedBytes);
    downloader.setOutputPath(outputDir.filePath(QStringLiteral("content.gz")));

    expectLogMessage("Utilities.QGCFileDownload", QtWarningMsg, QRegularExpression("Decompression failed"));
    QVERIFY(downloader.start(sourcePath));
    QVERIFY_SIGNAL_WAIT(finishedSpy, TestTimeout::mediumMs());
    verifyExpectedLogMessage();

    const QList<QVariant> args = finishedSpy.first();
    QVERIFY(!args.at(0).toBool());
    QVERIFY(args.at(1).toString().isEmpty());
    // The error names the download as the caller knows it
    const QString error = args.at(2).toString();
    const QRegularExpression expected(QStringLiteral("^Decompression failed: ") +
                                          QRegularExpression::escape(outputDir.filePath("content.gz")) + reason,
                                      QRegularExpression::CaseInsensitiveOption);
    QVERIFY2(error.contains(expected), qPrintable(error));
    QCOMPARE(downloader.state(), QGCFileDownload::State::Failed);
    QVERIFY(QDir(outputDir.path()).isEmpty(QDir::AllEntries | QDir::Hidden | QDir::NoDotAndDotDot));
}

// Cancelling a download while its file is decompressed ends it at once, and the decompression leaves no file once it
// has ended. The decompression, which still waits for a thread, does not complete the next download.
void QGCFileDownloadTest::_testAutoDecompressCancelled()
{
    QTemporaryDir outputDir;
    QVERIFY(outputDir.isValid());

    TestFixtures::ThreadPoolBlocker blocker;
    QVERIFY(blocker.isBlocking());

    QGCFileDownload downloader(this);
    QSignalSpy finishedSpy(&downloader, &QGCFileDownload::finished);
    downloader.setAutoDecompress(true);
    downloader.setOutputPath(outputDir.filePath(QStringLiteral("manifest.json.gz")));
    QVERIFY(downloader.start(":/unittest/manifest.json.gz"));
    QTRY_COMPARE_WITH_TIMEOUT(downloader.state(), QGCFileDownload::State::Decompressing, TestTimeout::mediumMs());
    QVERIFY(downloader.isRunning());
    const QPointer<CompressionJob> job = downloader.findChild<CompressionJob*>();
    QVERIFY(job);

    downloader.cancel();
    QCOMPARE(finishedSpy.count(), 1);
    QVERIFY(!finishedSpy.first().at(0).toBool());
    QVERIFY(!QFileInfo::exists(outputDir.filePath(QStringLiteral("manifest.json.gz"))));
    QVERIFY(!QFileInfo::exists(outputDir.filePath(QStringLiteral("manifest.json"))));

    // Saved under another name, so that a result of the cancelled decompression would show
    downloader.setOutputPath(outputDir.filePath(QStringLiteral("next.json.gz")));
    QVERIFY(downloader.start(":/unittest/manifest.json.gz"));
    blocker.release();
    QVERIFY_SIGNAL_COUNT_WAIT(finishedSpy, 2, TestTimeout::mediumMs());
    QTRY_VERIFY_WITH_TIMEOUT(job.isNull(), TestTimeout::mediumMs());
    QCOMPARE(finishedSpy.count(), 2);
    const QList<QVariant> args = finishedSpy.last();
    QVERIFY2(args.at(0).toBool(), qPrintable(args.at(2).toString()));
    const QString decompressedPath = outputDir.filePath(QStringLiteral("next.json"));
    QCOMPARE(args.at(1).toString(), decompressedPath);
    QVERIFY(TestFixtures::readFile(decompressedPath).contains("\"name\""));
    QCOMPARE(QDir(outputDir.path()).entryList(QDir::Files | QDir::Hidden), QStringList{QStringLiteral("next.json")});
}

// ============================================================================
// Integration Tests (Download + Compression)
// ============================================================================
void QGCFileDownloadTest::_testDownloadAndExtractZip()
{
    QTemporaryDir tempDir;
    // Integration test: Download a ZIP archive and extract it
    // This tests the full workflow of QGCFileDownload + Compression
    QGCFileDownload* const downloader = new QGCFileDownload(this);
    QString resultLocalFile;
    QString resultErrorMsg;
    (void)connect(downloader, &QGCFileDownload::finished, this,
                  [&resultLocalFile, &resultErrorMsg](bool success, const QString& localFile, const QString& errorMsg) {
                      QVERIFY(success);
                      resultLocalFile = localFile;
                      resultErrorMsg = errorMsg;
                  });
    QSignalSpy spy(downloader, &QGCFileDownload::finished);
    QVERIFY(downloader->start(":/unittest/manifest.json.zip"));
    QVERIFY_SIGNAL_WAIT(spy, TestTimeout::mediumMs());
    downloader->deleteLater();
    // Verify download succeeded
    QVERIFY2(resultErrorMsg.isEmpty(), qPrintable(resultErrorMsg));
    QVERIFY2(QFile::exists(resultLocalFile), qPrintable("Downloaded file doesn't exist: " + resultLocalFile));
    // Create temp directory for extraction
    const QString extractDir = tempDir.path() + "/extract_zip";
    QDir().mkpath(extractDir);
    QVERIFY(!extractDir.isEmpty());
    // Extract the downloaded archive using Compression
    const Compression::Outcome extractResult = Compression::extractArchive(resultLocalFile, extractDir);
    QVERIFY2(extractResult, qPrintable("Extraction failed: " + extractResult.message));
    // Verify extracted content
    const QString extractedFile = extractDir + "/manifest.json";
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

UT_REGISTER_TEST(QGCFileDownloadTest, TestLabel::Integration, TestLabel::Utilities)
