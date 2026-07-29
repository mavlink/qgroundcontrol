#include "AutoConnectSettingsTest.h"

#include <QtCore/QSettings>

#include "AutoConnectSettings.h"

void AutoConnectSettingsTest::init()
{
    UnitTest::init();

    QSettings settings;
    settings.beginGroup(AutoConnectSettings::settingsGroup);
    _hadNmeaSource = settings.contains(AutoConnectSettings::nmeaSourceName);
    _savedNmeaSource = settings.value(AutoConnectSettings::nmeaSourceName);
    _hadNmeaPort = settings.contains(AutoConnectSettings::autoConnectNmeaPortName);
    _savedNmeaPort = settings.value(AutoConnectSettings::autoConnectNmeaPortName);
    settings.endGroup();
}

void AutoConnectSettingsTest::cleanup()
{
    QSettings settings;
    settings.beginGroup(AutoConnectSettings::settingsGroup);
    if (_hadNmeaSource) {
        settings.setValue(AutoConnectSettings::nmeaSourceName, _savedNmeaSource);
    } else {
        settings.remove(AutoConnectSettings::nmeaSourceName);
    }
    if (_hadNmeaPort) {
        settings.setValue(AutoConnectSettings::autoConnectNmeaPortName, _savedNmeaPort);
    } else {
        settings.remove(AutoConnectSettings::autoConnectNmeaPortName);
    }
    settings.endGroup();

    UnitTest::cleanup();
}

void AutoConnectSettingsTest::_legacyNmeaPortMigration_data()
{
    QTest::addColumn<QString>("legacyValue");
    QTest::addColumn<int>("expectedSource");
    QTest::addColumn<QString>("expectedPort");

    QTest::newRow("disabled") << "Disabled"
                              << static_cast<int>(AutoConnectSettings::NmeaSourceDisabled) << "";
    QTest::newRow("no serial ports placeholder") << "Serial <none available>"
                              << static_cast<int>(AutoConnectSettings::NmeaSourceDisabled) << "";
    QTest::newRow("empty") << ""
                              << static_cast<int>(AutoConnectSettings::NmeaSourceDisabled) << "";
    QTest::newRow("udp") << "UDP Port"
                              << static_cast<int>(AutoConnectSettings::NmeaSourceUdp) << "";
    QTest::newRow("serial device") << "/dev/tty.usbmodem01"
                              << static_cast<int>(AutoConnectSettings::NmeaSourceSerial) << "/dev/tty.usbmodem01";
}

void AutoConnectSettingsTest::_legacyNmeaPortMigration()
{
    QFETCH(QString, legacyValue);
    QFETCH(int, expectedSource);
    QFETCH(QString, expectedPort);

    QSettings settings;
    settings.beginGroup(AutoConnectSettings::settingsGroup);
    settings.remove(AutoConnectSettings::nmeaSourceName);
    settings.setValue(AutoConnectSettings::autoConnectNmeaPortName, legacyValue);
    settings.endGroup();

    // Migration runs in the constructor. SettingsFacts ignore QSettings under unit tests,
    // so assert against the raw stored values rather than the facts.
    const AutoConnectSettings acSettings;
    settings.beginGroup(AutoConnectSettings::settingsGroup);
    QCOMPARE(settings.value(AutoConnectSettings::nmeaSourceName,
                            static_cast<int>(AutoConnectSettings::NmeaSourceDisabled)).toInt(), expectedSource);
    QCOMPARE(settings.contains(AutoConnectSettings::autoConnectNmeaPortName), !expectedPort.isEmpty());
    if (!expectedPort.isEmpty()) {
        QCOMPARE(settings.value(AutoConnectSettings::autoConnectNmeaPortName).toString(), expectedPort);
    }
    settings.endGroup();
}

void AutoConnectSettingsTest::_nmeaMigrationSkippedWhenSourceAlreadySet()
{
    // Once nmeaSource exists the port value must never be reinterpreted, even if it
    // happens to match a legacy token.
    QSettings settings;
    settings.beginGroup(AutoConnectSettings::settingsGroup);
    settings.setValue(AutoConnectSettings::nmeaSourceName, static_cast<int>(AutoConnectSettings::NmeaSourceSerial));
    settings.setValue(AutoConnectSettings::autoConnectNmeaPortName, "Disabled");
    settings.endGroup();

    const AutoConnectSettings acSettings;
    settings.beginGroup(AutoConnectSettings::settingsGroup);
    QCOMPARE(settings.value(AutoConnectSettings::nmeaSourceName).toInt(),
             static_cast<int>(AutoConnectSettings::NmeaSourceSerial));
    QCOMPARE(settings.value(AutoConnectSettings::autoConnectNmeaPortName).toString(), QStringLiteral("Disabled"));
    settings.endGroup();
}

UT_REGISTER_TEST(AutoConnectSettingsTest, TestLabel::Unit)
