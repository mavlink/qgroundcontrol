#include "NTRIPSettingsUITest.h"

#include <QtQuick/QQuickItem>
#include <QtQuick/QQuickWindow>
#include <QtTest/QTest>

#include "NTRIPSettings.h"
#include "SettingsManager.h"

UT_REGISTER_TEST(NTRIPSettingsUITest, TestLabel::Integration)

void NTRIPSettingsUITest::init()
{
    UnitTest::init();

    // Every Fact a test changes is saved here, so cleanup() restores it for the next test.
    _settings = std::make_unique<TestFixtures::SettingsFixture>();
    NTRIPSettings* ntrip = SettingsManager::instance()->ntripSettings();
    _settings->setFactValue(ntrip->ntripServerConnectEnabled(), false);
    _settings->setFactValue(ntrip->ntripServerHostAddress(), QString());
    _settings->setFactValue(ntrip->ntripUseTls(), false);
    _settings->setFactValue(ntrip->ntripAllowSelfSignedCerts(), false);
}

void NTRIPSettingsUITest::cleanup()
{
    QmlUITestBase::cleanup();
    _settings.reset();
}

bool NTRIPSettingsUITest::_navigateToNtripPage()
{
    if (!clickToolSelectDropdownButton(QStringLiteral("toolbar_viewSettings"))) {
        return false;
    }

    QQuickItem* btn = findVisibleItem(_rootItem, QStringLiteral("settingsButton_RTK Corrections"));
    if (!btn) {
        QTest::qFail("Settings page button not found: settingsButton_RTK Corrections", __FILE__, __LINE__);
        return false;
    }

    scrollIntoView(btn, QStringLiteral("settings_buttonList"));

    const QPointF center = btn->mapToScene(QPointF(btn->width() / 2, btn->height() / 2));
    QTest::mouseClick(_window, Qt::LeftButton, Qt::NoModifier, center.toPoint());
    QTest::qWait(_pageDelay);

    // Page root objectNames are sanitized to [A-Za-z0-9_], so "RTK Corrections" becomes "RTKCorrections"
    if (!findVisibleItem(_rootItem, QStringLiteral("settingsPage_RTKCorrections"))) {
        QTest::qFail("RTK Corrections settings page wrapper not found: settingsPage_RTKCorrections", __FILE__,
                     __LINE__);
        return false;
    }
    return true;
}

void NTRIPSettingsUITest::_testControlsGated()
{
    startUI();
    if (QTest::currentTestFailed()) {
        return;
    }

    QVERIFY(_navigateToNtripPage());
    QVERIFY2(findVisibleItem(_rootItem, QStringLiteral("settingsTextField_ntripServerHostAddress")),
             "Host field not found on NTRIP page");
    NTRIPSettings* ntrip = SettingsManager::instance()->ntripSettings();

    QVERIFY(verifyEnabled(QStringLiteral("ntripConnectButton"), false, QStringLiteral("empty host")));
    QVERIFY(verifyEnabled(QStringLiteral("ntripBrowseButton"), false, QStringLiteral("empty host")));
    ntrip->ntripServerHostAddress()->setRawValue(QStringLiteral("caster.example.com"));
    QVERIFY(verifyEnabled(QStringLiteral("ntripConnectButton"), true, QStringLiteral("host set")));
    QVERIFY(verifyEnabled(QStringLiteral("ntripBrowseButton"), true, QStringLiteral("host set")));

    QVERIFY(
        verifyEnabled(QStringLiteral("settingsCheckBox_ntripAllowSelfSignedCerts"), false, QStringLiteral("TLS off")));
    ntrip->ntripUseTls()->setRawValue(true);
    QVERIFY(
        verifyEnabled(QStringLiteral("settingsCheckBox_ntripAllowSelfSignedCerts"), true, QStringLiteral("TLS on")));

    stopUI();
}
