#include "NMEASourceManagerTest.h"

#include <memory>
#include <utility>

#include <QtCore/QRegularExpression>
#include <QtCore/QScopeGuard>
#include <QtNetwork/QUdpSocket>
#include <QtQml/QQmlComponent>
#include <QtQml/QQmlEngine>
#include <QtQml/QQmlExpression>

#include "AutoConnectSettings.h"
#include "Fixtures/RAIIFixtures.h"
#include "LogManager.h"
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
    source.update();
    verifyExpectedLogMessage();
    QVERIFY(source._sourceInstalled);
    const QString category = QStringLiteral("GPS.NMEA.NMEASourceManager");
    const auto initialLogCount = LogManager::capturedMessages(category).size();
    const auto initialHealth = position.nmeaHealth();
    for (int i = 0; i < 3; ++i) {
        source.update();
    }
    QCOMPARE(LogManager::capturedMessages(category).size(), initialLogCount);
    QCOMPARE(position.nmeaHealth(), initialHealth);
    QUdpSocket sender;
    QCOMPARE(sender.writeDatagram(kFix, QHostAddress::LocalHost, firstPort), kFix.size());
    QTRY_VERIFY_WITH_TIMEOUT(position.gcsPosition().isValid(), TestTimeout::mediumMs());
    QVERIFY(qAbs(position.gcsPosition().latitude() - 53.361337) < 0.0001);
    QVERIFY(spare.bind(QHostAddress::LocalHost, 0));
    const quint16 secondPort = spare.localPort();
    spare.close();
    settings->nmeaUdpPort()->setRawValue(secondPort);
    expectLogMessage(
        "GPS.NMEA.NMEASourceManager", QtDebugMsg,
        QRegularExpression(
            QStringLiteral("NMEA input retired:.*reason: UDP port setting changed.*port: %1").arg(firstPort)));
    expectLogMessage("GPS.NMEA.NMEASourceManager", QtDebugMsg,
                     QRegularExpression(QStringLiteral("NMEA input started:.*source: UDP.*port: %1").arg(secondPort)));
    source.update();
    verifyExpectedLogMessage();
    verifyExpectedLogMessage();
    QVERIFY(!position.gcsPosition().isValid());
    QVERIFY(source._sourceInstalled);
    QCOMPARE(sender.writeDatagram(kFix, QHostAddress::LocalHost, firstPort), kFix.size());
    QVERIFY(spare.bind(QHostAddress::LocalHost, firstPort, QUdpSocket::DontShareAddress));
    QCOMPARE(sender.writeDatagram(kFix, QHostAddress::LocalHost, secondPort), kFix.size());
    QTRY_VERIFY_WITH_TIMEOUT(position.gcsPosition().isValid(), TestTimeout::mediumMs());
    saved.setFactValue(settings->autoConnectNmeaPort(), QStringLiteral("/test/missing-nmea"));
    settings->nmeaSource()->setRawValue(AutoConnectSettings::NmeaSourceSerial);
    expectLogMessage(
        "GPS.NMEA.NMEASourceManager", QtDebugMsg,
        QRegularExpression(
            QStringLiteral("NMEA input retired:.*reason: source setting changed.*port: %1").arg(secondPort)));
    source.update();
    verifyExpectedLogMessage();
    QVERIFY(!position.gcsPosition().isValid());
    QVERIFY(!source._sourceInstalled);
    QVERIFY(!source._udp);
    settings->nmeaSource()->setRawValue(AutoConnectSettings::NmeaSourceUdp);
    expectLogMessage("GPS.NMEA.NMEASourceManager", QtDebugMsg,
                     QRegularExpression(QStringLiteral("NMEA input started:.*source: UDP.*port: %1").arg(secondPort)));
    source.update();
    verifyExpectedLogMessage();
    QCOMPARE(sender.writeDatagram(kFix, QHostAddress::LocalHost, secondPort), kFix.size());
    QTRY_VERIFY_WITH_TIMEOUT(position.gcsPosition().isValid(), TestTimeout::mediumMs());
    settings->nmeaSource()->setRawValue(AutoConnectSettings::NmeaSourceDisabled);
    expectLogMessage(
        "GPS.NMEA.NMEASourceManager", QtDebugMsg,
        QRegularExpression(QStringLiteral("NMEA input retired:.*reason: source disabled.*port: %1").arg(secondPort)));
    source.update();
    verifyExpectedLogMessage();
    QVERIFY(!position.gcsPosition().isValid());
    QVERIFY(!source._sourceInstalled);
    QVERIFY(!source._udp);
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
        QVERIFY(!source._sourceInstalled);
        QVERIFY(!source._udp);
        occupied.close();
        expectLogMessage("GPS.NMEA.NMEASourceManager", QtDebugMsg,
                         QRegularExpression(QStringLiteral("NMEA input started:.*source: UDP.*port: %1").arg(port)));
        source.update();
        verifyExpectedLogMessage();
        QVERIFY(source._sourceInstalled);
        QUdpSocket sender;
        QCOMPARE(sender.writeDatagram(kFix, QHostAddress::LocalHost, port), kFix.size());
        QTRY_VERIFY_WITH_TIMEOUT(position.gcsPosition().isValid(), TestTimeout::mediumMs());
        expectLogMessage("GPS.NMEA.NMEASourceManager", QtDebugMsg,
                         QRegularExpression(QStringLiteral("NMEA source manager shutdown:")));
        expectLogMessage(
            "GPS.NMEA.NMEASourceManager", QtDebugMsg,
            QRegularExpression(QStringLiteral("NMEA input retired:.*reason: shutdown.*port: %1").arg(port)));
    }
    verifyExpectedLogMessage();
    verifyExpectedLogMessage();
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
        QVERIFY(position.nmeaHealth());
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
        if (source->_udp) {
            retired = source->_udp.get();
        }
        if (action == QStringLiteral("delete")) {
            source.reset();
        } else if (action == QStringLiteral("stop")) {
            source->stop();
        } else {
            settings->nmeaUdpPort()->setRawValue(replacementPort);
            source->update();
            QVERIFY(source->_udp);
            replacement = source->_udp.get();
        }
    };
    if (phase == QStringLiteral("position")) {
        connect(&position, &QGCPositionManager::gcsPositionChanged, &observer, supersede);
    } else {
        connect(&position, &QGCPositionManager::nmeaSourceChanged, &observer, [&] {
            if ((position.nmeaHealth() != nullptr) == (phase == QStringLiteral("installed"))) {
                supersede();
            }
        });
    }
    if (retiring) {
        retired = source->_udp.get();
        source->stop();
    } else {
        source->update();
    }
    QVERIFY(handled);
    QVERIFY(retired.isNull());
    if (action == QStringLiteral("replace")) {
        QVERIFY(source->_sourceInstalled);
        QCOMPARE(source->_udp.get(), replacement.data());
        QCOMPARE(source->_udp->localPort(), replacementPort);
        QVERIFY(position.nmeaHealth());
        QUdpSocket sender;
        QCOMPARE(sender.writeDatagram(kFix, QHostAddress::LocalHost, replacementPort), kFix.size());
        QTRY_VERIFY_WITH_TIMEOUT(position.gcsPosition().isValid(), TestTimeout::mediumMs());
    } else {
        QVERIFY(!position.nmeaHealth());
        QVERIFY(!position.gcsPosition().isValid());
        if (source) {
            QVERIFY(!source->_sourceInstalled);
            QVERIFY(!source->_udp);
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
    auto source = std::make_unique<NMEASourceManager>(settings, &position);
    QObject observer;
    bool replaced = false;
    if (duringInstall) {
        connect(&position, &QGCPositionManager::nmeaSourceChanged, &observer, [&] {
            if (!std::exchange(replaced, true)) {
                position.setNmeaSourceDevice(&replacement);
            }
        });
    }
    source->update();
    if (!duringInstall) {
        position.setNmeaSourceDevice(&replacement);
    }
    QCOMPARE(position.nmeaSourceDevice(), &replacement);
    const QPointer<GPSSourceHealth> health(position.nmeaHealth());
    QVERIFY(health);
    source->stop();
    source.reset();
    QCOMPARE(position.nmeaHealth(), health.data());
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
        QVERIFY(!source._sourceInstalled);
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
    QTest::addColumn<bool>("customBaudSupported");
    QTest::newRow("nmea-settings") << QStringLiteral("NmeaGpsSettings.qml") << true;
    QTest::newRow("remote-id-settings") << QStringLiteral("RemoteIDGpsLocation.qml") << false;
}

void NMEASourceManagerTest::_settingsUseSharedSerialInventory()
{
    QFETCH(QString, qmlFile);
    QFETCH(bool, customBaudSupported);
    TestFixtures::SettingsFixture saved;
    auto* settings = SettingsManager::instance()->autoConnectSettings();
    saved.setFactValue(settings->nmeaSource(), AutoConnectSettings::NmeaSourceUdp);
    saved.setFactValue(settings->autoConnectNmeaBaud(), 123457);
    saved.setFactValue(settings->autoConnectNmeaPort(), QStringLiteral("/test/saved-nmea"));
    QQmlEngine engine;
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
    if (customBaudSupported) {
        auto* customBaud = root->findChild<QObject*>(QStringLiteral("customNmeaBaudField"));
        QVERIFY(customBaud);
        QVERIFY(baudCombo->property("isCustomBaud").toBool());
        QCOMPARE(customBaud->property("text").toString(), QStringLiteral("123457"));
    } else {
        QCOMPARE(baudCombo->property("currentIndex").toInt(), -1);
    }
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
    if (!customBaudSupported) {
        settings->autoConnectNmeaBaud()->setRawValue(115200);
        QQmlExpression baudIndex(qmlContext(root.get()), root.get(),
                                 QStringLiteral("_serialBaudRates.indexOf('115200')"));
        const int expectedIndex = baudIndex.evaluate().toInt();
        QVERIFY(!baudIndex.hasError());
        QCOMPARE(baudCombo->property("currentIndex").toInt(), expectedIndex);
    }
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
    source.update();
    verifyExpectedLogMessage();
    QVERIFY(position.nmeaHealth());
    QVERIFY(!position.nmeaReceiving());
    QVERIFY(!position.nmeaHasData());
    QCOMPARE(position.nmeaHealth()->satellitesInUseCount(), -1);
    QUdpSocket sender;
    const auto data = kFix + NMEAUtils::repairChecksum("$GPGSV,1,1,01,01,40,083,41");
    QCOMPARE(sender.writeDatagram(data, QHostAddress::LocalHost, port), data.size());
    QTRY_VERIFY_WITH_TIMEOUT(position.nmeaReceiving(), TestTimeout::shortMs());
    QTRY_VERIFY_WITH_TIMEOUT(position.nmeaHealth()->usable(), TestTimeout::mediumMs());
    QTRY_COMPARE_WITH_TIMEOUT(position.nmeaHealth()->satellitesInViewCount(), 1, TestTimeout::shortMs());
    QCOMPARE(position.nmeaHealth()->satellitesInUseCount(), 8);
    const QPointer<GPSSourceHealth> originalHealth(position.nmeaHealth());
    const QPointer<UdpIODevice> originalSocket(source._udp.get());
    QSignalSpy peerChanges(source._udp.get(), &UdpIODevice::peerReplaced);
    QUdpSocket replacement;
    {
        QSignalSpy ready(source._udp.get(), &QIODevice::readyRead);
        const QByteArray otherData("other sender\n");
        QCOMPARE(replacement.writeDatagram(otherData, QHostAddress::LocalHost, port), otherData.size());
        QVERIFY(!ready.wait(100));
        QVERIFY(peerChanges.isEmpty());
    }
    QTRY_VERIFY_WITH_TIMEOUT(!position.nmeaReceiving(), TestTimeout::mediumMs());
    QVERIFY(position.nmeaHasData());
    QTRY_VERIFY_WITH_TIMEOUT(!position.nmeaHealth()->usable(), TestTimeout::shortMs());
    QTRY_COMPARE_WITH_TIMEOUT(position.nmeaHealth()->satellitesInViewCount(), -1, TestTimeout::shortMs());
    QTRY_COMPARE_WITH_TIMEOUT(position.nmeaHealth()->satellitesInUseCount(), -1, TestTimeout::shortMs());
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
    }
    QCOMPARE(resumedSender.writeDatagram(resumed, QHostAddress::LocalHost, port), resumed.size());
    QTRY_VERIFY_WITH_TIMEOUT(position.nmeaReceiving(), TestTimeout::shortMs());
    QTRY_VERIFY_WITH_TIMEOUT(position.nmeaHealth()->usable(), TestTimeout::mediumMs());
    QCOMPARE(peerChanges.size(), replaceSender ? 1 : 0);
    QCOMPARE(originalHealth.isNull(), replaceSender);
    if (replaceSender) {
        verifyExpectedLogMessage();
    }
    QCOMPARE(LogManager::capturedMessages(category).size(), previousLogCount + (replaceSender ? 1 : 0));
    QCOMPARE(source._udp.get(), originalSocket.data());
    QCOMPARE(settings->nmeaUdpPort()->rawValue().toUInt(), uint(port));
    QCOMPARE(position.nmeaHealth()->satellitesInViewCount(), -1);
    QCOMPARE(position.nmeaHealth()->satellitesInUseCount(), 8);
    expectLogMessage(
        "GPS.NMEA.NMEASourceManager", QtDebugMsg,
        QRegularExpression(QStringLiteral("NMEA input retired:.*reason: stop requested.*port: %1").arg(port)));
    source.stop();
    verifyExpectedLogMessage();
    QVERIFY(!position.nmeaHealth());
    QVERIFY(!position.nmeaReceiving());
    QVERIFY(!position.nmeaHasData());
    expectLogMessage("GPS.NMEA.NMEASourceManager", QtDebugMsg,
                     QRegularExpression(QStringLiteral("NMEA source manager shutdown:")));
    sourceOwner.reset();
    verifyExpectedLogMessage();
}
