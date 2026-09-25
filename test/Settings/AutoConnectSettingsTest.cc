#include "AutoConnectSettingsTest.h"

#include <QtCore/QSettings>

#include "AutoConnectSettings.h"
#include "RTKSettings.h"

namespace {
QHash<QString, QVariant> readGroup(const QString& group)
{
    QSettings settings;
    settings.beginGroup(group);
    QHash<QString, QVariant> values;
    for (const QString& key : settings.childKeys()) {
        values.insert(key, settings.value(key));
    }
    return values;
}

void writeGroup(const QString& group, const QHash<QString, QVariant>& values)
{
    QSettings settings;
    settings.remove(group);
    settings.beginGroup(group);
    for (auto it = values.cbegin(); it != values.cend(); ++it) {
        settings.setValue(it.key(), it.value());
    }
}

QVariant stored(const char* group, const char* key)
{
    QSettings settings;
    settings.beginGroup(QLatin1String(group));
    return settings.value(QLatin1String(key));
}
}  // namespace

void AutoConnectSettingsTest::init()
{
    UnitTest::init();
    for (const auto* group : {AutoConnectSettings::settingsGroup, RTKSettings::settingsGroup}) {
        const QString name = QLatin1String(group);
        _savedGroups.insert(name, readGroup(name));
        writeGroup(name, {});
    }
}

void AutoConnectSettingsTest::cleanup()
{
    for (auto it = _savedGroups.cbegin(); it != _savedGroups.cend(); ++it) {
        writeGroup(it.key(), it.value());
    }
    _savedGroups.clear();
    UnitTest::cleanup();
}

// Migrations run in the constructors. SettingsFacts ignore QSettings under unit tests,
// so these assertions read the raw stored values rather than the facts.

void AutoConnectSettingsTest::_nmeaPositionSourceMigration()
{
    writeGroup(QLatin1String(AutoConnectSettings::settingsGroup), {{QStringLiteral("gcsPositionSource"), 2}});
    const AutoConnectSettings autoConnect;
    QCOMPARE(stored(AutoConnectSettings::settingsGroup, "gcsPositionSource").toInt(), 1);

    writeGroup(QLatin1String(AutoConnectSettings::settingsGroup), {{QStringLiteral("gcsPositionSource"), 3}});
    const AutoConnectSettings unchanged;
    QCOMPARE(stored(AutoConnectSettings::settingsGroup, "gcsPositionSource").toInt(), 3);
}

void AutoConnectSettingsTest::_nmeaInputBecomesPositionOnlyReceiver_data()
{
    QTest::addColumn<int>("source");
    QTest::addColumn<int>("connection");
    QTest::newRow("udp") << 1 << 2;
    QTest::newRow("serial") << 2 << 0;
    QTest::newRow("tcp") << 3 << 1;
}

void AutoConnectSettingsTest::_nmeaInputBecomesPositionOnlyReceiver()
{
    QFETCH(int, source);
    QFETCH(int, connection);
    writeGroup(QLatin1String(AutoConnectSettings::settingsGroup),
               {
                   {QStringLiteral("nmeaSource"), source},
                   {QStringLiteral("autoConnectNmeaPort"), QStringLiteral("/dev/ttyNMEA")},
                   {QStringLiteral("autoConnectNmeaBaud"), 9600},
                   {QStringLiteral("nmeaUdpPort"), 14555},
                   {QStringLiteral("nmeaTcpHost"), QStringLiteral("gnss.local")},
                   {QStringLiteral("nmeaTcpPort"), 2101},
               });
    const RTKSettings rtk;
    QCOMPARE(stored(RTKSettings::settingsGroup, "receiverRole").toInt(), 0);
    QCOMPARE(stored(RTKSettings::settingsGroup, "connectOnStartup").toBool(), true);
    QCOMPARE(stored(RTKSettings::settingsGroup, "connectionType").toInt(), connection);
    if (source == 1) {
        QCOMPARE(stored(RTKSettings::settingsGroup, "udpPort").toInt(), 14555);
    } else if (source == 2) {
        QCOMPARE(stored(RTKSettings::settingsGroup, "serialDevice").toString(), QStringLiteral("/dev/ttyNMEA"));
        QCOMPARE(stored(RTKSettings::settingsGroup, "serialBaudRate").toInt(), 9600);
    } else {
        QCOMPARE(stored(RTKSettings::settingsGroup, "tcpHost").toString(), QStringLiteral("gnss.local"));
        QCOMPARE(stored(RTKSettings::settingsGroup, "tcpPort").toInt(), 2101);
    }
    QVERIFY(readGroup(QLatin1String(AutoConnectSettings::settingsGroup)).isEmpty());

    // The legacy input is migrated once.
    const RTKSettings again;
    QCOMPARE(stored(RTKSettings::settingsGroup, "connectionType").toInt(), connection);
}

void AutoConnectSettingsTest::_nmeaPortLabelBecomesPositionOnlyReceiver_data()
{
    QTest::addColumn<QString>("port");
    QTest::addColumn<int>("connection");
    QTest::newRow("serial-device") << QStringLiteral("/dev/ttyNMEA") << 0;
    QTest::newRow("udp-label") << QStringLiteral("UDP Port") << 2;
    QTest::newRow("disabled-label") << QStringLiteral("Disabled") << -1;
    QTest::newRow("no-serial-label") << QStringLiteral("Serial <none available>") << -1;
}

void AutoConnectSettingsTest::_nmeaPortLabelBecomesPositionOnlyReceiver()
{
    QFETCH(QString, port);
    QFETCH(int, connection);
    // Settings saved before 5.1 have no nmeaSource; the port held the selected label or a serial device.
    writeGroup(QLatin1String(AutoConnectSettings::settingsGroup), {
                                                                      {QStringLiteral("autoConnectNmeaPort"), port},
                                                                      {QStringLiteral("autoConnectNmeaBaud"), 9600},
                                                                      {QStringLiteral("nmeaUdpPort"), 14555},
                                                                  });
    const RTKSettings rtk;
    QVERIFY(readGroup(QLatin1String(AutoConnectSettings::settingsGroup)).isEmpty());
    if (connection < 0) {
        QVERIFY(readGroup(QLatin1String(RTKSettings::settingsGroup)).isEmpty());
        return;
    }
    QCOMPARE(stored(RTKSettings::settingsGroup, "receiverRole").toInt(), 0);
    QCOMPARE(stored(RTKSettings::settingsGroup, "connectOnStartup").toBool(), true);
    QCOMPARE(stored(RTKSettings::settingsGroup, "connectionType").toInt(), connection);
    if (connection == 0) {
        QCOMPARE(stored(RTKSettings::settingsGroup, "serialDevice").toString(), port);
        QCOMPARE(stored(RTKSettings::settingsGroup, "serialBaudRate").toInt(), 9600);
    } else {
        QCOMPARE(stored(RTKSettings::settingsGroup, "udpPort").toInt(), 14555);
    }
}

void AutoConnectSettingsTest::_configuredReceiverKeepsSettings()
{
    writeGroup(QLatin1String(RTKSettings::settingsGroup),
               {{QStringLiteral("serialDevice"), QStringLiteral("/dev/ttyBase")},
                {QStringLiteral("baseReceiverManufacturers"), 4}});
    writeGroup(
        QLatin1String(AutoConnectSettings::settingsGroup),
        {{QStringLiteral("nmeaSource"), 2}, {QStringLiteral("autoConnectNmeaPort"), QStringLiteral("/dev/ttyNMEA")}});
    const RTKSettings rtk;
    QCOMPARE(stored(RTKSettings::settingsGroup, "serialDevice").toString(), QStringLiteral("/dev/ttyBase"));
    QCOMPARE(stored(RTKSettings::settingsGroup, "baseReceiverManufacturers").toInt(), 4);
    QVERIFY(!stored(RTKSettings::settingsGroup, "receiverRole").isValid());
    QVERIFY(readGroup(QLatin1String(AutoConnectSettings::settingsGroup)).isEmpty());
}

void AutoConnectSettingsTest::_passiveManufacturerBecomesRole()
{
    writeGroup(QLatin1String(RTKSettings::settingsGroup), {{QStringLiteral("baseReceiverManufacturers"), 7}});
    const RTKSettings rtk;
    QCOMPARE(stored(RTKSettings::settingsGroup, "receiverRole").toInt(), 1);
    QVERIFY(!stored(RTKSettings::settingsGroup, "baseReceiverManufacturers").isValid());
}

UT_REGISTER_TEST(AutoConnectSettingsTest, TestLabel::Unit)
