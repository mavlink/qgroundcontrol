#include "RTKSettingsTest.h"

#include <QtCore/QHash>
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

// Migrations run in the constructors. SettingsFacts ignore QSettings under unit tests,
// so these assertions read the raw stored values rather than the facts.

void RTKSettingsTest::_autoConnectMigration()
{
    writeGroup(QLatin1String(AutoConnectSettings::settingsGroup), {{QStringLiteral("autoConnectRTKGPS"), false}});
    const RTKSettings rtk;
    QVERIFY(stored(RTKSettings::settingsGroup, "autoConnect").isValid());
    QVERIFY(!stored(RTKSettings::settingsGroup, "autoConnect").toBool());
    QVERIFY(readGroup(QLatin1String(AutoConnectSettings::settingsGroup)).isEmpty());

    // A value already in the receiver settings is kept.
    writeGroup(QLatin1String(AutoConnectSettings::settingsGroup), {{QStringLiteral("autoConnectRTKGPS"), false}});
    writeGroup(QLatin1String(RTKSettings::settingsGroup), {{QStringLiteral("autoConnect"), true}});
    const RTKSettings kept;
    QVERIFY(stored(RTKSettings::settingsGroup, "autoConnect").toBool());
    QVERIFY(readGroup(QLatin1String(AutoConnectSettings::settingsGroup)).isEmpty());
}

void RTKSettingsTest::_nmeaInputBecomesPassiveReceiver_data()
{
    QTest::addColumn<QVariant>("source");
    QTest::addColumn<QString>("port");
    QTest::addColumn<QVariant>("rtkAutoConnect");
    QTest::addColumn<int>("connection");
    const QString device = QStringLiteral("/dev/ttyNMEA");
    const QVariant off(false);
    QTest::newRow("source-udp") << QVariant(1) << device << off << 2;
    QTest::newRow("source-serial") << QVariant(2) << device << off << 0;
    // Settings saved before 5.1 have no nmeaSource; the port held the selected label or a serial device.
    QTest::newRow("label-serial-device") << QVariant() << device << off << 0;
    QTest::newRow("label-udp") << QVariant() << QStringLiteral("UDP Port") << off << 2;
    QTest::newRow("label-disabled") << QVariant() << QStringLiteral("Disabled") << off << -1;
    QTest::newRow("label-no-serial") << QVariant() << QStringLiteral("Serial <none available>") << off << -1;
    // Serial chosen without ever picking a device configured no input.
    QTest::newRow("serial-without-device") << QVariant(2) << QString() << off << -1;
    // RTK GPS auto-connect left at its default is not a choice, so the input wins; turned on, the base stays.
    QTest::newRow("rtk-auto-connect-default") << QVariant(2) << device << QVariant() << 0;
    QTest::newRow("rtk-auto-connect-on") << QVariant(1) << device << QVariant(true) << -1;
}

void RTKSettingsTest::_nmeaInputBecomesPassiveReceiver()
{
    QFETCH(QVariant, source);
    QFETCH(QString, port);
    QFETCH(QVariant, rtkAutoConnect);
    QFETCH(int, connection);
    QHash<QString, QVariant> legacy{
        {QStringLiteral("autoConnectNmeaBaud"), 9600},
        {QStringLiteral("nmeaUdpPort"), 14555},
    };
    if (!port.isEmpty()) {
        legacy.insert(QStringLiteral("autoConnectNmeaPort"), port);
    }
    if (source.isValid()) {
        legacy.insert(QStringLiteral("nmeaSource"), source);
    }
    if (rtkAutoConnect.isValid()) {
        legacy.insert(QStringLiteral("autoConnectRTKGPS"), rtkAutoConnect);
    }
    writeGroup(QLatin1String(AutoConnectSettings::settingsGroup), legacy);
    writeGroup(QLatin1String(RTKSettings::settingsGroup), {});
    const RTKSettings rtk;
    QVERIFY(readGroup(QLatin1String(AutoConnectSettings::settingsGroup)).isEmpty());
    if (connection < 0) {
        // The RTK auto-connect choice is kept, and no receiver input or role is stored.
        QCOMPARE(stored(RTKSettings::settingsGroup, "autoConnect").isValid(), rtkAutoConnect.isValid());
        for (const char* key : {"receiverRole", "connectionType", "serialDevice", "udpPort", "forwardReceiverRtcm"}) {
            QVERIFY2(!stored(RTKSettings::settingsGroup, key).isValid(), key);
        }
        return;
    }
    QCOMPARE(stored(RTKSettings::settingsGroup, "receiverRole").toInt(), int(RTKSettings::Passive));
    QCOMPARE(stored(RTKSettings::settingsGroup, "forwardReceiverRtcm").toBool(), false);
    // The NMEA input always connected at startup, so it connects automatically.
    QCOMPARE(stored(RTKSettings::settingsGroup, "autoConnect").toBool(), true);
    QCOMPARE(stored(RTKSettings::settingsGroup, "connectionType").toInt(), connection);
    if (connection == RTKSettings::Serial) {
        QCOMPARE(stored(RTKSettings::settingsGroup, "serialDevice").toString(), port);
        QCOMPARE(stored(RTKSettings::settingsGroup, "serialBaudRate").toInt(), 9600);
    } else {
        QCOMPARE(stored(RTKSettings::settingsGroup, "udpPort").toInt(), 14555);
    }

    // The legacy input is migrated once.
    const RTKSettings again;
    QCOMPARE(stored(RTKSettings::settingsGroup, "connectionType").toInt(), connection);
}

void RTKSettingsTest::_configuredReceiverKeepsSettings()
{
    writeGroup(QLatin1String(RTKSettings::settingsGroup),
               {{QStringLiteral("serialDevice"), QStringLiteral("/dev/ttyBase")}});
    writeGroup(QLatin1String(AutoConnectSettings::settingsGroup),
               {{QStringLiteral("autoConnectRTKGPS"), false},
                {QStringLiteral("nmeaSource"), 2},
                {QStringLiteral("autoConnectNmeaPort"), QStringLiteral("/dev/ttyNMEA")}});
    const RTKSettings rtk;
    QCOMPARE(stored(RTKSettings::settingsGroup, "serialDevice").toString(), QStringLiteral("/dev/ttyBase"));
    QVERIFY(!stored(RTKSettings::settingsGroup, "receiverRole").isValid());
    QVERIFY(readGroup(QLatin1String(AutoConnectSettings::settingsGroup)).isEmpty());
}

void RTKSettingsTest::_manufacturerMigration_data()
{
    QTest::addColumn<QVariant>("role");
    QTest::addColumn<QVariant>("manufacturer");
    QTest::addColumn<QVariant>("migratedRole");
    QTest::addColumn<int>("migratedManufacturer");
    // Without a stored role the default applies, including one a custom build sets.
    QTest::newRow("fresh") << QVariant() << QVariant() << QVariant() << 0;
    // A profile without a role predates roles: its manufacturer was a settings-view filter, never the receiver family.
    QTest::newRow("legacy-view-filter") << QVariant() << QVariant(1) << QVariant() << 0;
    QTest::newRow("legacy-passive") << QVariant() << QVariant(7) << QVariant(1) << 0;
    QTest::newRow("modern") << QVariant(2) << QVariant(6) << QVariant(2) << 6;
}

void RTKSettingsTest::_manufacturerMigration()
{
    QFETCH(QVariant, role);
    QFETCH(QVariant, manufacturer);
    QFETCH(QVariant, migratedRole);
    QFETCH(int, migratedManufacturer);
    QHash<QString, QVariant> values;
    if (manufacturer.isValid()) {
        values.insert(QStringLiteral("baseReceiverManufacturers"), manufacturer);
    }
    if (role.isValid()) {
        values.insert(QStringLiteral("receiverRole"), role);
    }
    writeGroup(QLatin1String(RTKSettings::settingsGroup), values);
    RTKSettings rtk;
    QCOMPARE(stored(RTKSettings::settingsGroup, "receiverRole").isValid(), migratedRole.isValid());
    QCOMPARE(stored(RTKSettings::settingsGroup, "receiverRole").toInt(), migratedRole.toInt());
    // As a SettingsFact resolves it outside unit tests: the stored value, else the default.
    const QVariant saved = stored(RTKSettings::settingsGroup, "baseReceiverManufacturers");
    QCOMPARE(saved.isValid() ? saved.toInt() : rtk.baseReceiverManufacturers()->rawDefaultValue().toInt(),
             migratedManufacturer);

    // The migration runs once: a manufacturer chosen afterwards is kept, with or without a stored role.
    QSettings().setValue(QStringLiteral("%1/baseReceiverManufacturers").arg(QLatin1String(RTKSettings::settingsGroup)),
                         5);
    const RTKSettings again;
    QCOMPARE(stored(RTKSettings::settingsGroup, "baseReceiverManufacturers").toInt(), 5);
    QCOMPARE(stored(RTKSettings::settingsGroup, "receiverRole").isValid(), migratedRole.isValid());
    QCOMPARE(stored(RTKSettings::settingsGroup, "receiverRole").toInt(), migratedRole.toInt());
}

UT_REGISTER_TEST(RTKSettingsTest, TestLabel::Unit)
