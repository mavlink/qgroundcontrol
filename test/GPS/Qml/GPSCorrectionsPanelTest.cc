#include "GPSCorrectionsPanelTest.h"

#include <chrono>
#include <memory>

#include <QtCore/QPointer>
#include <QtCore/QRegularExpression>
#include <QtPositioning/QGeoCoordinate>
#include <QtQuick/QQuickItem>
#include <QtQuickTest/quicktest.h>
#include <QtTest/QSignalSpy>

#include "Fact.h"
#include "Fixtures/RAIIFixtures.h"
#include "GPSCorrectionManager.h"
#include "ManualScheduler.h"
#include "NTRIP/Support/MockNTRIPTransport.h"
#include "NTRIPManager.h"
#include "NTRIPSettings.h"
#include "NTRIPSourceTable.h"
#include "NTRIPSourceTableController.h"
#include "Protocols/Support/ProtocolTestPackets.h"
#include "QGCFormat.h"
#include "SettingsManager.h"
#include "Support/GPSQmlTestHelpers.h"
#include "Support/GPSTestHelpers.h"
#include "UnitsSettings.h"

using namespace GPSTest;

namespace {
/// An NTRIP manager whose caster connections are scripted; reconnect timers fire only when a test advances them.
struct ScriptedNTRIP
{
    ScriptedNTRIP() { manager.setConfiguration(mockCasterConfiguration()); }

    /// The transport the next connection attempt opens.
    MockNTRIPTransport* nextTransport(bool autoConnect = false) { return injectMockTransport(manager, autoConnect); }

    ManualScheduler scheduler;
    NTRIPManager manager{nullptr, &scheduler};
};

/// The NTRIP connection panel for @a manager inside @a window; null on failure, with the reason in lastError().
std::unique_ptr<QObject> createConnectionPanel(GPSTest::QmlEngine& engine, const GPSTest::QmlWindowFixture& window,
                                               NTRIPManager& manager, Fact& enabled)
{
    return engine.create(GPSTest::sourceQmlUrl(QStringLiteral("GPS/Qml/NTRIPConnectionStatus.qml")),
                         {{QStringLiteral("parent"), QVariant::fromValue(window.contentItem())},
                          {QStringLiteral("visible"), true},
                          {QStringLiteral("ntripManager"), QVariant::fromValue(&manager)},
                          {QStringLiteral("enabledFact"), QVariant::fromValue(&enabled)}});
}
}  // namespace

void GPSCorrectionsPanelTest::_correctionsStatusShowsSelectedStream()
{
    ManualScheduler scheduler;
    GPSCorrectionManager corrections(nullptr, &scheduler);
    const auto advanceHealthSample = [&scheduler] { return scheduler.advanceBy(std::chrono::seconds(1)); };
    GPSTest::QmlEngine engine;
    std::unique_ptr<QObject> status =
        engine.create(GPSTest::sourceQmlUrl(QStringLiteral("GPS/Qml/CorrectionsStatus.qml")),
                      {{QStringLiteral("corrections"), QVariant::fromValue(&corrections)}});
    QVERIFY2(status, qPrintable(engine.lastError()));
    auto* source = status->findChild<QObject*>(QStringLiteral("correctionsSelectedSource"));
    auto* stream = status->findChild<QObject*>(QStringLiteral("correctionsSelectedStream"));
    auto* rate = status->findChild<QObject*>(QStringLiteral("correctionsDataRate"));
    QVERIFY(source && stream && rate);
    QCOMPARE(source->property("labelText").toString(), QStringLiteral("None"));
    QVERIFY(!rate->property("visible").toBool());

    const QString instance = QStringLiteral("ntrip://caster.example.com:2101/MOUNT");
    // A stream is listed once it delivers corrections.
    auto ntrip = corrections.openSource(GPSCorrectionSettings::Ntrip, instance);
    const auto frame = GPSTest::rtcmMessage(1005, 20);
    QVERIFY(advanceHealthSample());
    ntrip->submit(frame, scheduler.nowMs());
    QVERIFY(advanceHealthSample());
    QCOMPARE(corrections.selectedStream().source, static_cast<int>(GPSCorrectionSettings::Ntrip));
    QCOMPARE(corrections.selectedBytesPerSecond(), quint64(frame.size()));
    QCOMPARE(source->property("labelText").toString(),
             GPSCorrectionManager::sourceName(static_cast<int>(GPSCorrectionSettings::Ntrip)));
    QVERIFY(stream->property("visible").toBool());
    QCOMPARE(stream->property("text").toString(), instance);
    QVERIFY(rate->property("visible").toBool());
    QCOMPARE(rate->property("labelText").toString(), QStringLiteral("%1/s").arg(QGC::bigSizeToString(frame.size())));

    ntrip.reset();
    QVERIFY(advanceHealthSample());
    QCOMPARE(corrections.state(), GPSCorrectionManager::State::Inactive);
    QCOMPARE(corrections.selectedBytesPerSecond(), 0ULL);
    QCOMPARE(source->property("labelText").toString(), QStringLiteral("None"));
}

void GPSCorrectionsPanelTest::_correctionsStatusCountsMessageTypes()
{
    using namespace std::chrono_literals;
    ManualScheduler scheduler;
    GPSCorrectionManager corrections(nullptr, &scheduler);
    GPSTest::QmlEngine engine;
    std::unique_ptr<QObject> status =
        engine.create(GPSTest::sourceQmlUrl(QStringLiteral("GPS/Qml/CorrectionsStatus.qml")),
                      {{QStringLiteral("corrections"), QVariant::fromValue(&corrections)}});
    QVERIFY2(status, qPrintable(engine.lastError()));
    auto* types = status->findChild<QQuickItem*>(QStringLiteral("correctionsMessageTypes"));
    QVERIFY(types);
    QVERIFY(!types->property("visible").toBool());
    const auto chips = [types]() {
        return GPSTest::findItems(types, [](QQuickItem* item) { return item->property("messageId").isValid(); });
    };
    const auto chipTexts = [&chips]() {
        QStringList texts;
        for (const QQuickItem* chip : chips()) {
            texts.append(chip->childItems().constFirst()->property("text").toString());
        }
        return texts;
    };
    const auto submit = [&scheduler](const std::unique_ptr<GPSCorrectionSourceHandle>& source, uint16_t messageId) {
        source->submit(GPSTest::rtcmMessage(messageId, 20), scheduler.nowMs());
    };

    // Every source is counted, not only NTRIP.
    const auto udp = corrections.openSource(GPSCorrectionSettings::Udp, QStringLiteral("192.0.2.1"));
    for (const uint16_t messageId : {1077, 1005, 1077}) {
        submit(udp, messageId);
    }
    QVERIFY(scheduler.advanceBy(100ms));
    QVERIFY(types->property("visible").toBool());
    QCOMPARE(chipTexts(), QStringList({QStringLiteral(u"1005 \u00d7 1"), QStringLiteral(u"1077 \u00d7 2")}));

    // Another frame changes the count on its chip. A rebuilt list would show new chips while the old ones await
    // deletion.
    const QList<QQuickItem*> shown = chips();
    submit(udp, 1077);
    QCOMPARE(chips(), shown);
    QCOMPARE(chipTexts(), QStringList({QStringLiteral(u"1005 \u00d7 1"), QStringLiteral(u"1077 \u00d7 3")}));

    // A stream that is not selected is not counted; once it is, the counts are its own.
    const auto ntrip = corrections.openSource(GPSCorrectionSettings::Ntrip, QStringLiteral("caster/MOUNT"));
    submit(ntrip, 1087);
    QCOMPARE(chips(), shown);
    QVERIFY(scheduler.advanceBy(GPSCorrectionSelector::SWITCH_HOLD_DOWN));
    submit(ntrip, 1087);
    QCOMPARE(chipTexts(), QStringList({QStringLiteral(u"1087 \u00d7 1")}));

    // A stream that goes stale is no longer selected, so its counts do not outlive it.
    QVERIFY(scheduler.advanceBy(GPSCorrectionSelector::FRESHNESS_TIMEOUT + 1s));
    QCOMPARE(corrections.state(), GPSCorrectionManager::State::Waiting);
    QVERIFY(!types->property("visible").toBool());
    QVERIFY(chips().isEmpty());
    submit(ntrip, 1087);
    QCOMPARE(chipTexts(), QStringList({QStringLiteral(u"1087 \u00d7 1")}));
}

void GPSCorrectionsPanelTest::_correctionsStatusShowsBaseStation()
{
    using namespace std::chrono_literals;
    using State = GPSCorrectionManager::State;
    TestFixtures::SettingsFixture saved;
    saved.setFactValue(SettingsManager::instance()->unitsSettings()->horizontalDistanceUnits(),
                       UnitsSettings::HorizontalDistanceUnitsMeters);
    ManualScheduler scheduler;
    GPSCorrectionManager corrections(nullptr, &scheduler);
    QSignalSpy positionChanges(&corrections, &GPSCorrectionManager::basePositionChanged);
    GPSTest::QmlEngine engine;
    std::unique_ptr<QObject> status =
        engine.create(GPSTest::sourceQmlUrl(QStringLiteral("GPS/Qml/CorrectionsStatus.qml")),
                      {{QStringLiteral("corrections"), QVariant::fromValue(&corrections)},
                       {QStringLiteral("gcsPosition"), QVariant::fromValue(QGeoCoordinate())}});
    QVERIFY2(status, qPrintable(engine.lastError()));
    auto* position = status->findChild<QObject*>(QStringLiteral("correctionsBasePosition"));
    auto* distance = status->findChild<QObject*>(QStringLiteral("correctionsBaseDistance"));
    QVERIFY(position && distance);
    // The RTCM 1005 frame of the recording Core/Protocols/fixtures/mixed.gps.
    const QByteArray stationFrame = QByteArray::fromHex("d300133ed000038a58d9493c872f34109d07d6af48205ad7f7");
    const QByteArray observations = GPSTest::rtcmMessage(1077, 20);

    // A stream shows no base station before it announces one.
    const auto ntrip = corrections.openSource(GPSCorrectionSettings::Ntrip, QStringLiteral("caster/MOUNT"));
    ntrip->submit(observations, scheduler.nowMs());
    QVERIFY(scheduler.advanceBy(100ms));
    QCOMPARE(corrections.state(), State::Fresh);
    QVERIFY(!position->property("visible").toBool());

    // A repeated announcement of the same position does not notify again.
    ntrip->submit(stationFrame, scheduler.nowMs());
    ntrip->submit(stationFrame, scheduler.nowMs());
    QCOMPARE(positionChanges.size(), 1);
    QVERIFY(position->property("visible").toBool());
    QCOMPARE(position->property("labelText").toString(), QStringLiteral("32.0658325, 34.7738190"));
    // The distance needs a ground station position.
    QVERIFY(!distance->property("visible").toBool());
    QVERIFY(status->setProperty("gcsPosition",
                                QVariant::fromValue(corrections.basePosition().atDistanceAndAzimuth(2500, 90))));
    QVERIFY(distance->property("visible").toBool());
    QCOMPARE(distance->property("labelText").toString(), QStringLiteral("2500 m"));
    // A panel that hides the coordinates, as the toolbar drop-down does, keeps the distance.
    QVERIFY(status->setProperty("showCoordinates", false));
    QVERIFY(!position->property("visible").toBool());
    QVERIFY(distance->property("visible").toBool());
    QVERIFY(status->setProperty("showCoordinates", true));

    // A stream that goes stale is no longer selected, and its base station is forgotten with it.
    QVERIFY(scheduler.advanceBy(GPSCorrectionSelector::FRESHNESS_TIMEOUT + 1s));
    QCOMPARE(corrections.state(), State::Waiting);
    QVERIFY(!corrections.basePosition().isValid());
    QVERIFY(!position->property("visible").toBool());
    QVERIFY(!distance->property("visible").toBool());
    ntrip->submit(stationFrame, scheduler.nowMs());
    QVERIFY(scheduler.advanceBy(100ms));
    QVERIFY(position->property("visible").toBool());

    // The stream selected next has its own base station, which it has not announced.
    const auto local = corrections.openSource(GPSCorrectionSettings::LocalReceiver, {});
    local->submit(observations, scheduler.nowMs());
    QVERIFY(corrections.basePosition().isValid());
    QVERIFY(scheduler.advanceBy(GPSCorrectionSelector::SWITCH_HOLD_DOWN));
    local->submit(observations, scheduler.nowMs());
    QVERIFY(!corrections.basePosition().isValid());
    QVERIFY(scheduler.advanceBy(100ms));
    QCOMPARE(corrections.selectedStream().source, static_cast<int>(GPSCorrectionSettings::LocalReceiver));
    QVERIFY(!position->property("visible").toBool());
}

void GPSCorrectionsPanelTest::_ntripErrorActionRetries()
{
    const auto fail = [this](MockNTRIPTransport* transport) {
        expectLogMessage("GPS.NTRIPManager", QtWarningMsg,
                         QRegularExpression(QStringLiteral("NTRIP error:.*Connection failed")));
        transport->simulateError(NTRIPError::AuthFailed, QStringLiteral("Connection failed"));
        deliverQueuedCalls();
        verifyExpectedLogMessage();
    };
    ScriptedNTRIP ntrip;
    // The manager deletes a failed transport when it retries.
    QPointer<MockNTRIPTransport> first = ntrip.nextTransport();
    ntrip.manager.init();
    fail(first);
    QCOMPARE(ntrip.manager.connectionStatus(), NTRIPManager::ConnectionStatus::Error);
    QPointer<MockNTRIPTransport> retry = ntrip.nextTransport();

    GPSTest::QmlWindowFixture window(640, 400);
    GPSTest::QmlEngine engine;
    Fact enabled(0, QStringLiteral("enabled"), FactMetaData::valueTypeBool);
    enabled.setRawValue(true);
    std::unique_ptr<QObject> panel = createConnectionPanel(engine, window, ntrip.manager, enabled);
    QVERIFY2(panel, qPrintable(engine.lastError()));
    QVERIFY(window.show());
    auto* button = panel->findChild<QObject*>(QStringLiteral("ntripConnectButton"));
    auto* disconnect = panel->findChild<QObject*>(QStringLiteral("ntripDisconnectButton"));
    QVERIFY(button);
    QVERIFY(disconnect);
    QCOMPARE(button->property("text").toString(), QStringLiteral("Retry"));
    QTRY_VERIFY_WITH_TIMEOUT(disconnect->property("visible").toBool(), TestTimeout::shortMs());
    QVERIFY(QMetaObject::invokeMethod(button, "click"));
    QVERIFY(retry);
    QCOMPARE(retry->startCount, 1);
    // Retrying starts a new transport; the failed one is never restarted.
    QVERIFY(!first || first->startCount == 1);
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

void GPSCorrectionsPanelTest::_ntripConnectionActionIsIdempotent_data()
{
    QTest::addColumn<int>("status");
    QTest::addColumn<bool>("enabledBefore");
    QTest::addColumn<bool>("enabledAfter");
    QTest::newRow("disconnect") << static_cast<int>(NTRIPManager::ConnectionStatus::Connected) << true << false;
    QTest::newRow("cancel-reconnect") << static_cast<int>(NTRIPManager::ConnectionStatus::Reconnecting) << true
                                      << false;
    QTest::newRow("connect") << static_cast<int>(NTRIPManager::ConnectionStatus::Disconnected) << false << true;
}

void GPSCorrectionsPanelTest::_ntripConnectionActionIsIdempotent()
{
    QFETCH(int, status);
    QFETCH(bool, enabledBefore);
    QFETCH(bool, enabledAfter);
    ScriptedNTRIP ntrip;
    if (status != static_cast<int>(NTRIPManager::ConnectionStatus::Disconnected)) {
        MockNTRIPTransport* transport = ntrip.nextTransport(true);
        ntrip.manager.init();
        if (status == static_cast<int>(NTRIPManager::ConnectionStatus::Reconnecting)) {
            expectLogMessage("GPS.NTRIPManager", QtWarningMsg,
                             QRegularExpression(QStringLiteral("NTRIP error:.*caster closed")));
            transport->simulateError(NTRIPError::ServerDisconnected, QStringLiteral("caster closed"));
            deliverQueuedCalls();
            verifyExpectedLogMessage();
        }
    }
    QCOMPARE(static_cast<int>(ntrip.manager.connectionStatus()), status);
    // A click that started a connection attempt would open this transport.
    QPointer<MockNTRIPTransport> unexpected = ntrip.nextTransport();
    GPSTest::QmlWindowFixture window(640, 400);
    GPSTest::QmlEngine engine;
    Fact enabled(0, QStringLiteral("enabled"), FactMetaData::valueTypeBool);
    enabled.setRawValue(enabledBefore);
    TestFixtures::SettingsFixture saved;
    saved.setFactValue(SettingsManager::instance()->ntripSettings()->ntripServerHostAddress(),
                       QStringLiteral("caster.example.com"));
    std::unique_ptr<QObject> panel = createConnectionPanel(engine, window, ntrip.manager, enabled);
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

void GPSCorrectionsPanelTest::_ntripMountpointLockedWhileActive()
{
    // The application's setting would start its NTRIP connection.
    Fact enabled(0, QStringLiteral("enabled"), FactMetaData::valueTypeBool);
    enabled.setRawValue(true);
    GPSTest::QmlEngine engine;
    std::unique_ptr<QObject> panel =
        engine.create(QUrl(QStringLiteral("qrc:/qml/QGroundControl/GPS/NTRIPMountpointBrowser.qml")),
                      {{QStringLiteral("enabledFact"), QVariant::fromValue(&enabled)}});
    QVERIFY2(panel, qPrintable(engine.lastError()));
    auto* list = panel->findChild<QObject*>(QStringLiteral("ntripMountpointList"));
    auto* field = panel->findChild<QObject*>(QStringLiteral("ntripMountpointField"));
    QVERIFY(list && field);
    QVERIFY(!list->property("enabled").toBool());
    QVERIFY(!field->property("enabled").toBool());
    enabled.setRawValue(false);
    QVERIFY(list->property("enabled").toBool());
    QVERIFY(field->property("enabled").toBool());
}

void GPSCorrectionsPanelTest::_ntripLongStatusWrapsWithinPanel()
{
    const auto fail = [this](MockNTRIPTransport* transport, const QString& detail) {
        expectLogMessage("GPS.NTRIPManager", QtWarningMsg, QRegularExpression(QStringLiteral("NTRIP error:")));
        transport->simulateError(NTRIPError::AuthFailed, detail);
        deliverQueuedCalls();
        verifyExpectedLogMessage();
    };
    ScriptedNTRIP ntrip;
    MockNTRIPTransport* first = ntrip.nextTransport();
    ntrip.manager.init();
    fail(first, QStringLiteral("Connection failed"));
    QCOMPARE(ntrip.manager.connectionStatus(), NTRIPManager::ConnectionStatus::Error);
    GPSTest::QmlWindowFixture window(640, 400);
    GPSTest::QmlEngine engine;
    Fact enabled(0, QStringLiteral("enabled"), FactMetaData::valueTypeBool);
    enabled.setRawValue(true);
    std::unique_ptr<QObject> panel = createConnectionPanel(engine, window, ntrip.manager, enabled);
    QVERIFY2(panel, qPrintable(engine.lastError()));
    auto* panelItem = qobject_cast<QQuickItem*>(panel.get());
    QVERIFY(panelItem);
    panelItem->setWidth(480);
    QVERIFY(window.show());
    const qreal shortWidth = panelItem->implicitWidth();

    const QString longMessage = QStringLiteral("certificate rejected ").repeated(20);
    MockNTRIPTransport* retry = ntrip.nextTransport();
    ntrip.manager.retryNTRIP();
    fail(retry, longMessage);
    QCOMPARE(ntrip.manager.connectionStatus(), NTRIPManager::ConnectionStatus::Error);
    QCOMPARE(ntrip.manager.statusMessage(), longMessage);
    const auto statusLabel = [&]() -> QQuickItem* {
        const auto labels = GPSTest::findItems(panelItem, [&](QQuickItem* item) {
            return item->property("text").toString() == longMessage && item->property("lineCount").isValid();
        });
        return labels.isEmpty() ? nullptr : labels.first();
    };
    QTRY_VERIFY_WITH_TIMEOUT(statusLabel(), TestTimeout::mediumMs());
    QVERIFY(QQuickTest::qWaitForPolish(window.window(), TestTimeout::mediumMs()));
    // The message wraps inside the panel instead of widening every settings group on the page.
    QCOMPARE(panelItem->implicitWidth(), shortWidth);
    QVERIFY(statusLabel()->property("lineCount").toInt() > 1);
    const auto buttons = GPSTest::findItems(
        panelItem, [](QQuickItem* item) { return item->objectName() == QLatin1String("ntripConnectButton"); });
    QCOMPARE(buttons.size(), 1);
    const QPointF buttonRight = buttons.first()->mapToItem(panelItem, QPointF(buttons.first()->width(), 0));
    QVERIFY2(buttonRight.x() <= panelItem->width(), qPrintable(QString::number(buttonRight.x())));
}

void GPSCorrectionsPanelTest::_ntripMountpointButtonsAligned()
{
    NTRIPSourceTableModel model;
    model.parseSourceTable(QStringLiteral(
        "STR;LONG;Long;RTCM 3.3;1005(1),1077(1);2;GPS+GLO+GAL+BDS+SBAS+QZSS;NET;FRA;0;0;0;0;gen;none;B;N;15200;\r\n"
        "STR;S;Short;RTCM 3.3;1005(1);2;GPS;NET;FRA;0;0;0;0;gen;none;N;N;0;\r\n"
        "ENDSOURCETABLE\r\n"));
    QCOMPARE(model.rowCount(), 2);
    GPSTest::QmlWindowFixture window(640, 400);
    GPSTest::QmlEngine engine;
    std::unique_ptr<QObject> list =
        engine.create(QUrl(QStringLiteral("qrc:/qml/QGroundControl/GPS/NTRIPMountpointList.qml")),
                      {{QStringLiteral("parent"), QVariant::fromValue(window.contentItem())},
                       {QStringLiteral("width"), 480},
                       {QStringLiteral("height"), 300},
                       {QStringLiteral("model"), QVariant::fromValue(&model)}});
    QVERIFY2(list, qPrintable(engine.lastError()));
    auto* listItem = qobject_cast<QQuickItem*>(list.get());
    QVERIFY(listItem);
    QVERIFY(window.show());
    const auto buttons = [&]() {
        return GPSTest::findItems(listItem, [](QQuickItem* item) {
            return item->inherits("QQuickAbstractButton") && item->isVisible() && item->width() > 0;
        });
    };
    QTRY_COMPARE_WITH_TIMEOUT(buttons().size(), 2, TestTimeout::mediumMs());
    const auto right = [listItem](QQuickItem* button) {
        return button->mapToItem(listItem, QPointF(button->width(), 0)).x();
    };
    QTRY_COMPARE_WITH_TIMEOUT(right(buttons().at(0)), right(buttons().at(1)), TestTimeout::mediumMs());
}

UT_REGISTER_TEST(GPSCorrectionsPanelTest, TestLabel::Unit)
