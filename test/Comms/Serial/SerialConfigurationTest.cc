#include "SerialConfigurationTest.h"

#include "SerialLink.h"
#include "QGCSerialPortInfo.h"
#include "QGCSerialPortTypes.h"

#include "Fixtures/RAIIFixtures.h"

#include <QtCore/QSettings>
#include <QtCore/QStringList>

UT_REGISTER_TEST(SerialConfigurationTest, TestLabel::Unit, TestLabel::Comms)

void SerialConfigurationTest::_testPortConfigEnumMapping()
{
    SerialConfiguration config(QStringLiteral("EnumMap"));
    config.setBaud(115200);
    config.setDataBits(static_cast<int>(QGCDataBits::Data7));
    config.setStopBits(static_cast<int>(QGCStopBits::TwoStop));
    config.setParity(static_cast<int>(QGCParity::Even));
    config.setFlowControl(static_cast<int>(QGCFlowControl::HardwareRtsCts));

    const SerialPortConfig cfg = config.portConfig();
    QCOMPARE(cfg.baud, 115200);
    QVERIFY(cfg.dataBits == QGCDataBits::Data7);
    QVERIFY(cfg.stopBits == QGCStopBits::TwoStop);
    QVERIFY(cfg.parity == QGCParity::Even);
    QVERIFY(cfg.flowControl == QGCFlowControl::HardwareRtsCts);
    QVERIFY(cfg.isValid());
}

void SerialConfigurationTest::_testParityMigrationFromLegacy()
{
    TestFixtures::TempDirFixture tmpDir;
    QVERIFY(tmpDir.isValid());
    const QString iniPath = tmpDir.path() + QStringLiteral("/settings.ini");
    const QString root = QStringLiteral("SerialConfigTest_LegacyParity");

    const int legacyToQgc[][2] = {{0, 0}, {1, 0}, {2, 2}, {3, 1}, {4, 4}, {5, 3}};
    for (const auto &pair : legacyToQgc) {
        QSettings settings(iniPath, QSettings::IniFormat);
        settings.remove(root);
        settings.beginGroup(root);
        settings.setValue(QStringLiteral("parity"), pair[0]);
        settings.endGroup();
        settings.sync();

        SerialConfiguration config(QStringLiteral("LegacyLoad"));
        config.loadSettings(settings, root);
        QCOMPARE(config.parity(), pair[1]);
    }
}

void SerialConfigurationTest::_testParityMigrationClampsOutOfRange()
{
    TestFixtures::TempDirFixture tmpDir;
    QVERIFY(tmpDir.isValid());
    const QString iniPath = tmpDir.path() + QStringLiteral("/settings.ini");
    const QString root = QStringLiteral("SerialConfigTest_ParityClamp");

    const int clamp[][2] = {{99, static_cast<int>(QGCParity::Mark)}, {-7, static_cast<int>(QGCParity::None)}};
    for (const auto &pair : clamp) {
        QSettings settings(iniPath, QSettings::IniFormat);
        settings.remove(root);
        settings.beginGroup(root);
        settings.setValue(QStringLiteral("parity"), pair[0]);
        settings.endGroup();
        settings.sync();

        SerialConfiguration config(QStringLiteral("ClampLoad"));
        config.loadSettings(settings, root);
        QCOMPARE(config.parity(), pair[1]);
    }
}

void SerialConfigurationTest::_testParityV2TakesPrecedenceOverLegacy()
{
    TestFixtures::TempDirFixture tmpDir;
    QVERIFY(tmpDir.isValid());
    const QString iniPath = tmpDir.path() + QStringLiteral("/settings.ini");
    const QString root = QStringLiteral("SerialConfigTest_ParityV2");

    QSettings settings(iniPath, QSettings::IniFormat);
    settings.beginGroup(root);
    settings.setValue(QStringLiteral("parity"), 2);
    settings.setValue(QStringLiteral("parityV2"), static_cast<int>(QGCParity::Odd));
    settings.endGroup();
    settings.sync();

    SerialConfiguration config(QStringLiteral("V2Load"));
    config.loadSettings(settings, root);
    QCOMPARE(config.parity(), static_cast<int>(QGCParity::Odd));
}

void SerialConfigurationTest::_testSettingsRoundtripWritesParityV2()
{
    TestFixtures::TempDirFixture tmpDir;
    QVERIFY(tmpDir.isValid());
    const QString iniPath = tmpDir.path() + QStringLiteral("/settings.ini");
    const QString root = QStringLiteral("SerialConfigTest_Roundtrip");
    QSettings settings(iniPath, QSettings::IniFormat);

    {
        SerialConfiguration config(QStringLiteral("Save"));
        config.setBaud(921600);
        config.setDataBits(static_cast<int>(QGCDataBits::Data7));
        config.setStopBits(static_cast<int>(QGCStopBits::TwoStop));
        config.setParity(static_cast<int>(QGCParity::Mark));
        config.setFlowControl(static_cast<int>(QGCFlowControl::HardwareRtsCts));
        config.setPortName(QStringLiteral("/dev/ttyACM0"));
        config.setUsbDirect(true);
        config.setdtrForceLow(true);
        config.saveSettings(settings, root);
    }

    QVERIFY(settings.contains(root + QStringLiteral("/parityV2")));
    QVERIFY(!settings.contains(root + QStringLiteral("/parity")));

    {
        SerialConfiguration config(QStringLiteral("Load"));
        config.loadSettings(settings, root);
        QCOMPARE(config.baud(), 921600);
        QCOMPARE(config.dataBits(), static_cast<int>(QGCDataBits::Data7));
        QCOMPARE(config.stopBits(), static_cast<int>(QGCStopBits::TwoStop));
        QCOMPARE(config.parity(), static_cast<int>(QGCParity::Mark));
        QCOMPARE(config.flowControl(), static_cast<int>(QGCFlowControl::HardwareRtsCts));
        QCOMPARE(config.portName(), QStringLiteral("/dev/ttyACM0"));
        QVERIFY(config.usbDirect());
        QVERIFY(config.dtrForceLow());
    }
}

void SerialConfigurationTest::_testSupportedBaudRatesSortedUniqueNonEmpty()
{
    const QStringList rates = QGCSerialPortInfo::supportedBaudRateStrings();
    QVERIFY(!rates.isEmpty());
    QVERIFY(rates.contains(QStringLiteral("57600")));
    QVERIFY(rates.contains(QStringLiteral("115200")));

    int previous = -1;
    for (const QString &rate : rates) {
        bool ok = false;
        const int value = rate.toInt(&ok);
        QVERIFY(ok);
        QVERIFY2(value > previous, qPrintable(QStringLiteral("rates must be strictly ascending: %1").arg(rate)));
        previous = value;
    }
}

void SerialConfigurationTest::_testCleanPortDisplayNameUnknownReturnsEmpty()
{
    QVERIFY(QGCSerialPortInfo::displayNameForLocation(QStringLiteral("/dev/does-not-exist-xyz")).isEmpty());
}
