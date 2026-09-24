#include "NMEASourceManagerTest.h"

#include <memory>
#include <utility>

#include <QtCore/QRegularExpression>
#include <QtCore/QScopeGuard>
#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>
#include <QtNetwork/QUdpSocket>
#include <QtQml/QQmlComponent>
#include <QtQml/QQmlEngine>
#include <QtQml/QQmlExpression>
#include <QtTest/QSignalSpy>

#include "AutoConnectSettings.h"
#include "ColoredSvgImageProvider.h"
#include "Fixtures/RAIIFixtures.h"
#include "LogManager.h"
#include "NMEADecoderSession.h"
#include "NMEASourceManager.h"
#include "NMEAUtils.h"
#include "PositionManager.h"
#include "QGCLoggingCategoryManager.h"
#include "SequentialTestDevice.h"
#include "SettingsManager.h"
#include "UdpIODevice.h"

namespace {
const QByteArray kFix =
    "$GPRMC,092750.000,A,5321.6802,N,00630.3372,W,0.02,31.66,280511,,,A*43\r\n"
    "$GPGGA,092750.000,5321.6802,N,00630.3372,W,1,8,1.03,61.7,M,55.2,M,,*76\r\n";

GPSSourceHealth* nmeaHealth(const QGCPositionManager& position)
{
    const auto* input = position.nmeaInput();
    return input ? input->health() : nullptr;
}

QRegularExpression decoderInstalled(quint64 generation)
{
    return QRegularExpression(
        QStringLiteral("^NMEA decoder installed: generation: %1 registered: true$").arg(generation));
}

QRegularExpression decoderRetired(const QString& reason, quint64 generation)
{
    return QRegularExpression(QStringLiteral("^NMEA decoder retired: %1 generation: %2$")
                                  .arg(QRegularExpression::escape(reason))
                                  .arg(generation));
}

auto enableSourceLogs()
{
    const QString category = QStringLiteral("GPS.NMEA.NMEASourceManager");
    auto* logging = QGCLoggingCategoryManager::instance();
    const bool wasEnabled = logging->isCategoryEnabled(category);
    if (!wasEnabled) {
        logging->setCategoryEnabled(category, true);
    }
    return qScopeGuard([logging, category, wasEnabled] {
        if (!wasEnabled) {
            logging->setCategoryEnabled(category, false);
        }
    });
}
}  // namespace

void NMEASourceManagerTest::init()
{
    UnitTest::init();
    ignoreLogMessage("PositionManager.QGCPositionManager", QtWarningMsg,
                     QRegularExpression(QStringLiteral("UpdateTimeoutError")));
}

void NMEASourceManagerTest::_udpSwitchAndDisable()
{
    const auto restoreLogging = enableSourceLogs();
    TestFixtures::SettingsFixture saved;
    auto* settings = SettingsManager::instance()->autoConnectSettings();
    saved.setFactValue(settings->nmeaSource(), AutoConnectSettings::NmeaSourceUdp);
    QUdpSocket spare;
    QVERIFY(spare.bind(QHostAddress::LocalHost, 0));
    const quint16 firstPort = spare.localPort();
    spare.close();
    saved.setFactValue(settings->nmeaUdpPort(), firstPort);
    QGCPositionManager position;
    expectLogMessage("GPS.NMEA.NMEASourceManager", QtDebugMsg,
                     QRegularExpression(QStringLiteral("^NMEASourceManager\\(")));
    auto sourceOwner = std::make_unique<NMEASourceManager>(settings, &position);
    auto& source = *sourceOwner;
    verifyExpectedLogMessage();
    expectLogMessage("GPS.NMEA.NMEASourceManager", QtDebugMsg,
                     QRegularExpression(QStringLiteral("NMEA input started:.*source: UDP.*port: %1").arg(firstPort)));
    expectLogMessage("GPS.NMEA.NMEASourceManager", QtDebugMsg, decoderInstalled(3));
    source.update();
    verifyExpectedLogMessage();
    verifyExpectedLogMessage();
    QVERIFY(source._input.binding.registration);
    const QString category = QStringLiteral("GPS.NMEA.NMEASourceManager");
    const auto initialLogCount = LogManager::capturedMessages(category).size();
    const auto initialHealth = nmeaHealth(position);
    for (int i = 0; i < 3; ++i) {
        source.update();
    }
    QCOMPARE(LogManager::capturedMessages(category).size(), initialLogCount);
    QCOMPARE(nmeaHealth(position), initialHealth);
    QUdpSocket sender;
    QCOMPARE(sender.writeDatagram(kFix, QHostAddress::LocalHost, firstPort), kFix.size());
    QTRY_VERIFY_WITH_TIMEOUT(position.gcsPosition().isValid(), TestTimeout::mediumMs());
    QVERIFY(qAbs(position.gcsPosition().latitude() - 53.361337) < 0.0001);
    QVERIFY(spare.bind(QHostAddress::LocalHost, 0));
    const quint16 secondPort = spare.localPort();
    spare.close();
    settings->nmeaUdpPort()->setRawValue(secondPort);
    expectLogMessage("GPS.NMEA.NMEASourceManager", QtDebugMsg,
                     decoderRetired(QStringLiteral("UDP port setting changed"), source._decoderGeneration + 1));
    expectLogMessage(
        "GPS.NMEA.NMEASourceManager", QtDebugMsg,
        QRegularExpression(
            QStringLiteral("NMEA input retired:.*reason: UDP port setting changed.*port: %1").arg(firstPort)));
    expectLogMessage("GPS.NMEA.NMEASourceManager", QtDebugMsg,
                     QRegularExpression(QStringLiteral("NMEA input started:.*source: UDP.*port: %1").arg(secondPort)));
    expectLogMessage("GPS.NMEA.NMEASourceManager", QtDebugMsg, decoderInstalled(source._decoderGeneration + 2));
    source.update();
    verifyExpectedLogMessage();
    verifyExpectedLogMessage();
    verifyExpectedLogMessage();
    verifyExpectedLogMessage();
    QVERIFY(!position.gcsPosition().isValid());
    QVERIFY(source._input.binding.registration);
    QCOMPARE(sender.writeDatagram(kFix, QHostAddress::LocalHost, firstPort), kFix.size());
    QVERIFY(spare.bind(QHostAddress::LocalHost, firstPort, QUdpSocket::DontShareAddress));
    QCOMPARE(sender.writeDatagram(kFix, QHostAddress::LocalHost, secondPort), kFix.size());
    QTRY_VERIFY_WITH_TIMEOUT(position.gcsPosition().isValid(), TestTimeout::mediumMs());
    saved.setFactValue(settings->autoConnectNmeaPort(), QStringLiteral("/test/missing-nmea"));
    settings->nmeaSource()->setRawValue(AutoConnectSettings::NmeaSourceSerial);
    expectLogMessage("GPS.NMEA.NMEASourceManager", QtDebugMsg,
                     decoderRetired(QStringLiteral("source setting changed"), source._decoderGeneration + 1));
    expectLogMessage(
        "GPS.NMEA.NMEASourceManager", QtDebugMsg,
        QRegularExpression(
            QStringLiteral("NMEA input retired:.*reason: source setting changed.*port: %1").arg(secondPort)));
    source.update();
    verifyExpectedLogMessage();
    verifyExpectedLogMessage();
    QVERIFY(!position.gcsPosition().isValid());
    QVERIFY(!source._input.binding.registration);
    QVERIFY(!source._input.udp);
    settings->nmeaSource()->setRawValue(AutoConnectSettings::NmeaSourceUdp);
    expectLogMessage("GPS.NMEA.NMEASourceManager", QtDebugMsg,
                     QRegularExpression(QStringLiteral("NMEA input started:.*source: UDP.*port: %1").arg(secondPort)));
    expectLogMessage("GPS.NMEA.NMEASourceManager", QtDebugMsg, decoderInstalled(source._decoderGeneration + 3));
    source.update();
    verifyExpectedLogMessage();
    verifyExpectedLogMessage();
    QCOMPARE(sender.writeDatagram(kFix, QHostAddress::LocalHost, secondPort), kFix.size());
    QTRY_VERIFY_WITH_TIMEOUT(position.gcsPosition().isValid(), TestTimeout::mediumMs());
    settings->nmeaSource()->setRawValue(AutoConnectSettings::NmeaSourceDisabled);
    expectLogMessage("GPS.NMEA.NMEASourceManager", QtDebugMsg,
                     decoderRetired(QStringLiteral("source disabled"), source._decoderGeneration + 1));
    expectLogMessage(
        "GPS.NMEA.NMEASourceManager", QtDebugMsg,
        QRegularExpression(QStringLiteral("NMEA input retired:.*reason: source disabled.*port: %1").arg(secondPort)));
    source.update();
    verifyExpectedLogMessage();
    verifyExpectedLogMessage();
    QVERIFY(!position.gcsPosition().isValid());
    QVERIFY(!source._input.binding.registration);
    QVERIFY(!source._input.udp);
    expectLogMessage("GPS.NMEA.NMEASourceManager", QtDebugMsg,
                     QRegularExpression(QStringLiteral("NMEA source manager shutdown:")));
    sourceOwner.reset();
    verifyExpectedLogMessage();
}

void NMEASourceManagerTest::_bindFailureAndTeardown()
{
    const auto restoreLogging = enableSourceLogs();
    TestFixtures::SettingsFixture saved;
    auto* settings = SettingsManager::instance()->autoConnectSettings();
    saved.setFactValue(settings->nmeaSource(), AutoConnectSettings::NmeaSourceUdp);
    QUdpSocket occupied;
    QVERIFY(occupied.bind(QHostAddress::AnyIPv4, 0, QUdpSocket::DontShareAddress));
    const quint16 port = occupied.localPort();
    saved.setFactValue(settings->nmeaUdpPort(), port);
    QGCPositionManager position;
    {
        expectLogMessage("GPS.NMEA.NMEASourceManager", QtDebugMsg,
                         QRegularExpression(QStringLiteral("^NMEASourceManager\\(")));
        NMEASourceManager source(settings, &position);
        verifyExpectedLogMessage();
        expectLogMessage("GPS.NMEA.NMEASourceManager", QtDebugMsg,
                         QRegularExpression(QStringLiteral("Cannot bind NMEA UDP port %1").arg(port)));
        source.update();
        verifyExpectedLogMessage();
        QVERIFY(!source._input.binding.registration);
        QVERIFY(!source._input.udp);
        QCOMPARE(position.nmeaInput(), &source);
        QCOMPARE(source.connectionState(), NMEASourceManager::ConnectionState::Error);
        QVERIFY(source.errorMessage().contains(QString::number(port)));
        QVERIFY(!nmeaHealth(position));
        QQmlEngine engine;
        engine.addImageProvider(QLatin1String(ColoredSvgImageProvider::ProviderId), new ColoredSvgImageProvider());
        engine.addImportPath(QStringLiteral("qrc:/qml"));
        QQmlComponent component(&engine,
                                QUrl(QStringLiteral("qrc:/qml/QGroundControl/AppSettings/NmeaGpsSettings.qml")));
        QTRY_VERIFY_WITH_TIMEOUT(!component.isLoading(), TestTimeout::mediumMs());
        std::unique_ptr<QObject> panel(component.createWithInitialProperties(
            {{QStringLiteral("positionManager"), QVariant::fromValue(&position)}}));
        QVERIFY2(panel, qPrintable(component.errorString()));
        auto* status = panel->findChild<QObject*>(QStringLiteral("nmeaConnectionStatus"));
        QVERIFY(status);
        QVERIFY(status->property("visible").toBool());
        QCOMPARE(status->property("text").toString(), source.errorMessage());
        occupied.close();
        expectLogMessage("GPS.NMEA.NMEASourceManager", QtDebugMsg,
                         QRegularExpression(QStringLiteral("NMEA input started:.*source: UDP.*port: %1").arg(port)));
        expectLogMessage("GPS.NMEA.NMEASourceManager", QtDebugMsg, decoderInstalled(source._decoderGeneration + 2));
        source.update();
        verifyExpectedLogMessage();
        verifyExpectedLogMessage();
        QVERIFY(source._input.binding.registration);
        QCOMPARE(source.connectionState(), NMEASourceManager::ConnectionState::Connected);
        QVERIFY(source.errorMessage().isEmpty());
        QVERIFY(!status->property("visible").toBool());
        QUdpSocket sender;
        QCOMPARE(sender.writeDatagram(kFix, QHostAddress::LocalHost, port), kFix.size());
        QTRY_VERIFY_WITH_TIMEOUT(position.gcsPosition().isValid(), TestTimeout::mediumMs());
        expectLogMessage("GPS.NMEA.NMEASourceManager", QtDebugMsg,
                         QRegularExpression(QStringLiteral("NMEA source manager shutdown:")));
        expectLogMessage("GPS.NMEA.NMEASourceManager", QtDebugMsg,
                         decoderRetired(QStringLiteral("shutdown"), source._decoderGeneration + 1));
        expectLogMessage(
            "GPS.NMEA.NMEASourceManager", QtDebugMsg,
            QRegularExpression(QStringLiteral("NMEA input retired:.*reason: shutdown.*port: %1").arg(port)));
    }
    verifyExpectedLogMessage();
    verifyExpectedLogMessage();
    verifyExpectedLogMessage();
    QVERIFY(!position.nmeaInput());
    QVERIFY(!position.gcsPosition().isValid());
    QVERIFY(occupied.bind(QHostAddress::AnyIPv4, port, QUdpSocket::DontShareAddress));
}

void NMEASourceManagerTest::_notificationSupersedesLifecycle_data()
{
    QTest::addColumn<QString>("phase");
    QTest::addColumn<QString>("action");
    for (const QString& phase : {QStringLiteral("install-reset"), QStringLiteral("installed"),
                                 QStringLiteral("retired"), QStringLiteral("position")}) {
        for (const QString& action : {QStringLiteral("stop"), QStringLiteral("replace"), QStringLiteral("delete")}) {
            QTest::newRow(qPrintable(phase + '-' + action)) << phase << action;
        }
    }
}

void NMEASourceManagerTest::_notificationSupersedesLifecycle()
{
    QFETCH(QString, phase);
    QFETCH(QString, action);
    TestFixtures::SettingsFixture saved;
    auto* settings = SettingsManager::instance()->autoConnectSettings();
    saved.setFactValue(settings->nmeaSource(), AutoConnectSettings::NmeaSourceUdp);
    QUdpSocket firstProbe;
    QUdpSocket replacementProbe;
    QVERIFY(firstProbe.bind(QHostAddress::LocalHost, 0));
    QVERIFY(replacementProbe.bind(QHostAddress::LocalHost, 0));
    const quint16 firstPort = firstProbe.localPort();
    const quint16 replacementPort = replacementProbe.localPort();
    firstProbe.close();
    replacementProbe.close();
    saved.setFactValue(settings->nmeaUdpPort(), firstPort);
    QGCPositionManager position;
    auto source = std::make_unique<NMEASourceManager>(settings, &position);
    const bool retiring = phase == QStringLiteral("retired") || phase == QStringLiteral("position");
    if (retiring) {
        source->update();
        QVERIFY(nmeaHealth(position));
        if (phase == QStringLiteral("position")) {
            QUdpSocket sender;
            QCOMPARE(sender.writeDatagram(kFix, QHostAddress::LocalHost, firstPort), kFix.size());
            QTRY_VERIFY_WITH_TIMEOUT(position.gcsPosition().isValid(), TestTimeout::mediumMs());
        }
    }
    QPointer<UdpIODevice> retired;
    QPointer<UdpIODevice> replacement;
    bool handled = false;
    QObject observer;
    const auto supersede = [&] {
        if (std::exchange(handled, true)) {
            return;
        }
        if (source->_input.udp) {
            retired = source->_input.udp.get();
        }
        if (action == QStringLiteral("delete")) {
            source.reset();
        } else if (action == QStringLiteral("stop")) {
            source->stop();
        } else {
            settings->nmeaUdpPort()->setRawValue(replacementPort);
            source->update();
            QVERIFY(source->_input.udp);
            replacement = source->_input.udp.get();
        }
    };
    if (phase == QStringLiteral("position")) {
        connect(&position, &QGCPositionManager::gcsPositionChanged, &observer, supersede);
    } else if (phase == QStringLiteral("install-reset")) {
        connect(&position, &QGCPositionManager::selectionChanged, &observer, supersede);
    } else {
        connect(source.get(), &NMEASourceManager::sourceChanged, &observer, [&] {
            if ((nmeaHealth(position) != nullptr) == (phase == QStringLiteral("installed"))) {
                supersede();
            }
        });
    }
    if (retiring) {
        retired = source->_input.udp.get();
        source->stop();
    } else {
        source->update();
    }
    QVERIFY(handled);
    QVERIFY(retired.isNull());
    if (action == QStringLiteral("replace")) {
        QVERIFY(source->_input.binding.registration);
        QCOMPARE(source->_input.udp.get(), replacement.data());
        QCOMPARE(source->_input.udp->localPort(), replacementPort);
        QVERIFY(nmeaHealth(position));
        QUdpSocket sender;
        QCOMPARE(sender.writeDatagram(kFix, QHostAddress::LocalHost, replacementPort), kFix.size());
        QTRY_VERIFY_WITH_TIMEOUT(position.gcsPosition().isValid(), TestTimeout::mediumMs());
    } else {
        QVERIFY(!nmeaHealth(position));
        QVERIFY(!position.gcsPosition().isValid());
        if (source) {
            QVERIFY(!source->_input.binding.registration);
            QVERIFY(!source->_input.udp);
        }
    }
    QVERIFY(firstProbe.bind(QHostAddress::AnyIPv4, firstPort, QUdpSocket::DontShareAddress));
}

void NMEASourceManagerTest::_externalReplacementKeepsOwnership_data()
{
    QTest::addColumn<bool>("duringInstall");
    QTest::newRow("during-install") << true;
    QTest::newRow("after-install") << false;
}

void NMEASourceManagerTest::_externalReplacementKeepsOwnership()
{
    QFETCH(bool, duringInstall);
    TestFixtures::SettingsFixture saved;
    auto* settings = SettingsManager::instance()->autoConnectSettings();
    saved.setFactValue(settings->nmeaSource(), AutoConnectSettings::NmeaSourceUdp);
    QUdpSocket probe;
    QVERIFY(probe.bind(QHostAddress::LocalHost, 0));
    const quint16 port = probe.localPort();
    probe.close();
    saved.setFactValue(settings->nmeaUdpPort(), port);
    QGCPositionManager position;
    SequentialTestDevice replacement;
    NMEADecoderSession replacementDecoder;
    QVERIFY(replacementDecoder.start(&replacement));
    GPSPositionSourceRegistration replacementRegistration;
    auto source = std::make_unique<NMEASourceManager>(settings, &position);
    QObject observer;
    bool replaced = false;
    if (duringInstall) {
        connect(source.get(), &NMEASourceManager::sourceChanged, &observer, [&] {
            if (!std::exchange(replaced, true)) {
                replacementRegistration =
                    position.registerPositionSource(GPSPositionService::SelectedSource::Nmea,
                                                    replacementDecoder.positionSource(), replacementDecoder.health());
            }
        });
    }
    source->update();
    if (!duringInstall) {
        replacementRegistration = position.registerPositionSource(
            GPSPositionService::SelectedSource::Nmea, replacementDecoder.positionSource(), replacementDecoder.health());
    }
    QCOMPARE(position._currentHealth.data(), replacementDecoder.health());
    const QPointer<GPSSourceHealth> health(position._currentHealth.data());
    QVERIFY(health);
    source->stop();
    source.reset();
    QCOMPARE(position._currentHealth.data(), health.data());
    replacement.feed(kFix);
    QTRY_VERIFY_WITH_TIMEOUT(position.gcsPosition().isValid(), TestTimeout::mediumMs());
    QVERIFY(probe.bind(QHostAddress::AnyIPv4, port, QUdpSocket::DontShareAddress));
}

UT_REGISTER_TEST(NMEASourceManagerTest, TestLabel::Unit)

void NMEASourceManagerTest::_configuredSerialRoutingSurvivesReconnect()
{
#ifndef QGC_NO_SERIAL_LINK
    TestFixtures::SettingsFixture saved;
    auto* settings = SettingsManager::instance()->autoConnectSettings();
    auto* ports = SerialPortManager::instance();
    const QString first = QStringLiteral("/test/nmea-first");
    const QString second = QStringLiteral("/test/nmea-second");
    saved.setFactValue(settings->nmeaSource(), AutoConnectSettings::NmeaSourceSerial);
    saved.setFactValue(settings->autoConnectNmeaPort(), first);
    QGCPositionManager position;
    {
        NMEASourceManager source(settings, &position);
        QVERIFY(!ports->canAutoConnectPort(first));
        QVERIFY(!ports->isPortReserved(first));
        source.update();
        QVERIFY(!source._input.binding.registration);
        QCOMPARE(source.connectionState(), NMEASourceManager::ConnectionState::WaitingForDevice);
        QVERIFY(!source.connectionStatusText().isEmpty());
        source.stop();
        QVERIFY(!ports->canAutoConnectPort(first));
        settings->autoConnectNmeaPort()->setRawValue(second);
        QVERIFY(ports->canAutoConnectPort(first));
        QVERIFY(!ports->canAutoConnectPort(second));
        settings->nmeaSource()->setRawValue(AutoConnectSettings::NmeaSourceDisabled);
        QVERIFY(ports->canAutoConnectPort(second));
        settings->nmeaSource()->setRawValue(AutoConnectSettings::NmeaSourceSerial);
        QVERIFY(!ports->canAutoConnectPort(second));
    }
    QVERIFY(ports->canAutoConnectPort(second));
#else
    QSKIP("Serial routing is unavailable in this build");
#endif
}

void NMEASourceManagerTest::_settingsUseSharedSerialInventory_data()
{
    QTest::addColumn<QString>("qmlFile");
    QTest::newRow("nmea-settings") << QStringLiteral("NmeaGpsSettings.qml");
    QTest::newRow("remote-id-settings") << QStringLiteral("RemoteIDGpsLocation.qml");
}

void NMEASourceManagerTest::_settingsUseSharedSerialInventory()
{
    QFETCH(QString, qmlFile);
    TestFixtures::SettingsFixture saved;
    auto* settings = SettingsManager::instance()->autoConnectSettings();
    saved.setFactValue(settings->nmeaSource(), AutoConnectSettings::NmeaSourceUdp);
    saved.setFactValue(settings->autoConnectNmeaBaud(), 123457);
    saved.setFactValue(settings->autoConnectNmeaPort(), QStringLiteral("/test/saved-nmea"));
    QQmlEngine engine;
    engine.addImageProvider(QLatin1String(ColoredSvgImageProvider::ProviderId), new ColoredSvgImageProvider());
    engine.addImportPath(QStringLiteral("qrc:/qml"));
    QQmlComponent component(&engine, QUrl(QStringLiteral("qrc:/qml/QGroundControl/AppSettings/") + qmlFile));
    QTRY_VERIFY_WITH_TIMEOUT(!component.isLoading(), TestTimeout::mediumMs());
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));
    std::unique_ptr<QObject> root(component.create());
    QVERIFY2(root, qPrintable(component.errorString()));
    auto* portCombo = root->findChild<QObject*>(QStringLiteral("nmeaPortCombo"));
    QVERIFY(portCombo);
    auto* baudCombo = root->findChild<QObject*>(QStringLiteral("nmeaBaudCombo"));
    QVERIFY(baudCombo);
    auto* customBaud = root->findChild<QObject*>(QStringLiteral("customNmeaBaudField"));
    QVERIFY(customBaud);
    QVERIFY(baudCombo->property("isCustomBaud").toBool());
    QCOMPARE(customBaud->property("text").toString(), QStringLiteral("123457"));
    QVERIFY(portCombo->property("currentText").toString().contains(QStringLiteral("/test/saved-nmea")));
    QCOMPARE(settings->autoConnectNmeaBaud()->rawValue().toInt(), 123457);
    QCOMPARE(settings->autoConnectNmeaPort()->rawValue().toString(), QStringLiteral("/test/saved-nmea"));

    QQmlExpression portCount(qmlContext(root.get()), root.get(), QStringLiteral("_serialPorts.length"));
    const int count = portCount.evaluate().toInt();
    QVERIFY(!portCount.hasError());
    QCOMPARE(portCombo->property("enabled").toBool(), count > 0);
    auto* manager = root->property("_serialPortManager").value<QObject*>();
#ifndef QGC_NO_SERIAL_LINK
    QCOMPARE(manager, SerialPortManager::instance());
    QCOMPARE(count, SerialPortManager::instance()->serialPorts().size());
#else
    QVERIFY(!manager);
#endif
    settings->autoConnectNmeaBaud()->setRawValue(115200);
    QQmlExpression standardBaud(qmlContext(root.get()), root.get(),
                                QStringLiteral("_serialBaudRates.includes('115200')"));
    const bool hasStandardBaud = standardBaud.evaluate().toBool();
    QVERIFY(!standardBaud.hasError());
    QCOMPARE(baudCombo->property("isCustomBaud").toBool(), !hasStandardBaud);
    QCOMPARE(hasStandardBaud ? baudCombo->property("currentText").toString() : customBaud->property("text").toString(),
             QStringLiteral("115200"));
    settings->autoConnectNmeaBaud()->setRawValue(76543);
    QVERIFY(baudCombo->property("isCustomBaud").toBool());
    QCOMPARE(customBaud->property("text").toString(), QStringLiteral("76543"));
}

void NMEASourceManagerTest::_tcpClientConnectsAndReconnects()
{
    TestFixtures::SettingsFixture saved;
    auto* settings = SettingsManager::instance()->autoConnectSettings();
    QTcpServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost));
    saved.setFactValue(settings->nmeaSource(), AutoConnectSettings::NmeaSourceTcp);
    saved.setFactValue(settings->nmeaTcpHost(), QString());
    saved.setFactValue(settings->nmeaTcpPort(), server.serverPort());
    QGCPositionManager position;
    NMEASourceManager source(settings, &position);
    source.update();
    QCOMPARE(source.connectionState(), NMEASourceManager::ConnectionState::Error);
    QVERIFY(!source._input.tcp);
    settings->nmeaTcpHost()->setRawValue(QStringLiteral("127.0.0.1"));
    source.update();
    QCOMPARE(source.connectionState(), NMEASourceManager::ConnectionState::WaitingForDevice);
    QVERIFY(source.connectionStatusText().contains(QStringLiteral("127.0.0.1")));
    QTRY_VERIFY_WITH_TIMEOUT(server.hasPendingConnections(), TestTimeout::mediumMs());
    QTcpSocket* peer = server.nextPendingConnection();
    QVERIFY(peer);
    QTRY_COMPARE_WITH_TIMEOUT(source.connectionState(), NMEASourceManager::ConnectionState::Connected,
                              TestTimeout::mediumMs());
    QVERIFY(position.nmeaInput() == &source);
    QCOMPARE(peer->write(kFix), kFix.size());
    QTRY_VERIFY_WITH_TIMEOUT(source.health() && source.health()->usable(), TestTimeout::mediumMs());
    peer->disconnectFromHost();
    QTRY_COMPARE_WITH_TIMEOUT(source.connectionState(), NMEASourceManager::ConnectionState::Error,
                              TestTimeout::mediumMs());
    QVERIFY(!source.health());
    source.update();
    QTRY_VERIFY_WITH_TIMEOUT(server.hasPendingConnections(), TestTimeout::mediumMs());
    QVERIFY(server.nextPendingConnection());
    QTRY_COMPARE_WITH_TIMEOUT(source.connectionState(), NMEASourceManager::ConnectionState::Connected,
                              TestTimeout::mediumMs());
    settings->nmeaSource()->setRawValue(AutoConnectSettings::NmeaSourceDisabled);
    source.update();
    QCOMPARE(source.connectionState(), NMEASourceManager::ConnectionState::Disabled);
    QVERIFY(!source._input.tcp);
}

void NMEASourceManagerTest::_tcpEndpointChangeUpdatesStatus()
{
    TestFixtures::SettingsFixture saved;
    auto* settings = SettingsManager::instance()->autoConnectSettings();
    QTcpServer first;
    QTcpServer second;
    QVERIFY(first.listen(QHostAddress::LocalHost));
    QVERIFY(second.listen(QHostAddress::LocalHost));
    saved.setFactValue(settings->nmeaSource(), AutoConnectSettings::NmeaSourceTcp);
    saved.setFactValue(settings->nmeaTcpHost(), QStringLiteral("127.0.0.1"));
    saved.setFactValue(settings->nmeaTcpPort(), first.serverPort());
    QGCPositionManager position;
    NMEASourceManager source(settings, &position);
    source.update();
    QCOMPARE(source.connectionState(), NMEASourceManager::ConnectionState::WaitingForDevice);
    QVERIFY(source.connectionStatusText().contains(QString::number(first.serverPort())));

    // The endpoint changes before the first connection completes, so the state stays WaitingForDevice.
    QSignalSpy states(&source, &NMEASourceManager::connectionStateChanged);
    settings->nmeaTcpPort()->setRawValue(second.serverPort());
    source.update();
    QCOMPARE(source.connectionState(), NMEASourceManager::ConnectionState::WaitingForDevice);
    QVERIFY(source.connectionStatusText().contains(QString::number(second.serverPort())));
    QVERIFY(!states.isEmpty());
    settings->nmeaSource()->setRawValue(AutoConnectSettings::NmeaSourceDisabled);
    source.update();
    QCOMPARE(source.connectionState(), NMEASourceManager::ConnectionState::Disabled);
}

void NMEASourceManagerTest::_udpActivityAndSatellites_data()
{
    QTest::addColumn<bool>("replaceSender");
    QTest::newRow("same-sender") << false;
    QTest::newRow("restarted-sender") << true;
}

void NMEASourceManagerTest::_udpActivityAndSatellites()
{
    QFETCH(bool, replaceSender);
    const auto restoreLogging = enableSourceLogs();
    TestFixtures::SettingsFixture saved;
    auto* settings = SettingsManager::instance()->autoConnectSettings();
    saved.setFactValue(settings->nmeaSource(), AutoConnectSettings::NmeaSourceUdp);
    QUdpSocket spare;
    QVERIFY(spare.bind(QHostAddress::LocalHost, 0));
    const quint16 port = spare.localPort();
    spare.close();
    saved.setFactValue(settings->nmeaUdpPort(), port);
    QGCPositionManager position;
    expectLogMessage("GPS.NMEA.NMEASourceManager", QtDebugMsg,
                     QRegularExpression(QStringLiteral("^NMEASourceManager\\(")));
    auto sourceOwner = std::make_unique<NMEASourceManager>(settings, &position);
    auto& source = *sourceOwner;
    verifyExpectedLogMessage();
    expectLogMessage("GPS.NMEA.NMEASourceManager", QtDebugMsg,
                     QRegularExpression(QStringLiteral("NMEA input started:.*source: UDP.*port: %1").arg(port)));
    expectLogMessage("GPS.NMEA.NMEASourceManager", QtDebugMsg, decoderInstalled(3));
    source.update();
    verifyExpectedLogMessage();
    verifyExpectedLogMessage();
    QVERIFY(nmeaHealth(position));
    QVERIFY(!(position.nmeaInput() && position.nmeaInput()->receiving()));
    QVERIFY(!(position.nmeaInput() && position.nmeaInput()->hasData()));
    QCOMPARE(nmeaHealth(position)->satellitesInUseCount(), -1);
    QUdpSocket sender;
    const auto data = kFix + NMEAUtils::repairChecksum("$GPGSV,1,1,01,01,40,083,41");
    QCOMPARE(sender.writeDatagram(data, QHostAddress::LocalHost, port), data.size());
    QTRY_VERIFY_WITH_TIMEOUT((position.nmeaInput() && position.nmeaInput()->receiving()), TestTimeout::shortMs());
    QTRY_VERIFY_WITH_TIMEOUT(nmeaHealth(position)->usable(), TestTimeout::mediumMs());
    QTRY_COMPARE_WITH_TIMEOUT(nmeaHealth(position)->satellitesInViewCount(), 1, TestTimeout::shortMs());
    QCOMPARE(nmeaHealth(position)->satellitesInUseCount(), 8);
    const QPointer<GPSSourceHealth> originalHealth(nmeaHealth(position));
    const QPointer<UdpIODevice> originalSocket(source._input.udp.get());
    QSignalSpy peerChanges(source._input.udp.get(), &UdpIODevice::peerReplaced);
    QUdpSocket replacement;
    {
        QSignalSpy ready(source._input.udp.get(), &QIODevice::readyRead);
        const QByteArray otherData("other sender\n");
        QCOMPARE(replacement.writeDatagram(otherData, QHostAddress::LocalHost, port), otherData.size());
        // Tight negative window: this only rejects an immediate datagram from the wrong peer.
        QVERIFY_NO_SIGNAL_WAIT(ready, 100);
        QVERIFY(peerChanges.isEmpty());
    }
    QTRY_VERIFY_WITH_TIMEOUT(!(position.nmeaInput() && position.nmeaInput()->receiving()), TestTimeout::mediumMs());
    QVERIFY((position.nmeaInput() && position.nmeaInput()->hasData()));
    QTRY_VERIFY_WITH_TIMEOUT(!nmeaHealth(position)->usable(), TestTimeout::shortMs());
    QTRY_COMPARE_WITH_TIMEOUT(nmeaHealth(position)->satellitesInViewCount(), -1, TestTimeout::shortMs());
    QTRY_COMPARE_WITH_TIMEOUT(nmeaHealth(position)->satellitesInUseCount(), -1, TestTimeout::shortMs());
    QByteArray resumed;
    for (auto line : kFix.split('\n')) {
        if (!line.trimmed().isEmpty()) {
            line.replace("092750.000", "092751.000");
            resumed += NMEAUtils::repairChecksum(line);
        }
    }
    // Reusing the old UTC epoch requires retiring Qt's previous decoder on sender replacement.
    if (replaceSender)
        resumed = kFix;
    auto& resumedSender = replaceSender ? replacement : sender;
    const QString category = QStringLiteral("GPS.NMEA.NMEASourceManager");
    const auto previousLogCount = LogManager::capturedMessages(category).size();
    if (replaceSender) {
        expectLogMessage("GPS.NMEA.NMEASourceManager", QtDebugMsg,
                         QRegularExpression(QStringLiteral("NMEA session restart:.*reason: UDP peer replaced.*port: %1"
                                                           ".*previousPeer:.*:%2.*peer:.*:%3")
                                                .arg(port)
                                                .arg(sender.localPort())
                                                .arg(replacement.localPort())));
        expectLogMessage("GPS.NMEA.NMEASourceManager", QtDebugMsg,
                         decoderRetired(QStringLiteral("device replacement"), source._decoderGeneration + 1));
        expectLogMessage("GPS.NMEA.NMEASourceManager", QtDebugMsg, decoderInstalled(source._decoderGeneration + 1));
    }
    QCOMPARE(resumedSender.writeDatagram(resumed, QHostAddress::LocalHost, port), resumed.size());
    QTRY_VERIFY_WITH_TIMEOUT((position.nmeaInput() && position.nmeaInput()->receiving()), TestTimeout::shortMs());
    QTRY_VERIFY_WITH_TIMEOUT(nmeaHealth(position)->usable(), TestTimeout::mediumMs());
    QCOMPARE(peerChanges.size(), replaceSender ? 1 : 0);
    QCOMPARE(originalHealth.isNull(), replaceSender);
    if (replaceSender) {
        verifyExpectedLogMessage();
        verifyExpectedLogMessage();
        verifyExpectedLogMessage();
    }
    QCOMPARE(LogManager::capturedMessages(category).size(), previousLogCount + (replaceSender ? 3 : 0));
    QCOMPARE(source._input.udp.get(), originalSocket.data());
    QCOMPARE(settings->nmeaUdpPort()->rawValue().toUInt(), uint(port));
    QCOMPARE(nmeaHealth(position)->satellitesInViewCount(), -1);
    QCOMPARE(nmeaHealth(position)->satellitesInUseCount(), 8);
    expectLogMessage("GPS.NMEA.NMEASourceManager", QtDebugMsg,
                     decoderRetired(QStringLiteral("stop requested"), source._decoderGeneration + 1));
    expectLogMessage(
        "GPS.NMEA.NMEASourceManager", QtDebugMsg,
        QRegularExpression(QStringLiteral("NMEA input retired:.*reason: stop requested.*port: %1").arg(port)));
    source.stop();
    verifyExpectedLogMessage();
    verifyExpectedLogMessage();
    QVERIFY(!nmeaHealth(position));
    QVERIFY(!(position.nmeaInput() && position.nmeaInput()->receiving()));
    QVERIFY(!(position.nmeaInput() && position.nmeaInput()->hasData()));
    expectLogMessage("GPS.NMEA.NMEASourceManager", QtDebugMsg,
                     QRegularExpression(QStringLiteral("NMEA source manager shutdown:")));
    sourceOwner.reset();
    verifyExpectedLogMessage();
}
