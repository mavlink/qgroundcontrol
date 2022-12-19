/****************************************************************************
 *
 * (c) 2021 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#include "ComponentInformationTranslationTest.h"

void ComponentInformationTranslationTest::_basic_test()
{
    QString translationJson = ":/unittest/TranslationTest.json";
    QString translationTs = ":/unittest/TranslationTest_de_DE.ts";
    ComponentInformationTranslation* translation = new ComponentInformationTranslation(this, new QGCCachedFileDownload(this, ""));
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
    QTEST_ASSERT(parseError.error == QJsonParseError::NoError);
    QVERIFY(!jsonDoc.isEmpty());
}

