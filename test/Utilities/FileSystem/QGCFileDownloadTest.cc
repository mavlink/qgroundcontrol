#include "QGCFileDownloadTest.h"

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QRegularExpression>
#include <QtCore/QStandardPaths>
#include <QtCore/QTemporaryDir>
#include <QtCore/QTemporaryFile>
#include <QtTest/QSignalSpy>

#include "FileFixtures.h"
#include "LocalHttpTestServer.h"
#include "QGCCompression.h"
#include "QGCFileDownload.h"
#include "QGCFileHelper.h"

namespace {

/// True if the file that carried a download to memory, and the directory made for it, are gone
bool downloadToMemoryLeftNothing(const QGCFileDownload& downloader)
{
    const QFileInfo carrier(downloader.localPath());
    return !downloader.localPath().isEmpty() && !carrier.exists() && !carrier.dir().exists();
}

enum DownloadTestSource
{
    DownloadFromHttp,
    DownloadFromRedirect,
    DownloadFromResource,
};

enum DownloadTestFailure
{
    DownloadFailsHttp404,
    DownloadFailsHttp500,
    DownloadFailsConnection,
    DownloadFailsTruncated,
    DownloadFailsMissingFile,
    DownloadFailsHash,
    DownloadFailsDecompression,
};

/// What a server sends for a download, under a name that ends in .gz unless the body is DownloadBodyPlain
enum DownloadTestBody
{
    DownloadBodyPlain,            ///< The content, under a name that tells nothing
    DownloadBodyGzip,             ///< The content as a gzip stream
    DownloadBodyPlainNamedGzip,   ///< The content as it is
    DownloadBodyDecodedByClient,  ///< The gzip stream as a content encoding, which the HTTP client takes off
    DownloadBodyTruncatedGzip,    ///< The first half of the gzip stream
};

QByteArray downloadTestRedirectResponse(int httpStatus, const QString& location)
{
    return "HTTP/1.1 " + QByteArray::number(httpStatus) + " Redirect\r\nLocation: " + location.toUtf8() +
           "\r\nConnection: close\r\nContent-Length: 0\r\n\r\n";
}

}  // namespace

void QGCFileDownloadTest::_testFileDownload()
{
    QGCFileDownload downloader(this);
    QSignalSpy finishedSpy(&downloader, &QGCFileDownload::finished);

    QVERIFY(downloader.start(":/unittest/arducopter.apj"));
    QVERIFY_SIGNAL_WAIT(finishedSpy, TestTimeout::shortMs());
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
    QVERIFY(!QFile::exists(downloader.localPath()));
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

    downloader.setAutoDecompress(true);
    QVERIFY(downloader.start(":/unittest/manifest.json.gz"));
    downloader.cancel();

    QTRY_COMPARE_WITH_TIMEOUT(finishedSpy.count(), 1, TestTimeout::mediumMs());
    QVERIFY(!downloader.isRunning());

    const QList<QVariant> args = finishedSpy.first();
    QVERIFY(!args.at(0).toBool());
    QVERIFY(!QFile::exists(downloader.localPath()));
}

void QGCFileDownloadTest::_testFileDownloadSameFileNameStaysSeparate()
{
    QTemporaryDir sourceDir;
    const QString fileName = QStringLiteral("same-name.apj");
    const QByteArray firstPayload("first source");
    const QByteArray secondPayload("second source");
    QVERIFY(QDir(sourceDir.path()).mkpath(QStringLiteral("first")));
    QVERIFY(QDir(sourceDir.path()).mkpath(QStringLiteral("second")));
    const QString firstSource = sourceDir.filePath(QStringLiteral("first/") + fileName);
    const QString secondSource = sourceDir.filePath(QStringLiteral("second/") + fileName);
    QVERIFY(TestFixtures::writeFile(firstSource, firstPayload));
    QVERIFY(TestFixtures::writeFile(secondSource, secondPayload));

    QGCFileDownload firstDownloader(this);
    QGCFileDownload secondDownloader(this);
    QSignalSpy firstFinishedSpy(&firstDownloader, &QGCFileDownload::finished);
    QSignalSpy secondFinishedSpy(&secondDownloader, &QGCFileDownload::finished);

    QVERIFY(firstDownloader.start(firstSource));
    QVERIFY(secondDownloader.start(secondSource));
    QVERIFY_SIGNAL_COUNT_WAIT(firstFinishedSpy, 1, TestTimeout::mediumMs());
    QVERIFY_SIGNAL_COUNT_WAIT(secondFinishedSpy, 1, TestTimeout::mediumMs());
    QVERIFY(firstFinishedSpy.first().at(0).toBool());
    QVERIFY(secondFinishedSpy.first().at(0).toBool());

    // Callers tell the file type from the name, so only the directory may differ
    const QString firstPath = firstFinishedSpy.first().at(1).toString();
    const QString secondPath = secondFinishedSpy.first().at(1).toString();
    QCOMPARE(QFileInfo(firstPath).fileName(), fileName);
    QCOMPARE(QFileInfo(secondPath).fileName(), fileName);
    QCOMPARE(TestFixtures::readFile(firstPath), firstPayload);
    QCOMPARE(TestFixtures::readFile(secondPath), secondPayload);

    QFile::remove(firstPath);
    QFile::remove(secondPath);
}

void QGCFileDownloadTest::_testFileDownloadLocalSourceInTempDirSurvives()
{
    const QByteArray payload("source in the temp directory");
    QTemporaryFile source(QDir(QStandardPaths::writableLocation(QStandardPaths::TempLocation))
                              .filePath(QStringLiteral("qgc_download_source_XXXXXX.bin")));
    QVERIFY(source.open());
    QCOMPARE(source.write(payload), payload.size());
    source.close();

    QGCFileDownload downloader(this);
    QSignalSpy finishedSpy(&downloader, &QGCFileDownload::finished);
    QVERIFY(downloader.start(source.fileName()));
    QVERIFY_SIGNAL_WAIT(finishedSpy, TestTimeout::mediumMs());
    QVERIFY(finishedSpy.first().at(0).toBool());

    const QString localPath = finishedSpy.first().at(1).toString();
    QCOMPARE(TestFixtures::readFile(source.fileName()), payload);
    QCOMPARE(TestFixtures::readFile(localPath), payload);

    QFile::remove(localPath);
}

void QGCFileDownloadTest::_testFileDownloadFailureRemovesPartialFile_data()
{
    QTest::addColumn<bool>("customOutputPath");
    QTest::newRow("default-path") << false;
    QTest::newRow("custom-path") << true;
}

void QGCFileDownloadTest::_testFileDownloadFailureRemovesPartialFile()
{
    QFETCH(bool, customOutputPath);

    // Announces more than it sends, so the download fails after part of the body was written
    TestFixtures::LocalHttpTestServer server;
    QVERIFY2(server.listen(), "Could not start local test HTTP server");
    server.installRawResponder("HTTP/1.1 200 OK\r\nConnection: close\r\nContent-Length: 64\r\n\r\npartial body");

    QTemporaryDir outputDir;
    QGCFileDownload downloader(this);
    if (customOutputPath) {
        downloader.setOutputPath(outputDir.filePath(QStringLiteral("partial.bin")));
    }
    QSignalSpy finishedSpy(&downloader, &QGCFileDownload::finished);

    expectLogMessage("Utilities.QGCFileDownload", QtWarningMsg, QRegularExpression("Download error:"));
    QVERIFY(downloader.start(server.url(QStringLiteral("/partial.bin"))));
    QVERIFY_SIGNAL_WAIT(finishedSpy, TestTimeout::mediumMs());
    verifyExpectedLogMessage();
    QVERIFY(!finishedSpy.first().at(0).toBool());
    QVERIFY(downloader.bytesReceived() > 0);

    const QFileInfo output(downloader.localPath());
    QVERIFY(!output.exists());
    // Only a directory that was made for the download goes with it
    QCOMPARE(output.dir().exists(), customOutputPath);
}

void QGCFileDownloadTest::_testFileDownloadHttpErrorStatus_data()
{
    QTest::addColumn<int>("httpStatus");
    QTest::addColumn<QString>("reason");  // the reason phrase of the server, which the reported error names

    QTest::newRow("400") << 400 << QStringLiteral("Bad Request");
    QTest::newRow("404") << 404 << QStringLiteral("Not Found");
    QTest::newRow("500") << 500 << QStringLiteral("Internal Server Error");
}

void QGCFileDownloadTest::_testFileDownloadHttpErrorStatus()
{
    QFETCH(int, httpStatus);
    QFETCH(QString, reason);

    TestFixtures::LocalHttpTestServer server;
    QVERIFY2(server.listen(), "Could not start local test HTTP server");
    server.installHttpResponder("error page", httpStatus, "text/plain");

    QGCFileDownload downloader(this);
    QSignalSpy finishedSpy(&downloader, &QGCFileDownload::finished);

    expectLogMessage("Utilities.QGCFileDownload", QtWarningMsg, QRegularExpression("Download error:"));
    QVERIFY(downloader.start(server.url(QStringLiteral("/status.bin"))));
    QVERIFY_SIGNAL_WAIT(finishedSpy, TestTimeout::mediumMs());
    verifyExpectedLogMessage();
    QCOMPARE(finishedSpy.count(), 1);

    const QList<QVariant> args = finishedSpy.first();
    QVERIFY(!args.at(0).toBool());
    QVERIFY(args.at(1).toString().isEmpty());
    QVERIFY2(args.at(2).toString().contains(reason), qPrintable(args.at(2).toString()));
    QCOMPARE(downloader.state(), QGCFileDownload::State::Failed);
    QVERIFY(!downloader.errorString().isEmpty());
    // The error page is not a download
    QVERIFY(!QFileInfo::exists(downloader.localPath()));
}

void QGCFileDownloadTest::_testFileDownloadFollowsRedirect_data()
{
    QTest::addColumn<int>("httpStatus");

    QTest::newRow("301") << 301;
    QTest::newRow("302") << 302;
    QTest::newRow("303") << 303;
    QTest::newRow("307") << 307;
    QTest::newRow("308") << 308;
}

void QGCFileDownloadTest::_testFileDownloadFollowsRedirect()
{
    QFETCH(int, httpStatus);

    const QByteArray payload("redirect target");
    TestFixtures::LocalHttpTestServer targetServer;
    QVERIFY2(targetServer.listen(), "Could not start local test HTTP server");
    targetServer.installHttpResponder(payload);
    TestFixtures::LocalHttpTestServer redirectingServer;
    QVERIFY2(redirectingServer.listen(), "Could not start local test HTTP server");
    redirectingServer.installRawResponder(
        downloadTestRedirectResponse(httpStatus, targetServer.url(QStringLiteral("/target.bin"))));

    QGCFileDownload downloader(this);
    QSignalSpy finishedSpy(&downloader, &QGCFileDownload::finished);
    QVERIFY(downloader.start(redirectingServer.url(QStringLiteral("/moved.bin"))));
    QVERIFY_SIGNAL_WAIT(finishedSpy, TestTimeout::mediumMs());

    const QList<QVariant> args = finishedSpy.first();
    QVERIFY2(args.at(0).toBool(), qPrintable(args.at(2).toString()));
    const QString localPath = args.at(1).toString();
    QCOMPARE(TestFixtures::readFile(localPath), payload);
    // Callers tell the file type from the name, which stays the one that was asked for
    QCOMPARE(QFileInfo(localPath).fileName(), QStringLiteral("moved.bin"));

    QFile::remove(localPath);
}

void QGCFileDownloadTest::_testFileDownloadRefusesRedirect_data()
{
    QTest::addColumn<bool>("allowRedirects");
    QTest::addColumn<bool>("toLocalFile");
    QTest::addColumn<QString>("warning");  // empty: the redirect itself is the reply, which is not a network error
    QTest::addColumn<QString>("error");

    QTest::newRow("redirects not allowed") << false << false << QString() << QStringLiteral("^HTTP error 302: Found$");
    QTest::newRow("to a local file") << true << true << QStringLiteral("Download error:") << QStringLiteral("protocol");
}

void QGCFileDownloadTest::_testFileDownloadRefusesRedirect()
{
    QFETCH(bool, allowRedirects);
    QFETCH(bool, toLocalFile);
    QFETCH(QString, warning);
    QFETCH(QString, error);

    const QByteArray payload("redirect target");
    QTemporaryDir targetDir;
    const QString targetFile = targetDir.filePath(QStringLiteral("target.bin"));
    QVERIFY(TestFixtures::writeFile(targetFile, payload));
    TestFixtures::LocalHttpTestServer targetServer;
    QVERIFY2(targetServer.listen(), "Could not start local test HTTP server");
    targetServer.installHttpResponder(payload);
    const QString location =
        toLocalFile ? QUrl::fromLocalFile(targetFile).toString() : targetServer.url(QStringLiteral("/target.bin"));
    TestFixtures::LocalHttpTestServer redirectingServer;
    QVERIFY2(redirectingServer.listen(), "Could not start local test HTTP server");
    redirectingServer.installRawResponder(downloadTestRedirectResponse(302, location));

    QGCNetworkHelper::RequestConfig config;
    config.allowRedirects = allowRedirects;
    QGCFileDownload downloader(this);
    QSignalSpy finishedSpy(&downloader, &QGCFileDownload::finished);
    if (!warning.isEmpty()) {
        expectLogMessage("Utilities.QGCFileDownload", QtWarningMsg, QRegularExpression(warning));
    }
    QVERIFY(downloader.start(redirectingServer.url(QStringLiteral("/moved.bin")), config));
    QVERIFY_SIGNAL_WAIT(finishedSpy, TestTimeout::mediumMs());
    if (!warning.isEmpty()) {
        verifyExpectedLogMessage();
    }

    const QList<QVariant> args = finishedSpy.first();
    QVERIFY(!args.at(0).toBool());
    QVERIFY(args.at(1).toString().isEmpty());
    QVERIFY2(QRegularExpression(error).match(args.at(2).toString()).hasMatch(), qPrintable(args.at(2).toString()));
    QVERIFY(!QFileInfo::exists(downloader.localPath()));
}

void QGCFileDownloadTest::_testStartToMemoryDeliversContent_data()
{
    QTest::addColumn<int>("source");

    QTest::newRow("http") << int(DownloadFromHttp);
    QTest::newRow("redirect") << int(DownloadFromRedirect);
    QTest::newRow("resource") << int(DownloadFromResource);
}

void QGCFileDownloadTest::_testStartToMemoryDeliversContent()
{
    QFETCH(int, source);

    QByteArray payload("content that is wanted in memory");
    TestFixtures::LocalHttpTestServer server;
    TestFixtures::LocalHttpTestServer redirectingServer;
    QString url;
    if (source == DownloadFromResource) {
        url = QStringLiteral(":/unittest/arducopter.apj");
        payload = TestFixtures::readFile(url);
        QVERIFY(!payload.isEmpty());
    } else {
        QVERIFY2(server.listen(), "Could not start local test HTTP server");
        server.installHttpResponder(payload);
        url = server.url(QStringLiteral("/content.bin"));
        if (source == DownloadFromRedirect) {
            QVERIFY2(redirectingServer.listen(), "Could not start local test HTTP server");
            redirectingServer.installRawResponder(downloadTestRedirectResponse(302, url));
            url = redirectingServer.url(QStringLiteral("/moved.bin"));
        }
    }

    QTemporaryDir outputDir;
    const QString outputPath = outputDir.filePath(QStringLiteral("not-used.bin"));
    QGCFileDownload downloader(this);
    downloader.setOutputPath(outputPath);
    QSignalSpy memorySpy(&downloader, &QGCFileDownload::finishedToMemory);
    QSignalSpy fileSpy(&downloader, &QGCFileDownload::finished);

    QVERIFY(downloader.startToMemory(url));
    QVERIFY(downloader.isRunning());
    QVERIFY_SIGNAL_WAIT(memorySpy, TestTimeout::mediumMs());
    QCOMPARE(memorySpy.count(), 1);

    const QList<QVariant> args = memorySpy.first();
    QVERIFY2(args.at(0).toBool(), qPrintable(args.at(2).toString()));
    QCOMPARE(args.at(1).toByteArray(), payload);
    QVERIFY(args.at(2).toString().isEmpty());
    QCOMPARE(fileSpy.count(), 0);
    QCOMPARE(downloader.state(), QGCFileDownload::State::Completed);
    QVERIFY(downloadToMemoryLeftNothing(downloader));
    QVERIFY(!QFile::exists(outputPath));
}

void QGCFileDownloadTest::_testStartToMemoryFailsAsStart_data()
{
    QTest::addColumn<int>("failure");
    QTest::addColumn<QStringList>("warnings");  // of each of the two downloads, as "category|pattern"

    const QStringList downloadError{QStringLiteral("Utilities.QGCFileDownload|Download error:")};
    QTest::newRow("HTTP 404") << int(DownloadFailsHttp404) << downloadError;
    QTest::newRow("HTTP 500") << int(DownloadFailsHttp500) << downloadError;
    QTest::newRow("connection refused") << int(DownloadFailsConnection) << downloadError;
    QTest::newRow("truncated body") << int(DownloadFailsTruncated) << downloadError;
    QTest::newRow("missing local file") << int(DownloadFailsMissingFile) << downloadError;
    QTest::newRow("hash mismatch") << int(DownloadFailsHash) << QStringList();
    QTest::newRow("damaged compressed file")
        << int(DownloadFailsDecompression)
        << QStringList{QStringLiteral("Utilities.QGClibarchive|truncated gzip input"),
                       QStringLiteral("Utilities.QGCFileDownload|Decompression failed:")};
}

void QGCFileDownloadTest::_testStartToMemoryFailsAsStart()
{
    QFETCH(int, failure);
    QFETCH(QStringList, warnings);

    TestFixtures::LocalHttpTestServer server;
    QVERIFY2(server.listen(), "Could not start local test HTTP server");
    QTemporaryDir sourceDir;
    QString url = server.url(QStringLiteral("/failing.bin"));
    QString expectedHash;
    bool autoDecompress = false;
    switch (failure) {
        case DownloadFailsHttp404:
            server.installHttpResponder("error page", 404, "text/plain");
            break;
        case DownloadFailsHttp500:
            server.installHttpResponder("error page", 500, "text/plain");
            break;
        case DownloadFailsConnection:
            server.close();
            break;
        case DownloadFailsTruncated:
            server.installRawResponder(
                "HTTP/1.1 200 OK\r\nConnection: close\r\nContent-Length: 64\r\n\r\npartial body");
            break;
        case DownloadFailsMissingFile:
            url = sourceDir.filePath(QStringLiteral("missing.bin"));
            break;
        case DownloadFailsHash:
            server.installHttpResponder("content with another hash");
            expectedHash = QString(64, QLatin1Char('0'));
            break;
        case DownloadFailsDecompression: {
            const QByteArray gzip = TestFixtures::gzipMember(QByteArray(64 * 1024, 'x'));
            url = sourceDir.filePath(QStringLiteral("damaged.json.gz"));
            QVERIFY(TestFixtures::writeFile(url, gzip.left(gzip.size() / 2)));
            autoDecompress = true;
            break;
        }
        default:
            QFAIL("Unknown failure");
    }

    const auto expectWarnings = [this, &warnings]() {
        for (const QString& warning : warnings) {
            expectLogMessage(qPrintable(warning.section(QLatin1Char('|'), 0, 0)), QtWarningMsg,
                             QRegularExpression(warning.section(QLatin1Char('|'), 1)));
        }
    };
    const auto verifyWarnings = [this, &warnings]() {
        for (qsizetype i = 0; i < warnings.count(); i++) {
            verifyExpectedLogMessage();
        }
    };
    // The error of a failed decompression names the compressed file, which each download keeps in a directory
    // of its own
    const auto withoutDownloadDir = [](const QString& error, const QGCFileDownload& downloader) {
        return QString(error).remove(QFileInfo(downloader.localPath()).path());
    };

    QGCFileDownload fileDownloader(this);
    fileDownloader.setExpectedHash(expectedHash);
    fileDownloader.setAutoDecompress(autoDecompress);
    QSignalSpy fileSpy(&fileDownloader, &QGCFileDownload::finished);
    expectWarnings();
    QVERIFY(fileDownloader.start(url));
    QVERIFY_SIGNAL_WAIT(fileSpy, TestTimeout::mediumMs());
    verifyWarnings();
    QVERIFY(!fileSpy.first().at(0).toBool());
    const QString fileError = fileSpy.first().at(2).toString();
    QVERIFY(!fileError.isEmpty());
    // A failed decompression hands the compressed file over
    const QString handedOverFile = fileSpy.first().at(1).toString();
    QCOMPARE(!handedOverFile.isEmpty(), failure == DownloadFailsDecompression);
    if (!handedOverFile.isEmpty()) {
        QVERIFY(QFile::remove(handedOverFile));
    }

    QGCFileDownload memoryDownloader(this);
    memoryDownloader.setExpectedHash(expectedHash);
    memoryDownloader.setAutoDecompress(autoDecompress);
    QSignalSpy memorySpy(&memoryDownloader, &QGCFileDownload::finishedToMemory);
    expectWarnings();
    QVERIFY(memoryDownloader.startToMemory(url));
    QVERIFY_SIGNAL_WAIT(memorySpy, TestTimeout::mediumMs());
    verifyWarnings();

    const QList<QVariant> args = memorySpy.first();
    QVERIFY(!args.at(0).toBool());
    QVERIFY(args.at(1).toByteArray().isEmpty());
    QCOMPARE(withoutDownloadDir(args.at(2).toString(), memoryDownloader),
             withoutDownloadDir(fileError, fileDownloader));
    QCOMPARE(withoutDownloadDir(memoryDownloader.errorString(), memoryDownloader),
             withoutDownloadDir(fileDownloader.errorString(), fileDownloader));
    QCOMPARE(memoryDownloader.state(), fileDownloader.state());
    QVERIFY(downloadToMemoryLeftNothing(memoryDownloader));
}

void QGCFileDownloadTest::_testStartToMemoryLimit_data()
{
    QTest::addColumn<int>("contentSize");
    QTest::addColumn<int>("body");
    QTest::addColumn<QStringList>("warnings");  // as "category|pattern"; empty: the content is handed over
    QTest::addColumn<QString>("error");

    constexpr int limit = 4096;
    const QString tooLarge = QStringLiteral("Download is larger than the limit of 4096 bytes");
    const QStringList tooLargeWarning{QStringLiteral("Utilities.QGCFileDownload|") + tooLarge};
    QTest::newRow("at the limit") << limit << int(DownloadBodyPlain) << QStringList() << QString();
    QTest::newRow("one byte more") << (limit + 1) << int(DownloadBodyPlain) << tooLargeWarning << tooLarge;
    QTest::newRow("many times more") << (256 * limit) << int(DownloadBodyPlain) << tooLargeWarning << tooLarge;
    QTest::newRow("decompressed, at the limit") << limit << int(DownloadBodyGzip) << QStringList() << QString();
    QTest::newRow("decompressed, one byte more")
        << (limit + 1) << int(DownloadBodyGzip)
        << QStringList{QStringLiteral("Utilities.QGClibarchive|Size limit exceeded"),
                       QStringLiteral("Utilities.QGCFileDownload|Decompression failed:")}
        << QStringLiteral("Decompression failed: .*size limit exceeded");
    // A body that is not compressed under a name that says so is handed over as it is, within the same limit
    QTest::newRow("not compressed, at the limit")
        << limit << int(DownloadBodyPlainNamedGzip) << QStringList() << QString();
    QTest::newRow("not compressed, one byte more")
        << (limit + 1) << int(DownloadBodyPlainNamedGzip) << tooLargeWarning << tooLarge;
}

void QGCFileDownloadTest::_testStartToMemoryLimit()
{
    QFETCH(int, contentSize);
    QFETCH(int, body);
    QFETCH(QStringList, warnings);
    QFETCH(QString, error);
    const bool namedGzip = (body != DownloadBodyPlain);

    // Compresses to a fraction of the limit, so only the decompressed size can exceed it
    const QByteArray content(contentSize, 'x');
    TestFixtures::LocalHttpTestServer server;
    QVERIFY2(server.listen(), "Could not start local test HTTP server");
    server.installHttpResponder((body == DownloadBodyGzip) ? TestFixtures::gzipMember(content) : content);

    QGCFileDownload downloader(this);
    downloader.setAutoDecompress(namedGzip);
    QSignalSpy memorySpy(&downloader, &QGCFileDownload::finishedToMemory);
    for (const QString& warning : warnings) {
        expectLogMessage(qPrintable(warning.section(QLatin1Char('|'), 0, 0)), QtWarningMsg,
                         QRegularExpression(warning.section(QLatin1Char('|'), 1)));
    }
    QVERIFY(downloader.startToMemory(
        server.url(namedGzip ? QStringLiteral("/content.bin.gz") : QStringLiteral("/content.bin")), 4096));
    QVERIFY_SIGNAL_WAIT(memorySpy, TestTimeout::mediumMs());
    for (qsizetype i = 0; i < warnings.count(); i++) {
        verifyExpectedLogMessage();
    }

    const QList<QVariant> args = memorySpy.first();
    if (error.isEmpty()) {
        QVERIFY2(args.at(0).toBool(), qPrintable(args.at(2).toString()));
        QCOMPARE(args.at(1).toByteArray(), content);
    } else {
        QVERIFY(!args.at(0).toBool());
        QVERIFY(args.at(1).toByteArray().isEmpty());
        QVERIFY2(QRegularExpression(QStringLiteral("^%1$").arg(error), QRegularExpression::CaseInsensitiveOption)
                     .match(args.at(2).toString())
                     .hasMatch(),
                 qPrintable(args.at(2).toString()));
        QCOMPARE(downloader.errorString(), args.at(2).toString());
        QCOMPARE(downloader.state(), QGCFileDownload::State::Failed);
    }
    QVERIFY(downloadToMemoryLeftNothing(downloader));
}

void QGCFileDownloadTest::_testStartToMemoryAutoDecompress_data()
{
    QTest::addColumn<bool>("autoDecompress");

    QTest::newRow("decompressed") << true;
    QTest::newRow("as downloaded") << false;
}

void QGCFileDownloadTest::_testStartToMemoryAutoDecompress()
{
    QFETCH(bool, autoDecompress);

    const QString url = QStringLiteral(":/unittest/manifest.json.gz");
    const QByteArray compressed = TestFixtures::readFile(url);
    QByteArray decompressed;
    QVERIFY(QGCCompression::decompressData(compressed, decompressed));
    QVERIFY(!decompressed.isEmpty());
    QVERIFY(decompressed != compressed);

    QGCFileDownload downloader(this);
    downloader.setAutoDecompress(autoDecompress);
    QSignalSpy memorySpy(&downloader, &QGCFileDownload::finishedToMemory);
    QVERIFY(downloader.startToMemory(url));
    QVERIFY_SIGNAL_WAIT(memorySpy, TestTimeout::mediumMs());

    const QList<QVariant> args = memorySpy.first();
    QVERIFY2(args.at(0).toBool(), qPrintable(args.at(2).toString()));
    QCOMPARE(args.at(1).toByteArray(), autoDecompress ? decompressed : compressed);
    QVERIFY(downloadToMemoryLeftNothing(downloader));
}

void QGCFileDownloadTest::_testStartToMemoryCancel()
{
    QGCFileDownload downloader(this);
    QSignalSpy memorySpy(&downloader, &QGCFileDownload::finishedToMemory);
    QSignalSpy fileSpy(&downloader, &QGCFileDownload::finished);

    QVERIFY(downloader.startToMemory(":/unittest/manifest.json.gz"));
    downloader.cancel();

    QTRY_COMPARE_WITH_TIMEOUT(memorySpy.count(), 1, TestTimeout::mediumMs());
    QVERIFY(!downloader.isRunning());
    const QList<QVariant> args = memorySpy.first();
    QVERIFY(!args.at(0).toBool());
    QVERIFY(args.at(1).toByteArray().isEmpty());
    QVERIFY(args.at(2).toString().contains(QStringLiteral("cancel"), Qt::CaseInsensitive));
    QCOMPARE(fileSpy.count(), 0);
    QVERIFY(downloadToMemoryLeftNothing(downloader));
}

void QGCFileDownloadTest::_testAutoDecompressGzip()
{
    // Test downloading a .gz file with autoDecompress=true
    // Uses manifest.json.gz from compression test resources
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
    // Download with autoDecompress=true
    downloader->setAutoDecompress(true);
    QVERIFY(downloader->start(":/unittest/manifest.json.gz"));
    QVERIFY_SIGNAL_WAIT(spy, TestTimeout::mediumMs());
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
                  [&resultLocalFile, &resultErrorMsg](bool success, const QString& localFile, const QString& errorMsg) {
                      QVERIFY(success);
                      resultLocalFile = localFile;
                      resultErrorMsg = errorMsg;
                  });
    QSignalSpy spy(downloader, &QGCFileDownload::finished);
    // Download with autoDecompress=false (default)
    downloader->setAutoDecompress(false);
    QVERIFY(downloader->start(":/unittest/manifest.json.gz"));
    QVERIFY_SIGNAL_WAIT(spy, TestTimeout::mediumMs());
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
                  [&resultLocalFile, &resultErrorMsg](bool success, const QString& localFile, const QString& errorMsg) {
                      QVERIFY(success);
                      resultLocalFile = localFile;
                      resultErrorMsg = errorMsg;
                  });
    QSignalSpy spy(downloader, &QGCFileDownload::finished);
    // Download non-compressed file with autoDecompress=true
    downloader->setAutoDecompress(true);
    QVERIFY(downloader->start(":/unittest/arducopter.apj"));
    QVERIFY_SIGNAL_WAIT(spy, TestTimeout::mediumMs());
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

void QGCFileDownloadTest::_testAutoDecompressBodyNotCompressed_data()
{
    QTest::addColumn<int>("body");
    QTest::addColumn<bool>("toMemory");

    // A server that sends the file with a content encoding: the client decodes the body, and the name the file
    // is saved under still says that it is compressed
    QTest::newRow("decoded by the client, to a file") << int(DownloadBodyDecodedByClient) << false;
    QTest::newRow("decoded by the client, to memory") << int(DownloadBodyDecodedByClient) << true;
    // A server that has the content under that name
    QTest::newRow("not compressed, to a file") << int(DownloadBodyPlainNamedGzip) << false;
    QTest::newRow("not compressed, to memory") << int(DownloadBodyPlainNamedGzip) << true;
    // What is compressed as its name says has to decompress
    QTest::newRow("truncated gzip, to a file") << int(DownloadBodyTruncatedGzip) << false;
    QTest::newRow("truncated gzip, to memory") << int(DownloadBodyTruncatedGzip) << true;
}

void QGCFileDownloadTest::_testAutoDecompressBodyNotCompressed()
{
    QFETCH(int, body);
    QFETCH(bool, toMemory);

    const QByteArray content = QByteArray("{\"name\": \"manifest\"}\n").repeated(200);
    const QByteArray gzip = TestFixtures::gzipMember(content);
    TestFixtures::LocalHttpTestServer server;
    QVERIFY2(server.listen(), "Could not start local test HTTP server");
    switch (body) {
        case DownloadBodyDecodedByClient:
            server.installRawResponder(
                "HTTP/1.1 200 OK\r\nConnection: close\r\nContent-Type: application/json\r\n"
                "Content-Encoding: gzip\r\nContent-Length: " +
                QByteArray::number(gzip.size()) + "\r\n\r\n" + gzip);
            break;
        case DownloadBodyPlainNamedGzip:
            server.installHttpResponder(content);
            break;
        case DownloadBodyTruncatedGzip:
            server.installHttpResponder(gzip.left(gzip.size() / 2));
            break;
        default:
            QFAIL("Unknown row");
    }
    const bool delivered = (body != DownloadBodyTruncatedGzip);
    if (!delivered) {
        expectLogMessage("Utilities.QGClibarchive", QtWarningMsg, QRegularExpression("(?i)truncated gzip input"));
        expectLogMessage("Utilities.QGCFileDownload", QtWarningMsg, QRegularExpression("Decompression failed:"));
    }

    QGCFileDownload downloader(this);
    downloader.setAutoDecompress(true);
    QSignalSpy fileSpy(&downloader, &QGCFileDownload::finished);
    QSignalSpy memorySpy(&downloader, &QGCFileDownload::finishedToMemory);
    const QString url = server.url(QStringLiteral("/manifest.json.gz"));
    QVERIFY(toMemory ? downloader.startToMemory(url) : downloader.start(url));
    QVERIFY_SIGNAL_WAIT(toMemory ? memorySpy : fileSpy, TestTimeout::mediumMs());
    if (!delivered) {
        verifyExpectedLogMessage();
        verifyExpectedLogMessage();
    }

    if (toMemory) {
        const QList<QVariant> args = memorySpy.first();
        QVERIFY2(args.at(0).toBool() == delivered, qPrintable(args.at(2).toString()));
        QCOMPARE(args.at(1).toByteArray(), delivered ? content : QByteArray());
        QVERIFY(downloadToMemoryLeftNothing(downloader));
        return;
    }

    const QList<QVariant> args = fileSpy.first();
    QVERIFY2(args.at(0).toBool() == delivered, qPrintable(args.at(2).toString()));
    const QFileInfo localFile(args.at(1).toString());
    if (delivered) {
        // As after a decompression: the content under the name without the extension, and nothing under the
        // name it was downloaded with
        QCOMPARE(localFile.fileName(), QStringLiteral("manifest.json"));
        QCOMPARE(downloader.localPath(), localFile.filePath());
        QCOMPARE(TestFixtures::readFile(localFile.filePath()), content);
        QCOMPARE(localFile.dir().entryList(QDir::Files), QStringList{QStringLiteral("manifest.json")});
    } else {
        // A failed decompression hands over the file as it was downloaded
        QCOMPARE(localFile.fileName(), QStringLiteral("manifest.json.gz"));
    }
    QVERIFY(QFile::remove(localFile.filePath()));
}

// ============================================================================
// Integration Tests (Download + Compression)
// ============================================================================
void QGCFileDownloadTest::_testDownloadAndExtractZip()
{
    QTemporaryDir tempDir;
    // Integration test: Download a ZIP archive and extract it
    // This tests the full workflow of QGCFileDownload + QGCCompression
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
    // Download ZIP archive (no auto-decompress for archives)
    downloader->setAutoDecompress(false);
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
    // Extract the downloaded archive using QGCCompression
    const QGCCompression::Outcome extractResult = QGCCompression::extractArchive(resultLocalFile, extractDir);
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

void QGCFileDownloadTest::_testDownloadDecompressAndParse()
{
    // Integration test: Download a gzipped JSON file, decompress it, and parse
    // This tests the full workflow with auto-decompression
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
    // Download with auto-decompress enabled
    downloader->setAutoDecompress(true);
    QVERIFY(downloader->start(":/unittest/manifest.json.gz"));
    QVERIFY_SIGNAL_WAIT(spy, TestTimeout::mediumMs());
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
