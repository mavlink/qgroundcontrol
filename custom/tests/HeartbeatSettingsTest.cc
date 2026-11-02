#include <iterator>

#include <QtCore/QCoreApplication>
#include <QtCore/QMetaEnum>
#include <QtCore/QSettings>
#include <QtCore/QTemporaryDir>
#include <QtTest/QTest>

#include "CustomPlugin.h"
#include "MavlinkSettings.h"
#include "SettingsManager.h"
#include "UnitsSettings.h"

namespace {
struct UnitPolicy {
    const char *name;
    Fact *(UnitsSettings::*getter)();
    int metric;
    int imperial;
};

const UnitPolicy UNIT_POLICIES[] = {
    {UnitsSettings::horizontalDistanceUnitsName, &UnitsSettings::horizontalDistanceUnits,
     UnitsSettings::HorizontalDistanceUnitsMeters, UnitsSettings::HorizontalDistanceUnitsFeet},
    {UnitsSettings::verticalDistanceUnitsName, &UnitsSettings::verticalDistanceUnits,
     UnitsSettings::VerticalDistanceUnitsMeters, UnitsSettings::VerticalDistanceUnitsFeet},
    {UnitsSettings::areaUnitsName, &UnitsSettings::areaUnits,
     UnitsSettings::AreaUnitsSquareMeters, UnitsSettings::AreaUnitsAcres},
    {UnitsSettings::speedUnitsName, &UnitsSettings::speedUnits,
     UnitsSettings::SpeedUnitsMetersPerSecond, UnitsSettings::SpeedUnitsMilesPerHour},
    {UnitsSettings::temperatureUnitsName, &UnitsSettings::temperatureUnits,
     UnitsSettings::TemperatureUnitsCelsius, UnitsSettings::TemperatureUnitsFarenheit},
    {UnitsSettings::weightUnitsName, &UnitsSettings::weightUnits,
     UnitsSettings::WeightUnitsKg, UnitsSettings::WeightUnitsLbs},
};
}

class HeartbeatSettingsTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase()
    {
        QVERIFY(_settingsDirectory.isValid());
        QCoreApplication::setOrganizationName(QStringLiteral("CODRONE-Heartbeat-Test"));
        QCoreApplication::setApplicationName(QStringLiteral("HeartbeatSettingsTest"));
        QSettings::setDefaultFormat(QSettings::IniFormat);
        QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, _settingsDirectory.path());
    }

    void init() { QSettings().clear(); }

    void cleanup()
    {
        auto *manager = SettingsManager::instance();
        manager->unit = nullptr;
        manager->app = nullptr;
        manager->mavlink = nullptr;
    }

    void metricStartupIgnoresSavedImperial_data()
    {
        QTest::addColumn<int>("unitIndex");
        QTest::addColumn<bool>("savedImperial");
        for (int i = 0; i < int(std::size(UNIT_POLICIES)); ++i) {
            QTest::newRow(qPrintable(QStringLiteral("%1-clean").arg(UNIT_POLICIES[i].name))) << i << false;
            QTest::newRow(qPrintable(QStringLiteral("%1-imperial").arg(UNIT_POLICIES[i].name))) << i << true;
        }
    }

    void metricStartupIgnoresSavedImperial()
    {
        QFETCH(int, unitIndex);
        QFETCH(bool, savedImperial);
        const auto &policy = UNIT_POLICIES[unitIndex];
        const QString key = QStringLiteral("%1/%2").arg(UnitsSettings::settingsGroup, policy.name);
        if (savedImperial) {
            QSettings().setValue(key, policy.imperial);
        }
        UnitsSettings units;
        Fact *fact = (units.*policy.getter)();
        QCOMPARE(fact->rawValue().toInt(), policy.metric);
        QCOMPARE(fact->rawDefaultValue().toInt(), policy.metric);
        QVERIFY(!fact->property("userVisible").toBool());
        QVERIFY(fact->readOnly());
    }

    void metricMetadataIsHiddenAndReadOnly_data()
    {
        QTest::addColumn<int>("unitIndex");
        for (int i = 0; i < int(std::size(UNIT_POLICIES)); ++i) {
            QTest::newRow(UNIT_POLICIES[i].name) << i;
        }
    }

    void metricMetadataIsHiddenAndReadOnly()
    {
        QFETCH(int, unitIndex);
        const auto &policy = UNIT_POLICIES[unitIndex];
        FactMetaData metadata(FactMetaData::valueTypeUint32);
        metadata.setName(QString::fromLatin1(policy.name));
        metadata.setRawDefaultValue(policy.imperial);
        bool visible = true;
        CustomPlugin plugin;
        plugin.adjustSettingMetaData(UnitsSettings::settingsGroup, metadata, visible);
        QVERIFY(!visible);
        QVERIFY(metadata.readOnly());
        QCOMPARE(metadata.rawDefaultValue().toInt(), policy.metric);
    }

    void metricEnumMetadataIsPreserved()
    {
        UnitsSettings units;
        for (const auto &policy : UNIT_POLICIES) {
            Fact *fact = (units.*policy.getter)();
            QByteArray enumName(policy.name);
            enumName[0] = enumName[0] - 'a' + 'A';
            const int index = UnitsSettings::staticMetaObject.indexOfEnumerator(enumName.constData());
            QVERIFY(index >= 0);
            const QMetaEnum enumeration = UnitsSettings::staticMetaObject.enumerator(index);
            QCOMPARE(fact->enumValues().size(), enumeration.keyCount());
            QCOMPARE(fact->enumStrings().size(), enumeration.keyCount());
            for (int i = 0; i < enumeration.keyCount(); ++i) {
                QCOMPARE(fact->enumValues()[i].toInt(), enumeration.value(i));
                QVERIFY(!fact->enumStrings()[i].isEmpty());
            }
        }
    }

    void metricRegistrationPersistsPolicy_data() { metricStartupIgnoresSavedImperial_data(); }

    void metricRegistrationPersistsPolicy()
    {
        QFETCH(int, unitIndex);
        QFETCH(bool, savedImperial);
        const auto &policy = UNIT_POLICIES[unitIndex];
        const QString key = QStringLiteral("%1/%2").arg(UnitsSettings::settingsGroup, policy.name);
        if (savedImperial) {
            QSettings().setValue(key, policy.imperial);
        }
        UnitsSettings units;
        // Materialize first: hidden SettingsFact construction deliberately does not persist defaults.
        Fact *fact = (units.*policy.getter)();
        AppSettings app;
        MavlinkSettings mavlink;
        _register(&units, &app, &mavlink);
        QCOMPARE(fact->rawValue().toInt(), policy.metric);
        QSettings saved;
        saved.sync();
        QVERIFY(saved.contains(key));
        QCOMPARE(saved.value(key).toInt(), policy.metric);
        UnitsSettings reopened;
        QCOMPARE((reopened.*policy.getter)()->rawValue().toInt(), policy.metric);
    }

    void metricRuntimeWritesAreReverted_data()
    {
        QTest::addColumn<int>("unitIndex");
        QTest::addColumn<int>("alternate");
        QTest::addColumn<int>("writePath");
        UnitsSettings units;
        const char *paths[] = {"raw", "cooked", "raw-property", "value-property", "force", "container"};
        for (int i = 0; i < int(std::size(UNIT_POLICIES)); ++i) {
            const auto &policy = UNIT_POLICIES[i];
            for (const auto &value : (units.*policy.getter)()->enumValues()) {
                if (value.toInt() == policy.metric) {
                    continue;
                }
                for (int path = 0; path < int(std::size(paths)); ++path) {
                    const QString row = QStringLiteral("%1-%2-%3").arg(policy.name).arg(value.toInt()).arg(paths[path]);
                    QTest::newRow(qPrintable(row)) << i << value.toInt() << path;
                }
            }
        }
    }

    void metricRuntimeWritesAreReverted()
    {
        QFETCH(int, unitIndex);
        QFETCH(int, alternate);
        QFETCH(int, writePath);
        const auto &policy = UNIT_POLICIES[unitIndex];
        UnitsSettings units;
        Fact *fact = (units.*policy.getter)();
        // Separate runtime enforcement from startup metadata: start at the required value even in RED.
        fact->forceSetRawValue(policy.metric);
        AppSettings app;
        MavlinkSettings mavlink;
        _register(&units, &app, &mavlink);
        switch (writePath) {
        case 0: fact->setRawValue(alternate); break;
        case 1: fact->setCookedValue(alternate); break;
        case 2: QVERIFY(fact->setProperty("rawValue", alternate)); break;
        case 3: QVERIFY(fact->setProperty("value", alternate)); break;
        case 4: fact->forceSetRawValue(alternate); break;
        case 5: fact->containerSetRawValue(alternate); break;
        }
        QCOMPARE(fact->rawValue().toInt(), policy.metric);
        QCOMPARE(fact->cookedValue().toInt(), policy.metric);
        QSettings saved;
        saved.sync();
        const QString key = QStringLiteral("%1/%2").arg(UnitsSettings::settingsGroup, policy.name);
        QVERIFY(saved.contains(key));
        QCOMPARE(saved.value(key).toInt(), policy.metric);
        QVERIFY(!mavlink.sendGCSHeartbeat()->rawValue().toBool());
        QVERIFY(app.virtualJoystick()->rawValue().toBool());
    }

    void metricMetadataOverrideIsScoped()
    {
        CustomPlugin plugin;
        for (const auto &policy : UNIT_POLICIES) {
            FactMetaData metadata(FactMetaData::valueTypeUint32);
            metadata.setName(QString::fromLatin1(policy.name));
            metadata.setRawDefaultValue(policy.imperial);
            bool visible = true;
            plugin.adjustSettingMetaData(QStringLiteral("OtherGroup"), metadata, visible);
            QVERIFY(visible);
            QVERIFY(!metadata.readOnly());
            QCOMPARE(metadata.rawDefaultValue().toInt(), policy.imperial);
        }
        UnitsSettings units;
        Fact *coordinates = units.coordinateFormat();
        QVERIFY(coordinates->property("userVisible").toBool());
        QVERIFY(!coordinates->readOnly());
        coordinates->setRawValue(UnitsSettings::CoordinateFormatDMS);
        QCOMPARE(coordinates->rawValue().toInt(), int(UnitsSettings::CoordinateFormatDMS));
    }

    void startupIgnoresEnabledSettings_data()
    {
        QTest::addColumn<QString>("savedKey");
        QTest::newRow("clean-install") << QString();
        QTest::newRow("persisted-true") << QStringLiteral("sendGCSHeartbeat");
        QTest::newRow("deprecated-true") << QStringLiteral("gcsHeartbeatEnabled");
    }

    void startupIgnoresEnabledSettings()
    {
        QFETCH(QString, savedKey);
        if (!savedKey.isEmpty()) {
            QSettings().setValue(savedKey, true);
        }
        MavlinkSettings settings;
        Fact *heartbeat = settings.sendGCSHeartbeat();
        // This is the same raw-value gate used before every GCS heartbeat send.
        QVERIFY2(!heartbeat->rawValue().toBool(), "Heartbeat is enabled before its first use");
        QVERIFY(!heartbeat->property("userVisible").toBool());
        QVERIFY(heartbeat->readOnly());
        QVERIFY(!heartbeat->rawDefaultValue().toBool());

        AppSettings app;
        auto *manager = SettingsManager::instance();
        manager->app = &app;
        manager->mavlink = &settings;
        CustomPlugin plugin;
        plugin.registerCustomSettings(manager);
        QVERIFY(QSettings().contains(QStringLiteral("sendGCSHeartbeat")));
        QVERIFY(!QSettings().value(QStringLiteral("sendGCSHeartbeat")).toBool());
        manager->mavlink = nullptr;
    }

    void runtimeCannotEnableHeartbeat_data()
    {
        QTest::addColumn<int>("writePath");
        QTest::newRow("raw-setter") << 0;
        QTest::newRow("cooked-setter") << 1;
        QTest::newRow("qml-raw-property") << 2;
        QTest::newRow("qml-value-property") << 3;
        QTest::newRow("force-setter") << 4;
        QTest::newRow("container-setter") << 5;
    }

    void runtimeCannotEnableHeartbeat()
    {
        QFETCH(int, writePath);
        MavlinkSettings settings;
        AppSettings app;
        auto *manager = SettingsManager::instance();
        manager->app = &app;
        manager->mavlink = &settings;
        CustomPlugin plugin;
        plugin.registerCustomSettings(manager);
        Fact *heartbeat = settings.sendGCSHeartbeat();
        switch (writePath) {
        case 0: heartbeat->setRawValue(true); break;
        case 1: heartbeat->setCookedValue(true); break;
        case 2: QVERIFY(heartbeat->setProperty("rawValue", true)); break;
        case 3: QVERIFY(heartbeat->setProperty("value", true)); break;
        case 4: heartbeat->forceSetRawValue(true); break;
        case 5: heartbeat->containerSetRawValue(true); break;
        }
        QVERIFY2(!heartbeat->rawValue().toBool(), "Runtime setting write re-enabled GCS heartbeat");
        QVERIFY(!QSettings().value(QStringLiteral("sendGCSHeartbeat"), false).toBool());
        manager->mavlink = nullptr;
    }

    void virtualJoystickIgnoresSavedOff_data()
    {
        QTest::addColumn<bool>("savedOff");
        QTest::newRow("clean-install") << false;
        QTest::newRow("persisted-off") << true;
    }

    void virtualJoystickIgnoresSavedOff()
    {
        QFETCH(bool, savedOff);
        if (savedOff) {
            QSettings().setValue(QStringLiteral("virtualJoystick"), false);
        }
        AppSettings app;
        Fact *joystick = app.virtualJoystick();
        QVERIFY2(joystick->rawValue().toBool(), "Virtual joysticks must start enabled even with saved off");
        QVERIFY(joystick->readOnly());
        QVERIFY(!joystick->property("userVisible").toBool());
        MavlinkSettings mavlink;
        auto *manager = SettingsManager::instance();
        manager->app = &app;
        manager->mavlink = &mavlink;
        CustomPlugin plugin;
        plugin.registerCustomSettings(manager);
        QVERIFY(QSettings().value(QStringLiteral("virtualJoystick"), false).toBool());
        manager->app = nullptr;
        manager->mavlink = nullptr;
    }

    void virtualJoystickCannotBeDisabled_data() { runtimeCannotEnableHeartbeat_data(); }

    void virtualJoystickCannotBeDisabled()
    {
        QFETCH(int, writePath);
        AppSettings app;
        MavlinkSettings mavlink;
        auto *manager = SettingsManager::instance();
        manager->app = &app;
        manager->mavlink = &mavlink;
        CustomPlugin plugin;
        plugin.registerCustomSettings(manager);
        Fact *joystick = app.virtualJoystick();
        switch (writePath) {
        case 0: joystick->setRawValue(false); break;
        case 1: joystick->setCookedValue(false); break;
        case 2: QVERIFY(joystick->setProperty("rawValue", false)); break;
        case 3: QVERIFY(joystick->setProperty("value", false)); break;
        case 4: joystick->forceSetRawValue(false); break;
        case 5: joystick->containerSetRawValue(false); break;
        }
        QVERIFY2(joystick->rawValue().toBool(), "Runtime setting write disabled virtual joysticks");
        QVERIFY(QSettings().value(QStringLiteral("virtualJoystick"), false).toBool());
        QVERIFY(!mavlink.sendGCSHeartbeat()->rawValue().toBool());
        manager->app = nullptr;
        manager->mavlink = nullptr;
    }

    void metadataOverrideIsScoped_data()
    {
        QTest::addColumn<QString>("group");
        QTest::addColumn<QString>("name");
        QTest::newRow("different-group") << QStringLiteral("OtherGroup") << QStringLiteral("sendGCSHeartbeat");
        QTest::newRow("different-setting") << QString(MavlinkSettings::settingsGroup) << QStringLiteral("otherSetting");
    }

    void metadataOverrideIsScoped()
    {
        QFETCH(QString, group);
        QFETCH(QString, name);
        FactMetaData metadata(FactMetaData::valueTypeBool);
        metadata.setName(name);
        metadata.setRawDefaultValue(true);
        bool visible = true;
        CustomPlugin plugin;
        plugin.adjustSettingMetaData(group, metadata, visible);
        QVERIFY(visible);
        QVERIFY(metadata.rawDefaultValue().toBool());
        QVERIFY(!metadata.readOnly());
    }

private:
    void _register(UnitsSettings *units, AppSettings *app, MavlinkSettings *mavlink)
    {
        auto *manager = SettingsManager::instance();
        manager->unit = units;
        manager->app = app;
        manager->mavlink = mavlink;
        CustomPlugin plugin;
        plugin.registerCustomSettings(manager);
    }

    QTemporaryDir _settingsDirectory;
};

QTEST_GUILESS_MAIN(HeartbeatSettingsTest)
#include "HeartbeatSettingsTest.moc"
