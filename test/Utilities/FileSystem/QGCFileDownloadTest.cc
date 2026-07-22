#include "QGCFileDownloadTest.h"

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QPointer>
#include <QtCore/QRegularExpression>
#include <QtCore/QTemporaryDir>
#include <QtCore/QTimer>
#include <QtTest/QSignalSpy>

#include "QGCCompression.h"
#include "QGCCompressionJob.h"
#include "QGCFileDownload.h"
#include "QGCFileHelper.h"

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

void QGCFileDownloadTest::_testFileDownloadStartupReentrancyRejected()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    QGCFileDownload downloader(this);
    downloader.setOutputPath(tempDir.filePath(QStringLiteral("first.json.gz")));
    QSignalSpy finishedSpy(&downloader, &QGCFileDownload::finished);
    bool nestedStartAttempted = false;
    bool nestedStartSucceeded = true;
    connect(&downloader, &QGCFileDownload::urlChanged, this, [&](const QUrl&) {
        if (!nestedStartAttempted) {
            nestedStartAttempted = true;
            nestedStartSucceeded = downloader.start(QStringLiteral(":/unittest/arducopter.apj"));
        }
    });

    expectLogMessage("Utilities.QGCFileDownload", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Download already in progress")));
    QVERIFY(downloader.start(QStringLiteral(":/unittest/manifest.json.gz")));
    QVERIFY_SIGNAL_WAIT(finishedSpy, TestTimeout::mediumMs());
    verifyExpectedLogMessage();

    QVERIFY(nestedStartAttempted);
    QVERIFY(!nestedStartSucceeded);
    QCOMPARE(finishedSpy.size(), 1);
    QVERIFY(finishedSpy.first().at(0).toBool());
}

void QGCFileDownloadTest::_testFileDownloadStartupCancellation()
{
    for (const bool cancelFromLocalPath : {false, true}) {
        QGCFileDownload downloader(this);
        QSignalSpy finishedSpy(&downloader, &QGCFileDownload::finished);
        bool cancelRequested = false;
        if (cancelFromLocalPath) {
            connect(&downloader, &QGCFileDownload::localPathChanged, this, [&](const QString&) {
                cancelRequested = true;
                downloader.cancel();
            });
        } else {
            connect(&downloader, &QGCFileDownload::urlChanged, this, [&](const QUrl&) {
                cancelRequested = true;
                downloader.cancel();
            });
        }

        QVERIFY(downloader.start(QStringLiteral(":/unittest/manifest.json.gz")));
        QVERIFY(cancelRequested);
        QCOMPARE(finishedSpy.size(), 1);
        QVERIFY(!finishedSpy.first().at(0).toBool());
        QVERIFY(finishedSpy.first().at(2).toString().contains(QStringLiteral("cancelled"), Qt::CaseInsensitive));
        QCOMPARE(downloader.state(), QGCFileDownload::State::Cancelled);
        QVERIFY(!downloader.isRunning());
    }
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
}

void QGCFileDownloadTest::_testFileDownloadMaximumSize()
{
    const QString sourcePath = QStringLiteral(":/unittest/arducopter.apj");
    const qint64 sourceSize = QFileInfo(sourcePath).size();
    QVERIFY(sourceSize > 1);

    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    QGCFileDownload oversizedDownload(this);
    const QString oversizedOutput = tempDir.filePath(QStringLiteral("oversized.apj"));
    oversizedDownload.setOutputPath(oversizedOutput);
    QSignalSpy oversizedSpy(&oversizedDownload, &QGCFileDownload::finished);
    QGCNetworkHelper::RequestConfig oversizedConfig;
    oversizedConfig.maximumDownloadBytes = sourceSize - 1;
    expectLogMessage("Utilities.QGCFileDownload", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Download exceeds maximum size")));
    QVERIFY(oversizedDownload.start(sourcePath, oversizedConfig));
    if (oversizedSpy.isEmpty()) {
        QVERIFY_SIGNAL_WAIT(oversizedSpy, TestTimeout::mediumMs());
    }
    verifyExpectedLogMessage();
    QCOMPARE(oversizedSpy.size(), 1);
    QVERIFY(!oversizedSpy.first().at(0).toBool());
    QVERIFY(oversizedSpy.first().at(2).toString().contains(QStringLiteral("maximum size")));
    QVERIFY(!QFileInfo::exists(oversizedOutput));

    QGCFileDownload boundaryDownload(this);
    const QString boundaryOutput = tempDir.filePath(QStringLiteral("boundary.apj"));
    boundaryDownload.setOutputPath(boundaryOutput);
    QSignalSpy boundarySpy(&boundaryDownload, &QGCFileDownload::finished);
    QGCNetworkHelper::RequestConfig boundaryConfig;
    boundaryConfig.maximumDownloadBytes = sourceSize;
    QVERIFY(boundaryDownload.start(sourcePath, boundaryConfig));
    QVERIFY_SIGNAL_WAIT(boundarySpy, TestTimeout::mediumMs());
    QCOMPARE(boundarySpy.size(), 1);
    QVERIFY(boundarySpy.first().at(0).toBool());
    QCOMPARE(QFileInfo(boundaryOutput).size(), sourceSize);
}

void QGCFileDownloadTest::_testFileDownloadSizeLimitReentrantStart()
{
    const QString oversizedSource = QStringLiteral(":/unittest/arducopter.apj");
    const QString restartSource = QStringLiteral(":/unittest/manifest.json.gz");
    const qint64 restartSize = QFileInfo(restartSource).size();
    QVERIFY(restartSize > 0);

    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    QGCFileDownload downloader(this);
    const QString oversizedOutput = tempDir.filePath(QStringLiteral("oversized.apj"));
    const QString restartOutput = tempDir.filePath(QStringLiteral("restart.json.gz"));
    downloader.setOutputPath(oversizedOutput);

    QSignalSpy finishedSpy(&downloader, &QGCFileDownload::finished);
    bool restartAttempted = false;
    bool restartSucceeded = false;
    connect(&downloader, &QGCFileDownload::runningChanged, this, [&](bool running) {
        if (!running && !restartAttempted && (downloader.state() == QGCFileDownload::State::Failed)) {
            restartAttempted = true;
            downloader.setOutputPath(restartOutput);
            QGCNetworkHelper::RequestConfig restartConfig;
            restartConfig.maximumDownloadBytes = restartSize;
            restartSucceeded = downloader.start(restartSource, restartConfig);
        }
    });

    QGCNetworkHelper::RequestConfig oversizedConfig;
    oversizedConfig.maximumDownloadBytes = 1;
    expectLogMessage("Utilities.QGCFileDownload", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Download exceeds maximum size")));
    QVERIFY(downloader.start(oversizedSource, oversizedConfig));
    QVERIFY_SIGNAL_COUNT_WAIT(finishedSpy, 2, TestTimeout::mediumMs());
    verifyExpectedLogMessage();

    QVERIFY(restartAttempted);
    QVERIFY(restartSucceeded);
    QCOMPARE(finishedSpy.size(), 2);
    QVERIFY(!finishedSpy.at(0).at(0).toBool());
    QVERIFY(finishedSpy.at(1).at(0).toBool());
    QVERIFY(!QFileInfo::exists(oversizedOutput));
    QCOMPARE(QFileInfo(restartOutput).size(), restartSize);
}

void QGCFileDownloadTest::_testFileDownloadCompletionReentrantStartPreservesResult()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QString firstOutput = tempDir.filePath(QStringLiteral("first.json.gz"));
    const QString secondOutput = tempDir.filePath(QStringLiteral("second.apj"));
    QGCFileDownload downloader(this);
    downloader.setOutputPath(firstOutput);
    QSignalSpy finishedSpy(&downloader, &QGCFileDownload::finished);
    bool restartAttempted = false;
    bool restartAccepted = false;
    connect(&downloader, &QGCFileDownload::runningChanged, this, [&](bool running) {
        if (!running && !restartAttempted && (downloader.state() == QGCFileDownload::State::Completed)) {
            restartAttempted = true;
            downloader.setOutputPath(secondOutput);
            restartAccepted = downloader.start(QStringLiteral(":/unittest/arducopter.apj"));
        }
    });

    QVERIFY(downloader.start(QStringLiteral(":/unittest/manifest.json.gz")));
    QVERIFY_SIGNAL_COUNT_WAIT(finishedSpy, 2, TestTimeout::mediumMs());

    QVERIFY(restartAttempted);
    QVERIFY(restartAccepted);
    QCOMPARE(finishedSpy.size(), 2);
    QVERIFY(finishedSpy.at(0).at(0).toBool());
    QCOMPARE(finishedSpy.at(0).at(1).toString(), firstOutput);
    QVERIFY(finishedSpy.at(1).at(0).toBool());
    QCOMPARE(finishedSpy.at(1).at(1).toString(), secondOutput);
    QVERIFY(QFileInfo::exists(firstOutput));
    QVERIFY(QFileInfo::exists(secondOutput));
}

void QGCFileDownloadTest::_testFileDownloadDeferredStartFailureCompletes()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    QGCFileDownload downloader(this);
    downloader.setOutputPath(tempDir.filePath(QStringLiteral("first.json.gz")));
    QSignalSpy finishedSpy(&downloader, &QGCFileDownload::finished);
    bool deferredStartAccepted = false;
    connect(&downloader, &QGCFileDownload::finished, this, [&](bool success, const QString&, const QString&) {
        if (success && (finishedSpy.size() == 1)) {
            deferredStartAccepted = downloader.start(QString());
        }
    });

    expectLogMessage("Utilities.QGCFileDownload", QtWarningMsg, QRegularExpression(QStringLiteral("Empty URL")));
    QVERIFY(downloader.start(QStringLiteral(":/unittest/manifest.json.gz")));
    QVERIFY_SIGNAL_COUNT_WAIT(finishedSpy, 2, TestTimeout::mediumMs());
    verifyExpectedLogMessage();

    QVERIFY(deferredStartAccepted);
    QVERIFY(finishedSpy.at(0).at(0).toBool());
    QVERIFY(!finishedSpy.at(1).at(0).toBool());
    QVERIFY(finishedSpy.at(1).at(2).toString().contains(QStringLiteral("Empty URL")));
    QCOMPARE(downloader.state(), QGCFileDownload::State::Failed);
}

void QGCFileDownloadTest::_testFileDownloadDirectConnectionDeletion()
{
    {
        QPointer<QGCFileDownload> downloader = new QGCFileDownload;
        connect(downloader, &QGCFileDownload::urlChanged, downloader,
                [downloader](const QUrl&) { delete downloader.data(); });
        QVERIFY(!downloader->start(QStringLiteral(":/unittest/manifest.json.gz")));
        QVERIFY(!downloader);
    }

    {
        QPointer<QGCFileDownload> downloader = new QGCFileDownload;
        connect(downloader, &QGCFileDownload::runningChanged, downloader, [downloader](bool running) {
            if (running) {
                delete downloader.data();
            }
        });
        QVERIFY(!downloader->start(QStringLiteral(":/unittest/manifest.json.gz")));
        QVERIFY(!downloader);
    }

    {
        QPointer<QGCFileDownload> downloader = new QGCFileDownload;
        QSignalSpy finishedSpy(downloader, &QGCFileDownload::finished);
        connect(downloader, &QGCFileDownload::finished, downloader,
                [downloader](bool, const QString&, const QString&) { delete downloader.data(); });
        QVERIFY(downloader->start(QStringLiteral(":/unittest/manifest.json.gz")));
        QVERIFY_SIGNAL_WAIT(finishedSpy, TestTimeout::mediumMs());
        QVERIFY(!downloader);
        QCOMPARE(finishedSpy.size(), 1);
    }

    {
        QPointer<QGCFileDownload> downloader = new QGCFileDownload;
        QPointer<QGCCompressionJob> decompressionJob;
        downloader->setAutoDecompress(true);
        connect(downloader, &QGCFileDownload::stateChanged, downloader,
                [downloader, &decompressionJob](QGCFileDownload::State state) {
                    if (state == QGCFileDownload::State::Decompressing) {
                        decompressionJob = downloader->findChild<QGCCompressionJob*>();
                        delete downloader.data();
                    }
                });
        QVERIFY(downloader->start(QStringLiteral(":/unittest/manifest.json.gz")));
        QTRY_VERIFY_WITH_TIMEOUT(!downloader, TestTimeout::mediumMs());
        QVERIFY(!decompressionJob);
    }
}

void QGCFileDownloadTest::_testFileDownloadProgressReentrantRestart()
{
    const QString firstSource = QStringLiteral(":/unittest/arducopter.apj");
    const QString restartSource = QStringLiteral(":/unittest/manifest.json.gz");
    const qint64 restartSize = QFileInfo(restartSource).size();
    QVERIFY(QFileInfo(firstSource).size() > restartSize);

    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    const QString firstOutput = tempDir.filePath(QStringLiteral("first.apj"));
    const QString restartOutput = tempDir.filePath(QStringLiteral("restart.json.gz"));

    QGCFileDownload downloader(this);
    downloader.setOutputPath(firstOutput);
    QSignalSpy finishedSpy(&downloader, &QGCFileDownload::finished);
    bool restartAttempted = false;
    bool restartStarted = false;
    connect(&downloader, &QGCFileDownload::bytesReceivedChanged, this, [&](qint64 bytesReceived) {
        if ((bytesReceived <= 0) || restartAttempted) {
            return;
        }

        restartAttempted = true;
        downloader.cancel();
        downloader.setOutputPath(restartOutput);
        QGCNetworkHelper::RequestConfig restartConfig;
        restartConfig.maximumDownloadBytes = restartSize;
        restartStarted = downloader.start(restartSource, restartConfig);
    });

    QVERIFY(downloader.start(firstSource));
    QVERIFY_SIGNAL_COUNT_WAIT(finishedSpy, 2, TestTimeout::mediumMs());

    QVERIFY(restartAttempted);
    QVERIFY(restartStarted);
    QCOMPARE(finishedSpy.size(), 2);
    QVERIFY(!finishedSpy.at(0).at(0).toBool());
    QVERIFY(finishedSpy.at(1).at(0).toBool());
    QCOMPARE(finishedSpy.at(1).at(1).toString(), restartOutput);
    QCOMPARE(QFileInfo(restartOutput).size(), restartSize);
    QCOMPARE(downloader.state(), QGCFileDownload::State::Completed);
}

void QGCFileDownloadTest::_testFileDownloadFailurePreservesExistingOutput()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QString outputPath = tempDir.filePath(QStringLiteral("existing.apj"));
    const QByteArray originalContents("existing-valid-file");
    QFile existingFile(outputPath);
    QVERIFY(existingFile.open(QIODevice::WriteOnly));
    QCOMPARE(existingFile.write(originalContents), originalContents.size());
    existingFile.close();

    QGCFileDownload downloader(this);
    downloader.setOutputPath(outputPath);
    downloader.setExpectedHash(QString(64, QLatin1Char('0')));
    QSignalSpy finishedSpy(&downloader, &QGCFileDownload::finished);
    QVERIFY(downloader.start(QStringLiteral(":/unittest/arducopter.apj")));
    QVERIFY_SIGNAL_WAIT(finishedSpy, TestTimeout::mediumMs());

    QCOMPARE(finishedSpy.size(), 1);
    QVERIFY(!finishedSpy.first().at(0).toBool());
    QFile preservedFile(outputPath);
    QVERIFY(preservedFile.open(QIODevice::ReadOnly));
    QCOMPARE(preservedFile.readAll(), originalContents);
}

void QGCFileDownloadTest::_testAutoDecompressCancelRestart()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    QGCFileDownload downloader(this);
    downloader.setAutoDecompress(true);
    downloader.setOutputPath(tempDir.filePath(QStringLiteral("first.json.gz")));
    QSignalSpy finishedSpy(&downloader, &QGCFileDownload::finished);

    bool restartScheduled = false;
    bool restartStarted = false;
    bool cancelledJobStayedOwned = false;
    connect(&downloader, &QGCFileDownload::stateChanged, this, [&](QGCFileDownload::State state) {
        if ((state == QGCFileDownload::State::Decompressing) && !restartScheduled) {
            restartScheduled = true;
            QTimer::singleShot(0, &downloader, [&]() {
                if (downloader.state() != QGCFileDownload::State::Decompressing) {
                    return;
                }
                QPointer<QGCCompressionJob> job = downloader.findChild<QGCCompressionJob*>();
                QVERIFY(job);
                downloader.cancel();
                cancelledJobStayedOwned = job && (job->parent() == &downloader);
                downloader.setOutputPath(tempDir.filePath(QStringLiteral("second.json.gz")));
                restartStarted = downloader.start(QStringLiteral(":/unittest/manifest.json.gz"));
            });
        }
    });

    QVERIFY(downloader.start(QStringLiteral(":/unittest/manifest.json.gz")));
    QVERIFY_SIGNAL_COUNT_WAIT(finishedSpy, 2, TestTimeout::mediumMs());

    QVERIFY(restartScheduled);
    QVERIFY(restartStarted);
    QVERIFY(cancelledJobStayedOwned);
    QCOMPARE(finishedSpy.size(), 2);
    QVERIFY(!finishedSpy.at(0).at(0).toBool());
    QVERIFY(finishedSpy.at(1).at(0).toBool());
    QCOMPARE(finishedSpy.at(1).at(1).toString(), tempDir.filePath(QStringLiteral("second.json")));
    QCOMPARE(downloader.state(), QGCFileDownload::State::Completed);
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

void QGCFileDownloadTest::_testAutoDecompressMaximumSize()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QString compressedOutput = tempDir.filePath(QStringLiteral("limited.json.gz"));
    const QString decompressedOutput = tempDir.filePath(QStringLiteral("limited.json"));
    QGCFileDownload downloader(this);
    downloader.setAutoDecompress(true);
    downloader.setOutputPath(compressedOutput);
    QSignalSpy finishedSpy(&downloader, &QGCFileDownload::finished);
    QGCNetworkHelper::RequestConfig config;
    config.maximumDecompressedBytes = 1;

    expectLogMessage("Utilities.QGClibarchive", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Size limit exceeded")));
    expectLogMessage("Utilities.QGCFileDownload", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Decompression failed")));
    QVERIFY(downloader.start(QStringLiteral(":/unittest/manifest.json.gz"), config));
    QVERIFY_SIGNAL_WAIT(finishedSpy, TestTimeout::mediumMs());
    verifyExpectedLogMessage();
    verifyExpectedLogMessage();

    QCOMPARE(finishedSpy.size(), 1);
    QVERIFY(!finishedSpy.first().at(0).toBool());
    QCOMPARE(finishedSpy.first().at(1).toString(), compressedOutput);
    QVERIFY(finishedSpy.first().at(2).toString().contains(QStringLiteral("maximum size")));
    QVERIFY(QFileInfo::exists(compressedOutput));
    QVERIFY(!QFileInfo::exists(decompressedOutput));
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
    const bool extractResult = QGCCompression::extractArchive(resultLocalFile, extractDir);
    QVERIFY2(extractResult, qPrintable("Extraction failed: " + QGCCompression::lastErrorString()));
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
