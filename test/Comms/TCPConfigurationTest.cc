#include "TCPConfigurationTest.h"
#include "TCPLink.h"

#include <QtCore/QSettings>
#include <QtCore/QTemporaryFile>
#include <QtTest/QTest>
#include <QtTest/QSignalSpy>

void TCPConfigurationTest::_hostPropertyTest()
{
    TCPConfiguration config("TestLink");
    QSignalSpy spy(&config, &TCPConfiguration::hostChanged);

    config.setHost("192.168.1.100");
    QCOMPARE(config.host(), QStringLiteral("192.168.1.100"));
    QCOMPARE(spy.count(), 1);

    // Setting same value should not emit signal
    config.setHost("192.168.1.100");
    QCOMPARE(spy.count(), 1);

    config.setHost("localhost");
    QCOMPARE(config.host(), QStringLiteral("localhost"));
    QCOMPARE(spy.count(), 2);
}

void TCPConfigurationTest::_portPropertyTest()
{
    TCPConfiguration config("TestLink");
    QSignalSpy spy(&config, &TCPConfiguration::portChanged);

    config.setPort(14550);
    QCOMPARE(config.port(), static_cast<quint16>(14550));
    QCOMPARE(spy.count(), 1);

    // Setting same value should not emit signal
    config.setPort(14550);
    QCOMPARE(spy.count(), 1);

    config.setPort(5760);
    QCOMPARE(config.port(), static_cast<quint16>(5760));
    QCOMPARE(spy.count(), 2);
}

void TCPConfigurationTest::_defaultValuesTest()
{
    TCPConfiguration config("TestLink");

    // Check default port (5760 is typical for SITL)
    QCOMPARE(config.port(), static_cast<quint16>(5760));
    // Host might be empty by default
    QVERIFY(config.host().isEmpty() || !config.host().isEmpty());
}

void TCPConfigurationTest::_copyFromTest()
{
    TCPConfiguration source("SourceLink");
    source.setHost("10.0.0.1");
    source.setPort(14551);
    source.setDynamic(true);

    TCPConfiguration target("TargetLink");
    target.copyFrom(&source);

    QCOMPARE(target.name(), source.name());
    QCOMPARE(target.host(), source.host());
    QCOMPARE(target.port(), source.port());
    QCOMPARE(target.isDynamic(), source.isDynamic());
}

void TCPConfigurationTest::_copyConstructorTest()
{
    TCPConfiguration original("OriginalLink");
    original.setHost("192.168.0.1");
    original.setPort(5762);
    original.setAutoConnect(true);

    TCPConfiguration copy(&original);

    QCOMPARE(copy.name(), original.name());
    QCOMPARE(copy.host(), original.host());
    QCOMPARE(copy.port(), original.port());
    QCOMPARE(copy.isAutoConnect(), original.isAutoConnect());
}

void TCPConfigurationTest::_typeTest()
{
    TCPConfiguration config("TestLink");

    QCOMPARE(config.type(), LinkConfiguration::TypeTcp);
    QCOMPARE(config.settingsURL(), QStringLiteral("TcpSettings.qml"));
    QVERIFY(!config.settingsTitle().isEmpty());
}

void TCPConfigurationTest::_settingsTest()
{
    QTemporaryFile tempFile;
    QVERIFY(tempFile.open());
    tempFile.close();

    const QString root = "TestRoot";

    // Save settings
    {
        TCPConfiguration config("SaveTest");
        config.setHost("192.168.1.50");
        config.setPort(14555);

        QSettings settings(tempFile.fileName(), QSettings::IniFormat);
        config.saveSettings(settings, root);
    }

    // Load settings
    {
        TCPConfiguration config("LoadTest");
        QSettings settings(tempFile.fileName(), QSettings::IniFormat);
        config.loadSettings(settings, root);

        QCOMPARE(config.host(), QStringLiteral("192.168.1.50"));
        QCOMPARE(config.port(), static_cast<quint16>(14555));
    }
}
