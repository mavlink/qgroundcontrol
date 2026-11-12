#include "GPSCorrectionsPanelTest.h"

#include <chrono>
#include <memory>

#include <QtCore/QDir>
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
#include "QmlTesting/QmlFixtures.h"
#include "SettingsManager.h"
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
std::unique_ptr<QObject> createConnectionPanel(TestFixtures::QmlEngine& engine,
                                               const TestFixtures::QmlWindowFixture& window, NTRIPManager& manager)
{
    return engine.create(TestFixtures::sourceQmlUrl(QStringLiteral("GPS/Qml/NTRIPConnectionStatus.qml")),
                         {{QStringLiteral("parent"), QVariant::fromValue(window.contentItem())},
                          {QStringLiteral("visible"), true},
                          {QStringLiteral("ntripManager"), QVariant::fromValue(&manager)}});
}

/// Whether a label of @a panel shows @a text.
bool showsText(QObject* panel, const QString& text)
{
    return !TestFixtures::findItems(qobject_cast<QQuickItem*>(panel), [&text](QQuickItem* item) {
                return item->property("lineCount").isValid() && item->property("text").toString() == text;
            }).isEmpty();
}
}  // namespace

void GPSCorrectionsPanelTest::_correctionsStatusShowsSelectedStream()
{
    ManualScheduler scheduler;
    GPSCorrectionManager corrections(nullptr, &scheduler);
    const auto advanceHealthSample = [&scheduler] { return scheduler.advanceBy(std::chrono::seconds(1)); };
    TestFixtures::QmlEngine engine;
    std::unique_ptr<QObject> status =
        engine.create(TestFixtures::sourceQmlUrl(QStringLiteral("GPS/Qml/CorrectionsStatus.qml")),
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
    TestFixtures::QmlEngine engine;
    std::unique_ptr<QObject> status =
        engine.create(TestFixtures::sourceQmlUrl(QStringLiteral("GPS/Qml/CorrectionsStatus.qml")),
                      {{QStringLiteral("corrections"), QVariant::fromValue(&corrections)}});
    QVERIFY2(status, qPrintable(engine.lastError()));
    auto* types = status->findChild<QQuickItem*>(QStringLiteral("correctionsMessageTypes"));
    QVERIFY(types);
    QVERIFY(!types->property("visible").toBool());
    const auto chips = [types]() {
        return TestFixtures::findItems(types, [](QQuickItem* item) { return item->property("messageId").isValid(); });
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
    QCOMPARE(chipTexts(), QStringList({QStringLiteral(u"1005 \u00d7 1"), QStringLiteral(u"1077 \u00d7 3, 10 Hz")}));

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

void GPSCorrectionsPanelTest::_correctionsStatusShowsMessageRates()
{
    using namespace std::chrono_literals;
    ManualScheduler scheduler;
    GPSCorrectionManager corrections(nullptr, &scheduler);
    TestFixtures::QmlEngine engine;
    std::unique_ptr<QObject> status =
        engine.create(TestFixtures::sourceQmlUrl(QStringLiteral("GPS/Qml/CorrectionsStatus.qml")),
                      {{QStringLiteral("corrections"), QVariant::fromValue(&corrections)}});
    QVERIFY2(status, qPrintable(engine.lastError()));
    auto* types = status->findChild<QQuickItem*>(QStringLiteral("correctionsMessageTypes"));
    QVERIFY(types);
    const auto chipTexts = [types]() {
        QStringList texts;
        for (const QQuickItem* chip :
             TestFixtures::findItems(types, [](QQuickItem* item) { return item->property("messageId").isValid(); })) {
            texts.append(chip->childItems().constFirst()->property("text").toString());
        }
        return texts;
    };
    const auto ntrip = corrections.openSource(GPSCorrectionSettings::Ntrip, QStringLiteral("caster/MOUNT"));
    const auto submit = [&](uint16_t messageId) {
        ntrip->submit(GPSTest::rtcmMessage(messageId, 20), scheduler.nowMs());
    };

    // A type shows how often it arrives once a second message tells: in whole seconds, or per second from two a
    // second.
    submit(1005);
    for (int second = 0; second < 2; ++second) {
        submit(1077);
        for (int fifth = 0; fifth < 5; ++fifth) {
            submit(1087);
            QVERIFY(scheduler.advanceBy(200ms));
        }
    }
    submit(1077);
    QCOMPARE(chipTexts(), QStringList({QStringLiteral(u"1005 \u00d7 1"), QStringLiteral(u"1077 \u00d7 3, every 1 s"),
                                       QStringLiteral(u"1087 \u00d7 10, 5 Hz")}));

    // A type that stops arriving, while the stream goes on, keeps its count without a rate.
    for (int second = 0; second < 3; ++second) {
        QVERIFY(scheduler.advanceBy(1s));
        submit(1077);
    }
    QCOMPARE(corrections.state(), GPSCorrectionManager::State::Fresh);
    QCOMPARE(chipTexts(), QStringList({QStringLiteral(u"1005 \u00d7 1"), QStringLiteral(u"1077 \u00d7 6, every 1 s"),
                                       QStringLiteral(u"1087 \u00d7 10")}));
}

void GPSCorrectionsPanelTest::_correctionsStatusShowsBaseStation()
{
    using namespace std::chrono_literals;
    using State = GPSCorrectionManager::State;
    TestFixtures::SettingsFixture saved;
    Fact* const units = SettingsManager::instance()->unitsSettings()->horizontalDistanceUnits();
    saved.setFactValue(units, UnitsSettings::HorizontalDistanceUnitsMeters);
    ManualScheduler scheduler;
    GPSCorrectionManager corrections(nullptr, &scheduler);
    QSignalSpy positionChanges(&corrections, &GPSCorrectionManager::basePositionChanged);
    TestFixtures::QmlEngine engine;
    std::unique_ptr<QObject> status =
        engine.create(TestFixtures::sourceQmlUrl(QStringLiteral("GPS/Qml/CorrectionsStatus.qml")),
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
    QCOMPARE(distance->property("labelText").toString(), QStringLiteral("2.5 km"));
    // The distance follows the units setting while it is shown.
    units->setRawValue(UnitsSettings::HorizontalDistanceUnitsFeet);
    QCOMPARE(distance->property("labelText").toString(), QStringLiteral("1.6 mi"));
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

void GPSCorrectionsPanelTest::_correctionsStatusBaseDistanceUnits_data()
{
    QTest::addColumn<int>("units");
    QTest::addColumn<double>("meters");
    QTest::addColumn<QString>("shown");
    const auto row = [](const char* name, UnitsSettings::HorizontalDistanceUnits units, double meters,
                        const char* shown) {
        QTest::newRow(name) << int(units) << meters << QString::fromLatin1(shown);
    };
    constexpr double metersPerMile = 1609.344;
    // Whole meters or feet up to a kilometer or a mile, then those with one decimal, and whole from 100 on.
    row("meters", UnitsSettings::HorizontalDistanceUnitsMeters, 999, "999 m");
    row("kilometer", UnitsSettings::HorizontalDistanceUnitsMeters, 1000, "1.0 km");
    row("kilometers", UnitsSettings::HorizontalDistanceUnitsMeters, 99900, "99.9 km");
    row("whole-kilometers", UnitsSettings::HorizontalDistanceUnitsMeters, 100000, "100 km");
    row("feet", UnitsSettings::HorizontalDistanceUnitsFeet, 1609, "5279 ft");
    row("mile", UnitsSettings::HorizontalDistanceUnitsFeet, metersPerMile, "1.0 mi");
    row("miles", UnitsSettings::HorizontalDistanceUnitsFeet, 99.9 * metersPerMile, "99.9 mi");
    row("whole-miles", UnitsSettings::HorizontalDistanceUnitsFeet, 100 * metersPerMile, "100 mi");
}

void GPSCorrectionsPanelTest::_correctionsStatusBaseDistanceUnits()
{
    using namespace std::chrono_literals;
    QFETCH(int, units);
    QFETCH(double, meters);
    QFETCH(QString, shown);
    TestFixtures::SettingsFixture saved;
    saved.setFactValue(SettingsManager::instance()->unitsSettings()->horizontalDistanceUnits(), units);
    ManualScheduler scheduler;
    GPSCorrectionManager corrections(nullptr, &scheduler);
    const auto ntrip = corrections.openSource(GPSCorrectionSettings::Ntrip, QStringLiteral("caster/MOUNT"));
    // The RTCM 1005 frame of the recording Core/Protocols/fixtures/mixed.gps.
    ntrip->submit(QByteArray::fromHex("d300133ed000038a58d9493c872f34109d07d6af48205ad7f7"), scheduler.nowMs());
    QVERIFY(scheduler.advanceBy(100ms));
    QVERIFY(corrections.basePosition().isValid());
    TestFixtures::QmlEngine engine;
    std::unique_ptr<QObject> status =
        engine.create(TestFixtures::sourceQmlUrl(QStringLiteral("GPS/Qml/CorrectionsStatus.qml")),
                      {{QStringLiteral("corrections"), QVariant::fromValue(&corrections)},
                       {QStringLiteral("gcsPosition"),
                        QVariant::fromValue(corrections.basePosition().atDistanceAndAzimuth(meters, 90))}});
    QVERIFY2(status, qPrintable(engine.lastError()));
    auto* distance = status->findChild<QObject*>(QStringLiteral("correctionsBaseDistance"));
    QVERIFY(distance && distance->property("visible").toBool());
    QCOMPARE(distance->property("labelText").toString(), shown);
}

void GPSCorrectionsPanelTest::_correctionsStatusShowsMessageFilterError()
{
    ManualScheduler scheduler;
    GPSCorrectionManager corrections(nullptr, &scheduler);
    TestFixtures::QmlEngine engine;
    std::unique_ptr<QObject> status =
        engine.create(TestFixtures::sourceQmlUrl(QStringLiteral("GPS/Qml/CorrectionsStatus.qml")),
                      {{QStringLiteral("corrections"), QVariant::fromValue(&corrections)}});
    QVERIFY2(status, qPrintable(engine.lastError()));
    auto* error = status->findChild<QObject*>(QStringLiteral("correctionsMessageFilterError"));
    QVERIFY(error);
    QVERIFY(!error->property("visible").toBool());

    // A filter that routes nothing says why, instead of only showing a stream that never becomes fresh.
    expectLogMessage("GPS.Corrections.GPSCorrectionManager", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Invalid RTCM message filter entry")));
    corrections.setConfiguration({.udpInput = {}, .udpOutput = {}, .messageFilter = QStringLiteral("1005 1077")});
    verifyExpectedLogMessage();
    QVERIFY(error->property("visible").toBool());
    QCOMPARE(error->property("text").toString(), corrections.messageFilterError());
    QVERIFY(error->property("text").toString().contains(QStringLiteral("\"1005 1077\"")));

    corrections.setConfiguration({.udpInput = {}, .udpOutput = {}, .messageFilter = QStringLiteral("1005,1077")});
    QVERIFY(!error->property("visible").toBool());
}

void GPSCorrectionsPanelTest::_correctionsLogSwitch()
{
    TestFixtures::TempDirFixture directory;
    QString folder = directory.path();
    ManualScheduler scheduler;
    GPSCorrectionManager corrections(nullptr, &scheduler, {.logDirectory = [&folder] { return folder; }});
    TestFixtures::QmlEngine engine;
    std::unique_ptr<QObject> log =
        engine.create(TestFixtures::sourceQmlUrl(QStringLiteral("GPS/Qml/CorrectionsLogSwitch.qml")),
                      {{QStringLiteral("corrections"), QVariant::fromValue(&corrections)}});
    QVERIFY2(log, qPrintable(engine.lastError()));
    auto* toggle = log->findChild<QObject*>(QStringLiteral("correctionsLogSwitch"));
    auto* file = log->findChild<QObject*>(QStringLiteral("correctionsLogFile"));
    auto* error = log->findChild<QObject*>(QStringLiteral("correctionsLogError"));
    QVERIFY(toggle && file && error);
    QVERIFY(!toggle->property("checked").toBool());
    QVERIFY(!file->property("visible").toBool());
    QVERIFY(!error->property("visible").toBool());

    QVERIFY(QMetaObject::invokeMethod(toggle, "click"));
    QVERIFY(corrections.logToFile());
    QVERIFY(corrections.logFile().startsWith(folder));
    QVERIFY(file->property("visible").toBool());
    QCOMPARE(file->property("text").toString(),
             QCoreApplication::translate("CorrectionsLogSwitch", "Log file: %1").arg(corrections.logFile()));

    // The switch follows the log: one that ends, as at its size limit, turns it off and is no longer named.
    corrections.setLogToFile(false);
    QVERIFY(!toggle->property("checked").toBool());
    QVERIFY(!file->property("visible").toBool());

    // A log that cannot start, here for a regular file where its folder should be, leaves the switch off and says
    // why.
    folder = directory.createFile(QStringLiteral("GPSCorrections"), "not a folder");
    expectLogMessage("GPS.Corrections.RTCMLogFile", QtWarningMsg, QRegularExpression(QStringLiteral("file error")));
    QVERIFY(QMetaObject::invokeMethod(toggle, "click"));
    verifyExpectedLogMessage();
    QVERIFY(!corrections.logToFile());
    QVERIFY(!toggle->property("checked").toBool());
    QVERIFY(!file->property("visible").toBool());
    QVERIFY(error->property("visible").toBool());
    QVERIFY(!corrections.logError().isEmpty());
    QCOMPARE(error->property("text").toString(), corrections.logError());

    // The reason goes once a log starts. A log started within the same millisecond would be named as the first one,
    // so this one goes to a folder of its own.
    folder = QDir(directory.path()).filePath(QStringLiteral("next"));
    QVERIFY(QMetaObject::invokeMethod(toggle, "click"));
    QVERIFY(toggle->property("checked").toBool());
    QVERIFY(!error->property("visible").toBool());
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
    // What the panel's actions ask the connection setting to be.
    QSignalSpy requests(&ntrip.manager, &NTRIPManager::enabledRequested);

    TestFixtures::QmlWindowFixture window(640, 400);
    TestFixtures::QmlEngine engine;
    std::unique_ptr<QObject> panel = createConnectionPanel(engine, window, ntrip.manager);
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
    QCOMPARE(requests.size(), 1);
    QVERIFY(requests.constLast().constFirst().toBool());
    // The attempt can be stopped by the row's own action, so the second one goes.
    QCOMPARE(button->property("text").toString(), QStringLiteral("Disconnect"));
    QVERIFY(button->property("enabled").toBool());
    QTRY_VERIFY_WITH_TIMEOUT(!disconnect->property("visible").toBool(), TestTimeout::shortMs());
    fail(retry);
    QCOMPARE(ntrip.manager.connectionStatus(), NTRIPManager::ConnectionStatus::Error);
    QTRY_VERIFY_WITH_TIMEOUT(disconnect->property("visible").toBool(), TestTimeout::shortMs());
    QVERIFY(QMetaObject::invokeMethod(disconnect, "click"));
    QCOMPARE(requests.size(), 2);
    QVERIFY(!requests.constLast().constFirst().toBool());
    QCOMPARE(retry->startCount, 1);
}

void GPSCorrectionsPanelTest::_ntripRetryNeedsCaster()
{
    ScriptedNTRIP ntrip;
    // A connection that is on without a caster host fails as it starts, and stays on.
    NTRIPManager::Configuration configuration = ntrip.manager.configuration();
    configuration.connection.host.clear();
    ntrip.manager.setConfiguration(configuration);
    expectLogMessage("GPS.NTRIPManager", QtWarningMsg, QRegularExpression(QStringLiteral("NTRIP config invalid")));
    ntrip.manager.init();
    verifyExpectedLogMessage();
    QCOMPARE(ntrip.manager.connectionStatus(), NTRIPManager::ConnectionStatus::Error);

    TestFixtures::QmlWindowFixture window(640, 400);
    TestFixtures::QmlEngine engine;
    std::unique_ptr<QObject> panel = createConnectionPanel(engine, window, ntrip.manager);
    QVERIFY2(panel, qPrintable(engine.lastError()));
    auto* button = panel->findChild<QObject*>(QStringLiteral("ntripConnectButton"));
    auto* disconnect = panel->findChild<QObject*>(QStringLiteral("ntripDisconnectButton"));
    QVERIFY(button && disconnect);
    // Retrying would fail the same way; turning the connection off unlocks the settings that name a caster.
    QCOMPARE(button->property("text").toString(), QStringLiteral("Retry"));
    QVERIFY(!button->property("enabled").toBool());
    QVERIFY(disconnect->property("enabled").toBool());
    ntrip.manager.setConfiguration(mockCasterConfiguration());
    QVERIFY(button->property("enabled").toBool());
}

void GPSCorrectionsPanelTest::_ntripConnectionActionIsIdempotent_data()
{
    QTest::addColumn<int>("status");
    // What the action asks the connection setting to be.
    QTest::addColumn<bool>("requested");
    QTest::newRow("disconnect") << static_cast<int>(NTRIPManager::ConnectionStatus::Connected) << false;
    QTest::newRow("stop-connecting") << static_cast<int>(NTRIPManager::ConnectionStatus::Connecting) << false;
    QTest::newRow("cancel-reconnect") << static_cast<int>(NTRIPManager::ConnectionStatus::Reconnecting) << false;
    QTest::newRow("connect") << static_cast<int>(NTRIPManager::ConnectionStatus::Disconnected) << true;
}

void GPSCorrectionsPanelTest::_ntripConnectionActionIsIdempotent()
{
    QFETCH(int, status);
    QFETCH(bool, requested);
    ScriptedNTRIP ntrip;
    if (status != static_cast<int>(NTRIPManager::ConnectionStatus::Disconnected)) {
        // A caster that does not answer leaves the attempt connecting.
        MockNTRIPTransport* transport =
            ntrip.nextTransport(status != static_cast<int>(NTRIPManager::ConnectionStatus::Connecting));
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
    TestFixtures::QmlWindowFixture window(640, 400);
    TestFixtures::QmlEngine engine;
    // Whether there is a caster to connect to is the manager's to say, whatever the application's settings hold.
    TestFixtures::SettingsFixture saved;
    saved.setFactValue(SettingsManager::instance()->ntripSettings()->ntripServerHostAddress(), QString());
    std::unique_ptr<QObject> panel = createConnectionPanel(engine, window, ntrip.manager);
    QVERIFY2(panel, qPrintable(engine.lastError()));
    auto* button = panel->findChild<QObject*>(QStringLiteral("ntripConnectButton"));
    QVERIFY(button);
    QVERIFY(button->property("enabled").toBool());
    // The row shows the manager's message alone, which names a state that has no more to say.
    QVERIFY(!ntrip.manager.statusMessage().isEmpty());
    QVERIFY(showsText(panel.get(), ntrip.manager.statusMessage()));
    QSignalSpy requests(&ntrip.manager, &NTRIPManager::enabledRequested);
    // The manager applies the setting after a debounce, so its status still reflects the first click.
    for (int click = 0; click < 2; ++click) {
        QVERIFY(QMetaObject::invokeMethod(button, "click"));
        QCOMPARE(requests.size(), click + 1);
        QCOMPARE(requests.constLast().constFirst().toBool(), requested);
    }
    QCOMPARE(static_cast<int>(ntrip.manager.connectionStatus()), status);
    QVERIFY(unexpected);
    QCOMPARE(unexpected->startCount, 0);
}

void GPSCorrectionsPanelTest::_ntripDoubleClickActsOnce()
{
    ScriptedNTRIP ntrip;
    QPointer<MockNTRIPTransport> first = ntrip.nextTransport();
    ntrip.manager.init();
    expectLogMessage("GPS.NTRIPManager", QtWarningMsg,
                     QRegularExpression(QStringLiteral("NTRIP error:.*Connection failed")));
    first->simulateError(NTRIPError::AuthFailed, QStringLiteral("Connection failed"));
    deliverQueuedCalls();
    verifyExpectedLogMessage();
    QCOMPARE(ntrip.manager.connectionStatus(), NTRIPManager::ConnectionStatus::Error);
    QPointer<MockNTRIPTransport> retry = ntrip.nextTransport();
    QSignalSpy requests(&ntrip.manager, &NTRIPManager::enabledRequested);

    TestFixtures::QmlWindowFixture window(640, 400);
    TestFixtures::QmlEngine engine;
    std::unique_ptr<QObject> panel = createConnectionPanel(engine, window, ntrip.manager);
    QVERIFY2(panel, qPrintable(engine.lastError()));
    QVERIFY(window.show());
    auto* button = panel->findChild<QQuickItem*>(QStringLiteral("ntripConnectButton"));
    QVERIFY(button);
    QVERIFY(QQuickTest::qWaitForPolish(window.window(), TestTimeout::mediumMs()));
    QCOMPARE(button->property("text").toString(), QStringLiteral("Retry"));
    const QPoint center = button->mapToScene(QPointF(button->width() / 2, button->height() / 2)).toPoint();

    // Retry starts an attempt at once, and the button then reads "Disconnect". The second click of a double click is
    // not a click on that: it would stop the attempt it has just started and turn the connection off.
    QTest::mouseDClick(window.window(), Qt::LeftButton, Qt::NoModifier, center);
    QVERIFY(retry);
    QCOMPARE(retry->startCount, 1);
    QCOMPARE(ntrip.manager.connectionStatus(), NTRIPManager::ConnectionStatus::Connecting);
    QCOMPARE(button->property("text").toString(), QStringLiteral("Disconnect"));
    QCOMPARE(requests.size(), 1);
    QVERIFY(requests.constLast().constFirst().toBool());

    // A click of its own stops the attempt.
    QTest::mouseClick(window.window(), Qt::LeftButton, Qt::NoModifier, center);
    QCOMPARE(requests.size(), 2);
    QVERIFY(!requests.constLast().constFirst().toBool());
}

void GPSCorrectionsPanelTest::_ntripMountpointLockedWhileActive()
{
    // The application's setting would start its NTRIP connection.
    Fact enabled(0, QStringLiteral("enabled"), FactMetaData::valueTypeBool);
    enabled.setRawValue(true);
    TestFixtures::QmlEngine engine;
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
    TestFixtures::QmlWindowFixture window(640, 400);
    TestFixtures::QmlEngine engine;
    std::unique_ptr<QObject> panel = createConnectionPanel(engine, window, ntrip.manager);
    QVERIFY2(panel, qPrintable(engine.lastError()));
    auto* panelItem = qobject_cast<QQuickItem*>(panel.get());
    QVERIFY(panelItem);
    panelItem->setWidth(480);
    QVERIFY(window.show());
    const qreal shortWidth = panelItem->implicitWidth();

    const QString longMessage = QStringLiteral("certificate rejected ").repeated(20);
    MockNTRIPTransport* retry = ntrip.nextTransport();
    ntrip.manager.connectCaster();
    fail(retry, longMessage);
    QCOMPARE(ntrip.manager.connectionStatus(), NTRIPManager::ConnectionStatus::Error);
    QCOMPARE(ntrip.manager.statusMessage(), longMessage);
    const auto statusLabel = [&]() -> QQuickItem* {
        const auto labels = TestFixtures::findItems(panelItem, [&](QQuickItem* item) {
            return item->property("text").toString() == longMessage && item->property("lineCount").isValid();
        });
        return labels.isEmpty() ? nullptr : labels.first();
    };
    QTRY_VERIFY_WITH_TIMEOUT(statusLabel(), TestTimeout::mediumMs());
    QVERIFY(QQuickTest::qWaitForPolish(window.window(), TestTimeout::mediumMs()));
    // The message wraps inside the panel instead of widening every settings group on the page.
    QCOMPARE(panelItem->implicitWidth(), shortWidth);
    QVERIFY(statusLabel()->property("lineCount").toInt() > 1);
    const auto buttons = TestFixtures::findItems(
        panelItem, [](QQuickItem* item) { return item->objectName() == QLatin1String("ntripConnectButton"); });
    QCOMPARE(buttons.size(), 1);
    const QPointF buttonRight = buttons.first()->mapToItem(panelItem, QPointF(buttons.first()->width(), 0));
    QVERIFY2(buttonRight.x() <= panelItem->width(), qPrintable(QString::number(buttonRight.x())));
}

void GPSCorrectionsPanelTest::_ntripMountpointRowLayout()
{
    NTRIPSourceTableModel model;
    // The last row only names its mountpoint, as a short source-table record does.
    model.parseSourceTable(QStringLiteral(
        "STR;LONG;Long;RTCM 3.3;1005(1),1077(1);2;GPS+GLO+GAL+BDS+SBAS+QZSS;NET;FRA;0;0;0;0;gen;none;B;N;15200;\r\n"
        "STR;S;Short;RTCM 3.3;1005(1);2;GPS;NET;FRA;0;0;0;0;gen;none;N;N;0;\r\n"
        "STR;BARE\r\n"
        "ENDSOURCETABLE\r\n"));
    QCOMPARE(model.rowCount(), 3);
    TestFixtures::QmlWindowFixture window(640, 400);
    TestFixtures::QmlEngine engine;
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
        return TestFixtures::findItems(listItem, [](QQuickItem* item) {
            return item->inherits("QQuickAbstractButton") && item->isVisible() && item->width() > 0;
        });
    };
    QTRY_COMPARE_WITH_TIMEOUT(buttons().size(), 3, TestTimeout::mediumMs());
    const auto right = [listItem](QQuickItem* button) {
        return button->mapToItem(listItem, QPointF(button->width(), 0)).x();
    };
    QTRY_COMPARE_WITH_TIMEOUT(right(buttons().at(0)), right(buttons().at(1)), TestTimeout::mediumMs());
    QTRY_COMPARE_WITH_TIMEOUT(right(buttons().at(0)), right(buttons().at(2)), TestTimeout::mediumMs());

    // A row without details has no line for them.
    const auto rows =
        TestFixtures::findItems(listItem, [](QQuickItem* item) { return item->property("mountpoint").isValid(); });
    QCOMPARE(rows.size(), 3);
    QCOMPARE(rows.at(2)->property("mountpoint").toString(), QStringLiteral("BARE"));
    QVERIFY(rows.at(2)->property("details").toString().isEmpty());
    QCOMPARE(rows.at(0)->height(), rows.at(1)->height());
    QVERIFY2(rows.at(2)->height() < rows.at(1)->height(),
             qPrintable(QStringLiteral("%1 and %2").arg(rows.at(2)->height()).arg(rows.at(1)->height())));
}

UT_REGISTER_TEST(GPSCorrectionsPanelTest, TestLabel::Unit)
