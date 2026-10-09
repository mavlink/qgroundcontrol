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

UT_REGISTER_TEST_LIGHTWEIGHT(NTRIPConfigurationTest, TestLabel::Unit)
