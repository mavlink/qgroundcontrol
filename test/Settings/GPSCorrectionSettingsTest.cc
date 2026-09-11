#include "GPSCorrectionSettingsTest.h"

#include <QtCore/QFile>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QScopeGuard>
#include <QtCore/QSettings>
#include <QtQml/QQmlComponent>
#include <QtQml/QQmlEngine>
#include <QtTest/QSignalSpy>

#include <memory>

#include "GPSCorrectionSettings.h"
#include "NTRIPSettings.h"
#include "SettingsManager.h"

void GPSCorrectionSettingsTest::_legacyKeysAndFactIdentity_data()
{
    QTest::addColumn<QString>("name");
    QTest::addColumn<QVariant>("savedValue");
    QTest::addColumn<QVariant>("replacement");
    QTest::newRow("udp-enabled") << "rtcmUdpInputEnabled" << QVariant(false) << QVariant(true);
    QTest::newRow("udp-port") << "rtcmUdpInputPort" << QVariant(13321U) << QVariant(13322U);
    QTest::newRow("udp-validation") << "rtcmUdpValidate" << QVariant(true) << QVariant(false);
    QTest::newRow("source") << "correctionSource" << QVariant(1U) << QVariant(3U);
    QTest::newRow("instance") << "correctionSourceInstance" << QVariant("legacy-stream") << QVariant("other-stream");
    QTest::newRow("receiver-injection") << "injectLocalReceiver" << QVariant(false) << QVariant(true);
}

void GPSCorrectionSettingsTest::_legacyKeysAndFactIdentity()
{
    QFETCH(QString, name);
    QFETCH(QVariant, savedValue);
    QFETCH(QVariant, replacement);
    QSettings storage;
    const QString legacyKey = QStringLiteral("NTRIP/") + name;
    const QString newKey = QStringLiteral("GPSCorrection/") + name;
    const bool hadLegacy = storage.contains(legacyKey);
    const bool hadNew = storage.contains(newKey);
    const QVariant originalLegacy = storage.value(legacyKey);
    const QVariant originalNew = storage.value(newKey);
    const auto restore = qScopeGuard([&]() {
        if (hadLegacy) {
            storage.setValue(legacyKey, originalLegacy);
        } else {
            storage.remove(legacyKey);
        }
        if (hadNew) {
            storage.setValue(newKey, originalNew);
        } else {
            storage.remove(newKey);
        }
    });
    storage.setValue(legacyKey, savedValue);
    storage.remove(newKey);
    GPSCorrectionSettings corrections;
    NTRIPSettings legacy(corrections);
    QCOMPARE(static_cast<SettingsGroup*>(&corrections)->settingsGroup(), QStringLiteral("NTRIP"));
    auto* canonical = corrections.property(name.toUtf8().constData()).value<Fact*>();
    auto* compatibility = legacy.property(name.toUtf8().constData()).value<Fact*>();
    QVERIFY(canonical);
    QCOMPARE(compatibility, canonical);
    QCOMPARE(canonical->parent(), &corrections);
    // Unit mode deliberately ignores stored values; construction must still leave the old key intact.
    QCOMPARE(storage.value(legacyKey), savedValue);
    QSignalSpy changed(canonical, &Fact::rawValueChanged);
    compatibility->setRawValue(replacement);
    QCOMPARE(changed.size(), 1);
    QCOMPARE(canonical->rawValue(), replacement);
    QCOMPARE(storage.value(legacyKey), replacement);
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
    QCOMPARE(corrections, QStringList({"correctionSource", "correctionSourceInstance", "injectLocalReceiver",
                                       "rtcmUdpInputEnabled", "rtcmUdpInputPort", "rtcmUdpValidate"}));
    const QStringList ntrip = names(QStringLiteral(":/json/NTRIP.SettingsGroup.json"));
    QVERIFY(!ntrip.isEmpty());
    for (const auto& name : corrections) {
        QVERIFY(!ntrip.contains(name));
    }
    QVERIFY(ntrip.contains(QStringLiteral("ntripUdpForwardEnabled")));
    QVERIFY(ntrip.contains(QStringLiteral("ntripGgaPositionSource")));
}

void GPSCorrectionSettingsTest::_qmlRegistration()
{
    auto* settings = SettingsManager::instance();
    QVERIFY(settings->gpsCorrectionSettings());
    QCOMPARE(settings->property("gpsCorrectionSettings").value<QObject*>(), settings->gpsCorrectionSettings());
    QCOMPARE(settings->ntripSettings()->rtcmUdpInputPort(), settings->gpsCorrectionSettings()->rtcmUdpInputPort());
    QQmlEngine engine;
    engine.addImportPath(QStringLiteral("qrc:/qml"));
    QQmlComponent component(&engine);
    component.setData(R"(
        import QtQml
        import QGroundControl
        QtObject {
            readonly property var corrections: QGroundControl.settingsManager.gpsCorrectionSettings
            readonly property var port: corrections.rtcmUdpInputPort
            readonly property int automatic: GPSCorrectionSettings.Automatic
            readonly property int all: GPSCorrectionSettings.All
        }
    )",
                      QUrl());
    QTRY_VERIFY_WITH_TIMEOUT(!component.isLoading(), TestTimeout::mediumMs());
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));
    std::unique_ptr<QObject> object(component.create());
    QVERIFY2(object, qPrintable(component.errorString()));
    QCOMPARE(object->property("corrections").value<QObject*>(), settings->gpsCorrectionSettings());
    QCOMPARE(object->property("port").value<Fact*>(), settings->gpsCorrectionSettings()->rtcmUdpInputPort());
    QCOMPARE(object->property("automatic").toInt(), int(GPSCorrectionSettings::Automatic));
    QCOMPARE(object->property("all").toInt(), int(GPSCorrectionSettings::All));
}

UT_REGISTER_TEST(GPSCorrectionSettingsTest, TestLabel::Unit)
