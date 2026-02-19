#include "ComponentInformationTranslationTest.h"

#include <QtCore/QTextStream>
#include <QtTest/QSignalSpy>

#include "QGCCachedFileDownload.h"
#include "UnitTest.h"

#include "ComponentInformationTranslation.h"

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

void ComponentInformationTranslationTest::readJson(const QByteArray& bytes, QJsonDocument& jsonDoc)
{
    QJsonParseError parseError;
    jsonDoc = QJsonDocument::fromJson(bytes, &parseError);
    QVERIFY2(parseError.error == QJsonParseError::NoError, qPrintable(parseError.errorString()));
    QVERIFY(!jsonDoc.isEmpty());
}

void ComponentInformationTranslationTest::_downloadAndTranslateFromSummary_test()
{
    const QString summaryPath = tempPath(QStringLiteral("summary.json"));
    const QString locale = QLocale::system().name();
    QFile summaryFile(summaryPath);
    QVERIFY(summaryFile.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text));
    QTextStream summaryStream(&summaryFile);
    summaryStream << "{\n";
    summaryStream << "  \"" << locale << "\": {\n";
    summaryStream << "    \"url\": \":/unittest/TranslationTest_de_DE.ts\"\n";
    summaryStream << "  }\n";
    summaryStream << "}\n";
    summaryFile.close();

    QGCCachedFileDownload cachedDownloader(tempDirPath(), this);
    ComponentInformationTranslation translation(this, &cachedDownloader);

    QSignalSpy completeSpy(&translation, &ComponentInformationTranslation::downloadComplete);
    QVERIFY(completeSpy.isValid());

    QVERIFY(translation.downloadAndTranslate(summaryPath, QStringLiteral(":/unittest/TranslationTest.json"), 3600));
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
    const QString summaryPath = tempPath(QStringLiteral("summary_missing_locale.json"));
    QFile summaryFile(summaryPath);
    QVERIFY(summaryFile.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text));
    QTextStream summaryStream(&summaryFile);
    summaryStream << "{\n";
    summaryStream << "  \"zz_ZZ\": {\n";
    summaryStream << "    \"url\": \":/unittest/TranslationTest_de_DE.ts\"\n";
    summaryStream << "  }\n";
    summaryStream << "}\n";
    summaryFile.close();

    QGCCachedFileDownload cachedDownloader(tempDirPath(), this);
    ComponentInformationTranslation translation(this, &cachedDownloader);

    QSignalSpy completeSpy(&translation, &ComponentInformationTranslation::downloadComplete);
    QVERIFY(!translation.downloadAndTranslate(summaryPath, QStringLiteral(":/unittest/TranslationTest.json"), 3600));
    QCOMPARE(completeSpy.count(), 0);
}

void ComponentInformationTranslationTest::_downloadAndTranslateMissingUrl_test()
{
    const QString summaryPath = tempPath(QStringLiteral("summary_missing_url.json"));
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

    QGCCachedFileDownload cachedDownloader(tempDirPath(), this);
    ComponentInformationTranslation translation(this, &cachedDownloader);

    QSignalSpy completeSpy(&translation, &ComponentInformationTranslation::downloadComplete);
    QVERIFY(!translation.downloadAndTranslate(summaryPath, QStringLiteral(":/unittest/TranslationTest.json"), 3600));
    QCOMPARE(completeSpy.count(), 0);
}

void ComponentInformationTranslationTest::_downloadAndTranslateInvalidSummaryJson_test()
{
    const QString summaryPath = tempPath(QStringLiteral("summary_invalid.json"));
    QFile summaryFile(summaryPath);
    QVERIFY(summaryFile.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text));
    summaryFile.write("{ invalid json");
    summaryFile.close();

    QGCCachedFileDownload cachedDownloader(tempDirPath(), this);
    ComponentInformationTranslation translation(this, &cachedDownloader);

    QSignalSpy completeSpy(&translation, &ComponentInformationTranslation::downloadComplete);
    QVERIFY(!translation.downloadAndTranslate(summaryPath, QStringLiteral(":/unittest/TranslationTest.json"), 3600));
    QCOMPARE(completeSpy.count(), 0);
}

void ComponentInformationTranslationTest::_onDownloadCompletedFailurePropagatesError_test()
{
    QGCCachedFileDownload cachedDownloader(tempDirPath(), this);
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
    QGCCachedFileDownload cachedDownloader(tempDirPath(), this);
    ComponentInformationTranslation translation(this, &cachedDownloader);
    translation._toTranslateJsonFile = QStringLiteral(":/unittest/TranslationTest.json");

    QSignalSpy completeSpy(&translation, &ComponentInformationTranslation::downloadComplete);
    QVERIFY(completeSpy.isValid());

    translation.onDownloadCompleted(true, tempPath(QStringLiteral("missing.ts")), QString(), false);
    QCOMPARE(completeSpy.count(), 1);

    const QList<QVariant> args = completeSpy.first();
    QVERIFY(args.at(0).toString().isEmpty());
    QVERIFY(!args.at(1).toString().isEmpty());
}

UT_REGISTER_TEST(ComponentInformationTranslationTest, TestLabel::Unit, TestLabel::Vehicle)
