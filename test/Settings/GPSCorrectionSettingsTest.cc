#include "GPSCorrectionSettingsTest.h"

#include <memory>

#include <QtCore/QFile>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QSettings>
#include <QtTest/QSignalSpy>

#include "GPSCorrectionSettings.h"
#include "NTRIPSettings.h"
#include "SettingsFact.h"
#include "SettingsManager.h"
#include "Support/GPSQmlTestHelpers.h"

void GPSCorrectionSettingsTest::_storageNamespace_data()
{
    QTest::addColumn<QString>("name");
    QTest::addColumn<QVariant>("savedValue");
    QTest::addColumn<QVariant>("replacement");
    QTest::newRow("udp-enabled") << "rtcmUdpInputEnabled" << QVariant(false) << QVariant(true);
    QTest::newRow("udp-port") << "rtcmUdpInputPort" << QVariant(13321U) << QVariant(13322U);
}

void GPSCorrectionSettingsTest::_storageNamespace()
{
    QFETCH(QString, name);
    QFETCH(QVariant, savedValue);
    QFETCH(QVariant, replacement);
    QSettings storage;
    const QString storedKey = QStringLiteral("NTRIP/") + name;
    const QString newKey = QStringLiteral("GPSCorrection/") + name;
    storage.setValue(storedKey, savedValue);
    GPSCorrectionSettings corrections;
    QCOMPARE(static_cast<SettingsGroup*>(&corrections)->settingsGroup(), QStringLiteral("NTRIP"));
    auto* canonical = corrections.property(name.toUtf8().constData()).value<Fact*>();
    QVERIFY(canonical);
    QCOMPARE(canonical->parent(), &corrections);
    // Unit mode defaults Facts without overwriting persisted values.
    QCOMPARE(storage.value(storedKey), savedValue);
    QSignalSpy changed(canonical, &Fact::rawValueChanged);
    canonical->setRawValue(replacement);
    QCOMPARE(changed.size(), 1);
    QCOMPARE(canonical->rawValue(), replacement);
    QCOMPARE(storage.value(storedKey), replacement);
    QVERIFY(!storage.contains(newKey));
}

void GPSCorrectionSettingsTest::_metadataPartition()
{
    const auto names = [](const QString& path) {
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly)) {
            return QStringList();
        }
        const auto document = QJsonDocument::fromJson(file.readAll());
        if (!document.isObject()) {
            return QStringList();
        }
        QStringList result;
        for (const auto& value : document.object().value(QStringLiteral("QGC.MetaData.Facts")).toArray()) {
            result.append(value.toObject().value(QStringLiteral("name")).toString());
        }
        result.sort();
        return result;
    };
    const QStringList corrections = names(QStringLiteral(":/json/GPSCorrection.SettingsGroup.json"));
    QCOMPARE(corrections, QStringList({"correctionSource", "rtcmUdpInputEnabled", "rtcmUdpInputPort",
                                       "rtcmUdpOutputAddress", "rtcmUdpOutputEnabled", "rtcmUdpOutputPort"}));
    const QStringList ntrip = names(QStringLiteral(":/json/NTRIP.SettingsGroup.json"));
    QVERIFY(!ntrip.isEmpty());
    NTRIPSettings ntripSettings;
    for (const auto& name : corrections) {
        QVERIFY(!ntrip.contains(name));
        QCOMPARE(ntripSettings.metaObject()->indexOfProperty(name.toUtf8().constData()), -1);
    }
    QVERIFY(!ntrip.contains(QStringLiteral("ntripUdpForwardEnabled")));
    QVERIFY(ntrip.contains(QStringLiteral("ntripGgaPositionSource")));
}

void GPSCorrectionSettingsTest::_udpOutputKeysMigrate()
{
    const QStringList legacyKeys = {QStringLiteral("ntripUdpForwardEnabled"), QStringLiteral("ntripUdpTargetAddress"),
                                    QStringLiteral("ntripUdpTargetPort")};
    QSettings storage;
    storage.beginGroup(QStringLiteral("NTRIP"));
    storage.setValue(QStringLiteral("ntripUdpForwardEnabled"), true);
    storage.setValue(QStringLiteral("ntripUdpTargetAddress"), QStringLiteral("192.0.2.10"));
    storage.setValue(QStringLiteral("ntripUdpTargetPort"), 9000);
    // A value already saved under the new key wins over the legacy one.
    storage.setValue(QStringLiteral("rtcmUdpOutputPort"), 9001);
    const GPSCorrectionSettings corrections;
    QCOMPARE(storage.value(QStringLiteral("rtcmUdpOutputEnabled")).toBool(), true);
    QCOMPARE(storage.value(QStringLiteral("rtcmUdpOutputAddress")).toString(), QStringLiteral("192.0.2.10"));
    QCOMPARE(storage.value(QStringLiteral("rtcmUdpOutputPort")).toInt(), 9001);
    for (const auto& legacy : legacyKeys) {
        QVERIFY(!storage.contains(legacy));
    }
}

void GPSCorrectionSettingsTest::_removedSettingsAreDropped_data()
{
    QTest::addColumn<QString>("key");
    QTest::addColumn<QVariant>("value");
    QTest::newRow("all-sources-choice") << QStringLiteral("correctionSource") << QVariant(4);
    QTest::newRow("stream-pin") << QStringLiteral("correctionSourceInstance")
                                << QVariant(QStringLiteral("ntrip://caster.example.com:2101/MOUNT"));
}

void GPSCorrectionSettingsTest::_removedSettingsAreDropped()
{
    QFETCH(QString, key);
    QFETCH(QVariant, value);
    QSettings storage;
    storage.beginGroup(QStringLiteral("NTRIP"));
    storage.setValue(key, value);
    const GPSCorrectionSettings corrections;
    QVERIFY(!storage.contains(key));
}

void GPSCorrectionSettingsTest::_qmlRegistration()
{
    auto* settings = SettingsManager::instance();
    QVERIFY(settings->gpsCorrectionSettings());
    QCOMPARE(settings->property("gpsCorrectionSettings").value<QObject*>(), settings->gpsCorrectionSettings());
    GPSTest::QmlEngine engine;
    std::unique_ptr<QObject> object = engine.create(QByteArray(R"(
        import QtQml
        import QGroundControl
        QtObject {
            readonly property var corrections: QGroundControl.settingsManager.gpsCorrectionSettings
            readonly property var port: corrections.rtcmUdpInputPort
            readonly property SettingsFact sourceFact: corrections.correctionSource as SettingsFact
            readonly property bool sourceVisible: sourceFact.userVisible
            readonly property int highestPriority: GPSCorrectionSettings.HighestPriority
            readonly property int udp: GPSCorrectionSettings.Udp
        }
    )"));
    QVERIFY2(object, qPrintable(engine.lastError()));
    QCOMPARE(object->property("corrections").value<QObject*>(), settings->gpsCorrectionSettings());
    QCOMPARE(object->property("port").value<Fact*>(), settings->gpsCorrectionSettings()->rtcmUdpInputPort());
    QCOMPARE(object->property("sourceFact").value<SettingsFact*>(),
             qobject_cast<SettingsFact*>(settings->gpsCorrectionSettings()->correctionSource()));
    QCOMPARE(object->property("sourceVisible").toBool(),
             settings->gpsCorrectionSettings()->correctionSource()->property("userVisible").toBool());
    QCOMPARE(object->property("highestPriority").toInt(), int(GPSCorrectionSettings::HighestPriority));
    QCOMPARE(object->property("udp").toInt(), int(GPSCorrectionSettings::Udp));
}

void GPSCorrectionSettingsTest::_routingDefaults()
{
    GPSCorrectionSettings corrections;
    QCOMPARE(corrections.correctionSource()->rawValue().toInt(), int(GPSCorrectionSettings::HighestPriority));
    QCOMPARE(corrections.correctionSource()->enumValues(), QVariantList({0U, 1U, 2U, 3U}));
}

UT_REGISTER_TEST(GPSCorrectionSettingsTest, TestLabel::Unit)
