#include "ComponentInformationTranslationTest.h"

#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QPointer>
#include <QtCore/QRegularExpression>
#include <QtCore/QStandardPaths>
#include <QtCore/QTemporaryDir>
#include <QtCore/QTextStream>
#include <QtCore/QThreadPool>
#include <QtCore/QUrl>
#include <QtCore/QUuid>
#include <QtTest/QSignalSpy>

#include "ArchiveFixtures.h"
#include "ComponentInformationTranslation.h"
#include "FileFixtures.h"
#include "LocalHttpTestServer.h"
#include "ProcessFixtures.h"
#include "QGCCachedFileDownload.h"
#include "UnitTest.h"

namespace {

bool translationSystemLocaleIsEnglish()
{
    return QLocale::system().name().startsWith(QLatin1String("en"));
}

}  // namespace

bool ComponentInformationTranslationTest::writeSummary(const QString& path, const QString& locale, const QString& url)
{
    QFile summaryFile(path);
    if (!summaryFile.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        return false;
    }
    QTextStream summaryStream(&summaryFile);
    summaryStream << "{\n";
    summaryStream << "  \"" << locale << "\": {\n";
    summaryStream << "    \"url\": \"" << url << "\"\n";
    summaryStream << "  }\n";
    summaryStream << "}\n";
    return true;
}

QStringList ComponentInformationTranslationTest::translatedFiles()
{
    return QDir(QStandardPaths::writableLocation(QStandardPaths::TempLocation))
        .entryList({QStringLiteral("qgc_translated_metadata_*.json")}, QDir::Files);
}

void ComponentInformationTranslationTest::_basic_test_data()
{
    QTest::addColumn<QString>("compression");

    QTest::newRow("plain") << QString();
    QTest::newRow("JSON and TS compressed") << QStringLiteral("gzip");
    // Decompressed, either file is larger than VehicleFileFetch::kMaxSmallFileBytes
    QTest::newRow("JSON over the limit") << QStringLiteral("json over the limit");
    QTest::newRow("TS over the limit") << QStringLiteral("ts over the limit");
}

void ComponentInformationTranslationTest::_basic_test()
{
    QFETCH(QString, compression);

    QString translationJson = ":/unittest/TranslationTest.json";
    QString translationTs = ":/unittest/TranslationTest_de_DE.ts";
    QTemporaryDir outputDir;
    QVERIFY(outputDir.isValid());
    // Both files are read through Compression, which decompresses them and holds them to the limit
    QTemporaryDir inputDir;
    QVERIFY(inputDir.isValid());
    const QString overTheLimit = inputDir.filePath(QStringLiteral("zeros.gz"));
    // 1025 members of 64 KiB: 64 KiB more than the limit
    QVERIFY(ArchiveFixtures::writeGzipOfZeros(overTheLimit, 1025));
    if (compression == QStringLiteral("gzip")) {
        const QString json = inputDir.filePath(QStringLiteral("TranslationTest.json.gz"));
        QVERIFY(TestFixtures::writeFile(json, TestFixtures::gzipMember(TestFixtures::readFile(translationJson))));
        const QString ts = inputDir.filePath(QStringLiteral("TranslationTest_de_DE.ts.gz"));
        QVERIFY(TestFixtures::writeFile(ts, TestFixtures::gzipMember(TestFixtures::readFile(translationTs))));
        translationJson = json;
        translationTs = ts;
    } else if (compression == QStringLiteral("json over the limit")) {
        translationJson = overTheLimit;
    } else if (compression == QStringLiteral("ts over the limit")) {
        translationTs = overTheLimit;
    }

    const bool overLimit = compression.endsWith(QStringLiteral("over the limit"));
    const ComponentInformationTranslation::Translation translation =
        ComponentInformationTranslation::translateJsonUsingTS(translationJson, translationTs, outputDir.path());
    if (overLimit) {
        QVERIFY(translation.fileName.isEmpty());
        QCOMPARE(translation.warnings.size(), 1);
        QVERIFY2(translation.warnings.first().contains(QStringLiteral("size limit exceeded")),
                 qPrintable(translation.warnings.first()));
        QCOMPARE(QDir(outputDir.path()).entryList(QDir::Files), QStringList());
        return;
    }
    QString tempFilename = translation.fileName;
    QVERIFY(!tempFilename.isEmpty());
    QCOMPARE(QFileInfo(tempFilename).absolutePath(), outputDir.path());
    QVERIFY2(translation.warnings.isEmpty(), qPrintable(translation.warnings.join(QLatin1Char('\n'))));
    // Compare json files
    QFile translationJsonFile(":/unittest/TranslationTest.json");
    QVERIFY(translationJsonFile.open(QFile::ReadOnly | QFile::Text));
    QByteArray expectedOutput = translationJsonFile.readAll().replace("translate-me", "TRANSLATED");
    QJsonDocument expectedJson;
    readJson(expectedOutput, expectedJson);
    QFile tempJson(tempFilename);
    QVERIFY(tempJson.open(QFile::ReadOnly | QFile::Text));
    QByteArray translatedOutput = tempJson.readAll();
    tempJson.close();
    QVERIFY(tempJson.remove());
    QJsonDocument translatedJson;
    readJson(translatedOutput, translatedJson);
    QVERIFY(expectedJson == translatedJson);
}

void ComponentInformationTranslationTest::readJson(const QByteArray& bytes, QJsonDocument& jsonDoc)
{
    QJsonParseError parseError;
    jsonDoc = QJsonDocument::fromJson(bytes, &parseError);
    QVERIFY2(parseError.error == QJsonParseError::NoError, qPrintable(parseError.errorString()));
    QVERIFY(!jsonDoc.isEmpty());
}

void ComponentInformationTranslationTest::_downloadAndTranslateFromSummary_test()
{
    // Skip test on English locales since translation is intentionally skipped
    if (translationSystemLocaleIsEnglish()) {
        QSKIP("Translation is skipped for English locales");
    }

    QTemporaryDir tempDir;
    TestFixtures::LocalHttpTestServer server;
    QVERIFY(server.listen());
    const QString summaryPath = tempDir.filePath(QStringLiteral("summary.json"));
    QVERIFY(writeSummary(summaryPath, QLocale::system().name(), serveTranslation(server)));

    QGCCachedFileDownload cachedDownloader(tempDir.path(), this);
    ComponentInformationTranslation translation(this, &cachedDownloader);

    QSignalSpy completeSpy(&translation, &ComponentInformationTranslation::downloadComplete);
    QVERIFY(completeSpy.isValid());

    QVERIFY(translation.downloadAndTranslate(summaryPath, QStringLiteral(":/unittest/TranslationTest.json"), 3600, QStringLiteral("TEST")));
    QVERIFY_SIGNAL_WAIT(completeSpy, TestTimeout::mediumMs());
    QCOMPARE(completeSpy.count(), 1);

    const QList<QVariant> args = completeSpy.first();
    const QString translatedPath = args.at(0).toString();
    const QString error = args.at(1).toString();
    QVERIFY2(error.isEmpty(), qPrintable(error));
    QVERIFY(!translatedPath.isEmpty());
    QVERIFY(QFile::exists(translatedPath));

    QFile translatedFile(translatedPath);
    QVERIFY(translatedFile.open(QIODevice::ReadOnly | QIODevice::Text));
    const QByteArray translatedData = translatedFile.readAll();
    translatedFile.close();
    QVERIFY(translatedFile.remove());
    QVERIFY(translatedData.contains("TRANSLATED"));
}

QString ComponentInformationTranslationTest::serveTranslation(TestFixtures::LocalHttpTestServer& server)
{
    QFile translation(QStringLiteral(":/unittest/TranslationTest_de_DE.ts"));
    if (!translation.open(QIODevice::ReadOnly)) {
        return QString();
    }
    server.installHttpResponder(translation.readAll());
    // A path of its own: the network cache of an earlier request must not answer for it
    return server.url(
        QStringLiteral("/%1/TranslationTest_de_DE.ts").arg(QUuid::createUuid().toString(QUuid::WithoutBraces)));
}

void ComponentInformationTranslationTest::_translationUrlMustBeHttp_test_data()
{
    QTest::addColumn<QString>("kind");

    QTest::newRow("Qt resource") << QStringLiteral("resource");
    QTest::newRow("file URL") << QStringLiteral("file URL");
    QTest::newRow("absolute path") << QStringLiteral("absolute path");
    QTest::newRow("relative path") << QStringLiteral("relative path");
    QTest::newRow("other scheme") << QStringLiteral("other scheme");
}

// The summary comes from the vehicle. The translation it names is fetched over HTTP(S) only: one on this computer is
// not read, and the metadata stays untranslated.
void ComponentInformationTranslationTest::_translationUrlMustBeHttp_test()
{
    QFETCH(QString, kind);

    if (translationSystemLocaleIsEnglish()) {
        QSKIP("The summary is not read for an English locale");
    }

    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    // A translation that would load, were it read
    const QString localFile = tempDir.filePath(QStringLiteral("translation.ts"));
    QVERIFY(QFile::copy(QStringLiteral(":/unittest/TranslationTest_de_DE.ts"), localFile));
    QString url = QStringLiteral("ftp://127.0.0.1/translation.ts");
    if (kind == QStringLiteral("resource")) {
        url = QStringLiteral(":/unittest/TranslationTest_de_DE.ts");
    } else if (kind == QStringLiteral("file URL")) {
        url = QUrl::fromLocalFile(localFile).toString();
    } else if (kind == QStringLiteral("absolute path")) {
        url = localFile;
    }
    // Relative to the working directory, and without "./" before it
    QTemporaryDir relativeDirectory(QStringLiteral("translation-test-XXXXXX"));
    if (kind == QStringLiteral("relative path")) {
        QVERIFY(relativeDirectory.isValid());
        url = QDir::current().relativeFilePath(relativeDirectory.filePath(QStringLiteral("translation.ts")));
        QVERIFY(QFile::copy(localFile, url));
        QVERIFY(!url.startsWith(QLatin1Char('.')));
    }
    const QString summaryPath = tempDir.filePath(QStringLiteral("summary.json"));
    QVERIFY(writeSummary(summaryPath, QLocale::system().name(), url));

    QGCCachedFileDownload cachedDownloader(tempDir.path(), this);
    ComponentInformationTranslation translation(this, &cachedDownloader);
    QSignalSpy completeSpy(&translation, &ComponentInformationTranslation::downloadComplete);
    QSignalSpy downloadStartedSpy(&cachedDownloader, &QGCCachedFileDownload::urlChanged);

    expectLogMessage("ComponentInformation.ComponentInformationTranslation", QtWarningMsg,
                     QRegularExpression(QStringLiteral("only HTTP\\(S\\) is fetched")));
    QVERIFY(translation.downloadAndTranslate(summaryPath, QStringLiteral(":/unittest/TranslationTest.json"), 3600,
                                             QStringLiteral("TEST")));
    QVERIFY_SIGNAL_WAIT(completeSpy, TestTimeout::mediumMs());
    verifyExpectedLogMessage();
    QVERIFY(completeSpy.first().at(0).toString().isEmpty());
    QCOMPARE(downloadStartedSpy.count(), 0);
}

void ComponentInformationTranslationTest::_summaryWithoutTranslation_test_data()
{
    QTest::addColumn<QByteArray>("summary");
    QTest::addColumn<QString>("warning");

    const QByteArray locale = QLocale::system().name().toUtf8();
    QTest::newRow("locale missing") << QByteArray(R"({"zz_ZZ":{"url":":/unittest/TranslationTest_de_DE.ts"}})")
                                    << QStringLiteral("not found in translation json");
    QTest::newRow("url missing") << QByteArray(R"({")" + locale + R"(":{"unused":"value"}})")
                                 << QStringLiteral("has no url in translation json");
    QTest::newRow("invalid json") << QByteArray("{ invalid json") << QStringLiteral("summary json file open failed");
}

// A summary that names no translation for the locale ends the request with neither a file nor an error, and a
// warning
void ComponentInformationTranslationTest::_summaryWithoutTranslation_test()
{
    QFETCH(QByteArray, summary);
    QFETCH(QString, warning);

    if (translationSystemLocaleIsEnglish()) {
        QSKIP("The summary is not read for an English locale");
    }

    QTemporaryDir tempDir;
    const QString summaryPath = tempDir.filePath(QStringLiteral("summary.json"));
    QFile summaryFile(summaryPath);
    QVERIFY(summaryFile.open(QIODevice::WriteOnly | QIODevice::Truncate));
    QCOMPARE(summaryFile.write(summary), summary.size());
    summaryFile.close();

    QGCCachedFileDownload cachedDownloader(tempDir.path(), this);
    ComponentInformationTranslation translation(this, &cachedDownloader);
    QSignalSpy completeSpy(&translation, &ComponentInformationTranslation::downloadComplete);

    expectLogMessage("ComponentInformation.ComponentInformationTranslation", QtWarningMsg, QRegularExpression(warning));
    QVERIFY(translation.downloadAndTranslate(summaryPath, QStringLiteral(":/unittest/TranslationTest.json"), 3600,
                                             QStringLiteral("TEST")));
    QVERIFY_SIGNAL_WAIT(completeSpy, TestTimeout::mediumMs());
    verifyExpectedLogMessage();
    QCOMPARE(completeSpy.count(), 1);
    QVERIFY(completeSpy.first().at(0).toString().isEmpty());
    QVERIFY(completeSpy.first().at(1).toString().isEmpty());
    QVERIFY(!cachedDownloader.isRunning());
}

void ComponentInformationTranslationTest::_onDownloadCompletedFailurePropagatesError_test()
{
    QTemporaryDir tempDir;
    QGCCachedFileDownload cachedDownloader(tempDir.path(), this);
    ComponentInformationTranslation translation(this, &cachedDownloader);
    translation._toTranslateJsonFile = QStringLiteral(":/unittest/TranslationTest.json");

    QSignalSpy completeSpy(&translation, &ComponentInformationTranslation::downloadComplete);
    QVERIFY(completeSpy.isValid());

    translation.onDownloadCompleted(false, QString(), QStringLiteral("simulated failure"), false);
    QCOMPARE(completeSpy.count(), 1);

    const QList<QVariant> args = completeSpy.first();
    QVERIFY(args.at(0).toString().isEmpty());
    QCOMPARE(args.at(1).toString(), QStringLiteral("simulated failure"));
}

void ComponentInformationTranslationTest::_onDownloadCompletedMissingTsPropagatesError_test()
{
    QTemporaryDir tempDir;
    QGCCachedFileDownload cachedDownloader(tempDir.path(), this);
    ComponentInformationTranslation translation(this, &cachedDownloader);
    translation._toTranslateJsonFile = QStringLiteral(":/unittest/TranslationTest.json");

    QSignalSpy completeSpy(&translation, &ComponentInformationTranslation::downloadComplete);
    QVERIFY(completeSpy.isValid());

    expectLogMessage("ComponentInformation.ComponentInformationTranslation", QtWarningMsg,
                     QRegularExpression("Failed reading TS file"));
    translation.onDownloadCompleted(true, tempDir.filePath(QStringLiteral("missing.ts")), QString(), false);
    QVERIFY_SIGNAL_WAIT(completeSpy, TestTimeout::mediumMs());
    verifyExpectedLogMessage();
    QCOMPARE(completeSpy.count(), 1);

    const QList<QVariant> args = completeSpy.first();
    QVERIFY(args.at(0).toString().isEmpty());
    QVERIFY(!args.at(1).toString().isEmpty());
}

// The summary comes from the vehicle, or from where it points to: it is read on a worker thread, and the download
// of the translation starts once it has been
void ComponentInformationTranslationTest::_summaryIsReadOnWorker_test()
{
    if (translationSystemLocaleIsEnglish()) {
        QSKIP("The summary is not read for an English locale");
    }

    QTemporaryDir tempDir;
    TestFixtures::LocalHttpTestServer server;
    QVERIFY(server.listen());
    const QString summaryPath = tempDir.filePath(QStringLiteral("summary.json"));
    QVERIFY(writeSummary(summaryPath, QLocale::system().name(), serveTranslation(server)));

    QGCCachedFileDownload cachedDownloader(tempDir.path(), this);
    ComponentInformationTranslation translation(this, &cachedDownloader);
    QSignalSpy completeSpy(&translation, &ComponentInformationTranslation::downloadComplete);
    QSignalSpy downloadStartedSpy(&cachedDownloader, &QGCCachedFileDownload::urlChanged);

    TestFixtures::ThreadPoolBlocker blocker;
    QVERIFY(blocker.isBlocking());
    QVERIFY(translation.downloadAndTranslate(summaryPath, QStringLiteral(":/unittest/TranslationTest.json"), 3600,
                                             QStringLiteral("TEST")));
    QCoreApplication::processEvents();
    QCOMPARE(downloadStartedSpy.count(), 0);

    blocker.release();
    QVERIFY_SIGNAL_WAIT(completeSpy, TestTimeout::mediumMs());
    QCOMPARE(downloadStartedSpy.count(), 1);
    const QString translatedPath = completeSpy.first().at(0).toString();
    QVERIFY(!translatedPath.isEmpty());
    QVERIFY(QFile::remove(translatedPath));
}

void ComponentInformationTranslationTest::_translationOnWorkerEndsWithItsRequest_test_data()
{
    QTest::addColumn<QString>("end");

    QTest::newRow("translated") << QStringLiteral("none");
    QTest::newRow("cancelled") << QStringLiteral("cancel");
    QTest::newRow("superseded") << QStringLiteral("newer request");
    QTest::newRow("translation object deleted") << QStringLiteral("delete");
}

// The translation reads both files and writes a third: it runs on a worker thread. A result that comes after its
// request was cancelled, replaced by a newer one, or deleted with the translation object is dropped, and the file
// it wrote removed.
void ComponentInformationTranslationTest::_translationOnWorkerEndsWithItsRequest_test()
{
    QFETCH(QString, end);

    if ((end == QStringLiteral("newer request")) && translationSystemLocaleIsEnglish()) {
        QSKIP("No request is made for an English locale");
    }

    QTemporaryDir tempDir;
    QGCCachedFileDownload cachedDownloader(tempDir.path(), this);
    QPointer<ComponentInformationTranslation> translation =
        new ComponentInformationTranslation(this, &cachedDownloader);
    translation->_toTranslateJsonFile = QStringLiteral(":/unittest/TranslationTest.json");
    QSignalSpy completeSpy(translation.data(), &ComponentInformationTranslation::downloadComplete);
    const QStringList filesBefore = translatedFiles();

    TestFixtures::ThreadPoolBlocker blocker;
    QVERIFY(blocker.isBlocking());
    translation->onDownloadCompleted(true, QStringLiteral(":/unittest/TranslationTest_de_DE.ts"), QString(), false);
    QCoreApplication::processEvents();
    QCOMPARE(completeSpy.count(), 0);

    if (end == QStringLiteral("cancel")) {
        translation->cancel();
    } else if (end == QStringLiteral("newer request")) {
        // The newer request has no translation, so what downloadComplete() reports is its own
        expectLogMessage("ComponentInformation.ComponentInformationTranslation", QtWarningMsg,
                         QRegularExpression(QStringLiteral("not found in translation json")));
        const QString summaryPath = tempDir.filePath(QStringLiteral("summary.json"));
        QVERIFY(
            writeSummary(summaryPath, QStringLiteral("zz_ZZ"), QStringLiteral(":/unittest/TranslationTest_de_DE.ts")));
        QVERIFY(translation->downloadAndTranslate(summaryPath, QStringLiteral(":/unittest/TranslationTest.json"), 3600,
                                                  QStringLiteral("TEST")));
    } else if (end == QStringLiteral("delete")) {
        delete translation.data();
    }

    blocker.release();
    if (end == QStringLiteral("none")) {
        QVERIFY_SIGNAL_WAIT(completeSpy, TestTimeout::mediumMs());
        const QString translatedPath = completeSpy.first().at(0).toString();
        QVERIFY(QFile::exists(translatedPath));
        QVERIFY(QFile::remove(translatedPath));
        return;
    }
    if (end == QStringLiteral("newer request")) {
        QVERIFY_SIGNAL_WAIT(completeSpy, TestTimeout::mediumMs());
        verifyExpectedLogMessage();
        QVERIFY(completeSpy.first().at(0).toString().isEmpty());
    }
    QVERIFY(QThreadPool::globalInstance()->waitForDone(TestTimeout::mediumMs()));
    QCoreApplication::sendPostedEvents();
    QCOMPARE(completeSpy.count(), (end == QStringLiteral("newer request")) ? 1 : 0);
    QCOMPARE(translatedFiles(), filesBefore);
    delete translation.data();
}

UT_REGISTER_TEST(ComponentInformationTranslationTest, TestLabel::Unit, TestLabel::Vehicle)
