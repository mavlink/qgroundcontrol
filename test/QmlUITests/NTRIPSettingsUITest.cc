#include "NTRIPSettingsUITest.h"

#include <functional>
#include <memory>

#include <QtCore/QCoreApplication>
#include <QtCore/QEvent>
#include <QtCore/QPointer>
#include <QtCore/QRegularExpression>
#include <QtQuick/QQuickItem>
#include <QtQuick/QQuickWindow>
#include <QtQuickTest/quicktest.h>
#include <QtTest/QTest>

#include "Fact.h"
#include "GPSQmlTestHelpers.h"
#include "ManualScheduler.h"
#include "MockNTRIPTransport.h"
#include "NTRIPManager.h"
#include "NTRIPSettings.h"
#include "NTRIPSourceTable.h"
#include "SettingsManager.h"

UT_REGISTER_TEST(NTRIPSettingsUITest, TestLabel::Integration)

namespace {
/// An NTRIP manager whose caster connections are scripted; reconnect timers fire only when a test advances them.
struct ScriptedNTRIP
{
    ScriptedNTRIP()
    {
        NTRIPManager::Configuration configuration;
        configuration.enabled = true;
        configuration.stream.connection.host = QStringLiteral("caster.example.com");
        configuration.stream.connection.port = 2101;
        configuration.stream.connection.mountpoint = QStringLiteral("TEST");
        manager.setConfiguration(configuration);
    }

    /// The transport the next connection attempt opens.
    MockNTRIPTransport* nextTransport(bool autoConnect = false)
    {
        auto* transport = new MockNTRIPTransport(&manager);
        transport->autoConnect = autoConnect;
        manager.setTransportForTest(transport);
        return transport;
    }

    ManualScheduler scheduler;
    NTRIPManager manager{nullptr, &scheduler};
};

/// Transport signals reach the manager through queued connections.
void deliverTransportSignals()
{
    QCoreApplication::sendPostedEvents(nullptr, QEvent::MetaCall);
}

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
    const auto fail = [this](MockNTRIPTransport* transport) {
        expectLogMessage("GPS.NTRIP.NTRIPManager", QtWarningMsg,
                         QRegularExpression(QStringLiteral("NTRIP error:.*Connection failed")));
        transport->simulateError(NTRIPError::AuthFailed, QStringLiteral("Connection failed"));
        deliverTransportSignals();
        verifyExpectedLogMessage();
    };
    ScriptedNTRIP ntrip;
    MockNTRIPTransport* first = ntrip.nextTransport();
    ntrip.manager.init();
    fail(first);
    QCOMPARE(ntrip.manager.connectionStatus(), NTRIPManager::ConnectionStatus::Error);
    QPointer<MockNTRIPTransport> retry = ntrip.nextTransport();

    QQuickWindow window;
    window.resize(640, 400);
    GPSTestHelpers::QmlEngine engine;
    Fact enabled(0, QStringLiteral("enabled"), FactMetaData::valueTypeBool);
    enabled.setRawValue(true);
    std::unique_ptr<QObject> panel =
        engine.create(QUrl(QStringLiteral("qrc:/qml/QGroundControl/AppSettings/NTRIPConnectionSettings.qml")),
                      {{QStringLiteral("parent"), QVariant::fromValue(window.contentItem())},
                       {QStringLiteral("visible"), true},
                       {QStringLiteral("_ntripMgr"), QVariant::fromValue(&ntrip.manager)},
                       {QStringLiteral("_enabled"), QVariant::fromValue(&enabled)}});
    QVERIFY2(panel, qPrintable(engine.lastError()));
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window, TestTimeout::mediumMs()));
    auto* button = panel->findChild<QObject*>(QStringLiteral("ntripConnectButton"));
    auto* disconnect = panel->findChild<QObject*>(QStringLiteral("ntripDisconnectButton"));
    QVERIFY(button);
    QVERIFY(disconnect);
    QCOMPARE(button->property("text").toString(), QStringLiteral("Retry"));
    QTRY_VERIFY_WITH_TIMEOUT(disconnect->property("visible").toBool(), TestTimeout::shortMs());
    QVERIFY(QMetaObject::invokeMethod(button, "click"));
    QVERIFY(retry);
    QCOMPARE(retry->startCount, 1);
    QCOMPARE(first->startCount, 1);
    QCOMPARE(ntrip.manager.connectionStatus(), NTRIPManager::ConnectionStatus::Connecting);
    QVERIFY(enabled.rawValue().toBool());
    QVERIFY(!button->property("enabled").toBool());
    QTRY_VERIFY_WITH_TIMEOUT(!disconnect->property("visible").toBool(), TestTimeout::shortMs());
    fail(retry);
    QCOMPARE(ntrip.manager.connectionStatus(), NTRIPManager::ConnectionStatus::Error);
    QTRY_VERIFY_WITH_TIMEOUT(disconnect->property("visible").toBool(), TestTimeout::shortMs());
    QVERIFY(QMetaObject::invokeMethod(disconnect, "click"));
    QVERIFY(!enabled.rawValue().toBool());
    QCOMPARE(retry->startCount, 1);
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
    ScriptedNTRIP ntrip;
    if (status != static_cast<int>(NTRIPManager::ConnectionStatus::Disconnected)) {
        MockNTRIPTransport* transport = ntrip.nextTransport(true);
        ntrip.manager.init();
        if (status == static_cast<int>(NTRIPManager::ConnectionStatus::Reconnecting)) {
            expectLogMessage("GPS.NTRIP.NTRIPManager", QtWarningMsg,
                             QRegularExpression(QStringLiteral("NTRIP error:.*caster closed")));
            transport->simulateError(NTRIPError::ServerDisconnected, QStringLiteral("caster closed"));
            deliverTransportSignals();
            verifyExpectedLogMessage();
        }
    }
    QCOMPARE(static_cast<int>(ntrip.manager.connectionStatus()), status);
    // A click that started a connection attempt would open this transport.
    QPointer<MockNTRIPTransport> unexpected = ntrip.nextTransport();
    QQuickWindow window;
    window.resize(640, 400);
    GPSTestHelpers::QmlEngine engine;
    Fact enabled(0, QStringLiteral("enabled"), FactMetaData::valueTypeBool);
    enabled.setRawValue(enabledBefore);
    SettingsManager::instance()->ntripSettings()->ntripServerHostAddress()->setRawValue(
        QStringLiteral("caster.example.com"));
    std::unique_ptr<QObject> panel =
        engine.create(QUrl(QStringLiteral("qrc:/qml/QGroundControl/AppSettings/NTRIPConnectionSettings.qml")),
                      {{QStringLiteral("parent"), QVariant::fromValue(window.contentItem())},
                       {QStringLiteral("visible"), true},
                       {QStringLiteral("_ntripMgr"), QVariant::fromValue(&ntrip.manager)},
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
    QCOMPARE(static_cast<int>(ntrip.manager.connectionStatus()), status);
    QVERIFY(unexpected);
    QCOMPARE(unexpected->startCount, 0);
}

void NTRIPSettingsUITest::_testMountpointLockedWhileActive()
{
    GPSTestHelpers::QmlEngine engine;
    std::unique_ptr<QObject> panel =
        engine.create(QUrl(QStringLiteral("qrc:/qml/QGroundControl/AppSettings/NTRIPMountpointBrowser.qml")),
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
    const auto fail = [this](MockNTRIPTransport* transport, const QString& detail) {
        expectLogMessage("GPS.NTRIP.NTRIPManager", QtWarningMsg, QRegularExpression(QStringLiteral("NTRIP error:")));
        transport->simulateError(NTRIPError::AuthFailed, detail);
        deliverTransportSignals();
        verifyExpectedLogMessage();
    };
    ScriptedNTRIP ntrip;
    MockNTRIPTransport* first = ntrip.nextTransport();
    ntrip.manager.init();
    fail(first, QStringLiteral("Connection failed"));
    QCOMPARE(ntrip.manager.connectionStatus(), NTRIPManager::ConnectionStatus::Error);
    QQuickWindow window;
    window.resize(640, 400);
    GPSTestHelpers::QmlEngine engine;
    Fact enabled(0, QStringLiteral("enabled"), FactMetaData::valueTypeBool);
    enabled.setRawValue(true);
    std::unique_ptr<QObject> panel =
        engine.create(QUrl(QStringLiteral("qrc:/qml/QGroundControl/AppSettings/NTRIPConnectionSettings.qml")),
                      {{QStringLiteral("parent"), QVariant::fromValue(window.contentItem())},
                       {QStringLiteral("visible"), true},
                       {QStringLiteral("_ntripMgr"), QVariant::fromValue(&ntrip.manager)},
                       {QStringLiteral("_enabled"), QVariant::fromValue(&enabled)}});
    QVERIFY2(panel, qPrintable(engine.lastError()));
    auto* panelItem = qobject_cast<QQuickItem*>(panel.get());
    QVERIFY(panelItem);
    panelItem->setWidth(480);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window, TestTimeout::mediumMs()));
    const qreal shortWidth = panelItem->implicitWidth();

    const QString longMessage = QStringLiteral("certificate rejected ").repeated(20);
    MockNTRIPTransport* retry = ntrip.nextTransport();
    ntrip.manager.retryNTRIP();
    fail(retry, longMessage);
    QCOMPARE(ntrip.manager.connectionStatus(), NTRIPManager::ConnectionStatus::Error);
    QCOMPARE(ntrip.manager.statusMessage(), longMessage);
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
    GPSTestHelpers::QmlEngine engine;
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
