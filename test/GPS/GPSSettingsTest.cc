#include "GPSSettingsTest.h"

#include <limits>

#include "AutoConnectSettings.h"
#include "Fixtures/RAIIFixtures.h"
#include "GPSReceiverTransportFactory.h"
#include "GPSSettings.h"
#include "RTKSettings.h"
#include "SettingsManager.h"

void GPSSettingsTest::_nmeaValidation_data()
{
    QTest::addColumn<int>("source");
    QTest::addColumn<QString>("host");
    QTest::addColumn<int>("port");
    QTest::addColumn<QString>("device");
    QTest::addColumn<int>("baud");
    QTest::addColumn<bool>("valid");
    QTest::newRow("disabled") << 0 << QStringLiteral("") << 0 << QStringLiteral("") << 0 << true;
    QTest::newRow("udp-min") << 1 << QStringLiteral("") << 1 << QStringLiteral("") << 0 << true;
    QTest::newRow("udp-max") << 1 << QStringLiteral("") << 65535 << QStringLiteral("") << 0 << true;
    QTest::newRow("udp-ephemeral") << 1 << QStringLiteral("") << 0 << QStringLiteral("") << 0 << true;
    QTest::newRow("udp-overflow") << 1 << QStringLiteral("") << 65536 << QStringLiteral("") << 0 << false;
    QTest::newRow("serial") << 2 << QStringLiteral("") << 0 << QStringLiteral("/test/gps") << 115200 << true;
    QTest::newRow("serial-no-device") << 2 << QStringLiteral("") << 0 << QStringLiteral("") << 115200 << false;
    QTest::newRow("serial-no-baud") << 2 << QStringLiteral("") << 0 << QStringLiteral("/test/gps") << 0 << false;
    QTest::newRow("tcp-ipv6") << 3 << QStringLiteral("::1") << 65535 << QStringLiteral("") << 0 << true;
    QTest::newRow("tcp-no-host") << 3 << QStringLiteral("") << 2101 << QStringLiteral("") << 0 << false;
    QTest::newRow("tcp-invalid-host") << 3 << QStringLiteral("bad/host") << 2101 << QStringLiteral("") << 0 << false;
    QTest::newRow("tcp-negative-port") << 3 << QStringLiteral("localhost") << -1 << QStringLiteral("") << 0 << false;
    QTest::newRow("unknown-source") << 4 << QStringLiteral("") << 0 << QStringLiteral("") << 0 << false;
}

void GPSSettingsTest::_nmeaValidation()
{
    QFETCH(int, source);
    QFETCH(QString, host);
    QFETCH(int, port);
    QFETCH(QString, device);
    QFETCH(int, baud);
    QFETCH(bool, valid);
    TestFixtures::SettingsFixture saved;
    auto* settings = SettingsManager::instance()->autoConnectSettings();
    saved.setFactValue(settings->nmeaSource(), source);
    saved.setFactValue(settings->nmeaTcpHost(), host);
    saved.setFactValue(settings->nmeaTcpPort(), port);
    saved.setFactValue(settings->nmeaUdpPort(), port);
    saved.setFactValue(settings->autoConnectNmeaPort(), device);
    saved.setFactValue(settings->autoConnectNmeaBaud(), baud);
    saved.setFactValue(settings->nmeaReceiverMode(), AutoConnectSettings::NmeaReceiverPassive);
    const auto connection = GPSSettings::nmea(*settings);
    QCOMPARE(connection.validationError.isEmpty(), valid);
    QCOMPARE(connection.profile.validationError().isEmpty(), valid);
}

void GPSSettingsTest::_rtkValidation_data()
{
    QTest::addColumn<QString>("field");
    QTest::addColumn<double>("value");
    QTest::addColumn<bool>("fixed");
    QTest::addColumn<bool>("valid");
    QTest::newRow("survey") << QStringLiteral("accuracy") << 2.0 << false << true;
    QTest::newRow("survey-zero") << QStringLiteral("accuracy") << 0.0 << false << false;
    QTest::newRow("survey-nan") << QStringLiteral("accuracy") << qQNaN() << false << false;
    QTest::newRow("survey-overflow") << QStringLiteral("accuracy") << 1e10 << false << false;
    QTest::newRow("duration-negative") << QStringLiteral("duration") << -1.0 << false << false;
    QTest::newRow("latitude-edge") << QStringLiteral("latitude") << -90.0 << true << true;
    QTest::newRow("latitude-outside") << QStringLiteral("latitude") << 90.1 << true << false;
    QTest::newRow("longitude-outside") << QStringLiteral("longitude") << 180.1 << true << false;
    QTest::newRow("altitude-infinite") << QStringLiteral("altitude") << std::numeric_limits<double>::infinity() << true
                                       << false;
    QTest::newRow("fixed-accuracy-negative") << QStringLiteral("fixedAccuracy") << -1.0 << true << false;
    QTest::newRow("fixed-zero-accuracy") << QStringLiteral("fixedAccuracy") << 0.0 << true << true;
    QTest::newRow("unused-survey") << QStringLiteral("accuracy") << qQNaN() << true << true;
    QTest::newRow("unused-fixed") << QStringLiteral("latitude") << qQNaN() << false << true;
    QTest::newRow("udp-ephemeral") << QStringLiteral("localPort") << 0.0 << false << true;
    QTest::newRow("udp-overflow") << QStringLiteral("localPort") << 65536.0 << false << false;
    QTest::newRow("remote-zero") << QStringLiteral("port") << 0.0 << false << false;
    QTest::newRow("remote-overflow") << QStringLiteral("port") << 65536.0 << false << false;
    QTest::newRow("unknown-receiver") << QStringLiteral("receiver") << 4.0 << false << false;
    QTest::newRow("unknown-transport") << QStringLiteral("transport") << 3.0 << false << false;
    QTest::newRow("unknown-base-mode") << QStringLiteral("mode") << 2.0 << false << false;
}

void GPSSettingsTest::_rtkValidation()
{
    ignoreLogMessage("API.QGCApplication.AppMessage", QtDebugMsg,
                     QRegularExpression(QStringLiteral("Restart application for changes to take effect")));
    QFETCH(QString, field);
    QFETCH(double, value);
    QFETCH(bool, fixed);
    QFETCH(bool, valid);
    TestFixtures::SettingsFixture saved;
    auto* settings = SettingsManager::instance();
    auto* rtk = settings->rtkSettings();
    saved.setFactValue(rtk->receiverRole(), RTKSettings::RTKBase);
    saved.setFactValue(rtk->connectionType(), RTKSettings::Udp);
    saved.setFactValue(rtk->networkReceiverType(), int(GPSType::u_blox));
    saved.setFactValue(rtk->networkBaseHost(), QStringLiteral("localhost"));
    saved.setFactValue(rtk->networkBasePort(), 2101);
    saved.setFactValue(rtk->udpLocalPort(), 0);
    saved.setFactValue(rtk->useFixedBasePosition(), fixed ? 1 : 0);
    saved.setFactValue(rtk->surveyInAccuracyLimit(), 2.0);
    saved.setFactValue(rtk->surveyInMinObservationDuration(), 180);
    saved.setFactValue(rtk->fixedBasePositionLatitude(), 0.0);
    saved.setFactValue(rtk->fixedBasePositionLongitude(), 0.0);
    saved.setFactValue(rtk->fixedBasePositionAltitude(), 0.0);
    saved.setFactValue(rtk->fixedBasePositionAccuracy(), 0.0);
    const QMap<QString, Fact*> fields{
        {QStringLiteral("accuracy"), rtk->surveyInAccuracyLimit()},
        {QStringLiteral("duration"), rtk->surveyInMinObservationDuration()},
        {QStringLiteral("latitude"), rtk->fixedBasePositionLatitude()},
        {QStringLiteral("longitude"), rtk->fixedBasePositionLongitude()},
        {QStringLiteral("altitude"), rtk->fixedBasePositionAltitude()},
        {QStringLiteral("fixedAccuracy"), rtk->fixedBasePositionAccuracy()},
        {QStringLiteral("localPort"), rtk->udpLocalPort()},
        {QStringLiteral("port"), rtk->networkBasePort()},
        {QStringLiteral("receiver"), rtk->networkReceiverType()},
        {QStringLiteral("transport"), rtk->connectionType()},
        {QStringLiteral("mode"), rtk->useFixedBasePosition()},
    };
    QVERIFY(fields.contains(field));
    saved.setFactValue(fields.value(field), value);
    const auto connection = GPSSettings::receiver(*rtk, *settings->autoConnectSettings());
    QCOMPARE(connection.validationError.isEmpty(), valid);
    QCOMPARE(connection.profile.validationError().isEmpty(), valid);
    if (field == QStringLiteral("mode")) {
        QCOMPARE(connection.validationError, QStringLiteral("Select a valid base mode"));
    }
}

void GPSSettingsTest::_settingsSnapshots()
{
    expectLogMessage("API.QGCApplication.AppMessage", QtDebugMsg,
                     QRegularExpression(QStringLiteral("Restart application for changes to take effect")));
    TestFixtures::SettingsFixture saved;
    auto* settings = SettingsManager::instance();
    auto* nmea = settings->autoConnectSettings();
    saved.setFactValue(nmea->nmeaSource(), AutoConnectSettings::NmeaSourceTcp);
    saved.setFactValue(nmea->nmeaTcpHost(), QStringLiteral("  localhost  "));
    saved.setFactValue(nmea->nmeaTcpPort(), 3100);
    const auto nmeaConfig = GPSSettings::nmea(*nmea).profile;
    QCOMPARE(nmeaConfig.endpoint.kind, GPSReceiverProfile::Endpoint::Kind::Tcp);
    QCOMPARE(nmeaConfig.endpoint.host, QStringLiteral("localhost"));
    nmea->nmeaTcpPort()->setRawValue(3200);
    QCOMPARE(nmeaConfig.endpoint.port, 3100);
    QCOMPARE(GPSSettings::nmea(*nmea).profile.endpoint.port, 3200);

    auto* rtk = settings->rtkSettings();
    saved.setFactValue(rtk->receiverRole(), RTKSettings::RTKBase);
    saved.setFactValue(rtk->connectionType(), RTKSettings::Udp);
    saved.setFactValue(rtk->networkReceiverType(), 3);
    saved.setFactValue(rtk->networkBaseHost(), QStringLiteral("  localhost  "));
    saved.setFactValue(rtk->surveyInAccuracyLimit(), 1.5);
    const auto rtkConfig = GPSSettings::receiver(*rtk, *nmea).profile;
    QCOMPARE(rtkConfig.endpoint.kind, GPSReceiverProfile::Endpoint::Kind::UdpPeer);
    QCOMPARE(rtkConfig.driverType, GPSType::femto);
    QCOMPARE(rtkConfig.endpoint.host, QStringLiteral("localhost"));
    rtk->surveyInAccuracyLimit()->setRawValue(2.5);
    QCOMPARE(rtkConfig.receiver.base.surveyInAccMeters, 1.5);
    QCOMPARE(GPSSettings::receiver(*rtk, *nmea).profile.receiver.base.surveyInAccMeters, 2.5);
    QCOMPARE(rtkConfig.receiver.role, GPSReceiverConfig::Role::RTKBase);
    rtk->receiverRole()->setRawValue(RTKSettings::Position);
    QCOMPARE(rtkConfig.receiver.role, GPSReceiverConfig::Role::RTKBase);
    QCOMPARE(GPSSettings::receiver(*rtk, *nmea).profile.receiver.role, GPSReceiverConfig::Role::Position);
    verifyExpectedLogMessage();
}

UT_REGISTER_TEST(GPSSettingsTest, TestLabel::Unit)

void GPSSettingsTest::_positionRoleValidation()
{
    ignoreLogMessage("API.QGCApplication.AppMessage", QtDebugMsg,
                     QRegularExpression(QStringLiteral("Restart application for changes to take effect")));
    TestFixtures::SettingsFixture saved;
    auto* settings = SettingsManager::instance();
    auto* rtk = settings->rtkSettings();
    saved.setFactValue(rtk->receiverRole(), RTKSettings::Position);
    saved.setFactValue(rtk->connectionType(), RTKSettings::Tcp);
    saved.setFactValue(rtk->networkReceiverType(), int(GPSType::u_blox));
    saved.setFactValue(rtk->networkBaseHost(), QStringLiteral("localhost"));
    saved.setFactValue(rtk->networkBasePort(), 2101);
    saved.setFactValue(rtk->useFixedBasePosition(), -1);
    saved.setFactValue(rtk->surveyInAccuracyLimit(), qQNaN());
    saved.setFactValue(rtk->fixedBasePositionLatitude(), qQNaN());
    QVERIFY(GPSSettings::receiver(*rtk, *settings->autoConnectSettings()).validationError.isEmpty());
    rtk->receiverRole()->setRawValue(2);
    QVERIFY(!GPSSettings::receiver(*rtk, *settings->autoConnectSettings()).validationError.isEmpty());
}

void GPSSettingsTest::_nmeaReceiverConfiguration()
{
    TestFixtures::SettingsFixture saved;
    auto* settings = SettingsManager::instance()->autoConnectSettings();
    saved.setFactValue(settings->nmeaSource(), AutoConnectSettings::NmeaSourceSerial);
    saved.setFactValue(settings->autoConnectNmeaPort(), QStringLiteral("/test/nmea"));
    saved.setFactValue(settings->autoConnectNmeaBaud(), 4800);
    saved.setFactValue(settings->nmeaReceiverMode(), AutoConnectSettings::NmeaReceiverPassive);
    const auto passive = GPSSettings::nmea(*settings).profile;
    QCOMPARE(passive.configurationPolicy, GPSReceiverProfile::ConfigurationPolicy::Passive);
    QCOMPARE(passive.endpoint.baud, 4800);
    settings->nmeaReceiverMode()->setRawValue(AutoConnectSettings::NmeaReceiverUblox);
    const auto managed = GPSSettings::nmea(*settings).profile;
    QCOMPARE(managed.configurationPolicy, GPSReceiverProfile::ConfigurationPolicy::Configure);
    QCOMPARE(managed.endpoint.baud, 0);
    QVERIFY(managed.validationError().isEmpty());
    QVERIFY(managed != passive);
    settings->autoConnectNmeaBaud()->setRawValue(9600);
    QCOMPARE(GPSSettings::nmea(*settings).profile, managed);
    auto invalid = managed;
    invalid.configurationPolicy = static_cast<GPSReceiverProfile::ConfigurationPolicy>(99);
    QVERIFY(!invalid.validationError().isEmpty());
    settings->nmeaSource()->setRawValue(AutoConnectSettings::NmeaSourceUdp);
    const auto udp = GPSSettings::nmea(*settings).profile;
    QCOMPARE(udp.configurationPolicy, GPSReceiverProfile::ConfigurationPolicy::Passive);
    settings->nmeaReceiverMode()->setRawValue(AutoConnectSettings::NmeaReceiverPassive);
    QCOMPARE(GPSSettings::nmea(*settings).profile, udp);
}

void GPSSettingsTest::_automaticIntent()
{
    TestFixtures::SettingsFixture saved;
    auto* settings = SettingsManager::instance();
    auto* automatic = settings->autoConnectSettings();
    auto* rtk = settings->rtkSettings();
    saved.setFactValue(automatic->nmeaAutoConnect(), true);
    saved.setFactValue(automatic->autoConnectRTKGPS(), true);
    saved.setFactValue(automatic->autoConnectNetworkRTKGPS(), false);
    saved.setFactValue(rtk->connectionType(), RTKSettings::Serial);
    const auto serial = GPSSettings::receiver(*rtk, *automatic);
    QVERIFY(serial.automatic);
    saved.setFactValue(rtk->connectionType(), RTKSettings::Udp);
    QVERIFY(!GPSSettings::receiver(*rtk, *automatic).automatic);
    QVERIFY(serial.automatic);
    QVERIFY(GPSSettings::nmea(*automatic).automatic);
    saved.setFactValue(automatic->nmeaAutoConnect(), false);
    QVERIFY(!GPSSettings::nmea(*automatic).automatic);
}
