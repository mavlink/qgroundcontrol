#include "APMParameterMetaDataTest.h"
#include "APMParameterMetaData.h"
#include "ParameterMetaData.h"
#include "ParameterMetaDataTestHelper.h"

#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QVersionNumber>

using namespace Qt::StringLiterals;

static const char *kAPMJson = R"({
    "TEST_": {
        "TEST_PARAM": {
            "DisplayName": "Test Parameter",
            "Description": "A test parameter for unit testing",
            "Range": {"low": "0.01", "high": "0.5"},
            "Units": "1/s",
            "Increment": "0.005",
            "User": "Standard",
            "RebootRequired": "True"
        },
        "TEST_ENUM": {
            "DisplayName": "Enum Parameter",
            "Description": "An enum test",
            "Values": {"10": "Expert", "2": "Auto", "0": "Disabled", "1": "Enabled"},
            "User": "Advanced"
        },
        "TEST_BITMASK": {
            "DisplayName": "Bitmask Parameter",
            "Description": "A bitmask test",
            "Bitmask": {"2": "Third", "10": "Eleventh", "0": "First", "1": "Second"}
        },
        "TEST_RANGE": {
            "DisplayName": "Range Param",
            "Description": "Range only",
            "Range": {"low": "-10.5", "high": "50.0"}
        },
        "TEST_RO": {
            "DisplayName": "Read Only",
            "Description": "Read only param",
            "ReadOnly": "True"
        },
        "ATC_RAT_RLL_P": {
            "DisplayName": "Roll P",
            "Description": "PID param"
        },
        "TEST_DUP": {
            "DisplayName": "First",
            "Description": "First desc"
        }
    },
    "DUP_": {
        "TEST_DUP": {
            "DisplayName": "Second",
            "Description": "Second desc"
        }
    }
})";

static APMParameterMetaData *_loadFromJson(const QByteArray &jsonData, QObject *parent)
{
    return loadMetaDataFromJson<APMParameterMetaData>(jsonData, parent);
}

void APMParameterMetaDataTest::_parseBasicParameter()
{
    QScopedPointer<APMParameterMetaData> meta(_loadFromJson(kAPMJson, nullptr));
    QVERIFY(meta);

    FactMetaData *fact = meta->getMetaDataForFact("TEST_PARAM", FactMetaData::valueTypeFloat);
    QVERIFY(fact);
    QCOMPARE(fact->name(), "TEST_PARAM");
    QCOMPARE(fact->shortDescription(), "Test Parameter");
    QCOMPARE(fact->longDescription(), "A test parameter for unit testing");
    QCOMPARE(fact->rawUnits(), "1/s");
    QCOMPARE(fact->category(), "Standard");
    QVERIFY(fact->vehicleRebootRequired());
    QVERIFY(qAbs(fact->rawIncrement() - 0.005) < 1e-6);
}

void APMParameterMetaDataTest::_parseRange()
{
    QScopedPointer<APMParameterMetaData> meta(_loadFromJson(kAPMJson, nullptr));
    QVERIFY(meta);

    FactMetaData *fact = meta->getMetaDataForFact("TEST_PARAM", FactMetaData::valueTypeFloat);
    QVERIFY(fact);
    QCOMPARE(fact->rawMin().toFloat(), 0.01f);
    QCOMPARE(fact->rawMax().toFloat(), 0.5f);
    QCOMPARE(fact->rawUserMin().toFloat(), 0.01f);
    QCOMPARE(fact->rawUserMax().toFloat(), 0.5f);

    FactMetaData *range = meta->getMetaDataForFact("TEST_RANGE", FactMetaData::valueTypeFloat);
    QVERIFY(range);
    QCOMPARE(range->rawMin().toFloat(), -10.5f);
    QCOMPARE(range->rawMax().toFloat(), 50.0f);
    QCOMPARE(range->rawUserMin().toFloat(), -10.5f);
    QCOMPARE(range->rawUserMax().toFloat(), 50.0f);
}

void APMParameterMetaDataTest::_parseEnumValues()
{
    QScopedPointer<APMParameterMetaData> meta(_loadFromJson(kAPMJson, nullptr));
    QVERIFY(meta);

    FactMetaData *fact = meta->getMetaDataForFact("TEST_ENUM", FactMetaData::valueTypeInt32);
    QVERIFY(fact);
    QCOMPARE(fact->enumStrings().count(), 4);
    QCOMPARE(fact->enumStrings()[0], "Disabled");
    QCOMPARE(fact->enumStrings()[1], "Enabled");
    QCOMPARE(fact->enumStrings()[2], "Auto");
    QCOMPARE(fact->enumStrings()[3], "Expert");
    QCOMPARE(fact->enumValues()[0].toInt(), 0);
    QCOMPARE(fact->enumValues()[1].toInt(), 1);
    QCOMPARE(fact->enumValues()[2].toInt(), 2);
    QCOMPARE(fact->enumValues()[3].toInt(), 10);
}

void APMParameterMetaDataTest::_parseBitmask()
{
    QScopedPointer<APMParameterMetaData> meta(_loadFromJson(kAPMJson, nullptr));
    QVERIFY(meta);

    FactMetaData *fact = meta->getMetaDataForFact("TEST_BITMASK", FactMetaData::valueTypeInt32);
    QVERIFY(fact);
    QCOMPARE(fact->bitmaskStrings().count(), 4);
    QCOMPARE(fact->bitmaskStrings()[0], "First");
    QCOMPARE(fact->bitmaskStrings()[1], "Second");
    QCOMPARE(fact->bitmaskStrings()[2], "Third");
    QCOMPARE(fact->bitmaskStrings()[3], "Eleventh");
    QCOMPARE(fact->bitmaskValues()[0].toInt(), 1);
    QCOMPARE(fact->bitmaskValues()[1].toInt(), 2);
    QCOMPARE(fact->bitmaskValues()[2].toInt(), 4);
    QCOMPARE(fact->bitmaskValues()[3].toInt(), 1024);
}

void APMParameterMetaDataTest::_parseReadOnly()
{
    QScopedPointer<APMParameterMetaData> meta(_loadFromJson(kAPMJson, nullptr));
    QVERIFY(meta);

    FactMetaData *fact = meta->getMetaDataForFact("TEST_RO", FactMetaData::valueTypeInt32);
    QVERIFY(fact);
    QVERIFY(fact->readOnly());
}

void APMParameterMetaDataTest::_parseRebootRequired()
{
    QScopedPointer<APMParameterMetaData> meta(_loadFromJson(kAPMJson, nullptr));
    QVERIFY(meta);

    FactMetaData *fact = meta->getMetaDataForFact("TEST_PARAM", FactMetaData::valueTypeFloat);
    QVERIFY(fact);
    QVERIFY(fact->vehicleRebootRequired());
}

void APMParameterMetaDataTest::_parseIncrement()
{
    QScopedPointer<APMParameterMetaData> meta(_loadFromJson(kAPMJson, nullptr));
    QVERIFY(meta);

    FactMetaData *fact = meta->getMetaDataForFact("TEST_PARAM", FactMetaData::valueTypeFloat);
    QVERIFY(fact);
    QVERIFY(qAbs(fact->rawIncrement() - 0.005) < 1e-6);
}

void APMParameterMetaDataTest::_handleDuplicateParam()
{
    QScopedPointer<APMParameterMetaData> meta(_loadFromJson(kAPMJson, nullptr));
    QVERIFY(meta);

    FactMetaData *fact = meta->getMetaDataForFact("TEST_DUP", FactMetaData::valueTypeInt32);
    QVERIFY(fact);
    // Duplicate param exists — last write wins (order depends on QJsonObject key sorting)
    QVERIFY(!fact->shortDescription().isEmpty());
}

void APMParameterMetaDataTest::_getMetaDataGenericFallback()
{
    QScopedPointer<APMParameterMetaData> meta(_loadFromJson(kAPMJson, nullptr));
    QVERIFY(meta);

    FactMetaData *fact = meta->getMetaDataForFact("NONEXISTENT", FactMetaData::valueTypeFloat);
    QVERIFY(fact);
    QCOMPARE(fact->category(), "Advanced");
}

void APMParameterMetaDataTest::_getMetaDataPIDDecimalPlaces()
{
    QScopedPointer<APMParameterMetaData> meta(_loadFromJson(kAPMJson, nullptr));
    QVERIFY(meta);

    FactMetaData *fact = meta->getMetaDataForFact("ATC_RAT_RLL_P", FactMetaData::valueTypeFloat);
    QVERIFY(fact);
    QCOMPARE(fact->decimalPlaces(), 6);
}

void APMParameterMetaDataTest::_parseCategory()
{
    QScopedPointer<APMParameterMetaData> meta(_loadFromJson(kAPMJson, nullptr));
    QVERIFY(meta);

    FactMetaData *fact = meta->getMetaDataForFact("TEST_PARAM", FactMetaData::valueTypeFloat);
    QVERIFY(fact);
    QCOMPARE(fact->category(), "Standard");

    FactMetaData *adv = meta->getMetaDataForFact("TEST_ENUM", FactMetaData::valueTypeInt32);
    QVERIFY(adv);
    QCOMPARE(adv->category(), "Advanced");
}

void APMParameterMetaDataTest::_parseUnits()
{
    QScopedPointer<APMParameterMetaData> meta(_loadFromJson(kAPMJson, nullptr));
    QVERIFY(meta);

    FactMetaData *fact = meta->getMetaDataForFact("TEST_PARAM", FactMetaData::valueTypeFloat);
    QVERIFY(fact);
    QCOMPARE(fact->rawUnits(), "1/s");
}

void APMParameterMetaDataTest::_loadGuard()
{
    QScopedPointer<APMParameterMetaData> meta(_loadFromJson(kAPMJson, nullptr));
    QVERIFY(meta);

    // Second load should be ignored
    meta->loadParameterFactMetaDataFile(QStringLiteral("nonexistent.json"));

    // Original data should still be intact
    FactMetaData *fact = meta->getMetaDataForFact("TEST_PARAM", FactMetaData::valueTypeFloat);
    QVERIFY(fact);
    QCOMPARE(fact->name(), "TEST_PARAM");
}

void APMParameterMetaDataTest::_loadMissingFile()
{
    APMParameterMetaData meta;
    meta.loadParameterFactMetaDataFile(QStringLiteral("/nonexistent/path/file.json"));

    FactMetaData *fact = meta.getMetaDataForFact("ANY", FactMetaData::valueTypeFloat);
    QVERIFY(fact);
    QCOMPARE(fact->category(), "Advanced");
}

void APMParameterMetaDataTest::_loadEmptyJson()
{
    QScopedPointer<APMParameterMetaData> meta(_loadFromJson("{}", nullptr));
    QVERIFY(meta);

    FactMetaData *fact = meta->getMetaDataForFact("ANY", FactMetaData::valueTypeFloat);
    QVERIFY(fact);
    QCOMPARE(fact->category(), "Advanced");
}

void APMParameterMetaDataTest::_loadBundledAPMMetaData()
{
    const QString file = QStringLiteral(":/FirmwarePlugin/APM/APMParameterFactMetaData.Copter.4.7.json");
    if (!QFile::exists(file)) {
        QSKIP("Bundled APM Copter 4.7 JSON metadata not available");
    }

    APMParameterMetaData meta;
    meta.loadParameterFactMetaDataFile(file);

    FactMetaData *thrFilt = meta.getMetaDataForFact("PILOT_THR_FILT", FactMetaData::valueTypeFloat);
    QVERIFY(thrFilt);
    QCOMPARE(thrFilt->name(), "PILOT_THR_FILT");
    QCOMPARE(thrFilt->rawUnits(), "Hz");

    FactMetaData *thrBhv = meta.getMetaDataForFact("PILOT_THR_BHV", FactMetaData::valueTypeInt32);
    QVERIFY(thrBhv);
    QVERIFY(thrBhv->bitmaskStrings().count() >= 3);

    FactMetaData *battMon = meta.getMetaDataForFact("BATT_MONITOR", FactMetaData::valueTypeFloat);
    QVERIFY(battMon);
    QVERIFY(!battMon->shortDescription().isEmpty());
}

void APMParameterMetaDataTest::_verifyFullAPMParse()
{
    const QString file = QStringLiteral(":/FirmwarePlugin/APM/APMParameterFactMetaData.Copter.4.7.json");
    if (!QFile::exists(file)) {
        QSKIP("Bundled APM Copter 4.7 JSON metadata not available");
    }

    // Load through parser
    APMParameterMetaData meta;
    meta.loadParameterFactMetaDataFile(file);

    // Load JSON directly to get param names
    QFile jsonFile(file);
    QVERIFY(jsonFile.open(QIODevice::ReadOnly));
    const QJsonDocument doc = QJsonDocument::fromJson(jsonFile.readAll());
    jsonFile.close();
    QVERIFY(doc.isObject());

    QStringList paramNames;
    const QJsonObject root = doc.object();
    for (auto groupIt = root.constBegin(); groupIt != root.constEnd(); ++groupIt) {
        if (!groupIt->isObject()) continue;
        const QJsonObject params = groupIt->toObject();
        for (auto paramIt = params.constBegin(); paramIt != params.constEnd(); ++paramIt) {
            if (paramIt->isObject() && !paramNames.contains(paramIt.key())) {
                paramNames.append(paramIt.key());
            }
        }
    }
    QVERIFY2(paramNames.count() > 1000, qPrintable(QString("Expected >1000 params, got %1").arg(paramNames.count())));

    int missingName = 0;
    int withDesc = 0;

    for (const QString &name : paramNames) {
        FactMetaData *fact = meta.getMetaDataForFact(name, FactMetaData::valueTypeFloat);
        QVERIFY2(fact, qPrintable(QString("null for %1").arg(name)));

        if (fact->name().isEmpty()) {
            missingName++;
            continue;
        }
        if (!fact->shortDescription().isEmpty()) withDesc++;
    }

    QCOMPARE(missingName, 0);
    QVERIFY2(withDesc > paramNames.count() / 2,
             qPrintable(QString("Only %1/%2 params have descriptions").arg(withDesc).arg(paramNames.count())));
}

void APMParameterMetaDataTest::_versionFromJsonDataAPMFormat()
{
    QJsonObject root;
    root[u"parameter_version_major"_s] = 4;
    root[u"parameter_version_minor"_s] = 7;
    const QByteArray data = QJsonDocument(root).toJson(QJsonDocument::Compact);

    const QVersionNumber version = ParameterMetaData::versionFromJsonData(data);
    QCOMPARE(version.majorVersion(), 4);
    QCOMPARE(version.minorVersion(), 7);

    // Major-only: minor defaults to 0
    QJsonObject majorOnly;
    majorOnly[u"parameter_version_major"_s] = 3;
    const QByteArray majorData = QJsonDocument(majorOnly).toJson(QJsonDocument::Compact);
    const QVersionNumber majorVer = ParameterMetaData::versionFromJsonData(majorData);
    QCOMPARE(majorVer.majorVersion(), 3);
    QCOMPARE(majorVer.minorVersion(), 0);
}

void APMParameterMetaDataTest::_invalidEnumKeySkipped()
{
    static const char *json = R"({
        "TEST_": {
            "TEST_INVENUM": {
                "DisplayName": "Inv Enum",
                "Description": "Enum with invalid key",
                "Values": {"abc": "Bad", "0": "Zero", "1": "One"}
            }
        }
    })";

    QScopedPointer<APMParameterMetaData> meta(_loadFromJson(json, nullptr));
    QVERIFY(meta);

    FactMetaData *fact = meta->getMetaDataForFact("TEST_INVENUM", FactMetaData::valueTypeInt32);
    QVERIFY(fact);
    QCOMPARE(fact->enumStrings().count(), 2);
    QCOMPARE(fact->enumStrings()[0], "Zero");
    QCOMPARE(fact->enumStrings()[1], "One");
}

void APMParameterMetaDataTest::_invalidBitmaskIndexSkipped()
{
    static const char *json = R"({
        "TEST_": {
            "TEST_INVBM": {
                "DisplayName": "Inv Bitmask",
                "Description": "Bitmask with non-numeric index",
                "Bitmask": {"abc": "Bad", "0": "First", "2": "Third"}
            }
        }
    })";

    QScopedPointer<APMParameterMetaData> meta(_loadFromJson(json, nullptr));
    QVERIFY(meta);

    FactMetaData *fact = meta->getMetaDataForFact("TEST_INVBM", FactMetaData::valueTypeInt32);
    QVERIFY(fact);
    QCOMPARE(fact->bitmaskStrings().count(), 2);
    QCOMPARE(fact->bitmaskStrings()[0], "First");
    QCOMPARE(fact->bitmaskStrings()[1], "Third");
    QCOMPARE(fact->bitmaskValues()[0].toInt(), 1);
    QCOMPARE(fact->bitmaskValues()[1].toInt(), 4);
}

void APMParameterMetaDataTest::_outOfRangeBitmaskIndexSkipped()
{
    static const char *json = R"({
        "TEST_": {
            "TEST_OORBM": {
                "DisplayName": "OOR Bitmask",
                "Description": "Bitmask with out-of-range index",
                "Bitmask": {"64": "TooHigh", "0": "First", "1": "Second"}
            }
        }
    })";

    QScopedPointer<APMParameterMetaData> meta(_loadFromJson(json, nullptr));
    QVERIFY(meta);

    FactMetaData *fact = meta->getMetaDataForFact("TEST_OORBM", FactMetaData::valueTypeInt32);
    QVERIFY(fact);
    QCOMPARE(fact->bitmaskStrings().count(), 2);
    QCOMPARE(fact->bitmaskStrings()[0], "First");
    QCOMPARE(fact->bitmaskStrings()[1], "Second");
}

UT_REGISTER_TEST(APMParameterMetaDataTest, TestLabel::Unit)
