#include "NTRIPSavedCastersTest.h"

#include <memory>

#include <QtQuick/QQuickItem>

#include "Fact.h"
#include "Fixtures/RAIIFixtures.h"
#include "NTRIPSettings.h"
#include "QmlTesting/QmlFixtures.h"
#include "SettingsManager.h"

namespace {

/// The application's NTRIP settings with a caster entered, the connection off and no saved casters, restored when
/// the fixture ends.
struct EnteredCaster
{
    TestFixtures::SettingsFixture saved;
    NTRIPSettings* settings = SettingsManager::instance()->ntripSettings();

    EnteredCaster()
    {
        saved.setFactValue(settings->ntripServerConnectEnabled(), false);
        saved.setFactValue(settings->ntripServerHostAddress(), QStringLiteral("field.example.com"));
        saved.setFactValue(settings->ntripServerPort(), 2101);
        saved.setFactValue(settings->ntripUsername(), QStringLiteral("field-user"));
        saved.setFactValue(settings->ntripPassword(), QStringLiteral("field-secret"));
        saved.setFactValue(settings->ntripMountpoint(), QStringLiteral("FIELD"));
        saved.setFactValue(settings->ntripUseTls(), false);
        saved.setFactValue(settings->ntripAllowSelfSignedCerts(), false);
        saved.setFactValue(settings->ntripPinnedCertificate(), QString());
        saved.setFactValue(settings->ntripSavedCasters(), QString());
        saved.setFactValue(settings->ntripFailoverEnabled(), false);
    }

    QString host() const { return settings->ntripServerHostAddress()->rawValue().toString(); }
};

/// The Saved Casters group inside @a window; null on failure, with the reason in lastError().
std::unique_ptr<QObject> createGroup(TestFixtures::QmlEngine& engine, const TestFixtures::QmlWindowFixture& window)
{
    return engine.create(
        TestFixtures::sourceQmlUrl(QStringLiteral("GPS/Qml/NTRIPSavedCasters.qml")),
        {{QStringLiteral("parent"), QVariant::fromValue(window.contentItem())}, {QStringLiteral("width"), 600}});
}

QQuickItem* part(const std::unique_ptr<QObject>& group, const char* name)
{
    return group->findChild<QQuickItem*>(QString::fromLatin1(name));
}

/// Presses the button @a name of @a group; false while it cannot be pressed.
bool press(const std::unique_ptr<QObject>& group, const char* name)
{
    QQuickItem* const button = part(group, name);
    return button && button->isVisible() && button->isEnabled() && QMetaObject::invokeMethod(button, "click");
}

bool shown(const std::unique_ptr<QObject>& group, const char* name)
{
    const QQuickItem* const item = part(group, name);
    return item && item->isVisible();
}

}  // namespace

void NTRIPSavedCastersTest::_savesAndUsesTheEnteredCaster()
{
    EnteredCaster entered;
    NTRIPSettings* const settings = entered.settings;
    TestFixtures::QmlWindowFixture window(640, 600);
    TestFixtures::QmlEngine engine;
    const std::unique_ptr<QObject> group = createGroup(engine, window);
    QVERIFY2(group, qPrintable(engine.lastError()));
    QVERIFY(window.show());

    // Nothing is saved yet, so there is nothing to pick.
    QVERIFY(!shown(group, "ntripSavedCasterPicker"));
    QVERIFY(part(group, "ntripSavedCasterName")->setProperty("text", QStringLiteral(" Field network ")));
    QVERIFY(press(group, "ntripSavedCasterSave"));
    QCOMPARE(settings->savedCasters().size(), 1);
    const NTRIPSavedCaster field = settings->savedCasters().constFirst();
    QCOMPARE(field.name, QStringLiteral("Field network"));
    QCOMPARE(field.host, QStringLiteral("field.example.com"));
    QCOMPARE(field.mountpoint, QStringLiteral("FIELD"));
    QCOMPARE(field.password, QStringLiteral("field-secret"));
    QVERIFY(shown(group, "ntripSavedCasterPicker"));
    QCOMPARE(part(group, "ntripSavedCasterPicker")->property("currentText").toString(), field.name);

    // The connection settings stay what is connected to: another caster is entered, and the saved one returns
    // only when it is used.
    settings->ntripServerHostAddress()->setRawValue(QStringLiteral("other.example.com"));
    settings->ntripMountpoint()->setRawValue(QStringLiteral("OTHER"));
    QVERIFY(press(group, "ntripSavedCasterUse"));
    QCOMPARE(entered.host(), field.host);
    QCOMPARE(settings->ntripMountpoint()->rawValue().toString(), field.mountpoint);
    QCOMPARE(settings->ntripPassword()->rawValue().toString(), field.password);

    QVERIFY(press(group, "ntripSavedCasterDelete"));
    QVERIFY(settings->savedCasters().isEmpty());
    QVERIFY(!shown(group, "ntripSavedCasterPicker"));
    // Deleting a saved caster leaves the one that is entered.
    QCOMPARE(entered.host(), field.host);
}

void NTRIPSavedCastersTest::_usingWaitsForADisconnect()
{
    EnteredCaster entered;
    NTRIPSettings* const settings = entered.settings;
    QVERIFY(settings->saveConnectionAsCaster(QStringLiteral("Field network")));
    settings->ntripServerHostAddress()->setRawValue(QStringLiteral("other.example.com"));
    settings->ntripServerConnectEnabled()->setRawValue(true);
    TestFixtures::QmlWindowFixture window(640, 600);
    TestFixtures::QmlEngine engine;
    const std::unique_ptr<QObject> group = createGroup(engine, window);
    QVERIFY2(group, qPrintable(engine.lastError()));
    QVERIFY(window.show());

    // The page locks the server and mountpoint settings while the connection is on, and a saved caster would write
    // them.
    QVERIFY(shown(group, "ntripSavedCasterUse"));
    QVERIFY(!press(group, "ntripSavedCasterUse"));
    QVERIFY(part(group, "ntripSavedCasterList")->property("note").toString().contains(QStringLiteral("Disconnect")));
    QCOMPARE(entered.host(), QStringLiteral("other.example.com"));

    // Saving and deleting leave the connection as it is.
    QVERIFY(part(group, "ntripSavedCasterName")->setProperty("text", QStringLiteral("Other network")));
    QVERIFY(press(group, "ntripSavedCasterSave"));
    QCOMPARE(settings->savedCasters().size(), 2);
    QVERIFY(settings->ntripServerConnectEnabled()->rawValue().toBool());

    settings->ntripServerConnectEnabled()->setRawValue(false);
    QVERIFY(!part(group, "ntripSavedCasterList")->property("note").toString().contains(QStringLiteral("Disconnect")));
    QVERIFY(part(group, "ntripSavedCasterPicker")->setProperty("currentIndex", 0));
    QVERIFY(press(group, "ntripSavedCasterUse"));
    QCOMPARE(entered.host(), QStringLiteral("field.example.com"));
}

void NTRIPSavedCastersTest::_saveSaysWhenItCannot()
{
    EnteredCaster entered;
    NTRIPSettings* const settings = entered.settings;
    TestFixtures::QmlWindowFixture window(640, 600);
    TestFixtures::QmlEngine engine;
    const std::unique_ptr<QObject> group = createGroup(engine, window);
    QVERIFY2(group, qPrintable(engine.lastError()));
    QVERIFY(window.show());
    QQuickItem* const name = part(group, "ntripSavedCasterName");
    QVERIFY(name);

    // A caster needs a name and a host to be saved.
    QVERIFY(!press(group, "ntripSavedCasterSave"));
    QVERIFY(name->setProperty("text", QStringLiteral("  ")));
    QVERIFY(!press(group, "ntripSavedCasterSave"));
    QVERIFY(name->setProperty("text", QStringLiteral("One more")));
    settings->ntripServerHostAddress()->setRawValue(QString());
    QVERIFY(!press(group, "ntripSavedCasterSave"));
    settings->ntripServerHostAddress()->setRawValue(QStringLiteral("field.example.com"));

    // A full list takes no other name, which the group says until the list changes.
    for (int i = 0; i < NTRIPSettings::MaxSavedCasters; ++i) {
        QVERIFY(settings->saveConnectionAsCaster(QStringLiteral("Caster %1").arg(i)));
    }
    QVERIFY(!shown(group, "ntripSavedCasterSaveFailed"));
    QVERIFY(press(group, "ntripSavedCasterSave"));
    QCOMPARE(settings->savedCasters().size(), NTRIPSettings::MaxSavedCasters);
    QVERIFY(shown(group, "ntripSavedCasterSaveFailed"));
    QVERIFY(press(group, "ntripSavedCasterDelete"));
    QVERIFY(!shown(group, "ntripSavedCasterSaveFailed"));
    QVERIFY(press(group, "ntripSavedCasterSave"));
    QCOMPARE(settings->savedCasters().constLast().name, QStringLiteral("One more"));
}

void NTRIPSavedCastersTest::_failoverSwitchSetsTheSetting()
{
    EnteredCaster entered;
    Fact* const failover = entered.settings->ntripFailoverEnabled();
    // The connection may be on: the manager takes a change of failover as it comes.
    entered.settings->ntripServerConnectEnabled()->setRawValue(true);
    TestFixtures::QmlWindowFixture window(640, 600);
    TestFixtures::QmlEngine engine;
    const std::unique_ptr<QObject> group = createGroup(engine, window);
    QVERIFY2(group, qPrintable(engine.lastError()));
    QVERIFY(window.show());
    QVERIFY(shown(group, "ntripFailoverSwitch"));
    QVERIFY(!part(group, "ntripFailoverSwitch")->property("checked").toBool());
    QVERIFY(press(group, "ntripFailoverSwitch"));
    QVERIFY(failover->rawValue().toBool());
    // The switch follows the setting, however that changes.
    failover->setRawValue(false);
    QVERIFY(!part(group, "ntripFailoverSwitch")->property("checked").toBool());
}

UT_REGISTER_TEST(NTRIPSavedCastersTest, TestLabel::Unit)
