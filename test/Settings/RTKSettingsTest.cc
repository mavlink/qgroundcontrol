#include "RTKSettingsTest.h"

#include <limits>

#include <QtCore/QHash>
#include <QtCore/QRegularExpression>
#include <QtCore/QSettings>
#include <QtTest/QSignalSpy>

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

QStringList siteNames(const RTKSettings& settings)
{
    QStringList names;
    for (const RTKBaseSite& site : settings.baseSites()) {
        names.append(site.name);
    }
    return names;
}

/// A saved base site entry as the settings store it.
QString siteJson(const QString& name, const QString& latitude = QStringLiteral("47.5"))
{
    return QStringLiteral(R"({"name":"%1","latitude":%2,"longitude":8.25,"altitude":512.5,"accuracy":0.75})")
        .arg(name, latitude);
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

void RTKSettingsTest::_baseSitesLoad_data()
{
    QTest::addColumn<QString>("saved");
    QTest::addColumn<QStringList>("names");
    QTest::addColumn<QString>("warning");
    const QString north = QStringLiteral("North field");
    const QString south = QStringLiteral("South field");
    const QString notArray = QStringLiteral("not a JSON array");
    QTest::newRow("none") << QString() << QStringList() << QString();
    QTest::newRow("sites") << QStringLiteral("[%1,%2]").arg(siteJson(north), siteJson(south))
                           << QStringList{north, south} << QString();
    QTest::newRow("not-json") << QStringLiteral("[{\"name\":") << QStringList() << notArray;
    QTest::newRow("not-an-array") << siteJson(north) << QStringList() << notArray;

    // Only whole sites are kept, the first of each name, without the spaces around it.
    const QStringList entries{
        siteJson(QStringLiteral("  North field ")),
        QStringLiteral("5"),
        QStringLiteral("{}"),
        siteJson(QStringLiteral(" ")),
        siteJson(QString(RTKSettings::MaxBaseSiteNameLength + 1, QLatin1Char('n'))),
        siteJson(QStringLiteral("Off the globe"), QStringLiteral("90.5")),
        siteJson(QStringLiteral("Not a number"), QStringLiteral("\"47.5\"")),
        QStringLiteral(R"({"name":"No accuracy","latitude":47.5,"longitude":8.25,"altitude":512.5})"),
        siteJson(north, QStringLiteral("-12")),
        siteJson(south),
    };
    QTest::newRow("invalid-and-repeated") << QStringLiteral("[%1]").arg(entries.join(QLatin1Char(',')))
                                          << QStringList{north, south} << QStringLiteral("Dropped 8 of 10 ");

    QStringList many;
    for (int i = 0; i < RTKSettings::MaxBaseSites + 10; ++i) {
        many.append(QStringLiteral("Site %1").arg(i));
    }
    QStringList manyEntries;
    for (const QString& name : many) {
        manyEntries.append(siteJson(name));
    }
    QTest::newRow("too-many") << QStringLiteral("[%1]").arg(manyEntries.join(QLatin1Char(',')))
                              << many.first(RTKSettings::MaxBaseSites)
                              << QStringLiteral("Dropped 10 of %1 ").arg(many.size());
}

void RTKSettingsTest::_baseSitesLoad()
{
    QFETCH(QString, saved);
    QFETCH(QStringList, names);
    QFETCH(QString, warning);
    RTKSettings rtk;
    QVERIFY(rtk.baseSites().isEmpty());
    // The constructor loads the saved value, which a SettingsFact ignores under unit tests, as a change of it does.
    if (!warning.isEmpty()) {
        expectLogMessage("GPS.Settings.RTKSettings", QtWarningMsg, QRegularExpression(warning));
    }
    rtk.savedBaseSites()->setRawValue(saved);
    if (!warning.isEmpty()) {
        verifyExpectedLogMessage();
    }
    QCOMPARE(siteNames(rtk), names);
    if (!names.isEmpty()) {
        const RTKBaseSite first{
            .name = names.first(), .latitude = 47.5, .longitude = 8.25, .altitude = 512.5, .accuracy = 0.75};
        QCOMPARE(rtk.baseSites().first(), first);
    }
}

void RTKSettingsTest::_baseSitesSaveAndRemove()
{
    writeGroup(QLatin1String(RTKSettings::settingsGroup), {});
    RTKSettings rtk;
    QSignalSpy changed(&rtk, &RTKSettings::baseSitesChanged);
    const RTKBaseSite north{.name = QStringLiteral("North field"),
                            .latitude = 47.123456789,
                            .longitude = 8.987654321,
                            .altitude = 512.25,
                            .accuracy = 0.75};
    RTKBaseSite south{.name = QStringLiteral(" South field "), .latitude = -33.5, .longitude = -70.25};
    QVERIFY(rtk.saveBaseSite(north));
    QVERIFY(rtk.saveBaseSite(south));
    south.name = QStringLiteral("South field");
    QCOMPARE(rtk.baseSites(), (QList<RTKBaseSite>{north, south}));
    QCOMPARE(changed.size(), 2);

    // The sites are one stored value, which a later start loads unchanged.
    RTKSettings restarted;
    restarted.savedBaseSites()->setRawValue(stored(RTKSettings::settingsGroup, "savedBaseSites"));
    QCOMPARE(restarted.baseSites(), rtk.baseSites());

    // A saved name keeps its place and takes the new position.
    RTKBaseSite moved = north;
    moved.altitude = 498.5;
    QVERIFY(rtk.saveBaseSite(moved));
    QCOMPARE(rtk.baseSites(), (QList<RTKBaseSite>{moved, south}));

    const auto invalid = [&north](auto change) {
        RTKBaseSite site = north;
        change(site);
        return site;
    };
    for (const RTKBaseSite& site :
         {invalid([](RTKBaseSite& site) { site.name = QStringLiteral("  "); }),
          invalid([](RTKBaseSite& site) { site.name = QString(RTKSettings::MaxBaseSiteNameLength + 1, u'n'); }),
          invalid([](RTKBaseSite& site) { site.latitude = -90.5; }),
          invalid([](RTKBaseSite& site) { site.longitude = 180.5; }),
          invalid([](RTKBaseSite& site) { site.altitude = std::numeric_limits<double>::quiet_NaN(); }),
          invalid([](RTKBaseSite& site) { site.accuracy = std::numeric_limits<double>::infinity(); })}) {
        QVERIFY(!rtk.saveBaseSite(site));
    }
    QCOMPARE(rtk.baseSites(), (QList<RTKBaseSite>{moved, south}));

    // A full list refuses another name, and still replaces a saved one.
    for (qsizetype i = rtk.baseSites().size(); i < RTKSettings::MaxBaseSites; ++i) {
        RTKBaseSite site = north;
        site.name = QStringLiteral("Site %1").arg(i);
        QVERIFY(rtk.saveBaseSite(site));
    }
    RTKBaseSite other = north;
    other.name = QStringLiteral("One too many");
    QVERIFY(!rtk.saveBaseSite(other));
    QVERIFY(rtk.saveBaseSite(north));
    QCOMPARE(rtk.baseSites().size(), RTKSettings::MaxBaseSites);
    QCOMPARE(rtk.baseSites().first(), north);

    changed.clear();
    QVERIFY(rtk.removeBaseSite(north.name));
    QVERIFY(!rtk.removeBaseSite(north.name));
    QCOMPARE(changed.size(), 1);
    QCOMPARE(rtk.baseSites().first(), south);
    QVERIFY(rtk.saveBaseSite(other));
}

void RTKSettingsTest::_baseSiteBecomesFixedPosition()
{
    RTKSettings rtk;
    const auto setFixedPosition = [&rtk](double latitude, double longitude, double altitude, double accuracy) {
        rtk.fixedBasePositionLatitude()->setRawValue(latitude);
        rtk.fixedBasePositionLongitude()->setRawValue(longitude);
        rtk.fixedBasePositionAltitude()->setRawValue(altitude);
        rtk.fixedBasePositionAccuracy()->setRawValue(accuracy);
    };
    const QString name = QStringLiteral("North field");
    setFixedPosition(47.123456789, 8.987654321, 512.25, 0.75);
    QVERIFY(rtk.saveFixedBasePositionAsSite(name));
    const RTKBaseSite north{
        .name = name, .latitude = 47.123456789, .longitude = 8.987654321, .altitude = 512.25, .accuracy = 0.75};
    QCOMPARE(rtk.baseSites(), QList<RTKBaseSite>{north});

    // The fixed position stays the one the receiver is configured from; a site only replaces it when used.
    setFixedPosition(-33.5, -70.25, 20., 2.);
    QVERIFY(!rtk.useBaseSite(QStringLiteral("South field")));
    QCOMPARE(rtk.fixedBasePositionLatitude()->rawValue().toDouble(), -33.5);
    QVERIFY(rtk.useBaseSite(name));
    QCOMPARE(rtk.fixedBasePositionLatitude()->rawValue().toDouble(), north.latitude);
    QCOMPARE(rtk.fixedBasePositionLongitude()->rawValue().toDouble(), north.longitude);
    QCOMPARE(rtk.fixedBasePositionAltitude()->rawValue().toDouble(), north.altitude);
    QCOMPARE(rtk.fixedBasePositionAccuracy()->rawValue().toDouble(), north.accuracy);
}

UT_REGISTER_TEST(RTKSettingsTest, TestLabel::Unit)
