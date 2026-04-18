#include "CameraSpecTest.h"

#include "CameraSpec.h"
#include "SettingsFact.h"

#include <QtCore/QJsonObject>
#include <QtCore/QJsonValue>
#include <QtCore/QRandomGenerator>
#include <QtCore/QString>
#include <QtTest/QSignalSpy>

namespace {

// Unique settings group per-test so persistent QSettings state from one test
// doesn't bleed into another.
QString uniqueGroup(const char* label)
{
    return QStringLiteral("UT_CameraSpecTest_%1_%2").arg(QString::fromLatin1(label)).arg(QRandomGenerator::global()->generate());
}

// Populate a CameraSpec with known non-default values so round-trip tests can
// detect incorrect key mapping in save()/load().
void populate(CameraSpec& spec)
{
    spec.sensorWidth()->setRawValue(13.2);
    spec.sensorHeight()->setRawValue(8.8);
    spec.imageWidth()->setRawValue(5472);
    spec.imageHeight()->setRawValue(3648);
    spec.focalLength()->setRawValue(10.26);
    spec.landscape()->setRawValue(false);
    spec.fixedOrientation()->setRawValue(true);
    spec.minTriggerInterval()->setRawValue(2.5);
}

// Minimal valid JSON object mirroring the CameraSpec v1 schema.
QJsonObject makeValidJson()
{
    QJsonObject obj;
    obj["SensorWidth"]        = 13.2;
    obj["SensorHeight"]       = 8.8;
    obj["ImageWidth"]         = 5472;
    obj["ImageHeight"]        = 3648;
    obj["FocalLength"]        = 10.26;
    obj["Landscape"]          = false;
    obj["FixedOrientation"]   = true;
    obj["MinTriggerInterval"] = 2.5;
    return obj;
}

} // namespace

// ---------------------------------------------------------------------------
// Defaults
// ---------------------------------------------------------------------------

void CameraSpecTest::_testDefaultValues()
{
    CameraSpec spec(uniqueGroup("defaults"));

    // Values come from src/MissionManager/CameraSpec.FactMetaData.json.
    QCOMPARE(spec.sensorWidth()->rawValue().toDouble(),         6.17);
    QCOMPARE(spec.sensorHeight()->rawValue().toDouble(),        4.55);
    QCOMPARE(spec.imageWidth()->rawValue().toUInt(),            4000u);
    QCOMPARE(spec.imageHeight()->rawValue().toUInt(),           3000u);
    QCOMPARE(spec.focalLength()->rawValue().toDouble(),         4.5);
    QCOMPARE(spec.landscape()->rawValue().toBool(),             true);
    QCOMPARE(spec.fixedOrientation()->rawValue().toBool(),      false);
    QCOMPARE(spec.minTriggerInterval()->rawValue().toDouble(),  1.0);
}

// ---------------------------------------------------------------------------
// save()
// ---------------------------------------------------------------------------

void CameraSpecTest::_testSaveProducesExpectedKeysAndTypes()
{
    CameraSpec spec(uniqueGroup("save"));
    populate(spec);

    QJsonObject json;
    spec.save(json);

    // All eight persistent keys are present.
    const QStringList requiredKeys = {
        QStringLiteral("SensorWidth"),
        QStringLiteral("SensorHeight"),
        QStringLiteral("ImageWidth"),
        QStringLiteral("ImageHeight"),
        QStringLiteral("FocalLength"),
        QStringLiteral("Landscape"),
        QStringLiteral("FixedOrientation"),
        QStringLiteral("MinTriggerInterval"),
    };
    for (const QString& key : requiredKeys) {
        QVERIFY2(json.contains(key), qPrintable(QStringLiteral("missing key: %1").arg(key)));
    }

    // Types: numeric → Double, orientation flags → Bool.
    QCOMPARE(json["SensorWidth"].type(),        QJsonValue::Double);
    QCOMPARE(json["ImageWidth"].type(),         QJsonValue::Double);
    QCOMPARE(json["FocalLength"].type(),        QJsonValue::Double);
    QCOMPARE(json["MinTriggerInterval"].type(), QJsonValue::Double);
    QCOMPARE(json["Landscape"].type(),          QJsonValue::Bool);
    QCOMPARE(json["FixedOrientation"].type(),   QJsonValue::Bool);

    // Values survive the save exactly.
    QCOMPARE(json["SensorWidth"].toDouble(),        13.2);
    QCOMPARE(json["ImageWidth"].toInt(),            5472);
    QCOMPARE(json["ImageHeight"].toInt(),           3648);
    QCOMPARE(json["FocalLength"].toDouble(),        10.26);
    QCOMPARE(json["Landscape"].toBool(),            false);
    QCOMPARE(json["FixedOrientation"].toBool(),     true);
    QCOMPARE(json["MinTriggerInterval"].toDouble(), 2.5);
}

// ---------------------------------------------------------------------------
// load() — happy path
// ---------------------------------------------------------------------------

void CameraSpecTest::_testLoadValidJsonRoundTrip()
{
    CameraSpec spec(uniqueGroup("loadValid"));

    const QJsonObject source = makeValidJson();
    QString errorString;
    QVERIFY2(spec.load(source, errorString), qPrintable(errorString));
    QVERIFY(errorString.isEmpty());

    QCOMPARE(spec.sensorWidth()->rawValue().toDouble(),         13.2);
    QCOMPARE(spec.sensorHeight()->rawValue().toDouble(),        8.8);
    QCOMPARE(spec.imageWidth()->rawValue().toUInt(),            5472u);
    QCOMPARE(spec.imageHeight()->rawValue().toUInt(),           3648u);
    QCOMPARE(spec.focalLength()->rawValue().toDouble(),         10.26);
    QCOMPARE(spec.landscape()->rawValue().toBool(),             false);
    QCOMPARE(spec.fixedOrientation()->rawValue().toBool(),      true);
    QCOMPARE(spec.minTriggerInterval()->rawValue().toDouble(),  2.5);

    // Save-after-load produces JSON identical to the input — full round trip.
    QJsonObject reSaved;
    spec.save(reSaved);
    QCOMPARE(reSaved, source);
}

// ---------------------------------------------------------------------------
// load() — error paths
// ---------------------------------------------------------------------------

void CameraSpecTest::_testLoadMissingKeyFails()
{
    CameraSpec spec(uniqueGroup("loadMissing"));

    QJsonObject source = makeValidJson();
    source.remove(QStringLiteral("FocalLength"));

    QString errorString;
    QVERIFY(!spec.load(source, errorString));
    QVERIFY2(!errorString.isEmpty(), "load() must populate errorString on failure");
    // Error message should identify the missing field so users can diagnose.
    QVERIFY2(errorString.contains(QStringLiteral("FocalLength")),
             qPrintable(errorString));
}

void CameraSpecTest::_testLoadWrongTypeFails()
{
    CameraSpec spec(uniqueGroup("loadType"));

    QJsonObject source = makeValidJson();
    // Boolean field supplied as a string — should be rejected by
    // JsonHelper::validateKeys before any setRawValue call.
    source["Landscape"] = QStringLiteral("not-a-bool");

    QString errorString;
    QVERIFY(!spec.load(source, errorString));
    QVERIFY(!errorString.isEmpty());
    QVERIFY2(errorString.contains(QStringLiteral("Landscape")),
             qPrintable(errorString));
}

// ---------------------------------------------------------------------------
// operator=
// ---------------------------------------------------------------------------

void CameraSpecTest::_testAssignmentCopiesAllFacts()
{
    CameraSpec source(uniqueGroup("assignSrc"));
    populate(source);

    CameraSpec dest(uniqueGroup("assignDst"));
    // Pre-check: dest is at defaults, not equal to source values.
    QVERIFY(dest.sensorWidth()->rawValue().toDouble() != source.sensorWidth()->rawValue().toDouble());

    dest = source;

    // Every persistent fact is copied.
    QCOMPARE(dest.sensorWidth()->rawValue().toDouble(),        source.sensorWidth()->rawValue().toDouble());
    QCOMPARE(dest.sensorHeight()->rawValue().toDouble(),       source.sensorHeight()->rawValue().toDouble());
    QCOMPARE(dest.imageWidth()->rawValue().toUInt(),           source.imageWidth()->rawValue().toUInt());
    QCOMPARE(dest.imageHeight()->rawValue().toUInt(),          source.imageHeight()->rawValue().toUInt());
    QCOMPARE(dest.focalLength()->rawValue().toDouble(),        source.focalLength()->rawValue().toDouble());
    QCOMPARE(dest.landscape()->rawValue().toBool(),            source.landscape()->rawValue().toBool());
    QCOMPARE(dest.fixedOrientation()->rawValue().toBool(),     source.fixedOrientation()->rawValue().toBool());
    QCOMPARE(dest.minTriggerInterval()->rawValue().toDouble(), source.minTriggerInterval()->rawValue().toDouble());

    // Self-assignment must be safe and leave values unchanged.
    const double beforeSensorWidth = source.sensorWidth()->rawValue().toDouble();
    // Route through a const reference to suppress -Wself-assign-overloaded while still exercising operator=.
    const CameraSpec &sourceRef = source;
    source = sourceRef;
    QCOMPARE(source.sensorWidth()->rawValue().toDouble(), beforeSensorWidth);
}

// ---------------------------------------------------------------------------
// Dirty flag
// ---------------------------------------------------------------------------

void CameraSpecTest::_testDirtyInitiallyFalse()
{
    CameraSpec spec(uniqueGroup("dirtyInit"));
    QVERIFY(!spec.dirty());
}

void CameraSpecTest::_testSetDirtyEmitsOnlyOnChange()
{
    CameraSpec spec(uniqueGroup("dirtySig"));

    QSignalSpy dirtySpy(&spec, &CameraSpec::dirtyChanged);

    // false -> false: no signal.
    spec.setDirty(false);
    QCOMPARE(dirtySpy.count(), 0);

    // false -> true: signal emitted with argument `true`.
    spec.setDirty(true);
    QCOMPARE(dirtySpy.count(), 1);
    QCOMPARE(dirtySpy.takeFirst().at(0).toBool(), true);
    QVERIFY(spec.dirty());

    // true -> true: no signal.
    spec.setDirty(true);
    QCOMPARE(dirtySpy.count(), 0);

    // true -> false: signal emitted with argument `false`.
    spec.setDirty(false);
    QCOMPARE(dirtySpy.count(), 1);
    QCOMPARE(dirtySpy.takeFirst().at(0).toBool(), false);
    QVERIFY(!spec.dirty());
}

UT_REGISTER_TEST(CameraSpecTest, TestLabel::Unit, TestLabel::MissionManager)
