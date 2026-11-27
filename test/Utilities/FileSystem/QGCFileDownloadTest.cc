#include "QGCFileDownloadTest.h"

#include <memory>

#include <QtCore/QDir>
#include <QtCore/QDirIterator>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QPointer>
#include <QtCore/QRegularExpression>
#include <QtCore/QTemporaryDir>
#include <QtCore/QThreadPool>
#include <QtTest/QSignalSpy>

#include "ArchiveFixtures.h"
#include "Compression.h"
#include "CompressionJob.h"
#include "FileFixtures.h"
#include "LocalHttpTestServer.h"
#include "ProcessFixtures.h"
#include "QGCFileDownload.h"
#include "QGCFileHelper.h"
#include "QGCWorkDirectory.h"

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
    QCOMPARE(ArchiveFixtures::entriesBelow(outputDir.path()), QStringList{QStringLiteral("manifest.json.gz")});
    QCOMPARE(TestFixtures::readFile(args.at(1).toString()), TestFixtures::readFile(":/unittest/manifest.json.gz"));
}

void QGCFileDownloadTest::_testFileDownloadNameFromUrl_data()
{
    QTest::addColumn<QString>("path");
    // Name of the file in the output directory, the only file written
    QTest::addColumn<QString>("fileName");

    // Decoded, "%5C" is a backslash, a separator on Windows
    QTest::newRow("encoded backslashes") << "/a/..%5C..%5Cescaped.json" << "escaped.json";
    // Names that Windows writes as something else take the name of a download without one
    QTest::newRow("relative to a drive") << "/a/C:escaped.json" << "DownloadedFile";
    QTest::newRow("a device") << "/a/COM1.json" << "DownloadedFile";
}

// The URL names the file in the output directory, never where it goes, and the same way on every system
void QGCFileDownloadTest::_testFileDownloadNameFromUrl()
{
    QFETCH(QString, path);
    QFETCH(QString, fileName);

    QTemporaryDir root;
    QVERIFY(root.isValid());
    // Two levels down, so that a name that climbs two levels would still land in the temporary directory
    const QString outputDirectory = root.filePath(QStringLiteral("a/b"));
    QVERIFY(QDir().mkpath(outputDirectory));
    TestFixtures::LocalHttpTestServer server;
    QVERIFY(server.listen());
    const QByteArray body("{}");
    server.installHttpResponder(body);

    QGCFileDownload downloader(this);
    downloader.setOutputDirectory(outputDirectory);
    QSignalSpy finishedSpy(&downloader, &QGCFileDownload::finished);
    QVERIFY(downloader.start(server.url(path)));
    QVERIFY_SIGNAL_WAIT(finishedSpy, TestTimeout::mediumMs());

    const QList<QVariant> args = finishedSpy.first();
    QVERIFY2(args.at(0).toBool(), qPrintable(args.at(2).toString()));
    QCOMPARE(args.at(1).toString(), QDir(outputDirectory).filePath(fileName));
    QStringList written;
    QDirIterator files(root.path(), QDir::Files | QDir::Hidden, QDirIterator::Subdirectories);
    while (files.hasNext()) {
        written.append(QDir(root.path()).relativeFilePath(files.next()));
    }
    QCOMPARE(written, QStringList{QStringLiteral("a/b/") + fileName});
    QCOMPARE(TestFixtures::readFile(args.at(1).toString()), body);
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

// A handler of finished() may start the next download when one is cancelled: the cancelled download is cleaned up
// before the signal, where after it the cleanup would end the new one
void QGCFileDownloadTest::_testFileDownloadRestartFromCancelHandler()
{
    QTemporaryDir outputDir;
    QVERIFY(outputDir.isValid());
    QGCFileDownload downloader(this);
    downloader.setOutputDirectory(outputDir.path());
    QSignalSpy finishedSpy(&downloader, &QGCFileDownload::finished);
    bool restarted = false;
    (void) connect(&downloader, &QGCFileDownload::finished, this, [&downloader, &restarted](bool success) {
        if (!success && !restarted) {
            restarted = downloader.start(":/unittest/arducopter.apj");
        }
    });

    QVERIFY(downloader.start(":/unittest/manifest.json.gz"));
    downloader.cancel();
    QVERIFY(restarted);
    QVERIFY_SIGNAL_COUNT_WAIT(finishedSpy, 2, TestTimeout::mediumMs());

    QVERIFY(!finishedSpy.first().at(0).toBool());
    const QList<QVariant> args = finishedSpy.last();
    QVERIFY2(args.at(0).toBool(), qPrintable(args.at(2).toString()));
    QCOMPARE(args.at(1).toString(), outputDir.filePath(QStringLiteral("arducopter.apj")));
    QCOMPARE(TestFixtures::readFile(args.at(1).toString()), TestFixtures::readFile(":/unittest/arducopter.apj"));
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
        expectLogMessage("Utilities.QGCFileDownload", QtWarningMsg, QRegularExpression("The file is larger than"));
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
        QVERIFY2(args.at(2).toString().startsWith(QStringLiteral("The file is larger than")),
                 qPrintable(args.at(2).toString()));
        QCOMPARE(ArchiveFixtures::entriesBelow(downloads.path()), QStringList());
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

    // The test manifest is a few bytes
    constexpr qint64 maxManifestBytes = 1024;
    QByteArray manifest;
    QVERIFY(
        Compression::decompressFileToMemory(QStringLiteral(":/unittest/manifest.json.gz"), manifest, maxManifestBytes));
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
    QCOMPARE(ArchiveFixtures::entriesBelow(outputDir.path()), QStringList{resultName});
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
    QTest::newRow("over the limit") << compressed << qint64{1024} << ": the data is larger than the size limit$";
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
    QCOMPARE(ArchiveFixtures::entriesBelow(outputDir.path()), QStringList());
}

// Cancelling a download while its file is decompressed ends it at once, and the decompression leaves no file once it
// has ended. The decompression, which still waits for a thread, does not complete the next download.
void QGCFileDownloadTest::_testAutoDecompressCancelled()
{
    QTemporaryDir outputDir;
    QVERIFY(outputDir.isValid());

    TestFixtures::ThreadPoolBlocker blocker(CompressionJob::threadPool());
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

    // cancel() returns without waiting for the worker, which the blocker holds
    downloader.cancel();
    QVERIFY(job);
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
    QCOMPARE(ArchiveFixtures::entriesBelow(outputDir.path()), QStringList{QStringLiteral("next.json")});
}

void QGCFileDownloadTest::_testAutoDecompressEndedWithoutResult_data()
{
    QTest::addColumn<bool>("destroy");
    QTest::addColumn<bool>("workerFinished");

    QTest::newRow("cancelled after its worker finished") << false << true;
    QTest::newRow("destroyed before its worker starts") << true << false;
    QTest::newRow("destroyed after its worker finished") << true << true;
}

// A download that is cancelled or destroyed while its file is decompressed leaves nothing, also once the worker has
// written the decompressed file
void QGCFileDownloadTest::_testAutoDecompressEndedWithoutResult()
{
    QFETCH(bool, destroy);
    QFETCH(bool, workerFinished);

    QTemporaryDir outputDir;
    QVERIFY(outputDir.isValid());

    // Holds the worker back until the download is decompressing
    TestFixtures::ThreadPoolBlocker blocker(CompressionJob::threadPool());
    QVERIFY(blocker.isBlocking());

    auto downloader = std::make_unique<QGCFileDownload>();
    bool succeeded = false;
    (void) connect(downloader.get(), &QGCFileDownload::finished, this,
                   [&succeeded](bool success, const QString&, const QString&) { succeeded = succeeded || success; });
    downloader->setAutoDecompress(true);
    downloader->setOutputPath(outputDir.filePath(QStringLiteral("manifest.json.gz")));
    QVERIFY(downloader->start(":/unittest/manifest.json.gz"));
    QTRY_COMPARE_WITH_TIMEOUT(downloader->state(), QGCFileDownload::State::Decompressing, TestTimeout::mediumMs());

    if (workerFinished) {
        // No event is handled here, so the download cannot learn what the worker has done
        blocker.release();
        QVERIFY(CompressionJob::threadPool()->waitForDone(TestTimeout::mediumMs()));
    }
    if (destroy) {
        downloader.reset();
    } else {
        downloader->cancel();
    }
    blocker.release();

    // A cancelled job ends on its own, and its work directory goes with it
    QVERIFY(!succeeded);
    QTRY_COMPARE_WITH_TIMEOUT(ArchiveFixtures::entriesBelow(outputDir.path()), QStringList(), TestTimeout::mediumMs());
}

// A download that is cancelled while its worker decompresses leaves nothing, also on Windows, where a file that the
// worker has open cannot be removed
void QGCFileDownloadTest::_testAutoDecompressCancelledWhileRunning()
{
    QTemporaryDir sourceDir;
    QTemporaryDir outputDir;
    QVERIFY(sourceDir.isValid());
    QVERIFY(outputDir.isValid());
    // Expands to 512 MiB, so the worker is still busy when its first progress report is handled here
    const QString sourcePath = sourceDir.filePath(QStringLiteral("zeros.gz"));
    QVERIFY(ArchiveFixtures::writeGzipOfZeros(sourcePath, 8192));

    QGCFileDownload downloader(this);
    QSignalSpy finishedSpy(&downloader, &QGCFileDownload::finished);
    downloader.setAutoDecompress(true);
    downloader.setOutputDirectory(outputDir.path());
    qreal firstProgress = 0.0;
    (void) connect(&downloader, &QGCFileDownload::decompressionProgress, this,
                   [&downloader, &firstProgress](qreal progress) {
                       if (qFuzzyIsNull(firstProgress)) {
                           firstProgress = progress;
                           downloader.cancel();
                       }
                   });
    QVERIFY(downloader.start(sourcePath));
    QVERIFY_SIGNAL_WAIT(finishedSpy, TestTimeout::longMs());

    QVERIFY(!finishedSpy.first().at(0).toBool());
    QVERIFY(firstProgress > 0.0);
    QVERIFY(firstProgress < 1.0);
    QCOMPARE(downloader.state(), QGCFileDownload::State::Cancelled);
    QTRY_COMPARE_WITH_TIMEOUT(ArchiveFixtures::entriesBelow(outputDir.path()), QStringList(), TestTimeout::mediumMs());
}

void QGCFileDownloadTest::_testAutoDecompressLeavesNewerFile_data()
{
    QTest::addColumn<bool>("cancel");

    QTest::newRow("completed") << false;
    QTest::newRow("cancelled") << true;
}

// A file written to the path of the download while it is decompressed, such as the next download of that name, is
// not the download's: it neither reads nor removes it, however the download ends
void QGCFileDownloadTest::_testAutoDecompressLeavesNewerFile()
{
    QFETCH(bool, cancel);

    QTemporaryDir outputDir;
    QVERIFY(outputDir.isValid());
    const QString downloadPath = outputDir.filePath(QStringLiteral("manifest.json.gz"));

    // Holds the worker back until the newer file is there
    TestFixtures::ThreadPoolBlocker blocker(CompressionJob::threadPool());
    QVERIFY(blocker.isBlocking());

    QGCFileDownload downloader(this);
    QSignalSpy finishedSpy(&downloader, &QGCFileDownload::finished);
    downloader.setAutoDecompress(true);
    downloader.setOutputPath(downloadPath);
    QVERIFY(downloader.start(":/unittest/manifest.json.gz"));
    QTRY_COMPARE_WITH_TIMEOUT(downloader.state(), QGCFileDownload::State::Decompressing, TestTimeout::mediumMs());
    const QByteArray newer = TestFixtures::gzipMember("newer");
    QVERIFY(TestFixtures::writeFile(downloadPath, newer));
    if (cancel) {
        downloader.cancel();
    }
    blocker.release();

    QVERIFY_SIGNAL_COUNT_WAIT(finishedSpy, 1, TestTimeout::mediumMs());
    const auto entries = [&outputDir]() { return ArchiveFixtures::entriesBelow(outputDir.path()); };
    if (cancel) {
        QVERIFY(!finishedSpy.first().at(0).toBool());
        // Once the cancelled job has ended
        QTRY_COMPARE_WITH_TIMEOUT(entries(), QStringList{QStringLiteral("manifest.json.gz")}, TestTimeout::mediumMs());
    } else {
        QVERIFY2(finishedSpy.first().at(0).toBool(), qPrintable(finishedSpy.first().at(2).toString()));
        QVERIFY(TestFixtures::readFile(outputDir.filePath(QStringLiteral("manifest.json"))).contains("\"name\""));
        QCOMPARE(entries(), (QStringList{QStringLiteral("manifest.json"), QStringLiteral("manifest.json.gz")}));
    }
    QCOMPARE(TestFixtures::readFile(downloadPath), newer);
}

void QGCFileDownloadTest::_testAutoDecompressReplacesExistingFile_data()
{
    // What fails: nothing, the transfer, the decompression, or the move of the result into place
    QTest::addColumn<QString>("failure");
    // Start of the warning the failure logs
    QTest::addColumn<QString>("warning");

    QTest::newRow("nothing") << QString() << QString();
    QTest::newRow("transfer") << QStringLiteral("transfer") << QStringLiteral("The file is larger than");
    QTest::newRow("decompression") << QStringLiteral("decompression") << QStringLiteral("Decompression failed");
    QTest::newRow("move into place") << QStringLiteral("move") << QStringLiteral("Cannot write the downloaded file");
}

// A file already at the path of the result is replaced in one step when the download succeeds, and stays as it was
// when the download fails, at whatever step
void QGCFileDownloadTest::_testAutoDecompressReplacesExistingFile()
{
    QFETCH(QString, failure);
    QFETCH(QString, warning);

    QTemporaryDir sourceDir;
    QTemporaryDir outputDir;
    QVERIFY(sourceDir.isValid());
    QVERIFY(outputDir.isValid());
    const QString resultPath = outputDir.filePath(QStringLiteral("manifest.json"));
    QVERIFY(TestFixtures::writeFile(resultPath, "existing"));
    QString source = QStringLiteral(":/unittest/manifest.json.gz");
    if (failure == QStringLiteral("decompression")) {
        const QByteArray compressed = TestFixtures::gzipMember(QByteArray(4096, 'x'));
        source = sourceDir.filePath(QStringLiteral("manifest.json.gz"));
        QVERIFY(TestFixtures::writeFile(source, compressed.first(compressed.size() / 2)));
    }

    // For the move: holds the worker back, so that its result can be taken away before the download moves it
    std::unique_ptr<TestFixtures::ThreadPoolBlocker> blocker;
    if (failure == QStringLiteral("move")) {
        blocker = std::make_unique<TestFixtures::ThreadPoolBlocker>(CompressionJob::threadPool());
        QVERIFY(blocker->isBlocking());
    }

    QGCFileDownload downloader(this);
    QSignalSpy finishedSpy(&downloader, &QGCFileDownload::finished);
    downloader.setAutoDecompress(true);
    downloader.setOutputDirectory(outputDir.path());
    if (failure == QStringLiteral("transfer")) {
        downloader.setMaxBytes(1);
    }
    if (!warning.isEmpty()) {
        expectLogMessage("Utilities.QGCFileDownload", QtWarningMsg, QRegularExpression(warning));
    }
    QVERIFY(downloader.start(source));

    if (blocker) {
        QTRY_COMPARE_WITH_TIMEOUT(downloader.state(), QGCFileDownload::State::Decompressing, TestTimeout::mediumMs());
        blocker->release();
        // No event is handled here, so the download has not seen the result yet
        QVERIFY(CompressionJob::threadPool()->waitForDone(TestTimeout::mediumMs()));
        const QStringList workDirectories =
            QDir(outputDir.path()).entryList({QStringLiteral(".qgc-download-*")}, QDir::Dirs | QDir::Hidden);
        QCOMPARE(workDirectories.size(), 1);
        QVERIFY(
            QFile::remove(QDir(outputDir.filePath(workDirectories.first())).filePath(QStringLiteral("manifest.json"))));
    }

    QVERIFY_SIGNAL_WAIT(finishedSpy, TestTimeout::mediumMs());
    if (!warning.isEmpty()) {
        verifyExpectedLogMessage();
    }
    const QList<QVariant> args = finishedSpy.first();
    QCOMPARE(args.at(0).toBool(), failure.isEmpty());
    if (failure.isEmpty()) {
        QVERIFY(TestFixtures::readFile(resultPath).contains("\"name\""));
    } else {
        QCOMPARE(TestFixtures::readFile(resultPath), QByteArray("existing"));
    }
    QCOMPARE(ArchiveFixtures::entriesBelow(outputDir.path()), QStringList{QStringLiteral("manifest.json")});
}

// What a download that crashed left beside the output path is removed when the next one starts. The work directory of a
// download that another QGC runs stays
void QGCFileDownloadTest::_testAutoDecompressRemovesStaleDirectories()
{
    QTemporaryDir outputDir;
    QVERIFY(outputDir.isValid());
    const QDir output(outputDir.path());
    const QString stale = QStringLiteral(".qgc-download-stale");
    QVERIFY(output.mkdir(stale));
    QVERIFY(TestFixtures::writeFile(output.filePath(stale + QStringLiteral("/manifest.json.gz")), "left"));
    const QGCWorkDirectory running(outputDir.path(), QStringLiteral(".qgc-download-"));
    QVERIFY(running.isValid());

    QGCFileDownload downloader(this);
    QSignalSpy finishedSpy(&downloader, &QGCFileDownload::finished);
    downloader.setAutoDecompress(true);
    downloader.setOutputDirectory(outputDir.path());
    QVERIFY(downloader.start(":/unittest/manifest.json.gz"));
    QVERIFY_SIGNAL_WAIT(finishedSpy, TestTimeout::mediumMs());
    QVERIFY2(finishedSpy.first().at(0).toBool(), qPrintable(finishedSpy.first().at(2).toString()));

    QCOMPARE(output.entryList(QDir::AllEntries | QDir::Hidden | QDir::NoDotAndDotDot, QDir::Name),
             (QStringList{QFileInfo(running.path()).fileName(), QStringLiteral("manifest.json")}));
}

UT_REGISTER_TEST(QGCFileDownloadTest, TestLabel::Integration, TestLabel::Utilities)
