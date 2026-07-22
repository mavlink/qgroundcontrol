#include "ComponentInformationTranslationTest.h"

#include <QtCore/QTemporaryDir>
#include <QtCore/QTextStream>
#include <QtTest/QSignalSpy>

#include "ComponentInformationTranslation.h"
#include "QGCCachedFileDownload.h"
#include "QGCCompression.h"
#include "UnitTest.h"

void ComponentInformationTranslationTest::_basic_test()
{
    QString translationJson = ":/unittest/TranslationTest.json";
    QString translationTs = ":/unittest/TranslationTest_de_DE.ts";
    ComponentInformationTranslation* translation =
        new ComponentInformationTranslation(this, new QGCCachedFileDownload("", this));
    QString tempFilename = translation->translateJsonUsingTS(translationJson, translationTs);
    QVERIFY(!tempFilename.isEmpty());
    // Compare json files
    QFile translationJsonFile(translationJson);
    QVERIFY(translationJsonFile.open(QFile::ReadOnly | QFile::Text));
    QByteArray expectedOutput = translationJsonFile.readAll().replace("translate-me", "TRANSLATED");
    QJsonDocument expectedJson;
    readJson(expectedOutput, expectedJson);
    QFile tempJson(tempFilename);
    QVERIFY(tempJson.open(QFile::ReadOnly | QFile::Text));
    QByteArray translatedOutput = tempJson.readAll();
    QJsonDocument translatedJson;
    readJson(translatedOutput, translatedJson);
    QVERIFY(expectedJson == translatedJson);
}

void ComponentInformationTranslationTest::_malformedTs_test()
{
    QTemporaryDir tempDir;
    const QString tsPath = tempDir.filePath(QStringLiteral("malformed.ts"));
    QFile tsFile(tsPath);
    QVERIFY(tsFile.open(QIODevice::WriteOnly | QIODevice::Truncate));
    const QByteArray malformedTs =
        QByteArrayLiteral("<TS><context><name>translate-me</name><message><translation>TRANSLATED</translation>");
    QCOMPARE(tsFile.write(malformedTs), malformedTs.size());
    tsFile.close();

    QGCCachedFileDownload cachedDownloader(tempDir.filePath(QStringLiteral("cache")), this);
    ComponentInformationTranslation translation(this, &cachedDownloader);
    expectLogMessage("ComponentInformation.ComponentInformationTranslation", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Badly formed TS")));
    const QString result = translation.translateJsonUsingTS(QStringLiteral(":/unittest/TranslationTest.json"), tsPath);
    verifyExpectedLogMessage();
    QVERIFY(result.isEmpty());
}

void ComponentInformationTranslationTest::_translatedOutputSizeLimit_test()
{
    QTemporaryDir tempDir;
    QFile sourceTs(QStringLiteral(":/unittest/TranslationTest_de_DE.ts"));
    QVERIFY(sourceTs.open(QIODevice::ReadOnly));
    QByteArray expandedTs = sourceTs.readAll();
    expandedTs.replace("TRANSLATED", QByteArray(4096, 'X'));

    const QString expandedTsPath = tempDir.filePath(QStringLiteral("expanded.ts"));
    QFile outputTs(expandedTsPath);
    QVERIFY(outputTs.open(QIODevice::WriteOnly | QIODevice::Truncate));
    QCOMPARE(outputTs.write(expandedTs), expandedTs.size());
    outputTs.close();

    QFile sourceJson(QStringLiteral(":/unittest/TranslationTest.json"));
    QVERIFY(sourceJson.open(QIODevice::ReadOnly));
    const QJsonDocument sourceDocument = QJsonDocument::fromJson(sourceJson.readAll());
    QVERIFY(!sourceDocument.isNull());
    const qint64 maximumOutputBytes = sourceDocument.toJson(QJsonDocument::Compact).size() + 64;

    QGCCachedFileDownload cachedDownloader(tempDir.filePath(QStringLiteral("cache")), this);
    ComponentInformationTranslation translation(this, &cachedDownloader);
    expectLogMessage("ComponentInformation.ComponentInformationTranslation", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Translated metadata exceeds output size limit")));
    const QString result = translation.translateJsonUsingTS(QStringLiteral(":/unittest/TranslationTest.json"),
                                                            expandedTsPath, maximumOutputBytes);
    verifyExpectedLogMessage();
    QVERIFY(result.isEmpty());
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
    QTemporaryDir tempDir;
    const QString summaryPath = tempDir.filePath(QStringLiteral("summary.json"));
    const QString locale = QLocale::system().name();

    // Skip test on English locales since translation is intentionally skipped
    if (locale.startsWith(QLatin1String("en"))) {
        QSKIP("Translation is skipped for English locales");
    }

    QFile summaryFile(summaryPath);
    QVERIFY(summaryFile.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text));
    QTextStream summaryStream(&summaryFile);
    summaryStream << "{\n";
    summaryStream << "  \"" << locale << "\": {\n";
    summaryStream << "    \"url\": \":/unittest/TranslationTest_de_DE.ts\"\n";
    summaryStream << "  }\n";
    summaryStream << "}\n";
    summaryFile.close();

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
    QVERIFY(translatedData.contains("TRANSLATED"));
}

void ComponentInformationTranslationTest::_downloadAndTranslateMissingLocale_test()
{
    // The warning is only emitted on non-English locales; English locales return early
    // before the summary is parsed, so this is environment-dependent noise.
    ignoreLogMessage("ComponentInformation.ComponentInformationTranslation", QtWarningMsg,
                     QRegularExpression(QStringLiteral("not found in translation json")));

    QTemporaryDir tempDir;
    const QString summaryPath = tempDir.filePath(QStringLiteral("summary_missing_locale.json"));
    QFile summaryFile(summaryPath);
    QVERIFY(summaryFile.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text));
    QTextStream summaryStream(&summaryFile);
    summaryStream << "{\n";
    summaryStream << "  \"zz_ZZ\": {\n";
    summaryStream << "    \"url\": \":/unittest/TranslationTest_de_DE.ts\"\n";
    summaryStream << "  }\n";
    summaryStream << "}\n";
    summaryFile.close();

    QGCCachedFileDownload cachedDownloader(tempDir.path(), this);
    ComponentInformationTranslation translation(this, &cachedDownloader);

    QSignalSpy completeSpy(&translation, &ComponentInformationTranslation::downloadComplete);
    QVERIFY(!translation.downloadAndTranslate(summaryPath, QStringLiteral(":/unittest/TranslationTest.json"), 3600, QStringLiteral("TEST")));
    QCOMPARE(completeSpy.count(), 0);
}

void ComponentInformationTranslationTest::_downloadAndTranslateMissingUrl_test()
{
    // The warning is only emitted on non-English locales; English locales return early
    // before the summary is parsed, so this is environment-dependent noise.
    ignoreLogMessage("ComponentInformation.ComponentInformationTranslation", QtWarningMsg,
                     QRegularExpression(QStringLiteral("has no url in translation json")));

    QTemporaryDir tempDir;
    const QString summaryPath = tempDir.filePath(QStringLiteral("summary_missing_url.json"));
    const QString locale = QLocale::system().name();

    QFile summaryFile(summaryPath);
    QVERIFY(summaryFile.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text));
    QTextStream summaryStream(&summaryFile);
    summaryStream << "{\n";
    summaryStream << "  \"" << locale << "\": {\n";
    summaryStream << "    \"unused\": \"value\"\n";
    summaryStream << "  }\n";
    summaryStream << "}\n";
    summaryFile.close();

    QGCCachedFileDownload cachedDownloader(tempDir.path(), this);
    ComponentInformationTranslation translation(this, &cachedDownloader);

    QSignalSpy completeSpy(&translation, &ComponentInformationTranslation::downloadComplete);
    QVERIFY(!translation.downloadAndTranslate(summaryPath, QStringLiteral(":/unittest/TranslationTest.json"), 3600, QStringLiteral("TEST")));
    QCOMPARE(completeSpy.count(), 0);
}

void ComponentInformationTranslationTest::_downloadAndTranslateInvalidSummaryJson_test()
{
    // The warning is only emitted on non-English locales; English locales return early
    // before the summary is parsed, so this is environment-dependent noise.
    ignoreLogMessage("ComponentInformation.ComponentInformationTranslation", QtWarningMsg,
                     QRegularExpression(QStringLiteral("summary json file open failed")));

    QTemporaryDir tempDir;
    const QString summaryPath = tempDir.filePath(QStringLiteral("summary_invalid.json"));
    QFile summaryFile(summaryPath);
    QVERIFY(summaryFile.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text));
    summaryFile.write("{ invalid json");
    summaryFile.close();

    QGCCachedFileDownload cachedDownloader(tempDir.path(), this);
    ComponentInformationTranslation translation(this, &cachedDownloader);

    QSignalSpy completeSpy(&translation, &ComponentInformationTranslation::downloadComplete);
    QVERIFY(!translation.downloadAndTranslate(summaryPath, QStringLiteral(":/unittest/TranslationTest.json"), 3600, QStringLiteral("TEST")));
    QCOMPARE(completeSpy.count(), 0);
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

    expectLogMessage("ComponentInformation.ComponentInformationTranslation", QtWarningMsg, QRegularExpression("Failed opening TS file"));
    translation.onDownloadCompleted(true, tempDir.filePath(QStringLiteral("missing.ts")), QString(), false);
    verifyExpectedLogMessage();
    QCOMPARE(completeSpy.count(), 1);

    const QList<QVariant> args = completeSpy.first();
    QVERIFY(args.at(0).toString().isEmpty());
    QVERIFY(!args.at(1).toString().isEmpty());
}

void ComponentInformationTranslationTest::_cancelInvalidatesDownload_test()
{
    QTemporaryDir tempDir;
    QGCCachedFileDownload cachedDownloader(tempDir.path(), this);
    ComponentInformationTranslation translation(this, &cachedDownloader);
    QSignalSpy completeSpy(&translation, &ComponentInformationTranslation::downloadComplete);
    QVERIFY(completeSpy.isValid());

    translation._running = true;
    translation._toTranslateJsonFile = QStringLiteral(":/unittest/TranslationTest.json");
    const quint64 generation = translation._generation;
    translation._downloadConnection = connect(
        &cachedDownloader, &QGCCachedFileDownload::finished, &translation,
        [&translation, generation](bool success, const QString& localFile, const QString& errorMsg, bool fromCache) {
            if (translation._running && (generation == translation._generation)) {
                translation.onDownloadCompleted(success, localFile, errorMsg, fromCache);
            }
        });

    translation.cancel();
    QVERIFY(!translation.running());
    QVERIFY(!translation._downloadConnection);
    QVERIFY(translation._toTranslateJsonFile.isEmpty());

    emit cachedDownloader.finished(false, QString(), QStringLiteral("stale completion"), false);
    QCOMPARE(completeSpy.size(), 0);
}

void ComponentInformationTranslationTest::_onDownloadCompletedSizeLimitPropagatesError_test()
{
    QTemporaryDir tempDir;
    QGCCachedFileDownload cachedDownloader(tempDir.path(), this);
    ComponentInformationTranslation translation(this, &cachedDownloader);
    translation._toTranslateJsonFile = QStringLiteral(":/unittest/TranslationTest.json");
    translation._maximumFileBytes = 1;

    QSignalSpy completeSpy(&translation, &ComponentInformationTranslation::downloadComplete);
    QVERIFY(completeSpy.isValid());

    expectLogMessage("Utilities.QGCCompression", QtWarningMsg,
                     QRegularExpression(QStringLiteral("File exceeds maximum size")));
    translation.onDownloadCompleted(true, QStringLiteral(":/unittest/TranslationTest_de_DE.ts"), QString(), false);
    verifyExpectedLogMessage();
    QCOMPARE(completeSpy.size(), 1);
    QVERIFY(completeSpy.first().at(0).toString().isEmpty());
    QVERIFY(completeSpy.first().at(1).toString().contains(QStringLiteral("Decompression")));
    QCOMPARE(QGCCompression::lastError(), QGCCompression::Error::SizeLimitExceeded);
}

UT_REGISTER_TEST(ComponentInformationTranslationTest, TestLabel::Unit, TestLabel::Vehicle)
