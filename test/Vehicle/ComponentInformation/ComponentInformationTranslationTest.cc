#include "ComponentInformationTranslationTest.h"
#include "ComponentInformationTranslation.h"
#include "QGCCachedFileDownload.h"

#include <QtCore/QFile>
#include <QtCore/QJsonArray>
#include <QtTest/QTest>

namespace {
    constexpr const char* kTranslationJson = ":/unittest/TranslationTest.json";
    constexpr const char* kTranslationTs = ":/unittest/TranslationTest_de_DE.ts";
}

// ============================================================================
// Construction Tests
// ============================================================================

void ComponentInformationTranslationTest::_testConstruction()
{
    auto* cachedDownload = new QGCCachedFileDownload("", this);
    auto* translation = new ComponentInformationTranslation(this, cachedDownload);
    VERIFY_NOT_NULL(translation);
}

// ============================================================================
// Basic Translation Tests
// ============================================================================

void ComponentInformationTranslationTest::_testBasicTranslation()
{
    auto* translation = new ComponentInformationTranslation(this, new QGCCachedFileDownload("", this));
    VERIFY_NOT_NULL(translation);

    QString tempFilename = translation->translateJsonUsingTS(kTranslationJson, kTranslationTs);
    QGC_VERIFY_NOT_EMPTY(tempFilename);

    QFile translationJsonFile(kTranslationJson);
    QVERIFY(translationJsonFile.open(QFile::ReadOnly | QFile::Text));
    QByteArray expectedOutput = translationJsonFile.readAll().replace("translate-me", "TRANSLATED");

    QJsonDocument expectedJson;
    readJson(expectedOutput, expectedJson);

    QFile tempJson(tempFilename);
    QVERIFY(tempJson.open(QFile::ReadOnly | QFile::Text));
    QByteArray translatedOutput = tempJson.readAll();
    QJsonDocument translatedJson;
    readJson(translatedOutput, translatedJson);

    QCOMPARE_EQ(expectedJson, translatedJson);
}

void ComponentInformationTranslationTest::_testOutputFileCreated()
{
    auto* translation = new ComponentInformationTranslation(this, new QGCCachedFileDownload("", this));
    QString tempFilename = translation->translateJsonUsingTS(kTranslationJson, kTranslationTs);

    QGC_VERIFY_NOT_EMPTY(tempFilename);
    QVERIFY(QFile::exists(tempFilename));

    QFile tempJson(tempFilename);
    QVERIFY(tempJson.open(QFile::ReadOnly | QFile::Text));
    QVERIFY(tempJson.size() > 0);
}

// ============================================================================
// Structure Translation Tests
// ============================================================================

void ComponentInformationTranslationTest::_testListElementsTranslated()
{
    auto* translation = new ComponentInformationTranslation(this, new QGCCachedFileDownload("", this));
    QString tempFilename = translation->translateJsonUsingTS(kTranslationJson, kTranslationTs);
    QGC_VERIFY_NOT_EMPTY(tempFilename);

    QFile tempJson(tempFilename);
    QVERIFY(tempJson.open(QFile::ReadOnly | QFile::Text));
    QJsonDocument translatedJson;
    readJson(tempJson.readAll(), translatedJson);

    QJsonObject root = translatedJson.object();
    QJsonArray firstList = root["first_element"].toObject()["first_list_element"].toArray();

    QCOMPARE_GE(firstList.size(), 3);

    QJsonObject element1 = firstList[0].toObject();
    QCOMPARE_EQ(element1["label"].toString(), QString("TRANSLATED-list1-1.0"));
    QCOMPARE_EQ(element1["text"].toString(), QString("TRANSLATED-list1-1.1"));

    QJsonObject element2 = firstList[1].toObject();
    QCOMPARE_EQ(element2["label"].toString(), QString("TRANSLATED-list1-2.0"));
    QCOMPARE_EQ(element2["text"].toString(), QString("TRANSLATED-list1-2.1"));
}

void ComponentInformationTranslationTest::_testObjectKeysTranslated()
{
    auto* translation = new ComponentInformationTranslation(this, new QGCCachedFileDownload("", this));
    QString tempFilename = translation->translateJsonUsingTS(kTranslationJson, kTranslationTs);
    QGC_VERIFY_NOT_EMPTY(tempFilename);

    QFile tempJson(tempFilename);
    QVERIFY(tempJson.open(QFile::ReadOnly | QFile::Text));
    QJsonDocument translatedJson;
    readJson(tempJson.readAll(), translatedJson);

    QJsonObject root = translatedJson.object();
    QJsonObject object2 = root["first_element"].toObject()["object2"].toObject();

    QCOMPARE_EQ(object2["key1"].toObject()["name"].toString(), QString("TRANSLATED-name1"));
    QCOMPARE_EQ(object2["key2"].toObject()["name"].toString(), QString("TRANSLATED-name2"));
    QCOMPARE_EQ(object2["key3"].toObject()["name"].toString(), QString("TRANSLATED-name3"));
}

void ComponentInformationTranslationTest::_testRecursiveDefinitionsTranslated()
{
    auto* translation = new ComponentInformationTranslation(this, new QGCCachedFileDownload("", this));
    QString tempFilename = translation->translateJsonUsingTS(kTranslationJson, kTranslationTs);
    QGC_VERIFY_NOT_EMPTY(tempFilename);

    QFile tempJson(tempFilename);
    QVERIFY(tempJson.open(QFile::ReadOnly | QFile::Text));
    QJsonDocument translatedJson;
    readJson(tempJson.readAll(), translatedJson);

    QJsonObject root = translatedJson.object();
    QJsonArray subgroups = root["third_element"].toObject()["subgroups"].toArray();

    QCOMPARE_GE(subgroups.size(), 1);

    QJsonObject firstSubgroup = subgroups[0].toObject();
    QCOMPARE_EQ(firstSubgroup["description"].toString(), QString("TRANSLATED-subgroup1"));

    QJsonArray nestedSubgroups = firstSubgroup["subgroups"].toArray();
    QCOMPARE_GE(nestedSubgroups.size(), 2);
    QCOMPARE_EQ(nestedSubgroups[0].toObject()["description"].toString(), QString("TRANSLATED-subgroup1-1"));
    QCOMPARE_EQ(nestedSubgroups[1].toObject()["description"].toString(), QString("TRANSLATED-subgroup1-2"));

    QJsonArray deeplyNested = nestedSubgroups[0].toObject()["subgroups"].toArray();
    QCOMPARE_GE(deeplyNested.size(), 1);
    QCOMPARE_EQ(deeplyNested[0].toObject()["description"].toString(), QString("TRANSLATED-subgroup1-1-1"));
}

void ComponentInformationTranslationTest::_testGlobalTranslationsApplied()
{
    auto* translation = new ComponentInformationTranslation(this, new QGCCachedFileDownload("", this));
    QString tempFilename = translation->translateJsonUsingTS(kTranslationJson, kTranslationTs);
    QGC_VERIFY_NOT_EMPTY(tempFilename);

    QFile tempJson(tempFilename);
    QVERIFY(tempJson.open(QFile::ReadOnly | QFile::Text));
    QJsonDocument translatedJson;
    readJson(tempJson.readAll(), translatedJson);

    QJsonObject root = translatedJson.object();
    QJsonObject secondElement = root["second_element"].toObject();

    QCOMPARE_EQ(secondElement["element1"].toObject()["category"].toString(), QString("TRANSLATED-global-cat1"));
    QCOMPARE_EQ(secondElement["element2"].toObject()["category"].toString(), QString("TRANSLATED-global-cat2"));
    QCOMPARE_EQ(secondElement["element3"].toObject()["category"].toString(), QString("TRANSLATED-global-cat1"));
    QCOMPARE_EQ(secondElement["element4"].toObject()["category"].toString(), QString("TRANSLATED-global-cat2"));
}

void ComponentInformationTranslationTest::_testSpecialCharactersHandled()
{
    auto* translation = new ComponentInformationTranslation(this, new QGCCachedFileDownload("", this));
    QString tempFilename = translation->translateJsonUsingTS(kTranslationJson, kTranslationTs);
    QGC_VERIFY_NOT_EMPTY(tempFilename);

    QFile tempJson(tempFilename);
    QVERIFY(tempJson.open(QFile::ReadOnly | QFile::Text));
    QJsonDocument translatedJson;
    readJson(tempJson.readAll(), translatedJson);

    QJsonObject root = translatedJson.object();
    QJsonObject secondElement = root["second_element"].toObject();

    QCOMPARE_EQ(secondElement["element5"].toObject()["category"].toString(),
                QString("TRANSLATED-global-cat <> special symbol"));
}

// ============================================================================
// Edge Case Tests
// ============================================================================

void ComponentInformationTranslationTest::_testEmptyTsFileReturnsEmpty()
{
    auto* translation = new ComponentInformationTranslation(this, new QGCCachedFileDownload("", this));
    QString tempFilename = translation->translateJsonUsingTS(kTranslationJson, QString());
    QVERIFY(tempFilename.isEmpty());
}

void ComponentInformationTranslationTest::_testNonExistentJsonReturnsEmpty()
{
    auto* translation = new ComponentInformationTranslation(this, new QGCCachedFileDownload("", this));
    QString tempFilename = translation->translateJsonUsingTS(":/nonexistent.json", kTranslationTs);
    QVERIFY(tempFilename.isEmpty());
}

void ComponentInformationTranslationTest::_testNonExistentTsReturnsEmpty()
{
    auto* translation = new ComponentInformationTranslation(this, new QGCCachedFileDownload("", this));
    QString tempFilename = translation->translateJsonUsingTS(kTranslationJson, ":/nonexistent.ts");
    QVERIFY(tempFilename.isEmpty());
}

// ============================================================================
// Helper Methods
// ============================================================================

void ComponentInformationTranslationTest::readJson(const QByteArray& bytes, QJsonDocument& jsonDoc)
{
    QJsonParseError parseError;
    jsonDoc = QJsonDocument::fromJson(bytes, &parseError);
    QCOMPARE_EQ(parseError.error, QJsonParseError::NoError);
    QVERIFY(!jsonDoc.isEmpty());
}

QJsonValue ComponentInformationTranslationTest::getNestedValue(const QJsonObject& obj, const QStringList& path)
{
    if (path.isEmpty()) {
        return QJsonValue();
    }

    QJsonValue current = obj[path.first()];
    for (int i = 1; i < path.size(); ++i) {
        if (current.isObject()) {
            current = current.toObject()[path[i]];
        } else if (current.isArray()) {
            bool ok;
            int index = path[i].toInt(&ok);
            if (ok && index >= 0 && index < current.toArray().size()) {
                current = current.toArray()[index];
            } else {
                return QJsonValue();
            }
        } else {
            return QJsonValue();
        }
    }
    return current;
}
