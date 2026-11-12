#include "NTRIPConfigurationTest.h"

#include <QtTest/QTest>

#include "NTRIPConfiguration.h"

void NTRIPConfigurationTest::_validity_data()
{
    QTest::addColumn<QString>("host");
    QTest::addColumn<int>("port");
    QTest::addColumn<QString>("username");
    QTest::addColumn<QString>("mountpoint");
    QTest::addColumn<bool>("valid");
    // A correction stream additionally needs a mountpoint.
    QTest::addColumn<bool>("streamValid");
    const QString host = QStringLiteral("caster.example.com");
    QTest::newRow("empty-host") << QString() << 2101 << QString() << QString() << false << false;
    QTest::newRow("default-port") << host << 2101 << QString() << QString() << true << false;
    QTest::newRow("lowest-port") << host << 1 << QString() << QString() << true << false;
    QTest::newRow("highest-port") << host << 65535 << QString() << QString() << true << false;
    QTest::newRow("zero-port") << host << 0 << QString() << QString() << false << false;
    QTest::newRow("negative-port") << host << -1 << QString() << QString() << false << false;
    QTest::newRow("port-overflow") << host << 65536 << QString() << QString() << false << false;
    QTest::newRow("username") << host << 2101 << QStringLiteral("username") << QString() << true << false;
    QTest::newRow("colon-in-username") << host << 2101 << QStringLiteral("user:name") << QString() << false << false;
    QTest::newRow("mountpoint") << host << 2101 << QString() << QStringLiteral("MP1") << true << true;
    QTest::newRow("blank-mountpoint") << host << 2101 << QString() << QStringLiteral("   ") << false << false;
    QTest::newRow("unicode-space-mountpoint")
        << host << 2101 << QString() << QStringLiteral("MP\u00A0B") << false << false;
    QTest::newRow("host-header-injection")
        << QStringLiteral("caster.example.com\r\nEvil: header") << 2101 << QString() << QString() << false << false;
    QTest::newRow("mountpoint-header-injection")
        << host << 2101 << QString() << QStringLiteral("MP\r\nInjected") << false << false;
}

void NTRIPConfigurationTest::_validity()
{
    QFETCH(QString, host);
    QFETCH(int, port);
    QFETCH(QString, username);
    QFETCH(QString, mountpoint);
    QFETCH(bool, valid);
    QFETCH(bool, streamValid);
    NTRIPConnectionConfig config;
    config.host = host;
    config.port = port;
    config.username = username;
    config.mountpoint = mountpoint;
    QCOMPARE(config.validationError().isEmpty(), valid);
    QCOMPARE(config.streamValidationError().isEmpty(), streamValid);
}

void NTRIPConfigurationTest::_endpointJoinsHostAndPort_data()
{
    QTest::addColumn<QString>("host");
    QTest::addColumn<int>("port");
    QTest::addColumn<QString>("endpoint");
    QTest::newRow("name") << QStringLiteral("caster.example.com") << 2101 << QStringLiteral("caster.example.com:2101");
    QTest::newRow("ipv4") << QStringLiteral("192.0.2.7") << 80 << QStringLiteral("192.0.2.7:80");
    // An IPv6 literal goes in brackets, as in a URL: "fe80::1:2101" reads as another address, without a port.
    QTest::newRow("ipv6") << QStringLiteral("fe80::1") << 2101 << QStringLiteral("[fe80::1]:2101");
    QTest::newRow("ipv6-loopback") << QStringLiteral("::1") << 443 << QStringLiteral("[::1]:443");
    QTest::newRow("ipv6-that-ends-as-a-port")
        << QStringLiteral("fe80::1:2101") << 443 << QStringLiteral("[fe80::1:2101]:443");
    QTest::newRow("ipv6-in-brackets") << QStringLiteral("[fe80::1]") << 2101 << QStringLiteral("[fe80::1]:2101");
    // A link-local IPv6 address with a numeric zone, as Windows writes it: the zone is no place marker.
    QTest::newRow("zone-index") << QStringLiteral("fe80::1%2") << 2101 << QStringLiteral("[fe80::1%2]:2101");
    QTest::newRow("first-marker") << QStringLiteral("a%1b") << 2101 << QStringLiteral("a%1b:2101");
    // Must not name the same caster as the host "a5".
    QTest::newRow("second-marker") << QStringLiteral("a%2") << 5 << QStringLiteral("a%2:5");
}

void NTRIPConfigurationTest::_endpointJoinsHostAndPort()
{
    QFETCH(QString, host);
    QFETCH(int, port);
    QFETCH(QString, endpoint);
    NTRIPConnectionConfig config;
    config.host = host;
    config.port = port;
    QCOMPARE(config.endpoint(), endpoint);
}

UT_REGISTER_TEST_LIGHTWEIGHT(NTRIPConfigurationTest, TestLabel::Unit)
