#include "GPSReceiverProfileTest.h"

#include <QtCore/QIODevice>
#include <QtNetwork/QUdpSocket>

#include "GPSConnectionConfig.h"
#include "GPSReceiverProfile.h"
#include "GPSReceiverTransportFactory.h"
#include "GPSTransport.h"
#include "NMEAConnectionAttempt.h"
#include "NMEAConnectionConfig.h"

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

void GPSReceiverProfileTest::_settingsAdaptersShareProfiles()
{
    NMEAConnectionConfig nmea;
    nmea.source = NMEAConnectionConfig::Serial;
    nmea.receiverMode = NMEAConnectionConfig::Ublox;
    nmea.device = QStringLiteral("/test/receiver");
    GPSConnectionConfig receiver;
    receiver.device = nmea.device;
    receiver.receiver.role = GPSReceiverConfig::Role::Position;
    receiver.receiver.outputProtocol = GPSReceiverConfig::OutputProtocol::NMEA;
    QCOMPARE(nmea.profile(), receiver.profile());
    QVERIFY(nmea.validationError().isEmpty());
    QVERIFY(receiver.validationError().isEmpty());

    nmea.source = NMEAConnectionConfig::Udp;
    nmea.port = 14550;
    receiver.transport = GPSConnectionConfig::Udp;
    receiver.host = QStringLiteral("localhost");
    receiver.port = nmea.port;
    QCOMPARE(nmea.profile().endpoint.kind, GPSReceiverProfile::Endpoint::Kind::UdpListener);
    QCOMPARE(receiver.profile().endpoint.kind, GPSReceiverProfile::Endpoint::Kind::UdpPeer);
    QVERIFY(nmea.profile() != receiver.profile());
    QVERIFY(!GPSReceiverTransportFactory::network(nmea.profile()));
    QVERIFY(GPSReceiverTransportFactory::network(receiver.profile()));
}

void GPSReceiverProfileTest::_inactiveFieldsDoNotChangeProfile()
{
    GPSConnectionConfig config;
    config.device = QStringLiteral("/test/receiver");
    config.receiver.role = GPSReceiverConfig::Role::Position;
    const auto original = config.profile();
    config.host = QStringLiteral("not a host");
    config.port = -20;
    config.localPort = 100000;
    config.receiver.base.fixedBaseLatitude = 1000;
    config.receiver.base.surveyInAccMeters = -20;
    QCOMPARE(config.profile(), original);
    QVERIFY(config.validationError().isEmpty());
    config.receiver.outputRateHz = 5;
    QVERIFY(config.profile() != original);

    NMEAConnectionConfig nmea;
    nmea.source = NMEAConnectionConfig::Tcp;
    nmea.host = QStringLiteral("localhost");
    nmea.port = 10000;
    const auto tcp = nmea;
    nmea.receiverMode = static_cast<NMEAConnectionConfig::ReceiverMode>(-1);
    nmea.device = QStringLiteral("/different/serial");
    nmea.baud = -10;
    QCOMPARE(nmea, tcp);
    QVERIFY(nmea.validationError().isEmpty());
    nmea.port = 10001;
    QVERIFY(nmea != tcp);
}

void GPSReceiverProfileTest::_passiveAttemptNeverConfigures()
{
    GPSReceiverProfile profile;
    profile.endpoint.kind = GPSReceiverProfile::Endpoint::Kind::UdpListener;
    NMEAConnectionAttempt passive(profile);
    QSignalSpy ready(&passive, &NMEAConnectionAttempt::deviceReady);
    passive.start();
    QCOMPARE(ready.size(), 1);
    QVERIFY(passive.device());
    QVERIFY(passive.device()->open(QIODevice::ReadOnly));
    QVERIFY(passive.device()->isReadable());
    QUdpSocket sender;
    const QByteArray bytes("passive-input\n");
    QCOMPARE(sender.writeDatagram(bytes, QHostAddress::LocalHost, passive.localPort()), bytes.size());
    QTRY_COMPARE_WITH_TIMEOUT(passive.device()->bytesAvailable(), bytes.size(), TestTimeout::mediumMs());
    QCOMPARE(passive.device()->readAll(), bytes);
    passive.shutdown();

    NMEAConnectionAttempt rejected(profile);
    QSignalSpy failed(&rejected, &NMEAConnectionAttempt::failed);
    bool factoryCalled = false;
    rejected.start([&](const std::atomic_bool&) {
        factoryCalled = true;
        return std::unique_ptr<GPSTransport>();
    });
    QCOMPARE(failed.size(), 1);
    QVERIFY(!factoryCalled);
    QVERIFY(!rejected.device());
    rejected.shutdown();
}

UT_REGISTER_TEST(GPSReceiverProfileTest, TestLabel::Unit)
