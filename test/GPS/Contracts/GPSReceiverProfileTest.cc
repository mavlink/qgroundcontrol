#include "GPSReceiverProfileTest.h"

#include "GPSReceiverProfile.h"

void GPSReceiverProfileTest::_endpointValidation_data()
{
    QTest::addColumn<int>("kind");
    QTest::addColumn<bool>("configured");
    QTest::addColumn<QString>("host");
    QTest::addColumn<int>("port");
    QTest::addColumn<int>("baud");
    QTest::addColumn<bool>("valid");
    using Kind = GPSReceiverProfile::Endpoint::Kind;
    QTest::newRow("udp-listener-ephemeral") << int(Kind::UdpListener) << false << QString() << 0 << 0 << true;
    QTest::newRow("udp-listener-ignores-peer")
        << int(Kind::UdpListener) << false << QString("bad host") << 14550 << 0 << true;
    QTest::newRow("udp-listener-cannot-configure")
        << int(Kind::UdpListener) << true << QString() << 14550 << 0 << false;
    QTest::newRow("udp-peer-requires-host") << int(Kind::UdpPeer) << true << QString() << 14550 << 0 << false;
    QTest::newRow("udp-peer-requires-port") << int(Kind::UdpPeer) << true << QString("localhost") << 0 << 0 << false;
    QTest::newRow("udp-peer") << int(Kind::UdpPeer) << true << QString("localhost") << 14550 << 0 << true;
    QTest::newRow("tcp-ipv6") << int(Kind::Tcp) << false << QString("::1") << 10000 << 0 << true;
    QTest::newRow("tcp-invalid-host") << int(Kind::Tcp) << false << QString("bad host") << 10000 << 0 << false;
    QTest::newRow("tcp-invalid-port") << int(Kind::Tcp) << true << QString("localhost") << 65536 << 0 << false;
    QTest::newRow("serial-passive") << int(Kind::Serial) << false << QString() << 0 << 9600 << true;
    QTest::newRow("serial-passive-needs-baud") << int(Kind::Serial) << false << QString() << 0 << 0 << false;
    QTest::newRow("serial-configured-ignores-passive-baud")
        << int(Kind::Serial) << true << QString() << 0 << -1 << true;
}

void GPSReceiverProfileTest::_endpointValidation()
{
    QFETCH(int, kind);
    QFETCH(bool, configured);
    QFETCH(QString, host);
    QFETCH(int, port);
    QFETCH(int, baud);
    QFETCH(bool, valid);
    GPSReceiverProfile profile;
    profile.endpoint.kind = static_cast<GPSReceiverProfile::Endpoint::Kind>(kind);
    profile.endpoint.device = QStringLiteral("/test/receiver");
    profile.endpoint.host = host;
    profile.endpoint.port = port;
    profile.endpoint.baud = baud;
    profile.configurationPolicy = configured ? GPSReceiverProfile::ConfigurationPolicy::Configure
                                             : GPSReceiverProfile::ConfigurationPolicy::Passive;
    QCOMPARE(profile.validationError().isEmpty(), valid);
    QCOMPARE(profile.normalized().validationError().isEmpty(), valid);
}

void GPSReceiverProfileTest::_inactiveFieldsDoNotChangeProfile()
{
    GPSReceiverProfile profile{
        .endpoint = {.kind = GPSReceiverProfile::Endpoint::Kind::Serial, .device = QStringLiteral("/test/receiver")},
        .configurationPolicy = GPSReceiverProfile::ConfigurationPolicy::Configure};
    const auto original = profile.normalized();
    profile.endpoint.host = QStringLiteral("not a host");
    profile.endpoint.port = -20;
    profile.endpoint.localPort = 100000;
    profile.receiver.base.fixedBaseLatitude = 1000;
    profile.receiver.base.surveyInAccMeters = -20;
    QCOMPARE(profile, original);
    QVERIFY(profile.validationError().isEmpty());
    profile.receiver.outputRateHz = 5;
    QVERIFY(profile != original);

    GPSReceiverProfile passive{.endpoint = {.kind = GPSReceiverProfile::Endpoint::Kind::Tcp,
                                            .host = QStringLiteral("localhost"),
                                            .port = 10000}};
    const auto tcp = passive;
    passive.endpoint.device = QStringLiteral("/different/serial");
    passive.endpoint.baud = -10;
    passive.driverType = GPSType::femto;
    QCOMPARE(passive, tcp);
    QVERIFY(passive.validationError().isEmpty());
    passive.endpoint.port = 10001;
    QVERIFY(passive != tcp);
}

UT_REGISTER_TEST(GPSReceiverProfileTest, TestLabel::Unit)
