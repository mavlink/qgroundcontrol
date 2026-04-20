#include "PX4ParameterMetaDataTest.h"
#include "ParameterMetaData.h"
#include "ParameterMetaDataTestHelper.h"
#include "PX4ParameterMetaData.h"

#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>

using namespace Qt::StringLiterals;

static QJsonObject _makeParam(const QString &name, const QString &type, const QJsonObject &extra = {})
{
    QJsonObject obj;
    obj[u"name"_s] = name;
    obj[u"type"_s] = type;
    for (auto it = extra.constBegin(); it != extra.constEnd(); ++it) {
        obj[it.key()] = it.value();
    }
    return obj;
}

static QJsonObject _makeRoot(const QJsonArray &params, int version = 1)
{
    QJsonObject root;
    root[u"version"_s] = version;
    root[u"parameters"_s] = params;
    return root;
}

static PX4ParameterMetaData *_loadFromJson(const QJsonArray &params, QObject *parent, int version = 1)
{
    const QByteArray data = QJsonDocument(_makeRoot(params, version)).toJson(QJsonDocument::Compact);
    return loadMetaDataFromJson<PX4ParameterMetaData>(data, parent);
}

void PX4ParameterMetaDataTest::_parseBasicParameter()
{
    QJsonObject extra;
    extra[u"shortDesc"_s] = u"Test parameter"_s;
    extra[u"longDesc"_s] = u"A longer description"_s;
    extra[u"default"_s] = 42;
    extra[u"min"_s] = 0;
    extra[u"max"_s] = 100;
    extra[u"units"_s] = u"m"_s;
    extra[u"decimalPlaces"_s] = 2;
    extra[u"increment"_s] = 1;
    extra[u"rebootRequired"_s] = true;
    extra[u"group"_s] = u"TEST"_s;
    extra[u"category"_s] = u"Standard"_s;

    QScopedPointer<PX4ParameterMetaData> meta(_loadFromJson({_makeParam(u"TEST_PARAM"_s, u"Int32"_s, extra)}, nullptr));
    QVERIFY(meta);

    FactMetaData *fact = meta->getMetaDataForFact("TEST_PARAM", FactMetaData::valueTypeInt32);
    QVERIFY(fact);
    QCOMPARE(fact->name(), "TEST_PARAM");
    QCOMPARE(fact->group(), "TEST");
    QCOMPARE(fact->category(), "Standard");
    QCOMPARE(fact->shortDescription(), "Test parameter");
    QCOMPARE(fact->longDescription(), "A longer description");
    QCOMPARE(fact->rawMin().toInt(), 0);
    QCOMPARE(fact->rawMax().toInt(), 100);
    QCOMPARE(fact->rawUnits(), "m");
    QCOMPARE(fact->decimalPlaces(), 2);
    QVERIFY(!qIsNaN(fact->rawIncrement()));
    QCOMPARE(fact->rawIncrement(), 1.0);
    QVERIFY(fact->vehicleRebootRequired());
    QVERIFY(fact->defaultValueAvailable());
    QCOMPARE(fact->rawDefaultValue().toInt(), 42);
}

void PX4ParameterMetaDataTest::_parseEnumValues()
{
    QJsonObject extra;
    extra[u"shortDesc"_s] = u"Enum param"_s;
    extra[u"default"_s] = 0;
    extra[u"values"_s] = QJsonArray({
        QJsonObject({{u"value"_s, 0}, {u"description"_s, u"Disabled"_s}}),
        QJsonObject({{u"value"_s, 1}, {u"description"_s, u"Enabled"_s}}),
        QJsonObject({{u"value"_s, 2}, {u"description"_s, u"Auto"_s}}),
    });

    QScopedPointer<PX4ParameterMetaData> meta(_loadFromJson({_makeParam(u"TEST_ENUM"_s, u"Int32"_s, extra)}, nullptr));
    QVERIFY(meta);

    FactMetaData *fact = meta->getMetaDataForFact("TEST_ENUM", FactMetaData::valueTypeInt32);
    QVERIFY(fact);
    QCOMPARE(fact->enumStrings().count(), 3);
    QCOMPARE(fact->enumStrings()[0], "Disabled");
    QCOMPARE(fact->enumStrings()[1], "Enabled");
    QCOMPARE(fact->enumStrings()[2], "Auto");
    QCOMPARE(fact->enumValues()[0].toInt(), 0);
    QCOMPARE(fact->enumValues()[1].toInt(), 1);
    QCOMPARE(fact->enumValues()[2].toInt(), 2);
}

void PX4ParameterMetaDataTest::_parseBitmask()
{
    QJsonObject extra;
    extra[u"shortDesc"_s] = u"Bitmask param"_s;
    extra[u"default"_s] = 0;
    extra[u"bitmask"_s] = QJsonArray({
        QJsonObject({{u"index"_s, 0}, {u"description"_s, u"First"_s}}),
        QJsonObject({{u"index"_s, 1}, {u"description"_s, u"Second"_s}}),
        QJsonObject({{u"index"_s, 2}, {u"description"_s, u"Third"_s}}),
    });

    QScopedPointer<PX4ParameterMetaData> meta(_loadFromJson({_makeParam(u"TEST_BITMASK"_s, u"Int32"_s, extra)}, nullptr));
    QVERIFY(meta);

    FactMetaData *fact = meta->getMetaDataForFact("TEST_BITMASK", FactMetaData::valueTypeInt32);
    QVERIFY(fact);
    QCOMPARE(fact->bitmaskStrings().count(), 3);
    QCOMPARE(fact->bitmaskStrings()[0], "First");
    QCOMPARE(fact->bitmaskStrings()[1], "Second");
    QCOMPARE(fact->bitmaskStrings()[2], "Third");
    QCOMPARE(fact->bitmaskValues()[0].toInt(), 1);
    QCOMPARE(fact->bitmaskValues()[1].toInt(), 2);
    QCOMPARE(fact->bitmaskValues()[2].toInt(), 4);
}

void PX4ParameterMetaDataTest::_parseVolatileReadonly()
{
    QJsonObject extra;
    extra[u"shortDesc"_s] = u"Volatile param"_s;
    extra[u"volatile"_s] = true;

    QScopedPointer<PX4ParameterMetaData> meta(_loadFromJson({_makeParam(u"TEST_VOL"_s, u"Float"_s, extra)}, nullptr));
    QVERIFY(meta);

    FactMetaData *fact = meta->getMetaDataForFact("TEST_VOL", FactMetaData::valueTypeFloat);
    QVERIFY(fact);
    QVERIFY(fact->readOnly());
    QVERIFY(fact->volatileValue());
}

void PX4ParameterMetaDataTest::_parseNonVolatileNotReadonly()
{
    QJsonObject extra;
    extra[u"shortDesc"_s] = u"Normal param"_s;

    QScopedPointer<PX4ParameterMetaData> meta(_loadFromJson({_makeParam(u"TEST_NORMAL"_s, u"Float"_s, extra)}, nullptr));
    QVERIFY(meta);

    FactMetaData *fact = meta->getMetaDataForFact("TEST_NORMAL", FactMetaData::valueTypeFloat);
    QVERIFY(fact);
    QVERIFY(!fact->readOnly());
    QVERIFY(!fact->volatileValue());
}

void PX4ParameterMetaDataTest::_parseNewlineReplacement()
{
    QJsonObject extra;
    extra[u"shortDesc"_s] = u"Line one\nLine two"_s;
    extra[u"longDesc"_s] = u"First\nSecond\nThird"_s;

    QScopedPointer<PX4ParameterMetaData> meta(_loadFromJson({_makeParam(u"TEST_NL"_s, u"Int32"_s, extra)}, nullptr));
    QVERIFY(meta);

    FactMetaData *fact = meta->getMetaDataForFact("TEST_NL", FactMetaData::valueTypeInt32);
    QVERIFY(fact);
    QCOMPARE(fact->shortDescription(), "Line one Line two");
    QCOMPARE(fact->longDescription(), "First Second Third");
}

void PX4ParameterMetaDataTest::_parseFloatDefault()
{
    QJsonObject extra;
    extra[u"shortDesc"_s] = u"Float param"_s;
    extra[u"default"_s] = 3.14;

    QScopedPointer<PX4ParameterMetaData> meta(_loadFromJson({_makeParam(u"TEST_FLOAT"_s, u"Float"_s, extra)}, nullptr));
    QVERIFY(meta);

    FactMetaData *fact = meta->getMetaDataForFact("TEST_FLOAT", FactMetaData::valueTypeFloat);
    QVERIFY(fact);
    QVERIFY(fact->defaultValueAvailable());
    QVERIFY(qAbs(fact->rawDefaultValue().toDouble() - 3.14) < 0.001);
}

void PX4ParameterMetaDataTest::_skipMissingNameOrType()
{
    QJsonArray params = {
        QJsonObject({{u"type"_s, u"Int32"_s}}),                        // missing name
        QJsonObject({{u"name"_s, u"NO_TYPE"_s}}),                      // missing type
        _makeParam(u"GOOD_PARAM"_s, u"Int32"_s),                       // valid
    };

    QScopedPointer<PX4ParameterMetaData> meta(_loadFromJson(params, nullptr));
    QVERIFY(meta);

    // Invalid entries skipped, valid one parsed
    FactMetaData *good = meta->getMetaDataForFact("GOOD_PARAM", FactMetaData::valueTypeInt32);
    QVERIFY(good);
    QCOMPARE(good->name(), "GOOD_PARAM");

    // Missing-name param should not exist — fallback is generic with empty name
    FactMetaData *noType = meta->getMetaDataForFact("NO_TYPE", FactMetaData::valueTypeInt32);
    QVERIFY(noType);
    QVERIFY(noType->name().isEmpty());
}

void PX4ParameterMetaDataTest::_parseRebootRequired()
{
    QJsonObject extra;
    extra[u"shortDesc"_s] = u"Test parameter"_s;
    extra[u"rebootRequired"_s] = true;

    QScopedPointer<PX4ParameterMetaData> meta(_loadFromJson({_makeParam(u"TEST_PARAM"_s, u"Int32"_s, extra)}, nullptr));
    QVERIFY(meta);

    FactMetaData *fact = meta->getMetaDataForFact("TEST_PARAM", FactMetaData::valueTypeInt32);
    QVERIFY(fact);
    QVERIFY(fact->vehicleRebootRequired());
}

void PX4ParameterMetaDataTest::_parseDecimalPlaces()
{
    QJsonObject extra;
    extra[u"shortDesc"_s] = u"Test parameter"_s;
    extra[u"decimalPlaces"_s] = 2;

    QScopedPointer<PX4ParameterMetaData> meta(_loadFromJson({_makeParam(u"TEST_PARAM"_s, u"Float"_s, extra)}, nullptr));
    QVERIFY(meta);

    FactMetaData *fact = meta->getMetaDataForFact("TEST_PARAM", FactMetaData::valueTypeFloat);
    QVERIFY(fact);
    QCOMPARE(fact->decimalPlaces(), 2);
}

void PX4ParameterMetaDataTest::_parseIncrement()
{
    QJsonObject extra;
    extra[u"shortDesc"_s] = u"Test parameter"_s;
    extra[u"increment"_s] = 1.0;

    QScopedPointer<PX4ParameterMetaData> meta(_loadFromJson({_makeParam(u"TEST_PARAM"_s, u"Int32"_s, extra)}, nullptr));
    QVERIFY(meta);

    FactMetaData *fact = meta->getMetaDataForFact("TEST_PARAM", FactMetaData::valueTypeInt32);
    QVERIFY(fact);
    QCOMPARE(fact->rawIncrement(), 1.0);
}

void PX4ParameterMetaDataTest::_parseMultipleGroups()
{
    QJsonObject extraA;
    extraA[u"shortDesc"_s] = u"Group A param"_s;
    extraA[u"group"_s] = u"GRP_A"_s;
    extraA[u"default"_s] = 1;

    QJsonObject extraB;
    extraB[u"shortDesc"_s] = u"Group B param"_s;
    extraB[u"group"_s] = u"GRP_B"_s;
    extraB[u"default"_s] = 3.14;

    QJsonArray params = {
        _makeParam(u"GRPA_PARAM"_s, u"Int32"_s, extraA),
        _makeParam(u"GRPB_PARAM"_s, u"Float"_s, extraB),
    };

    QScopedPointer<PX4ParameterMetaData> meta(_loadFromJson(params, nullptr));
    QVERIFY(meta);

    FactMetaData *factA = meta->getMetaDataForFact("GRPA_PARAM", FactMetaData::valueTypeInt32);
    QVERIFY(factA);
    QCOMPARE(factA->group(), "GRP_A");

    FactMetaData *factB = meta->getMetaDataForFact("GRPB_PARAM", FactMetaData::valueTypeFloat);
    QVERIFY(factB);
    QCOMPARE(factB->group(), "GRP_B");
}

void PX4ParameterMetaDataTest::_rejectOldVersion()
{
    QJsonObject extra;
    extra[u"shortDesc"_s] = u"Old"_s;

    QScopedPointer<PX4ParameterMetaData> meta(_loadFromJson({_makeParam(u"OLD_PARAM"_s, u"Int32"_s, extra)}, nullptr, 0));
    QVERIFY(meta);

    FactMetaData *fact = meta->getMetaDataForFact("OLD_PARAM", FactMetaData::valueTypeInt32);
    QVERIFY(fact);
    QVERIFY(fact->name().isEmpty());
}

void PX4ParameterMetaDataTest::_rejectInvalidType()
{
    QScopedPointer<PX4ParameterMetaData> meta(_loadFromJson({_makeParam(u"TEST_BAD"_s, u"BADTYPE"_s)}, nullptr));
    QVERIFY(meta);

    FactMetaData *fact = meta->getMetaDataForFact("TEST_BAD", FactMetaData::valueTypeInt32);
    QVERIFY(fact);
    QVERIFY(fact->name().isEmpty());
}

void PX4ParameterMetaDataTest::_handleDuplicateParam()
{
    QJsonObject extra1;
    extra1[u"shortDesc"_s] = u"First"_s;
    extra1[u"default"_s] = 1;

    QJsonObject extra2;
    extra2[u"shortDesc"_s] = u"Second"_s;
    extra2[u"default"_s] = 2;

    QJsonArray params = {
        _makeParam(u"DUP_PARAM"_s, u"Int32"_s, extra1),
        _makeParam(u"DUP_PARAM"_s, u"Int32"_s, extra2),
    };

    QScopedPointer<PX4ParameterMetaData> meta(_loadFromJson(params, nullptr));
    QVERIFY(meta);

    FactMetaData *fact = meta->getMetaDataForFact("DUP_PARAM", FactMetaData::valueTypeInt32);
    QVERIFY(fact);
    QCOMPARE(fact->shortDescription(), "Second");
    QCOMPARE(fact->rawDefaultValue().toInt(), 2);
}

void PX4ParameterMetaDataTest::_getMetaDataFallback()
{
    QJsonObject extra;
    extra[u"shortDesc"_s] = u"Test"_s;

    QScopedPointer<PX4ParameterMetaData> meta(_loadFromJson({_makeParam(u"TEST_PARAM"_s, u"Int32"_s, extra)}, nullptr));
    QVERIFY(meta);

    FactMetaData *fact = meta->getMetaDataForFact("NONEXISTENT", FactMetaData::valueTypeFloat);
    QVERIFY(fact);
    QCOMPARE(fact->type(), FactMetaData::valueTypeFloat);
}

void PX4ParameterMetaDataTest::_getParameterMetaDataVersionInfo()
{
    // Schema-only version (no parameter_version_major/minor) → null
    const QByteArray schemaData = QJsonDocument(_makeRoot({}, 1)).toJson(QJsonDocument::Compact);
    QTemporaryFile schemaFile;
    QVERIFY(writeJsonToTempFile(schemaFile, schemaData));
    QVERIFY(ParameterMetaData::versionFromMetaDataFile(schemaFile.fileName()).isNull());

    // Explicit parameter catalog version → extracted
    QJsonObject versioned = _makeRoot({}, 1);
    versioned[u"parameter_version_major"_s] = 1;
    versioned[u"parameter_version_minor"_s] = 15;
    const QByteArray versionedData = QJsonDocument(versioned).toJson(QJsonDocument::Compact);
    QTemporaryFile versionedFile;
    QVERIFY(writeJsonToTempFile(versionedFile, versionedData));
    const QVersionNumber version = ParameterMetaData::versionFromMetaDataFile(versionedFile.fileName());
    QCOMPARE(version.majorVersion(), 1);
    QCOMPARE(version.minorVersion(), 15);
}

void PX4ParameterMetaDataTest::_parseCategory()
{
    QJsonObject devExtra;
    devExtra[u"shortDesc"_s] = u"Developer param"_s;
    devExtra[u"category"_s] = u"Developer"_s;

    QScopedPointer<PX4ParameterMetaData> meta(_loadFromJson({_makeParam(u"TEST_CAT"_s, u"Int32"_s, devExtra)}, nullptr));
    QVERIFY(meta);

    FactMetaData *fact = meta->getMetaDataForFact("TEST_CAT", FactMetaData::valueTypeInt32);
    QVERIFY(fact);
    QCOMPARE(fact->category(), "Developer");

    // Default category should be "Standard"
    QScopedPointer<PX4ParameterMetaData> meta2(_loadFromJson({_makeParam(u"TEST_STD"_s, u"Int32"_s)}, nullptr));
    QVERIFY(meta2);
    FactMetaData *fact2 = meta2->getMetaDataForFact("TEST_STD", FactMetaData::valueTypeInt32);
    QVERIFY(fact2);
    QCOMPARE(fact2->category(), "Standard");
}

void PX4ParameterMetaDataTest::_getVersionInfoMissingFile()
{
    const QVersionNumber version = ParameterMetaData::versionFromMetaDataFile(
        QStringLiteral("/nonexistent/path/file.json"));
    QVERIFY(version.isNull());
}

void PX4ParameterMetaDataTest::_getVersionInfoInvalidJson()
{
    QTemporaryFile tmpFile;
    tmpFile.setFileTemplate(QStringLiteral("XXXXXX.json"));
    QVERIFY(tmpFile.open());
    tmpFile.write("not valid json{{{");
    tmpFile.close();

    const QVersionNumber version = ParameterMetaData::versionFromMetaDataFile(tmpFile.fileName());
    QVERIFY(version.isNull());
}

void PX4ParameterMetaDataTest::_loadGuard()
{
    QJsonObject extra;
    extra[u"shortDesc"_s] = u"Test"_s;

    QScopedPointer<PX4ParameterMetaData> first(_loadFromJson({_makeParam(u"TEST_PARAM"_s, u"Int32"_s, extra)}, nullptr));
    QVERIFY(first);

    // Second load should be ignored — _parameterMetaDataLoaded guard
    first->loadParameterFactMetaDataFile(QStringLiteral("nonexistent.json"));

    FactMetaData *fact = first->getMetaDataForFact("TEST_PARAM", FactMetaData::valueTypeInt32);
    QVERIFY(fact);
    QCOMPARE(fact->name(), "TEST_PARAM");
}

void PX4ParameterMetaDataTest::_loadMissingFile()
{
    PX4ParameterMetaData meta;
    meta.loadParameterFactMetaDataFile(QStringLiteral("/nonexistent/path/file.json"));

    FactMetaData *fact = meta.getMetaDataForFact("ANY_PARAM", FactMetaData::valueTypeFloat);
    QVERIFY(fact);
    QVERIFY(fact->name().isEmpty());
}

void PX4ParameterMetaDataTest::_loadBundledPX4MetaData()
{
    const QString file = QStringLiteral(":/FirmwarePlugin/PX4/PX4ParameterFactMetaData.json");
    QVERIFY(QFile::exists(file));

    PX4ParameterMetaData meta;
    meta.loadParameterFactMetaDataFile(file);

    // PX4's upstream parameters.json only stamps the schema "version" key,
    // which versionFromJsonData intentionally ignores. The bundled file
    // therefore yields a null parameter-set version — the cache layer
    // must tolerate this and is exercised separately.
    const QVersionNumber version = ParameterMetaData::versionFromMetaDataFile(file);
    QVERIFY(version.isNull());

    // Spot-check well-known PX4 parameters
    FactMetaData *adsb = meta.getMetaDataForFact("ADSB_CALLSIGN_1", FactMetaData::valueTypeInt32);
    QVERIFY(adsb);
    QCOMPARE(adsb->name(), "ADSB_CALLSIGN_1");
    QVERIFY(adsb->vehicleRebootRequired());

    FactMetaData *emergc = meta.getMetaDataForFact("ADSB_EMERGC", FactMetaData::valueTypeInt32);
    QVERIFY(emergc);
    QCOMPARE(emergc->rawMin().toInt(), 0);
    QCOMPARE(emergc->rawMax().toInt(), 6);
    QVERIFY(emergc->enumStrings().count() >= 6);
}

void PX4ParameterMetaDataTest::_verifyFullPX4Parse()
{
    const QString file = QStringLiteral(":/FirmwarePlugin/PX4/PX4ParameterFactMetaData.json");
    QVERIFY(QFile::exists(file));

    // First pass: extract all parameter names and types from JSON
    QFile jsonFile(file);
    QVERIFY(jsonFile.open(QIODevice::ReadOnly));
    const QJsonDocument doc = QJsonDocument::fromJson(jsonFile.readAll());
    jsonFile.close();
    QVERIFY(doc.isObject());

    const QJsonArray parameters = doc.object().value(u"parameters").toArray();
    QVERIFY2(parameters.count() > 1000, qPrintable(QString("Expected >1000 params, got %1").arg(parameters.count())));

    // Second pass: load through the parser and verify every parameter is accessible
    PX4ParameterMetaData meta;
    meta.loadParameterFactMetaDataFile(file);

    int withDesc = 0;
    int withMinMax = 0;
    int withEnum = 0;
    int withBitmask = 0;
    int withReboot = 0;

    for (const QJsonValue &paramVal : parameters) {
        const QJsonObject param = paramVal.toObject();
        const QString name = param.value(u"name").toString();
        const QString type = param.value(u"type").toString();

        bool unknownType = false;
        const FactMetaData::ValueType_t valueType = FactMetaData::stringToType(type, unknownType);
        if (unknownType) continue;

        FactMetaData *fact = meta.getMetaDataForFact(name, valueType);
        QVERIFY2(fact, qPrintable(QString("getMetaDataForFact returned null for %1").arg(name)));
        QCOMPARE(fact->name(), name);

        if (!fact->shortDescription().isEmpty()) withDesc++;
        if (!fact->rawMin().isNull() && !fact->maxIsDefaultForType()) withMinMax++;
        if (!fact->enumStrings().isEmpty()) withEnum++;
        if (!fact->bitmaskStrings().isEmpty()) withBitmask++;
        if (fact->vehicleRebootRequired()) withReboot++;
    }

    QVERIFY2(withDesc > parameters.count() / 2,
             qPrintable(QString("Only %1/%2 params have descriptions").arg(withDesc).arg(parameters.count())));
    QVERIFY2(withMinMax > 0, "Expected at least one PX4 parameter with min/max metadata");
    QVERIFY2(withEnum > 0, "Expected at least one PX4 parameter with enum metadata");
    QVERIFY2(withBitmask > 0, "Expected at least one PX4 parameter with bitmask metadata");
    QVERIFY2(withReboot > 0, "Expected at least one PX4 parameter with reboot-required metadata");
}

void PX4ParameterMetaDataTest::_versionFromFileName()
{
    QCOMPARE(ParameterMetaData::versionFromFileName("ParameterFactMetaData_3.4.7.json"),
             QVersionNumber(4, 7));
    QCOMPARE(ParameterMetaData::versionFromFileName("Foo.1.0.json"),
             QVersionNumber(1, 0));
    QCOMPARE(ParameterMetaData::versionFromFileName("cache/PX4.12.34.json"),
             QVersionNumber(12, 34));
}

void PX4ParameterMetaDataTest::_versionFromFileNameNoMatch()
{
    QVERIFY(ParameterMetaData::versionFromFileName("Foo.json").isNull());
    QVERIFY(ParameterMetaData::versionFromFileName("Foo.4.json").isNull());
    QVERIFY(ParameterMetaData::versionFromFileName("").isNull());
    QVERIFY(ParameterMetaData::versionFromFileName("Foo.4.7.xml").isNull());
}

void PX4ParameterMetaDataTest::_versionFromJsonDataAPMFormat()
{
    QJsonObject root;
    root[u"parameter_version_major"_s] = 4;
    root[u"parameter_version_minor"_s] = 7;
    const QByteArray data = QJsonDocument(root).toJson(QJsonDocument::Compact);

    const QVersionNumber version = ParameterMetaData::versionFromJsonData(data);
    QCOMPARE(version.majorVersion(), 4);
    QCOMPARE(version.minorVersion(), 7);
}

void PX4ParameterMetaDataTest::_versionFromJsonDataNoVersion()
{
    QJsonObject root;
    root[u"parameters"_s] = QJsonArray();
    const QByteArray data = QJsonDocument(root).toJson(QJsonDocument::Compact);

    QVERIFY(ParameterMetaData::versionFromJsonData(data).isNull());
    QVERIFY(ParameterMetaData::versionFromJsonData("not json{{{").isNull());
}

UT_REGISTER_TEST(PX4ParameterMetaDataTest, TestLabel::Unit)
