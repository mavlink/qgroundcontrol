#include "NTRIPSettingsUITest.h"

#include <QtQuick/QQuickItem>
#include <QtQuick/QQuickWindow>
#include <QtTest/QTest>

#include "Fact.h"
#include "NTRIPSettings.h"
#include "SettingsManager.h"

UT_REGISTER_TEST(NTRIPSettingsUITest, TestLabel::Integration)

void NTRIPSettingsUITest::init()
{
    UnitTest::init();

    NTRIPSettings *ntrip = SettingsManager::instance()->ntripSettings();
    ntrip->ntripServerConnectEnabled()->setRawValue(false);
    ntrip->ntripServerHostAddress()->setRawValue(QString());
    ntrip->ntripUseTls()->setRawValue(false);
    ntrip->ntripAllowSelfSignedCerts()->setRawValue(false);
}

bool NTRIPSettingsUITest::_navigateToNtripPage()
{
    if (!clickToolSelectDropdownButton(QStringLiteral("toolbar_viewSettings"))) {
        return false;
    }

    QQuickItem *btn = findVisibleItem(_rootItem, QStringLiteral("settingsButton_NTRIP/RTK"));
    if (!btn) {
        QTest::qFail("Settings page button not found: settingsButton_NTRIP/RTK", __FILE__, __LINE__);
        return false;
    }

    scrollIntoView(btn, QStringLiteral("settings_buttonList"));

    const QPointF center = btn->mapToScene(QPointF(btn->width() / 2, btn->height() / 2));
    QTest::mouseClick(_window, Qt::LeftButton, Qt::NoModifier, center.toPoint());
    QTest::qWait(_pageDelay);

    // Page root objectNames are sanitized to [A-Za-z0-9_], so "NTRIP/RTK" becomes "NTRIPRTK"
    if (!findVisibleItem(_rootItem, QStringLiteral("settingsPage_NTRIPRTK"))) {
        QTest::qFail("NTRIP settings page wrapper not found: settingsPage_NTRIPRTK", __FILE__, __LINE__);
        return false;
    }
    return true;
}

void NTRIPSettingsUITest::_testPageRenders()
{
    startUI();
    if (QTest::currentTestFailed()) return;

    QVERIFY(_navigateToNtripPage());
    QVERIFY2(findVisibleItem(_rootItem, QStringLiteral("ntripConnectButton")), "Connect button not found on NTRIP page");
    QVERIFY2(findVisibleItem(_rootItem, QStringLiteral("ntripHostField")), "Host field not found on NTRIP page");
    QVERIFY2(findVisibleItem(_rootItem, QStringLiteral("ntripBrowseButton")), "Browse button not found on NTRIP page");

    stopUI();
}

void NTRIPSettingsUITest::_testConnectGatedByHost()
{
    startUI();
    if (QTest::currentTestFailed()) return;

    QVERIFY(_navigateToNtripPage());

    QVERIFY(verifyEnabled(QStringLiteral("ntripConnectButton"), false, QStringLiteral("empty host")));

    SettingsManager::instance()->ntripSettings()->ntripServerHostAddress()->setRawValue(QStringLiteral("caster.example.com"));

    QVERIFY(verifyEnabled(QStringLiteral("ntripConnectButton"), true, QStringLiteral("host set")));

    stopUI();
}

void NTRIPSettingsUITest::_testBrowseGatedByHost()
{
    startUI();
    if (QTest::currentTestFailed()) return;

    QVERIFY(_navigateToNtripPage());

    QVERIFY(verifyEnabled(QStringLiteral("ntripBrowseButton"), false, QStringLiteral("empty host")));

    SettingsManager::instance()->ntripSettings()->ntripServerHostAddress()->setRawValue(QStringLiteral("caster.example.com"));

    QVERIFY(verifyEnabled(QStringLiteral("ntripBrowseButton"), true, QStringLiteral("host set")));

    stopUI();
}

void NTRIPSettingsUITest::_testSelfSignedGatedByTls()
{
    startUI();
    if (QTest::currentTestFailed()) return;

    QVERIFY(_navigateToNtripPage());

    QVERIFY(verifyEnabled(QStringLiteral("ntripAcceptSelfSignedSwitch"), false, QStringLiteral("TLS off")));

    SettingsManager::instance()->ntripSettings()->ntripUseTls()->setRawValue(true);

    QVERIFY(verifyEnabled(QStringLiteral("ntripAcceptSelfSignedSwitch"), true, QStringLiteral("TLS on")));

    stopUI();
}
