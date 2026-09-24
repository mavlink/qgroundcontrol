#include "NTRIPSettingsUITest.h"

#include <memory>

#include <QtQml/QQmlComponent>
#include <QtQml/QQmlEngine>
#include <QtQuick/QQuickItem>
#include <QtQuick/QQuickWindow>
#include <QtTest/QTest>

#include "Fact.h"
#include "NTRIPManager.h"
#include "NTRIPSettings.h"
#include "SettingsManager.h"

UT_REGISTER_TEST(NTRIPSettingsUITest, TestLabel::Integration)

namespace {
constexpr char kMockNtripManager[] = R"(
    import QtQml
    import QGroundControl
    QtObject {
        property int connectionStatus: NTRIPManager.Error
        property string statusMessage: "Connection failed"
        property string ggaSource: "Vehicle GPS"
        property string securityWarning: ""
        property QtObject connectionStats: QtObject {
            property bool dataStale: false
            property real correctionAgeSec: 0.5
            property int messagesReceived: 1
            property var messageCountsById: [[1005, 1]]
            property real bytesReceived: 25
            property real dataRateBytesPerSec: 25
        }
        property int retryCount: 0
        function retryNTRIP() {
            retryCount++
            connectionStatus = NTRIPManager.Connecting
        }
    }
)";
}  // namespace

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

void NTRIPSettingsUITest::_testErrorActionRetries()
{
    QQuickWindow window;
    window.resize(640, 400);
    QQmlEngine engine;
    engine.addImportPath(QStringLiteral("qrc:/qml"));
    QQmlComponent mockComponent(&engine);
    mockComponent.setData(QByteArray(kMockNtripManager), QUrl());
    QTRY_VERIFY_WITH_TIMEOUT(!mockComponent.isLoading(), TestTimeout::mediumMs());
    std::unique_ptr<QObject> manager(mockComponent.create());
    QVERIFY2(manager, qPrintable(mockComponent.errorString()));
    Fact enabled(0, QStringLiteral("enabled"), FactMetaData::valueTypeBool);
    enabled.setRawValue(true);
    QQmlComponent component(&engine,
                            QUrl(QStringLiteral("qrc:/qml/QGroundControl/AppSettings/NtripConnectionSettings.qml")));
    QTRY_VERIFY_WITH_TIMEOUT(!component.isLoading(), TestTimeout::mediumMs());
    std::unique_ptr<QObject> panel(
        component.createWithInitialProperties({{QStringLiteral("parent"), QVariant::fromValue(window.contentItem())},
                                               {QStringLiteral("visible"), true},
                                               {QStringLiteral("_ntripMgr"), QVariant::fromValue(manager.get())},
                                               {QStringLiteral("_enabled"), QVariant::fromValue(&enabled)}}));
    QVERIFY2(panel, qPrintable(component.errorString()));
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window, TestTimeout::mediumMs()));
    auto* button = panel->findChild<QObject*>(QStringLiteral("ntripConnectButton"));
    auto* disconnect = panel->findChild<QObject*>(QStringLiteral("ntripDisconnectButton"));
    QVERIFY(button);
    QVERIFY(disconnect);
    const auto errorStatus = manager->property("connectionStatus");
    QCOMPARE(button->property("text").toString(), QStringLiteral("Retry"));
    QTRY_VERIFY_WITH_TIMEOUT(disconnect->property("visible").toBool(), TestTimeout::shortMs());
    QVERIFY(QMetaObject::invokeMethod(button, "click"));
    QCOMPARE(manager->property("retryCount").toInt(), 1);
    QVERIFY(enabled.rawValue().toBool());
    QVERIFY(!button->property("enabled").toBool());
    QTRY_VERIFY_WITH_TIMEOUT(!disconnect->property("visible").toBool(), TestTimeout::shortMs());
    QVERIFY(manager->setProperty("connectionStatus", errorStatus));
    QCOMPARE(manager->property("connectionStatus"), errorStatus);
    QTRY_VERIFY_WITH_TIMEOUT(disconnect->property("visible").toBool(), TestTimeout::shortMs());
    QVERIFY(QMetaObject::invokeMethod(disconnect, "click"));
    QVERIFY(!enabled.rawValue().toBool());
    QCOMPARE(manager->property("retryCount").toInt(), 1);
}

void NTRIPSettingsUITest::_testConnectionActionIsIdempotent_data()
{
    QTest::addColumn<int>("status");
    QTest::addColumn<bool>("enabledBefore");
    QTest::addColumn<bool>("enabledAfter");
    QTest::newRow("disconnect") << static_cast<int>(NTRIPManager::ConnectionStatus::Connected) << true << false;
    QTest::newRow("cancel-reconnect") << static_cast<int>(NTRIPManager::ConnectionStatus::Reconnecting) << true
                                      << false;
    QTest::newRow("connect") << static_cast<int>(NTRIPManager::ConnectionStatus::Disconnected) << false << true;
}

void NTRIPSettingsUITest::_testConnectionActionIsIdempotent()
{
    QFETCH(int, status);
    QFETCH(bool, enabledBefore);
    QFETCH(bool, enabledAfter);
    QQuickWindow window;
    window.resize(640, 400);
    QQmlEngine engine;
    engine.addImportPath(QStringLiteral("qrc:/qml"));
    QQmlComponent mockComponent(&engine);
    mockComponent.setData(QByteArray(kMockNtripManager), QUrl());
    QTRY_VERIFY_WITH_TIMEOUT(!mockComponent.isLoading(), TestTimeout::mediumMs());
    std::unique_ptr<QObject> manager(mockComponent.createWithInitialProperties(
        {{QStringLiteral("connectionStatus"), status}, {QStringLiteral("statusMessage"), QString()}}));
    QVERIFY2(manager, qPrintable(mockComponent.errorString()));
    Fact enabled(0, QStringLiteral("enabled"), FactMetaData::valueTypeBool);
    enabled.setRawValue(enabledBefore);
    SettingsManager::instance()->ntripSettings()->ntripServerHostAddress()->setRawValue(
        QStringLiteral("caster.example.com"));
    QQmlComponent component(&engine,
                            QUrl(QStringLiteral("qrc:/qml/QGroundControl/AppSettings/NtripConnectionSettings.qml")));
    QTRY_VERIFY_WITH_TIMEOUT(!component.isLoading(), TestTimeout::mediumMs());
    std::unique_ptr<QObject> panel(
        component.createWithInitialProperties({{QStringLiteral("parent"), QVariant::fromValue(window.contentItem())},
                                               {QStringLiteral("visible"), true},
                                               {QStringLiteral("_ntripMgr"), QVariant::fromValue(manager.get())},
                                               {QStringLiteral("_enabled"), QVariant::fromValue(&enabled)}}));
    QVERIFY2(panel, qPrintable(component.errorString()));
    auto* button = panel->findChild<QObject*>(QStringLiteral("ntripConnectButton"));
    QVERIFY(button);
    QVERIFY(button->property("enabled").toBool());
    // The manager applies the setting after a debounce, so its status still reflects the first click.
    for (int click = 0; click < 2; ++click) {
        QVERIFY(QMetaObject::invokeMethod(button, "click"));
        QCOMPARE(enabled.rawValue().toBool(), enabledAfter);
    }
    QCOMPARE(manager->property("retryCount").toInt(), 0);
}

void NTRIPSettingsUITest::_testMountpointLockedWhileActive()
{
    QQmlEngine engine;
    engine.addImportPath(QStringLiteral("qrc:/qml"));
    QQmlComponent component(&engine,
                            QUrl(QStringLiteral("qrc:/qml/QGroundControl/AppSettings/NtripMountpointBrowser.qml")));
    QTRY_VERIFY_WITH_TIMEOUT(!component.isLoading(), TestTimeout::mediumMs());
    std::unique_ptr<QObject> panel(component.createWithInitialProperties({{QStringLiteral("_isActive"), true}}));
    QVERIFY2(panel, qPrintable(component.errorString()));
    auto* list = panel->findChild<QObject*>(QStringLiteral("ntripMountpointList"));
    QVERIFY(list);
    QVERIFY(!list->property("enabled").toBool());
    QVERIFY(panel->setProperty("_isActive", false));
    QVERIFY(list->property("enabled").toBool());
}
