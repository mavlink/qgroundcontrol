#include "NTRIPSettingsUITest.h"

#include <functional>
#include <memory>

#include <QtQuick/QQuickItem>
#include <QtQuick/QQuickWindow>
#include <QtQuickTest/quicktest.h>
#include <QtTest/QTest>

#include "Fact.h"
#include "GpsQmlTestHelpers.h"
#include "NTRIPManager.h"
#include "NTRIPSettings.h"
#include "NTRIPSourceTable.h"
#include "SettingsManager.h"

UT_REGISTER_TEST(NTRIPSettingsUITest, TestLabel::Integration)

namespace {
constexpr char kMockNtripManager[] = R"(
    import QtQml
    import QGroundControl
    import QGroundControl.GPS
    QtObject {
        property int connectionStatus: NTRIPManager.Error
        property string statusMessage: "Connection failed"
        property string ggaSource: "Vehicle GPS"
        property string securityWarning: ""
        property QtObject connectionStats: QtObject {
            property bool dataStale: false
            property real correctionAgeSec: 0.5
            property int messagesReceived: 1
            property var messageCountsById: [{ messageId: 1005, count: 1 }]
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

QList<QQuickItem*> findItems(QQuickItem* root, const std::function<bool(QQuickItem*)>& match)
{
    QList<QQuickItem*> found;
    for (auto* child : root->childItems()) {
        if (match(child)) {
            found.append(child);
        }
        found.append(findItems(child, match));
    }
    return found;
}
}  // namespace

void NTRIPSettingsUITest::init()
{
    UnitTest::init();

    NTRIPSettings* ntrip = SettingsManager::instance()->ntripSettings();
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

void NTRIPSettingsUITest::_testPageRenders()
{
    startUI();
    if (QTest::currentTestFailed())
        return;

    QVERIFY(_navigateToNtripPage());
    QVERIFY2(findVisibleItem(_rootItem, QStringLiteral("ntripConnectButton")),
             "Connect button not found on NTRIP page");
    QVERIFY2(findVisibleItem(_rootItem, QStringLiteral("ntripHostField")), "Host field not found on NTRIP page");
    QVERIFY2(findVisibleItem(_rootItem, QStringLiteral("ntripBrowseButton")), "Browse button not found on NTRIP page");

    stopUI();
}

void NTRIPSettingsUITest::_testConnectGatedByHost()
{
    startUI();
    if (QTest::currentTestFailed())
        return;

    QVERIFY(_navigateToNtripPage());

    QVERIFY(verifyEnabled(QStringLiteral("ntripConnectButton"), false, QStringLiteral("empty host")));

    SettingsManager::instance()->ntripSettings()->ntripServerHostAddress()->setRawValue(
        QStringLiteral("caster.example.com"));

    QVERIFY(verifyEnabled(QStringLiteral("ntripConnectButton"), true, QStringLiteral("host set")));

    stopUI();
}

void NTRIPSettingsUITest::_testBrowseGatedByHost()
{
    startUI();
    if (QTest::currentTestFailed())
        return;

    QVERIFY(_navigateToNtripPage());

    QVERIFY(verifyEnabled(QStringLiteral("ntripBrowseButton"), false, QStringLiteral("empty host")));

    SettingsManager::instance()->ntripSettings()->ntripServerHostAddress()->setRawValue(
        QStringLiteral("caster.example.com"));

    QVERIFY(verifyEnabled(QStringLiteral("ntripBrowseButton"), true, QStringLiteral("host set")));

    stopUI();
}

void NTRIPSettingsUITest::_testSelfSignedGatedByTls()
{
    startUI();
    if (QTest::currentTestFailed())
        return;

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
    GpsTestHelpers::QmlEngine engine;
    std::unique_ptr<QObject> manager = engine.create(QByteArray(kMockNtripManager));
    QVERIFY2(manager, qPrintable(engine.lastError()));
    Fact enabled(0, QStringLiteral("enabled"), FactMetaData::valueTypeBool);
    enabled.setRawValue(true);
    std::unique_ptr<QObject> panel =
        engine.create(QUrl(QStringLiteral("qrc:/qml/QGroundControl/AppSettings/NtripConnectionSettings.qml")),
                      {{QStringLiteral("parent"), QVariant::fromValue(window.contentItem())},
                       {QStringLiteral("visible"), true},
                       {QStringLiteral("_ntripMgr"), QVariant::fromValue(manager.get())},
                       {QStringLiteral("_enabled"), QVariant::fromValue(&enabled)}});
    QVERIFY2(panel, qPrintable(engine.lastError()));
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
    GpsTestHelpers::QmlEngine engine;
    std::unique_ptr<QObject> manager =
        engine.create(QByteArray(kMockNtripManager),
                      {{QStringLiteral("connectionStatus"), status}, {QStringLiteral("statusMessage"), QString()}});
    QVERIFY2(manager, qPrintable(engine.lastError()));
    Fact enabled(0, QStringLiteral("enabled"), FactMetaData::valueTypeBool);
    enabled.setRawValue(enabledBefore);
    SettingsManager::instance()->ntripSettings()->ntripServerHostAddress()->setRawValue(
        QStringLiteral("caster.example.com"));
    std::unique_ptr<QObject> panel =
        engine.create(QUrl(QStringLiteral("qrc:/qml/QGroundControl/AppSettings/NtripConnectionSettings.qml")),
                      {{QStringLiteral("parent"), QVariant::fromValue(window.contentItem())},
                       {QStringLiteral("visible"), true},
                       {QStringLiteral("_ntripMgr"), QVariant::fromValue(manager.get())},
                       {QStringLiteral("_enabled"), QVariant::fromValue(&enabled)}});
    QVERIFY2(panel, qPrintable(engine.lastError()));
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
    GpsTestHelpers::QmlEngine engine;
    std::unique_ptr<QObject> panel =
        engine.create(QUrl(QStringLiteral("qrc:/qml/QGroundControl/AppSettings/NtripMountpointBrowser.qml")),
                      {{QStringLiteral("_isActive"), true}});
    QVERIFY2(panel, qPrintable(engine.lastError()));
    auto* list = panel->findChild<QObject*>(QStringLiteral("ntripMountpointList"));
    QVERIFY(list);
    QVERIFY(!list->property("enabled").toBool());
    QVERIFY(panel->setProperty("_isActive", false));
    QVERIFY(list->property("enabled").toBool());
}

void NTRIPSettingsUITest::_testLongStatusWrapsWithinPanel()
{
    QQuickWindow window;
    window.resize(640, 400);
    GpsTestHelpers::QmlEngine engine;
    std::unique_ptr<QObject> manager = engine.create(QByteArray(kMockNtripManager));
    QVERIFY2(manager, qPrintable(engine.lastError()));
    Fact enabled(0, QStringLiteral("enabled"), FactMetaData::valueTypeBool);
    enabled.setRawValue(true);
    std::unique_ptr<QObject> panel =
        engine.create(QUrl(QStringLiteral("qrc:/qml/QGroundControl/AppSettings/NtripConnectionSettings.qml")),
                      {{QStringLiteral("parent"), QVariant::fromValue(window.contentItem())},
                       {QStringLiteral("visible"), true},
                       {QStringLiteral("_ntripMgr"), QVariant::fromValue(manager.get())},
                       {QStringLiteral("_enabled"), QVariant::fromValue(&enabled)}});
    QVERIFY2(panel, qPrintable(engine.lastError()));
    auto* panelItem = qobject_cast<QQuickItem*>(panel.get());
    QVERIFY(panelItem);
    panelItem->setWidth(480);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window, TestTimeout::mediumMs()));
    const qreal shortWidth = panelItem->implicitWidth();

    const QString longMessage =
        QStringLiteral("Reconnecting in 30s: ") + QStringLiteral("certificate rejected ").repeated(20);
    QVERIFY(manager->setProperty("statusMessage", longMessage));
    const auto statusLabel = [&]() -> QQuickItem* {
        const auto labels = findItems(panelItem, [&](QQuickItem* item) {
            return item->property("text").toString() == longMessage && item->property("lineCount").isValid();
        });
        return labels.isEmpty() ? nullptr : labels.first();
    };
    QTRY_VERIFY_WITH_TIMEOUT(statusLabel(), TestTimeout::mediumMs());
    QVERIFY(QQuickTest::qWaitForPolish(&window, TestTimeout::mediumMs()));
    // The message wraps inside the panel instead of widening every settings group on the page.
    QCOMPARE(panelItem->implicitWidth(), shortWidth);
    QVERIFY(statusLabel()->property("lineCount").toInt() > 1);
    const auto buttons = findItems(
        panelItem, [](QQuickItem* item) { return item->objectName() == QLatin1String("ntripConnectButton"); });
    QCOMPARE(buttons.size(), 1);
    const QPointF buttonRight = buttons.first()->mapToItem(panelItem, QPointF(buttons.first()->width(), 0));
    QVERIFY2(buttonRight.x() <= panelItem->width(), qPrintable(QString::number(buttonRight.x())));
}

void NTRIPSettingsUITest::_testMountpointButtonsAligned()
{
    NTRIPSourceTableModel model;
    model.parseSourceTable(QStringLiteral(
        "STR;LONG;Long;RTCM 3.3;1005(1),1077(1);2;GPS+GLO+GAL+BDS+SBAS+QZSS;NET;FRA;0;0;0;0;gen;none;B;N;15200;\r\n"
        "STR;S;Short;RTCM 3.3;1005(1);2;GPS;NET;FRA;0;0;0;0;gen;none;N;N;0;\r\n"
        "ENDSOURCETABLE\r\n"));
    QCOMPARE(model.rowCount(), 2);
    QQuickWindow window;
    window.resize(640, 400);
    GpsTestHelpers::QmlEngine engine;
    std::unique_ptr<QObject> list =
        engine.create(QUrl(QStringLiteral("qrc:/qml/QGroundControl/GPS/NTRIP/NTRIPMountpointList.qml")),
                      {{QStringLiteral("parent"), QVariant::fromValue(window.contentItem())},
                       {QStringLiteral("width"), 480},
                       {QStringLiteral("height"), 300},
                       {QStringLiteral("model"), QVariant::fromValue(&model)}});
    QVERIFY2(list, qPrintable(engine.lastError()));
    auto* listItem = qobject_cast<QQuickItem*>(list.get());
    QVERIFY(listItem);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window, TestTimeout::mediumMs()));
    const auto buttons = [&]() {
        return findItems(listItem, [](QQuickItem* item) {
            return item->inherits("QQuickAbstractButton") && item->isVisible() && item->width() > 0;
        });
    };
    QTRY_COMPARE_WITH_TIMEOUT(buttons().size(), 2, TestTimeout::mediumMs());
    const auto right = [listItem](QQuickItem* button) {
        return button->mapToItem(listItem, QPointF(button->width(), 0)).x();
    };
    QTRY_COMPARE_WITH_TIMEOUT(right(buttons().at(0)), right(buttons().at(1)), TestTimeout::mediumMs());
}
