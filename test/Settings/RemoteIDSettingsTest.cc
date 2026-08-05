#include "RemoteIDSettingsTest.h"

#include <QtCore/QSettings>
#include <QtTest/QSignalSpy>

#include "RemoteIDSettings.h"
#include "SettingsManager.h"

namespace {

// Official EN 4709-002 example operator ID
constexpr const char* kValidFullOperatorID = "FIN87astrdge12k8-xyz";
constexpr const char* kValidPublicOperatorID = "FIN87astrdge12k8";
constexpr const char* kInvalidOperatorID = "DEADBEEFDEADBEEFDEAD";

}  // namespace

void RemoteIDSettingsTest::init()
{
    UnitTest::init();

    RemoteIDSettings* settings = SettingsManager::instance()->remoteIDSettings();
    QVERIFY(settings);

    _savedOperatorIDEU = settings->operatorIDEU()->rawValue();
    _savedOperatorIDFAA = settings->operatorIDFAA()->rawValue();
    _savedOperatorIDType = settings->operatorIDType()->rawValue();
    _savedRegion = settings->region()->rawValue();
    _savedSendOperatorID = settings->sendOperatorID()->rawValue();
    _savedLocationType = settings->locationType()->rawValue();
    _savedBasicID = settings->basicID()->rawValue();
    _savedBasicIDType = settings->basicIDType()->rawValue();
    _savedBasicIDUaType = settings->basicIDUaType()->rawValue();
}

void RemoteIDSettingsTest::cleanup()
{
    RemoteIDSettings* settings = SettingsManager::instance()->remoteIDSettings();
    QVERIFY(settings);

    // Restore region first: its change handler writes sendOperatorID/locationType,
    // which are restored to their saved values afterwards.
    settings->region()->setRawValue(_savedRegion);
    settings->operatorIDType()->setRawValue(_savedOperatorIDType);
    settings->operatorIDEU()->setRawValue(_savedOperatorIDEU);
    settings->operatorIDFAA()->setRawValue(_savedOperatorIDFAA);
    settings->sendOperatorID()->setRawValue(_savedSendOperatorID);
    settings->locationType()->setRawValue(_savedLocationType);
    settings->basicID()->setRawValue(_savedBasicID);
    settings->basicIDType()->setRawValue(_savedBasicIDType);
    settings->basicIDUaType()->setRawValue(_savedBasicIDUaType);

    UnitTest::cleanup();
}

void RemoteIDSettingsTest::_euValidatorValidation_data()
{
    QTest::addColumn<QString>("operatorID");
    QTest::addColumn<bool>("expectedAccepted");

    // The EU operator ID fact rejects writes which fail EN 4709-002 validation
    QTest::newRow("valid dash form")          << "FIN87astrdge12k8-xyz" << true;
    QTest::newRow("valid no-dash form")       << "FIN87astrdge12k8xyz"  << true;
    QTest::newRow("empty always accepted")    << ""                     << true;
    QTest::newRow("invalid checksum")         << "DEADBEEFDEADBEEFDEAD" << false;
    QTest::newRow("dash at wrong position")   << "FIN87astrdge12k8x-yz" << false;
    QTest::newRow("digit country code")       << "12387astrdge12k8-xyz" << false;
    QTest::newRow("unicode upper country")    << "ÖST87astrdge12k8-xyz" << false;
    QTest::newRow("charset outside [0-9a-z]") << "FIN87astrdge12k8-x{z" << false;
    QTest::newRow("too short")                << "FIN87astrdge12k8-x"   << false;
    QTest::newRow("bare public part")         << "FIN87astrdge12k8"     << false;
}

void RemoteIDSettingsTest::_euValidatorValidation()
{
    QFETCH(QString, operatorID);
    QFETCH(bool, expectedAccepted);

    RemoteIDSettings* settings = SettingsManager::instance()->remoteIDSettings();
    settings->operatorIDEU()->setRawValue(QString());

    const QString errorString = settings->operatorIDEU()->validate(operatorID, false /* convertOnly */);
    QCOMPARE(errorString.isEmpty(), expectedAccepted);
}

// Re-committing the stored (already sanitized) value must be accepted even though the
// 16-char public part cannot be checksum-validated on its own
void RemoteIDSettingsTest::_euValidatorAcceptsNoOpRecommit()
{
    RemoteIDSettings* settings = SettingsManager::instance()->remoteIDSettings();

    settings->operatorIDEU()->setRawValue(QString::fromLatin1(kValidFullOperatorID));
    QCOMPARE(settings->operatorIDEU()->rawValue().toString(), QString::fromLatin1(kValidPublicOperatorID));

    // The stored sanitized value validates as a no-op commit
    QVERIFY(settings->operatorIDEU()->validate(QString::fromLatin1(kValidPublicOperatorID), false).isEmpty());

    // A different bare 16-char value is still rejected
    QVERIFY(!settings->operatorIDEU()->validate(QStringLiteral("FIN87astrdge12k9"), false).isEmpty());
}

// Storing a full valid EU ID truncates it to the 16-char public part (secret chars must not persist)
void RemoteIDSettingsTest::_euStoreSanitizes()
{
    RemoteIDSettings* settings = SettingsManager::instance()->remoteIDSettings();

    settings->operatorIDEU()->setRawValue(QString::fromLatin1(kValidFullOperatorID));
    QCOMPARE(settings->operatorIDEU()->rawValue().toString(), QString::fromLatin1(kValidPublicOperatorID));
}

// operatorIDValidForRegion reflects whether the current region's operator ID fact is set
void RemoteIDSettingsTest::_validForRegionFollowsRegionFacts()
{
    RemoteIDSettings* settings = SettingsManager::instance()->remoteIDSettings();

    settings->region()->setRawValue(static_cast<int>(RemoteIDSettings::RegionOperation::EU));
    settings->operatorIDEU()->setRawValue(QString());
    settings->operatorIDFAA()->setRawValue(QString());
    QVERIFY(!settings->operatorIDValidForRegion());

    settings->operatorIDEU()->setRawValue(QString::fromLatin1(kValidFullOperatorID));
    QVERIFY(settings->operatorIDValidForRegion());

    // FAA region looks at the FAA fact, which is still empty
    settings->region()->setRawValue(static_cast<int>(RemoteIDSettings::RegionOperation::FAA));
    QVERIFY(!settings->operatorIDValidForRegion());

    settings->operatorIDFAA()->setRawValue(QStringLiteral("FA1234567890"));
    QVERIFY(settings->operatorIDValidForRegion());
}

// Each region keeps its own operator ID across region switches
void RemoteIDSettingsTest::_regionSwitchPreservesPerRegionIDs()
{
    RemoteIDSettings* settings = SettingsManager::instance()->remoteIDSettings();

    settings->region()->setRawValue(static_cast<int>(RemoteIDSettings::RegionOperation::EU));
    settings->operatorIDEU()->setRawValue(QString::fromLatin1(kValidFullOperatorID));
    settings->region()->setRawValue(static_cast<int>(RemoteIDSettings::RegionOperation::FAA));
    settings->operatorIDFAA()->setRawValue(QStringLiteral("FA1234567890"));

    settings->region()->setRawValue(static_cast<int>(RemoteIDSettings::RegionOperation::EU));
    QCOMPARE(settings->operatorIDEU()->rawValue().toString(), QString::fromLatin1(kValidPublicOperatorID));

    settings->region()->setRawValue(static_cast<int>(RemoteIDSettings::RegionOperation::FAA));
    QCOMPARE(settings->operatorIDFAA()->rawValue().toString(), QStringLiteral("FA1234567890"));
}

void RemoteIDSettingsTest::_operatorIDValidForRegionSignalEmittedOncePerTransition()
{
    RemoteIDSettings* settings = SettingsManager::instance()->remoteIDSettings();

    settings->region()->setRawValue(static_cast<int>(RemoteIDSettings::RegionOperation::EU));
    settings->operatorIDEU()->setRawValue(QString());
    settings->operatorIDFAA()->setRawValue(QString());
    QVERIFY(!settings->operatorIDValidForRegion());

    QSignalSpy spy(settings, &RemoteIDSettings::operatorIDValidForRegionChanged);
    QVERIFY(spy.isValid());

    // false -> true: exactly one emission (sanitization must not cause a second)
    settings->operatorIDEU()->setRawValue(QString::fromLatin1(kValidFullOperatorID));
    QVERIFY(settings->operatorIDValidForRegion());
    QCOMPARE(spy.count(), 1);

    // true -> false: FAA region with empty FAA fact
    settings->region()->setRawValue(static_cast<int>(RemoteIDSettings::RegionOperation::FAA));
    QVERIFY(!settings->operatorIDValidForRegion());
    QCOMPARE(spy.count(), 2);

    // false -> true: FAA fact set
    settings->operatorIDFAA()->setRawValue(QStringLiteral("FA1234567890"));
    QVERIFY(settings->operatorIDValidForRegion());
    QCOMPARE(spy.count(), 3);

    // No transition: back to EU, whose fact is also set
    settings->region()->setRawValue(static_cast<int>(RemoteIDSettings::RegionOperation::EU));
    QVERIFY(settings->operatorIDValidForRegion());
    QCOMPARE(spy.count(), 3);
}

void RemoteIDSettingsTest::_basicIDValidRequiresAllFields()
{
    RemoteIDSettings* settings = SettingsManager::instance()->remoteIDSettings();

    settings->basicIDType()->setRawValue(0);
    settings->basicIDUaType()->setRawValue(0);
    settings->basicID()->setRawValue(QString());
    QVERIFY(!settings->basicIDValid());

    settings->basicID()->setRawValue(QStringLiteral("1234567890ABCDEF"));
    QVERIFY(settings->basicIDValid());

    settings->basicID()->setRawValue(QString());
    QVERIFY(!settings->basicIDValid());
}

void RemoteIDSettingsTest::_basicIDValidSignalEmittedOncePerTransition()
{
    RemoteIDSettings* settings = SettingsManager::instance()->remoteIDSettings();

    settings->basicIDType()->setRawValue(0);
    settings->basicIDUaType()->setRawValue(0);
    settings->basicID()->setRawValue(QString());
    QVERIFY(!settings->basicIDValid());

    QSignalSpy spy(settings, &RemoteIDSettings::basicIDValidChanged);
    QVERIFY(spy.isValid());

    // false -> true
    settings->basicID()->setRawValue(QStringLiteral("1234567890ABCDEF"));
    QVERIFY(settings->basicIDValid());
    QCOMPARE(spy.count(), 1);

    // No transition: a different non-empty value must not emit
    settings->basicID()->setRawValue(QStringLiteral("FEDCBA0987654321"));
    QCOMPARE(spy.count(), 1);

    // true -> false
    settings->basicID()->setRawValue(QString());
    QVERIFY(!settings->basicIDValid());
    QCOMPARE(spy.count(), 2);
}

// Switching to the EU region must force operator ID broadcast on (required by EU regulation)
void RemoteIDSettingsTest::_euRegionForcesSendOperatorID()
{
    RemoteIDSettings* settings = SettingsManager::instance()->remoteIDSettings();

    settings->region()->setRawValue(static_cast<int>(RemoteIDSettings::RegionOperation::FAA));
    settings->sendOperatorID()->setRawValue(false);

    settings->region()->setRawValue(static_cast<int>(RemoteIDSettings::RegionOperation::EU));

    QVERIFY(settings->sendOperatorID()->rawValue().toBool());
}

// Switching to the FAA region must force live GNSS location (FAA requires live operator position)
void RemoteIDSettingsTest::_faaRegionForcesLiveLocation()
{
    RemoteIDSettings* settings = SettingsManager::instance()->remoteIDSettings();

    settings->region()->setRawValue(static_cast<int>(RemoteIDSettings::RegionOperation::EU));
    settings->locationType()->setRawValue(static_cast<int>(RemoteIDSettings::LocationType::FIXED));

    settings->region()->setRawValue(static_cast<int>(RemoteIDSettings::RegionOperation::FAA));

    QCOMPARE(settings->locationType()->rawValue().toInt(), static_cast<int>(RemoteIDSettings::LocationType::LIVE));
}

// Legacy single-fact storage migrates into the matching per-region fact and removes old keys
void RemoteIDSettingsTest::_legacyOperatorIDMigration()
{
    RemoteIDSettings* settings = SettingsManager::instance()->remoteIDSettings();

    // Legacy EU-validated ID lands in the EU fact
    settings->operatorIDEU()->setRawValue(QString());
    settings->operatorIDFAA()->setRawValue(QString());
    {
        QSettings qSettings;
        qSettings.beginGroup(RemoteIDSettings::settingsGroup);
        qSettings.setValue(QStringLiteral("operatorID"), QString::fromLatin1(kValidPublicOperatorID));
        qSettings.setValue(QStringLiteral("operatorIDEUValid"), true);
        qSettings.endGroup();
    }
    settings->migrateLegacyOperatorID();
    QCOMPARE(settings->operatorIDEU()->rawValue().toString(), QString::fromLatin1(kValidPublicOperatorID));
    QCOMPARE(settings->operatorIDFAA()->rawValue().toString(), QString());

    // A full-length legacy EU value (including the 3 secret chars) is truncated to the
    // 16-char public part by migration itself: the sanitize handler is only connected
    // after migration runs in the constructor, so migration can't rely on it. Blocking
    // signals here reproduces those constructor conditions.
    settings->operatorIDEU()->setRawValue(QString());
    {
        QSettings qSettings;
        qSettings.beginGroup(RemoteIDSettings::settingsGroup);
        qSettings.setValue(QStringLiteral("operatorID"), QString::fromLatin1(kValidFullOperatorID));
        qSettings.setValue(QStringLiteral("operatorIDEUValid"), true);
        qSettings.endGroup();
    }
    {
        const QSignalBlocker blocker(settings->operatorIDEU());
        settings->migrateLegacyOperatorID();
    }
    QCOMPARE(settings->operatorIDEU()->rawValue().toString(), QString::fromLatin1(kValidPublicOperatorID));

    // Legacy non-validated ID lands in the FAA fact
    settings->operatorIDEU()->setRawValue(QString());
    settings->operatorIDFAA()->setRawValue(QString());
    {
        QSettings qSettings;
        qSettings.beginGroup(RemoteIDSettings::settingsGroup);
        qSettings.setValue(QStringLiteral("operatorID"), QString::fromLatin1(kInvalidOperatorID));
        qSettings.setValue(QStringLiteral("operatorIDEUValid"), false);
        qSettings.endGroup();
    }
    settings->migrateLegacyOperatorID();
    QCOMPARE(settings->operatorIDEU()->rawValue().toString(), QString());
    QCOMPARE(settings->operatorIDFAA()->rawValue().toString(), QString::fromLatin1(kInvalidOperatorID));

    // Legacy keys are removed: migration is idempotent
    {
        QSettings qSettings;
        qSettings.beginGroup(RemoteIDSettings::settingsGroup);
        QVERIFY(!qSettings.contains(QStringLiteral("operatorID")));
        QVERIFY(!qSettings.contains(QStringLiteral("operatorIDEUValid")));
        qSettings.endGroup();
    }
    settings->operatorIDFAA()->setRawValue(QString());
    settings->migrateLegacyOperatorID();
    QCOMPARE(settings->operatorIDFAA()->rawValue().toString(), QString());
}

UT_REGISTER_TEST(RemoteIDSettingsTest, TestLabel::Unit)
