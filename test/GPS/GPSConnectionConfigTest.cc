#include "GPSConnectionConfigTest.h"

#include <limits>

#include "AutoConnectSettings.h"
#include "Fixtures/RAIIFixtures.h"
#include "NMEAConnectionConfig.h"
#include "RTKConnectionConfig.h"
#include "RTKSettings.h"
#include "SettingsManager.h"

void GPSConnectionConfigTest::_nmeaValidation_data()
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

void GPSConnectionConfigTest::_nmeaValidation()
{
    QFETCH(int, source);
    QFETCH(QString, host);
    QFETCH(int, port);
    QFETCH(QString, device);
    QFETCH(int, baud);
    QFETCH(bool, valid);
    const NMEAConnectionConfig config{static_cast<NMEAConnectionConfig::Source>(source), host, port, device, baud};
    QCOMPARE(config.validationError().isEmpty(), valid);
}

void GPSConnectionConfigTest::_rtkValidation_data()
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

void GPSConnectionConfigTest::_rtkValidation()
{
    QFETCH(QString, field);
    QFETCH(double, value);
    QFETCH(bool, fixed);
    QFETCH(bool, valid);
    RTKConnectionConfig config;
    config.transport = RTKConnectionConfig::Udp;
    config.host = QStringLiteral("localhost");
    config.port = 2101;
    config.baseMode = fixed ? 1 : 0;
    config.receiver.base.useFixedBase = fixed;
    if (field == "accuracy")
        config.receiver.base.surveyInAccMeters = value;
    else if (field == "duration")
        config.receiver.base.surveyInDurationSecs = static_cast<int>(value);
    else if (field == "latitude")
        config.receiver.base.fixedBaseLatitude = value;
    else if (field == "longitude")
        config.receiver.base.fixedBaseLongitude = value;
    else if (field == "altitude")
        config.receiver.base.fixedBaseAltitudeMeters = value;
    else if (field == "fixedAccuracy")
        config.receiver.base.fixedBaseAccuracyMeters = value;
    else if (field == "localPort")
        config.localPort = static_cast<int>(value);
    else if (field == "port")
        config.port = static_cast<int>(value);
    else if (field == "receiver")
        config.receiverType = static_cast<GPSType>(static_cast<int>(value));
    else if (field == "transport")
        config.transport = static_cast<RTKConnectionConfig::Transport>(static_cast<int>(value));
    else if (field == "mode")
        config.baseMode = static_cast<int>(value);
    QCOMPARE(config.validationError().isEmpty(), valid);
}

void GPSConnectionConfigTest::_settingsSnapshots()
{
    expectLogMessage("API.QGCApplication.AppMessage", QtDebugMsg,
                     QRegularExpression(QStringLiteral("Restart application for changes to take effect")));
    TestFixtures::SettingsFixture saved;
    auto* settings = SettingsManager::instance();
    auto* nmea = settings->autoConnectSettings();
    saved.setFactValue(nmea->nmeaSource(), AutoConnectSettings::NmeaSourceTcp);
    saved.setFactValue(nmea->nmeaTcpHost(), QStringLiteral("  localhost  "));
    saved.setFactValue(nmea->nmeaTcpPort(), 3100);
    const auto nmeaConfig = NMEAConnectionConfig::fromSettings(*nmea);
    QCOMPARE(nmeaConfig.source, NMEAConnectionConfig::Tcp);
    QCOMPARE(nmeaConfig.host, QStringLiteral("localhost"));
    nmea->nmeaTcpPort()->setRawValue(3200);
    QCOMPARE(nmeaConfig.port, 3100);
    QCOMPARE(NMEAConnectionConfig::fromSettings(*nmea).port, 3200);

    auto* rtk = settings->rtkSettings();
    saved.setFactValue(rtk->receiverRole(), RTKSettings::RTKBase);
    saved.setFactValue(rtk->connectionType(), RTKSettings::Udp);
    saved.setFactValue(rtk->networkReceiverType(), 3);
    saved.setFactValue(rtk->networkBaseHost(), QStringLiteral("  localhost  "));
    saved.setFactValue(rtk->surveyInAccuracyLimit(), 1.5);
    const auto rtkConfig = RTKConnectionConfig::fromSettings(*rtk);
    QCOMPARE(rtkConfig.transport, RTKConnectionConfig::Udp);
    QCOMPARE(rtkConfig.receiverType, GPSType::femto);
    QCOMPARE(rtkConfig.host, QStringLiteral("localhost"));
    rtk->surveyInAccuracyLimit()->setRawValue(2.5);
    QCOMPARE(rtkConfig.receiver.base.surveyInAccMeters, 1.5);
    QCOMPARE(RTKConnectionConfig::fromSettings(*rtk).receiver.base.surveyInAccMeters, 2.5);
    QCOMPARE(rtkConfig.receiver.role, GPSReceiverConfig::Role::RTKBase);
    rtk->receiverRole()->setRawValue(RTKSettings::Position);
    QCOMPARE(rtkConfig.receiver.role, GPSReceiverConfig::Role::RTKBase);
    QCOMPARE(RTKConnectionConfig::fromSettings(*rtk).receiver.role, GPSReceiverConfig::Role::Position);
    verifyExpectedLogMessage();
}

UT_REGISTER_TEST(GPSConnectionConfigTest, TestLabel::Unit)

void GPSConnectionConfigTest::_positionRoleValidation()
{
    RTKConnectionConfig config;
    QCOMPARE(config.receiver.role, GPSReceiverConfig::Role::RTKBase);
    config.receiver.role = GPSReceiverConfig::Role::Position;
    config.baseMode = -1;
    config.receiver.base.surveyInAccMeters = qQNaN();
    config.receiver.base.fixedBaseLatitude = qQNaN();
    QVERIFY(config.validationError().isEmpty());
    config.receiver.base.useFixedBase = true;
    QVERIFY(config.validationError().isEmpty());
    config.transport = RTKConnectionConfig::Tcp;
    QVERIFY(!config.validationError().isEmpty());
    config.host = QStringLiteral("localhost");
    config.port = 2101;
    QVERIFY(config.validationError().isEmpty());
    config.receiver.role = static_cast<GPSReceiverConfig::Role>(2);
    QVERIFY(!config.validationError().isEmpty());
}
