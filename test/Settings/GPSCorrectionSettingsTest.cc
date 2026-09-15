#include "GPSCorrectionSettingsTest.h"

#include <memory>

#include <QtCore/QCoreApplication>
#include <QtCore/QFile>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QPointer>
#include <QtCore/QScopeGuard>
#include <QtCore/QSettings>
#include <QtQml/QQmlComponent>
#include <QtQml/QQmlEngine>
#include <QtQuick/QQuickItem>
#include <QtTest/QSignalSpy>

#include "ColoredSvgImageProvider.h"
#include "GPS/RTCM/RTCMTestFixtures.h"
#include "GPSCorrectionManager.h"
#include "GPSCorrectionRouter.h"
#include "GPSCorrectionSettings.h"
#include "NTRIPSettings.h"
#include "QmlUITestBase.h"
#include "RAIIFixtures.h"
#include "SettingsFact.h"
#include "SettingsManager.h"

void GPSCorrectionSettingsTest::_storageNamespace_data()
{
    QTest::addColumn<QString>("name");
    QTest::addColumn<QVariant>("savedValue");
    QTest::addColumn<QVariant>("replacement");
    QTest::newRow("udp-enabled") << "rtcmUdpInputEnabled" << QVariant(false) << QVariant(true);
    QTest::newRow("udp-port") << "rtcmUdpInputPort" << QVariant(13321U) << QVariant(13322U);
    QTest::newRow("udp-validation") << "rtcmUdpValidate" << QVariant(true) << QVariant(false);
}

void GPSCorrectionSettingsTest::_storageNamespace()
{
    QFETCH(QString, name);
    QFETCH(QVariant, savedValue);
    QFETCH(QVariant, replacement);
    QSettings storage;
    const QString storedKey = QStringLiteral("NTRIP/") + name;
    const QString newKey = QStringLiteral("GPSCorrection/") + name;
    const bool hadStored = storage.contains(storedKey);
    const bool hadNew = storage.contains(newKey);
    const QVariant originalStored = storage.value(storedKey);
    const QVariant originalNew = storage.value(newKey);
    const auto restore = qScopeGuard([&]() {
        if (hadStored) {
            storage.setValue(storedKey, originalStored);
        } else {
            storage.remove(storedKey);
        }
        if (hadNew) {
            storage.setValue(newKey, originalNew);
        } else {
            storage.remove(newKey);
        }
    });
    storage.setValue(storedKey, savedValue);
    storage.remove(newKey);
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
    QCOMPARE(corrections, QStringList({"correctionSource", "correctionSourceInstance", "rtcmUdpInputEnabled",
                                       "rtcmUdpInputPort", "rtcmUdpValidate"}));
    const QStringList ntrip = names(QStringLiteral(":/json/NTRIP.SettingsGroup.json"));
    QVERIFY(!ntrip.isEmpty());
    NTRIPSettings ntripSettings;
    for (const auto& name : corrections) {
        QVERIFY(!ntrip.contains(name));
        QCOMPARE(ntripSettings.metaObject()->indexOfProperty(name.toUtf8().constData()), -1);
    }
    QVERIFY(ntrip.contains(QStringLiteral("ntripUdpForwardEnabled")));
    QVERIFY(ntrip.contains(QStringLiteral("ntripGgaPositionSource")));
}

void GPSCorrectionSettingsTest::_qmlRegistration()
{
    auto* settings = SettingsManager::instance();
    QVERIFY(settings->gpsCorrectionSettings());
    QCOMPARE(settings->property("gpsCorrectionSettings").value<QObject*>(), settings->gpsCorrectionSettings());
    QQmlEngine engine;
    engine.addImportPath(QStringLiteral("qrc:/qml"));
    QQmlComponent component(&engine);
    component.setData(R"(
        import QtQml
        import QGroundControl
        QtObject {
            readonly property var corrections: QGroundControl.settingsManager.gpsCorrectionSettings
            readonly property var port: corrections.rtcmUdpInputPort
            readonly property SettingsFact sourceFact: corrections.correctionSource as SettingsFact
            readonly property bool sourceVisible: sourceFact.userVisible
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
    QCOMPARE(object->property("sourceFact").value<SettingsFact*>(),
             qobject_cast<SettingsFact*>(settings->gpsCorrectionSettings()->correctionSource()));
    QCOMPARE(object->property("sourceVisible").toBool(),
             settings->gpsCorrectionSettings()->correctionSource()->property("userVisible").toBool());
    QCOMPARE(object->property("automatic").toInt(), int(GPSCorrectionSettings::Automatic));
    QCOMPARE(object->property("all").toInt(), int(GPSCorrectionSettings::All));
}

void GPSCorrectionSettingsTest::_routingDefaults()
{
    GPSCorrectionSettings corrections;
    QCOMPARE(corrections.correctionSource()->rawValue().toInt(), int(GPSCorrectionSettings::Automatic));
    QCOMPARE(corrections.correctionSource()->enumValues(), QVariantList({0U, 1U, 2U, 3U, 4U}));
    QVERIFY(corrections.correctionSourceInstance()->rawValue().toString().isEmpty());
}

void GPSCorrectionSettingsTest::_routingPanel_data()
{
    QTest::addColumn<int>("source");
    QTest::addColumn<bool>("manual");
    QTest::addColumn<bool>("sourceVisible");
    QTest::addColumn<bool>("instanceVisible");
    QTest::newRow("automatic") << int(GPSCorrectionSettings::Automatic) << false << true << true;
    QTest::newRow("local") << int(GPSCorrectionSettings::LocalReceiver) << true << true << true;
    QTest::newRow("ntrip") << int(GPSCorrectionSettings::Ntrip) << true << true << true;
    QTest::newRow("udp") << int(GPSCorrectionSettings::Udp) << true << true << true;
    QTest::newRow("all") << int(GPSCorrectionSettings::All) << false << true << true;
    QTest::newRow("source-hidden") << int(GPSCorrectionSettings::Udp) << true << false << true;
    QTest::newRow("instance-hidden") << int(GPSCorrectionSettings::Udp) << true << true << false;
    QTest::newRow("both-hidden") << int(GPSCorrectionSettings::Udp) << true << false << false;
}

void GPSCorrectionSettingsTest::_routingPanel()
{
    QFETCH(int, source);
    QFETCH(bool, manual);
    QFETCH(bool, sourceVisible);
    QFETCH(bool, instanceVisible);
    auto* settings = SettingsManager::instance()->gpsCorrectionSettings();
    TestFixtures::SettingsFixture saved;
    saved.setFactValue(settings->correctionSource(), source);
    saved.setFactValue(settings->correctionSourceInstance(), QStringLiteral("offline-stream"));
    auto* sourceFact = qobject_cast<SettingsFact*>(settings->correctionSource());
    auto* instanceFact = qobject_cast<SettingsFact*>(settings->correctionSourceInstance());
    QVERIFY(sourceFact);
    QVERIFY(instanceFact);
    const bool originalSourceVisible = sourceFact->property("userVisible").toBool();
    const bool originalInstanceVisible = instanceFact->property("userVisible").toBool();
    const auto restoreVisibility = qScopeGuard([&]() {
        sourceFact->setUserVisible(originalSourceVisible);
        instanceFact->setUserVisible(originalInstanceVisible);
    });
    sourceFact->setUserVisible(sourceVisible);
    instanceFact->setUserVisible(instanceVisible);

    QQmlEngine engine;
    engine.addImportPath(QStringLiteral("qrc:/qml"));
    engine.addImageProvider(QLatin1String(ColoredSvgImageProvider::ProviderId), new ColoredSvgImageProvider());
    QQmlComponent component(&engine);
    component.setData(R"(
        import QGroundControl.AppSettings
        CorrectionRoutingSettings {}
    )",
                      QUrl());
    QTRY_VERIFY_WITH_TIMEOUT(!component.isLoading(), TestTimeout::mediumMs());
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));
    std::unique_ptr<QObject> panel(component.create());
    QVERIFY2(panel, qPrintable(component.errorString()));
    auto* sourceControl = panel->findChild<QObject*>(QStringLiteral("correctionSource"));
    auto* streamControl = panel->findChild<QObject*>(QStringLiteral("correctionStream"));
    QVERIFY(sourceControl);
    QVERIFY(streamControl);
    QCOMPARE(sourceControl->property("fact").value<Fact*>(), settings->correctionSource());
    QCOMPARE(panel->property("visible").toBool(), sourceVisible && settings->userVisible());
    QCOMPARE(streamControl->property("visible").toBool(),
             manual && sourceVisible && instanceVisible && settings->userVisible());
    QTRY_COMPARE_WITH_TIMEOUT(
        streamControl->property("currentText").toString(),
        QCoreApplication::translate("CorrectionRoutingSettings", "Unavailable: %1").arg("offline-stream"),
        TestTimeout::mediumMs());
    QCOMPARE(streamControl->property("currentValue").toString(), QStringLiteral("offline-stream"));
    QVERIFY(!panel->findChild<QObject*>(QStringLiteral("injectLocalReceiver")));

    if (sourceVisible) {
        QVERIFY(QMetaObject::invokeMethod(sourceControl, "activated", Q_ARG(int, source)));
        QCOMPARE(settings->correctionSourceInstance()->rawValue().toString(),
                 instanceVisible ? QString() : QStringLiteral("offline-stream"));
    }
}

void GPSCorrectionSettingsTest::_routingPanelTracksStreams()
{
    auto* settings = SettingsManager::instance()->gpsCorrectionSettings();
    TestFixtures::SettingsFixture saved;
    saved.setFactValue(settings->correctionSource(), int(GPSCorrectionSettings::Udp));
    saved.setFactValue(settings->correctionSourceInstance(), QStringLiteral("b"));
    QSignalSpy selectionChanged(settings->correctionSourceInstance(), &Fact::rawValueChanged);

    GPSCorrectionManager corrections;
    corrections.applyRoutingConfiguration(
        {GPSCorrectionManager::RoutingPolicy::Manual, GPSCorrectionSource::Udp, QStringLiteral("b")});
    auto udpListener = corrections.registerSource(GPSCorrectionSource::Udp);
    const QByteArray frame = GpsTestHelpers::buildRtcmFrame(1005, 20);
    const auto receivePeer = [&](const QString& instance, qint64 ageMs = 0) {
        corrections.acceptIngress(udpListener.token().event(frame, GPSCorrectionFrame::monotonicNowMs() - ageMs, 1005,
                                                            true, false, GPSCorrectionReason::None, instance));
    };
    const qint64 expiredAgeMs = GPSCorrectionRouter::FRESHNESS_TIMEOUT_MS + 1;
    // Expired observations expose peers without real-time waits.
    receivePeer(QStringLiteral("a"), expiredAgeMs);
    receivePeer(QStringLiteral("b"), expiredAgeMs);

    QQmlEngine engine;
    engine.addImportPath(QStringLiteral("qrc:/qml"));
    engine.addImageProvider(QLatin1String(ColoredSvgImageProvider::ProviderId), new ColoredSvgImageProvider());
    QQmlComponent component(&engine);
    component.setData(R"(
        import QtQuick.Layouts
        import QGroundControl
        import QGroundControl.AppSettings
        ColumnLayout {
            id: root
            property GPSCorrectionManager corrections: QGroundControl.gpsManager.corrections
            CorrectionRoutingSettings { corrections: root.corrections }
            CorrectionDiagnostics { corrections: root.corrections }
        }
    )",
                      QUrl());
    QTRY_VERIFY_WITH_TIMEOUT(!component.isLoading(), TestTimeout::mediumMs());
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));
    std::unique_ptr<QObject> panel(
        component.createWithInitialProperties({{QStringLiteral("corrections"), QVariant::fromValue(&corrections)}}));
    QVERIFY2(panel, qPrintable(component.errorString()));
    auto* panelItem = qobject_cast<QQuickItem*>(panel.get());
    QVERIFY(panelItem);
    const auto findItem = [panelItem](const QString& objectName) {
        return QmlUITestBase::findItem(panelItem, objectName);
    };
    auto* streamControl = findItem(QStringLiteral("correctionStream"));
    auto* selectionStatus = findItem(QStringLiteral("correctionSelectionStatus"));
    QVERIFY(streamControl);
    QVERIFY(selectionStatus);
    auto* comboBox = streamControl->property("comboBox").value<QObject*>();
    QVERIFY(comboBox);
    QCOMPARE(streamControl->property("textRole").toString(), QStringLiteral("label"));
    QCOMPARE(streamControl->property("valueRole").toString(), QStringLiteral("instanceId"));

    const QString waitingLabel =
        QCoreApplication::translate("CorrectionRoutingSettings", "No fresh corrections: %1").arg("b");
    const QString unavailableLabel =
        QCoreApplication::translate("CorrectionRoutingSettings", "Unavailable: %1").arg("b");
    const QString noSelection =
        QCoreApplication::translate("CorrectionDiagnostics", "No fresh stream is selected for vehicles.");
    const auto streamStatus = [&](const QString& instance) {
        const auto* row = findItem(QStringLiteral("correctionStreamState_3_%1").arg(instance));
        return row ? row->property("text").toString() : QString();
    };
    QTRY_COMPARE_WITH_TIMEOUT(comboBox->property("count").toInt(), 3, TestTimeout::mediumMs());
    QTRY_COMPARE_WITH_TIMEOUT(streamControl->property("currentText").toString(), waitingLabel, TestTimeout::mediumMs());
    QCOMPARE(streamControl->property("currentIndex").toInt(), 2);
    QCOMPARE(streamControl->property("currentValue").toString(), QStringLiteral("b"));
    QCOMPARE(selectionStatus->property("text").toString(), noSelection);
    QTRY_VERIFY_WITH_TIMEOUT(findItem(QStringLiteral("correctionStreamState_3_b")), TestTimeout::mediumMs());
    QVERIFY2(
        streamStatus(QStringLiteral("b"))
            .contains(QCoreApplication::translate("CorrectionDiagnostics", "Active; waiting for fresh corrections")),
        qPrintable(QStringLiteral("Stream b status: %1").arg(streamStatus(QStringLiteral("b")))));

    receivePeer(QStringLiteral("c"), expiredAgeMs);
    QTRY_COMPARE_WITH_TIMEOUT(comboBox->property("count").toInt(), 4, TestTimeout::mediumMs());
    QTRY_COMPARE_WITH_TIMEOUT(streamControl->property("currentIndex").toInt(), 2, TestTimeout::mediumMs());
    QCOMPARE(streamControl->property("currentText").toString(), waitingLabel);

    receivePeer(QStringLiteral("c"));
    QTRY_VERIFY_WITH_TIMEOUT(
        streamStatus(QStringLiteral("c"))
            .contains(QCoreApplication::translate("CorrectionDiagnostics", "Fresh; not selected for vehicles")),
        TestTimeout::mediumMs());
    QTRY_COMPARE_WITH_TIMEOUT(streamControl->property("currentIndex").toInt(), 2, TestTimeout::mediumMs());
    QCOMPARE(streamControl->property("currentText").toString(), waitingLabel);
    QCOMPARE(selectionStatus->property("text").toString(), noSelection);

    receivePeer(QStringLiteral("b"));
    QTRY_COMPARE_WITH_TIMEOUT(streamControl->property("currentText").toString(), QStringLiteral("b"),
                              TestTimeout::mediumMs());
    QCOMPARE(streamControl->property("currentIndex").toInt(), 2);
    QTRY_VERIFY_WITH_TIMEOUT(
        streamStatus(QStringLiteral("b"))
            .contains(QCoreApplication::translate("CorrectionDiagnostics", "Selected for vehicles; fresh")),
        TestTimeout::mediumMs());

    udpListener.reset();
    QVERIFY(corrections.sourceInstances().isEmpty());
    QTRY_COMPARE_WITH_TIMEOUT(comboBox->property("count").toInt(), 2, TestTimeout::mediumMs());
    QTRY_COMPARE_WITH_TIMEOUT(streamControl->property("currentIndex").toInt(), 1, TestTimeout::mediumMs());
    QCOMPARE(streamControl->property("currentText").toString(), unavailableLabel);
    QTRY_COMPARE_WITH_TIMEOUT(selectionStatus->property("text").toString(), noSelection, TestTimeout::mediumMs());
    QCOMPARE(settings->correctionSourceInstance()->rawValue().toString(), QStringLiteral("b"));

    udpListener = corrections.registerSource(GPSCorrectionSource::Udp);
    receivePeer(QStringLiteral("c"), expiredAgeMs);
    QTRY_COMPARE_WITH_TIMEOUT(comboBox->property("count").toInt(), 3, TestTimeout::mediumMs());
    QTRY_COMPARE_WITH_TIMEOUT(streamControl->property("currentIndex").toInt(), 2, TestTimeout::mediumMs());
    QCOMPARE(streamControl->property("currentText").toString(), unavailableLabel);
    receivePeer(QStringLiteral("b"), expiredAgeMs);
    QTRY_COMPARE_WITH_TIMEOUT(streamControl->property("currentText").toString(), waitingLabel, TestTimeout::mediumMs());
    QCOMPARE(streamControl->property("currentIndex").toInt(), 1);
    receivePeer(QStringLiteral("a"), expiredAgeMs);
    QTRY_COMPARE_WITH_TIMEOUT(comboBox->property("count").toInt(), 4, TestTimeout::mediumMs());
    QTRY_COMPARE_WITH_TIMEOUT(streamControl->property("currentIndex").toInt(), 2, TestTimeout::mediumMs());
    QCOMPARE(streamControl->property("currentText").toString(), waitingLabel);
    receivePeer(QStringLiteral("b"));
    QTRY_COMPARE_WITH_TIMEOUT(streamControl->property("currentText").toString(), QStringLiteral("b"),
                              TestTimeout::mediumMs());
    QCOMPARE(streamControl->property("currentIndex").toInt(), 2);
    QCOMPARE(settings->correctionSourceInstance()->rawValue().toString(), QStringLiteral("b"));

    udpListener.reset();
    QVERIFY(corrections.sourceInstances().isEmpty());
    QTRY_COMPARE_WITH_TIMEOUT(comboBox->property("count").toInt(), 2, TestTimeout::mediumMs());
    QTRY_COMPARE_WITH_TIMEOUT(streamControl->property("currentText").toString(), unavailableLabel,
                              TestTimeout::mediumMs());
    QTRY_COMPARE_WITH_TIMEOUT(selectionStatus->property("text").toString(), noSelection, TestTimeout::mediumMs());
    QCOMPARE(streamControl->property("currentIndex").toInt(), 1);
    QCOMPARE(settings->correctionSourceInstance()->rawValue().toString(), QStringLiteral("b"));
    QCOMPARE(selectionChanged.size(), 0);

    QVERIFY(!findItem(QStringLiteral("correctionEventHistory")));
    auto* historyToggle = findItem(QStringLiteral("correctionHistoryToggle"));
    QVERIFY(historyToggle);
    QVERIFY(historyToggle->setProperty("checked", true));
    QTRY_VERIFY_WITH_TIMEOUT(findItem(QStringLiteral("correctionEventHistory")), TestTimeout::mediumMs());
    const QPointer<QQuickItem> history = findItem(QStringLiteral("correctionEventHistory"));
    QVERIFY(historyToggle->setProperty("checked", false));
    QTRY_VERIFY_WITH_TIMEOUT(history.isNull(), TestTimeout::mediumMs());
    QTRY_VERIFY_WITH_TIMEOUT(!findItem(QStringLiteral("correctionEventHistory")), TestTimeout::mediumMs());

    QVERIFY(comboBox->setProperty("currentIndex", 0));
    QCOMPARE(settings->correctionSourceInstance()->rawValue().toString(), QStringLiteral("b"));
    QVERIFY(QMetaObject::invokeMethod(comboBox, "activated", Q_ARG(int, 0)));
    QVERIFY(settings->correctionSourceInstance()->rawValue().toString().isEmpty());
    QTRY_COMPARE_WITH_TIMEOUT(comboBox->property("count").toInt(), 1, TestTimeout::mediumMs());
    QTRY_COMPARE_WITH_TIMEOUT(streamControl->property("currentText").toString(),
                              QCoreApplication::translate("CorrectionRoutingSettings", "Automatic within source"),
                              TestTimeout::mediumMs());
    QCOMPARE(selectionChanged.size(), 1);

    settings->correctionSourceInstance()->setRawValue(QStringLiteral("external"));
    QTRY_COMPARE_WITH_TIMEOUT(streamControl->property("currentValue").toString(), QStringLiteral("external"),
                              TestTimeout::mediumMs());
    QTRY_COMPARE_WITH_TIMEOUT(comboBox->property("count").toInt(), 2, TestTimeout::mediumMs());
    QCOMPARE(streamControl->property("currentText").toString(),
             QCoreApplication::translate("CorrectionRoutingSettings", "Unavailable: %1").arg("external"));

    udpListener = corrections.registerSource(GPSCorrectionSource::Udp);
    receivePeer(QStringLiteral("external"));
    receivePeer(QStringLiteral("z"));
    QTRY_COMPARE_WITH_TIMEOUT(comboBox->property("count").toInt(), 3, TestTimeout::mediumMs());
    QCOMPARE(streamControl->property("currentValue").toString(), QStringLiteral("external"));
    QVERIFY(comboBox->setProperty("currentIndex", 2));
    QVERIFY(QMetaObject::invokeMethod(comboBox, "activated", Q_ARG(int, 2)));
    QCOMPARE(settings->correctionSourceInstance()->rawValue().toString(), QStringLiteral("z"));

    receivePeer(QStringLiteral("a"));
    QTRY_COMPARE_WITH_TIMEOUT(comboBox->property("count").toInt(), 4, TestTimeout::mediumMs());
    QTRY_COMPARE_WITH_TIMEOUT(streamControl->property("currentIndex").toInt(), 3, TestTimeout::mediumMs());
    QCOMPARE(streamControl->property("currentValue").toString(), QStringLiteral("z"));
    QCOMPARE(settings->correctionSourceInstance()->rawValue().toString(), QStringLiteral("z"));
    QCOMPARE(selectionChanged.size(), 3);
}

UT_REGISTER_TEST(GPSCorrectionSettingsTest, TestLabel::Unit)
