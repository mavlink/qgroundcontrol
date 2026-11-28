#include "ComponentInformationTranslationTest.h"

#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QJsonObject>
#include <QtCore/QPointer>
#include <QtCore/QRegularExpression>
#include <QtCore/QStandardPaths>
#include <QtCore/QTemporaryDir>
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

/// The locale of the test translation, TranslationTest_de_DE.ts. The tests translate for it whatever the locale of
/// the machine is: for an English one nothing is translated.
const QString translationTestLocale = QStringLiteral("de_DE");

}  // namespace

bool ComponentInformationTranslationTest::writeSummary(const QString& path, const QString& locale, const QString& url)
{
    const QJsonObject summary{{locale, QJsonObject{{QStringLiteral("url"), url}}}};
    return TestFixtures::writeFile(path, QJsonDocument(summary).toJson());
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
    const bool overLimit = compression.endsWith(QStringLiteral("over the limit"));
    if (overLimit) {
        // 1025 members of 64 KiB: 64 KiB more than the limit
        QVERIFY(ArchiveFixtures::writeGzipOfZeros(overTheLimit, 1025));
    }
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

    const ComponentInformationTranslation::Translation translation =
        ComponentInformationTranslation::translateJsonUsingTS(translationJson, translationTs, outputDir.path());
    if (overLimit) {
        QVERIFY(translation.fileName.isEmpty());
        QCOMPARE(translation.warnings.size(), 1);
        QVERIFY2(translation.warnings.first().contains(QStringLiteral("larger than the size limit")),
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

QString ComponentInformationTranslationTest::serveTranslation(TestFixtures::LocalHttpTestServer& server)
{
    const QByteArray translation = TestFixtures::readFile(QStringLiteral(":/unittest/TranslationTest_de_DE.ts"));
    if (translation.isEmpty()) {
        return QString();
    }
    server.installHttpResponder(translation);
    // A path of its own: the network cache of an earlier request must not answer for it
    return server.url(
        QStringLiteral("/%1/TranslationTest_de_DE.ts").arg(QUuid::createUuid().toString(QUuid::WithoutBraces)));
}

void ComponentInformationTranslationTest::_translationUrlMustBeHttp_test_data()
{
    QTest::addColumn<bool>("fileUrl");

    // The other forms that are no HTTP(S) URL are those of VehicleFileFetchTest::_testUriOtherThanFtpOrHttpRefused
    QTest::newRow("file URL") << true;
    QTest::newRow("absolute path") << false;
}

// The summary comes from the vehicle. The translation it names is fetched over HTTP(S) only: one on this computer is
// not read, and the metadata stays untranslated.
void ComponentInformationTranslationTest::_translationUrlMustBeHttp_test()
{
    QFETCH(bool, fileUrl);

    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    // A translation that would load, were it read
    const QString localFile = tempDir.filePath(QStringLiteral("translation.ts"));
    QVERIFY(QFile::copy(QStringLiteral(":/unittest/TranslationTest_de_DE.ts"), localFile));
    const QString summaryPath = tempDir.filePath(QStringLiteral("summary.json"));
    QVERIFY(writeSummary(summaryPath, translationTestLocale,
                         fileUrl ? QUrl::fromLocalFile(localFile).toString() : localFile));

    QGCCachedFileDownload cachedDownloader(tempDir.path(), this);
    ComponentInformationTranslation translation(this, &cachedDownloader);
    translation._localeName = translationTestLocale;
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

    const QByteArray locale = translationTestLocale.toUtf8();
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

    QTemporaryDir tempDir;
    const QString summaryPath = tempDir.filePath(QStringLiteral("summary.json"));
    QVERIFY(TestFixtures::writeFile(summaryPath, summary));

    QGCCachedFileDownload cachedDownloader(tempDir.path(), this);
    ComponentInformationTranslation translation(this, &cachedDownloader);
    translation._localeName = translationTestLocale;
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

void ComponentInformationTranslationTest::_onDownloadCompletedPropagatesError_test_data()
{
    QTest::addColumn<bool>("downloaded");

    QTest::newRow("download failed") << false;
    QTest::newRow("downloaded TS file missing") << true;
}

// A translation that could not be downloaded, or that cannot be read once it is, ends the request with an error
void ComponentInformationTranslationTest::_onDownloadCompletedPropagatesError_test()
{
    QFETCH(bool, downloaded);

    QTemporaryDir tempDir;
    QGCCachedFileDownload cachedDownloader(tempDir.path(), this);
    ComponentInformationTranslation translation(this, &cachedDownloader);
    translation._toTranslateJsonFile = QStringLiteral(":/unittest/TranslationTest.json");

    QSignalSpy completeSpy(&translation, &ComponentInformationTranslation::downloadComplete);
    QVERIFY(completeSpy.isValid());

    if (downloaded) {
        // The file is read on a worker
        expectLogMessage("ComponentInformation.ComponentInformationTranslation", QtWarningMsg,
                         QRegularExpression("Failed reading TS file"));
        translation.onDownloadCompleted(true, tempDir.filePath(QStringLiteral("missing.ts")), QString(), false);
        QVERIFY_SIGNAL_WAIT(completeSpy, TestTimeout::mediumMs());
        verifyExpectedLogMessage();
    } else {
        translation.onDownloadCompleted(false, QString(), QStringLiteral("simulated failure"), false);
    }
    QCOMPARE(completeSpy.count(), 1);

    const QList<QVariant> args = completeSpy.first();
    QVERIFY(args.at(0).toString().isEmpty());
    QCOMPARE(args.at(1).toString(),
             downloaded ? QStringLiteral("Failed to translate json file") : QStringLiteral("simulated failure"));
}

void ComponentInformationTranslationTest::_summaryIsReadOnWorker_test_data()
{
    QTest::addColumn<bool>("cancelled");

    QTest::newRow("read, translation downloaded") << false;
    QTest::newRow("cancelled while it is read") << true;
}

// The summary comes from the vehicle, or from where it points to: it is read on a worker thread, and the download
// of the translation starts once it has been. A summary whose request was cancelled meanwhile is dropped: it starts
// no download, which would keep the downloader from the next request.
void ComponentInformationTranslationTest::_summaryIsReadOnWorker_test()
{
    QFETCH(bool, cancelled);

    QTemporaryDir tempDir;
    TestFixtures::LocalHttpTestServer server;
    QVERIFY(server.listen());
    const QString summaryPath = tempDir.filePath(QStringLiteral("summary.json"));
    QVERIFY(writeSummary(summaryPath, translationTestLocale, serveTranslation(server)));

    QGCCachedFileDownload cachedDownloader(tempDir.path(), this);
    ComponentInformationTranslation translation(this, &cachedDownloader);
    translation._localeName = translationTestLocale;
    QSignalSpy completeSpy(&translation, &ComponentInformationTranslation::downloadComplete);
    QSignalSpy downloadStartedSpy(&cachedDownloader, &QGCCachedFileDownload::urlChanged);

    TestFixtures::ThreadPoolBlocker blocker;
    QVERIFY(blocker.isBlocking());
    QVERIFY(translation.downloadAndTranslate(summaryPath, QStringLiteral(":/unittest/TranslationTest.json"), 3600,
                                             QStringLiteral("TEST")));
    QCoreApplication::processEvents();
    QCOMPARE(downloadStartedSpy.count(), 0);

    if (cancelled) {
        translation.cancel();
        blocker.release();
        QVERIFY(QThreadPool::globalInstance()->waitForDone(TestTimeout::mediumMs()));
        QCoreApplication::sendPostedEvents();
        QCOMPARE(downloadStartedSpy.count(), 0);
        QCOMPARE(completeSpy.count(), 0);
        return;
    }

    blocker.release();
    QVERIFY_SIGNAL_WAIT(completeSpy, TestTimeout::mediumMs());
    QCOMPARE(completeSpy.count(), 1);
    QCOMPARE(downloadStartedSpy.count(), 1);
    const QString translatedPath = completeSpy.first().at(0).toString();
    const QString error = completeSpy.first().at(1).toString();
    QVERIFY2(error.isEmpty(), qPrintable(error));
    QVERIFY(QFile::exists(translatedPath));
    QVERIFY(TestFixtures::readFile(translatedPath).contains("TRANSLATED"));
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

    QTemporaryDir tempDir;
    QGCCachedFileDownload cachedDownloader(tempDir.path(), this);
    QPointer<ComponentInformationTranslation> translation =
        new ComponentInformationTranslation(this, &cachedDownloader);
    translation->_localeName = translationTestLocale;
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
