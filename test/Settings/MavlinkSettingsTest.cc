#include "MavlinkSettingsTest.h"

#include <QtCore/QSettings>
#include <QtCore/QVariant>
#include <QtTest/QTest>

#include "MavlinkSettings.h"

namespace {
constexpr const char* kDeprecatedNoInitialDownloadKey = "noInitialDownloadWhenFlying";
}

void MavlinkSettingsTest::_noInitialDownloadWhenFlyingMigration_data()
{
    QTest::addColumn<QVariant>("storedArmedValue");
    QTest::addColumn<bool>("expectedArmedValue");

    QTest::addRow("deprecated key migrates") << QVariant() << true;
    QTest::addRow("existing value wins") << QVariant(false) << false;
}

void MavlinkSettingsTest::_noInitialDownloadWhenFlyingMigration()
{
    QFETCH(QVariant, storedArmedValue);
    QFETCH(bool, expectedArmedValue);

    QSettings settings;
    settings.beginGroup(MavlinkSettings::settingsGroup);
    settings.setValue(kDeprecatedNoInitialDownloadKey, true);
    if (storedArmedValue.isValid()) {
        settings.setValue(MavlinkSettings::noInitialDownloadWhenArmedName, storedArmedValue);
    }
    settings.endGroup();

    // Migration runs in the constructor. SettingsFacts ignore QSettings under unit tests,
    // so assert against the raw stored values rather than the facts.
    const MavlinkSettings mavlinkSettings;
    settings.beginGroup(MavlinkSettings::settingsGroup);
    QVERIFY(settings.contains(MavlinkSettings::noInitialDownloadWhenArmedName));
    QCOMPARE(settings.value(MavlinkSettings::noInitialDownloadWhenArmedName).toBool(), expectedArmedValue);
    if (!storedArmedValue.isValid()) {
        QVERIFY(!settings.contains(kDeprecatedNoInitialDownloadKey));
    }
    settings.endGroup();
}

UT_REGISTER_TEST(MavlinkSettingsTest, TestLabel::Unit)
