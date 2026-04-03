#include "RemoteIDManagerTest.h"

#include "RemoteIDManager.h"
#include "RemoteIDSettings.h"
#include "SettingsManager.h"
#include "Vehicle.h"

namespace {

constexpr const char* kValidFullOperatorID = "FIN87astrdge12k8-xyz";
constexpr const char* kValidPublicOperatorID = "FIN87astrdge12k8";
constexpr const char* kInvalidOperatorID = "DEADBEEFDEADBEEF";

}  // namespace

void RemoteIDManagerTest::init()
{
    VehicleTestNoInitialConnect::init();

    RemoteIDSettings* settings = SettingsManager::instance()->remoteIDSettings();
    QVERIFY(settings);
    QVERIFY(vehicle());
    QVERIFY(vehicle()->remoteIDManager());

    _savedOperatorID = settings->operatorID()->rawValue();
    _savedOperatorIDType = settings->operatorIDType()->rawValue();
    _savedOperatorIDValid = settings->operatorIDValid()->rawValue();
    _savedRegion = settings->region()->rawValue();
}

void RemoteIDManagerTest::cleanup()
{
    RemoteIDSettings* settings = SettingsManager::instance()->remoteIDSettings();
    QVERIFY(settings);

    settings->region()->setRawValue(RemoteIDManager::FAA);
    settings->operatorID()->setRawValue(_savedOperatorID);
    settings->operatorIDValid()->setRawValue(_savedOperatorIDValid);
    settings->operatorIDType()->setRawValue(_savedOperatorIDType);
    settings->region()->setRawValue(_savedRegion);

    VehicleTestNoInitialConnect::cleanup();
}

void RemoteIDManagerTest::_validEUOperatorIDIsSanitized()
{
    RemoteIDSettings* settings = SettingsManager::instance()->remoteIDSettings();
    RemoteIDManager* manager = vehicle()->remoteIDManager();

    settings->region()->setRawValue(RemoteIDManager::EU);
    settings->operatorIDType()->setRawValue(0);
    settings->operatorIDValid()->setRawValue(false);
    settings->operatorID()->setRawValue(QString());

    settings->operatorID()->setRawValue(QString::fromLatin1(kValidFullOperatorID));

    QVERIFY(settings->operatorIDValid()->rawValue().toBool());
    QCOMPARE(settings->operatorID()->rawValue().toString(), QString::fromLatin1(kValidPublicOperatorID));
    QVERIFY(manager->operatorIDGood());
}

void RemoteIDManagerTest::_invalidEUOperatorIDClearsTrustedState()
{
    RemoteIDSettings* settings = SettingsManager::instance()->remoteIDSettings();
    RemoteIDManager* manager = vehicle()->remoteIDManager();

    settings->region()->setRawValue(RemoteIDManager::EU);
    settings->operatorIDType()->setRawValue(0);
    settings->operatorIDValid()->setRawValue(false);
    settings->operatorID()->setRawValue(QString::fromLatin1(kValidFullOperatorID));

    QVERIFY(settings->operatorIDValid()->rawValue().toBool());
    QVERIFY(manager->operatorIDGood());

    settings->operatorID()->setRawValue(QString::fromLatin1(kInvalidOperatorID));

    QVERIFY(!settings->operatorIDValid()->rawValue().toBool());
    QCOMPARE(settings->operatorID()->rawValue().toString(), QString::fromLatin1(kInvalidOperatorID));
    QVERIFY(!manager->operatorIDGood());
}

void RemoteIDManagerTest::_switchingToEUUsesValidatedOperatorID()
{
    RemoteIDSettings* settings = SettingsManager::instance()->remoteIDSettings();
    RemoteIDManager* manager = vehicle()->remoteIDManager();

    settings->region()->setRawValue(RemoteIDManager::FAA);
    settings->operatorIDType()->setRawValue(0);
    settings->operatorIDValid()->setRawValue(false);
    settings->operatorID()->setRawValue(QString::fromLatin1(kValidFullOperatorID));

    QVERIFY(settings->operatorIDValid()->rawValue().toBool());
    QCOMPARE(settings->operatorID()->rawValue().toString(), QString::fromLatin1(kValidFullOperatorID));
    QVERIFY(manager->operatorIDGood());

    settings->region()->setRawValue(RemoteIDManager::EU);

    QVERIFY(settings->operatorIDValid()->rawValue().toBool());
    QCOMPARE(settings->operatorID()->rawValue().toString(), QString::fromLatin1(kValidPublicOperatorID));
    QVERIFY(manager->operatorIDGood());
}

UT_REGISTER_TEST(RemoteIDManagerTest, TestLabel::Integration, TestLabel::Vehicle)
