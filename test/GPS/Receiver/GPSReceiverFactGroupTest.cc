#include "GPSReceiverFactGroupTest.h"

#include <chrono>
#include <cmath>
#include <limits>
#include <optional>

#include <QtTest/QSignalSpy>

#include "GPSReceiver.h"
#include "GPSReceiverFactGroup.h"
#include "ManualScheduler.h"
#include "Receiver/Support/GPSReceiverTestSupport.h"

using namespace GPSTest;

void GPSReceiverFactGroupTest::_defaultsWithoutReceiver()
{
    GPSReceiver receiver;
    QVERIFY(!receiver.facts()->telemetryAvailable());
    GPSReceiverFactGroup& facts = *receiver.facts();
    // The status without a receiver is what the Fact metadata declares as the default.
    for (const QString& name : facts.factNames()) {
        const Fact* fact = facts.getFact(name);
        const QVariant value = fact->rawValue();
        const QVariant expected = fact->rawDefaultValue();
        QVERIFY2(value == expected || (std::isnan(value.toDouble()) && std::isnan(expected.toDouble())),
                 qPrintable(name));
    }
    // Unreported values are unavailable rather than zero.
    QVERIFY(std::isnan(facts.currentAccuracy()->rawValue().toDouble()));
    QCOMPARE(solution(facts), Solution{});
}

void GPSReceiverFactGroupTest::_satelliteCounts_data()
{
    // -1 is a count the report leaves out, which the Facts show as unavailable.
    QTest::addColumn<int>("inView");
    QTest::addColumn<int>("used");
    QTest::newRow("unavailable") << -1 << -1;
    QTest::newRow("count-only") << -1 << 7;
    QTest::newRow("unknown-usage") << 3 << -1;
    QTest::newRow("none") << 0 << 0;
}

void GPSReceiverFactGroupTest::_satelliteCounts()
{
    QFETCH(int, inView);
    QFETCH(int, used);
    GPSSatelliteReport report;
    report.timestampUs = 1;
    if (inView >= 0) {
        report.inView = inView;
    }
    if (used >= 0) {
        report.used = used;
    }
    ScriptedGPSReceiver harness;
    QVERIFY(connectOverTcp(harness.receiver));
    GPSReceiverFactGroup& facts = *harness.receiver.facts();
    harness.workers.current()->satellites(report);
    QCOMPARE(solution(facts), (Solution{.inView = inView, .used = used}));
    harness.receiver.disconnectReceiver();
    QCOMPARE(solution(facts), Solution{});
}

void GPSReceiverFactGroupTest::_currentBaseSaveValidity_data()
{
    QTest::addColumn<QString>("field");
    QTest::addColumn<double>("value");
    QTest::addColumn<bool>("expected");
    const double nan = std::numeric_limits<double>::quiet_NaN();
    // Coordinate and wire limits are the base-station validator's; one row shows they apply.
    QTest::newRow("known-zero-accuracy") << "currentAccuracy" << 0.0 << true;
    QTest::newRow("unavailable-accuracy") << "currentAccuracy" << nan << false;
    QTest::newRow("negative-accuracy") << "currentAccuracy" << -1.0 << false;
    QTest::newRow("float-accuracy-overflow") << "currentAccuracy" << 1e100 << false;
    QTest::newRow("wire-altitude-overflow") << "currentAltitude" << 21474838.0 << false;
}

void GPSReceiverFactGroupTest::_currentBaseSaveValidity()
{
    QFETCH(QString, field);
    QFETCH(double, value);
    QFETCH(bool, expected);
    ScriptedGPSReceiver harness;
    QVERIFY(connectOverTcp(harness.receiver));
    ScriptedReceiverWorker* const worker = harness.workers.current();
    GPSReceiverFactGroup& facts = *harness.receiver.facts();
    GPSSurveyReport survey{.position = {.latitudeDegrees = 47.0, .longitudeDegrees = 8.0, .altitudeMeters = 500.0f},
                           .meanAccuracyMeters = 1.0};
    worker->survey(survey);
    QVERIFY(!facts.canSaveCurrentBasePosition());
    survey.valid = true;
    worker->survey(survey);
    QVERIFY(facts.canSaveCurrentBasePosition());
    QCOMPARE(facts.currentBasePosition(),
             std::optional(GPSBaseStationConfig::Fixed{.position = survey.position, .accuracyMeters = 1.0f}));
    QSignalSpy changes(&facts, &GPSReceiverFactGroup::currentBasePositionChanged);
    if (field == QLatin1String("currentAccuracy")) {
        survey.meanAccuracyMeters = std::isnan(value) ? std::nullopt : std::optional(value);
    } else {
        survey.position.altitudeMeters = static_cast<float>(value);
    }
    worker->survey(survey);
    // Notified only when the result changes.
    QCOMPARE(changes.size(), expected ? 0 : 1);
    QCOMPARE(facts.canSaveCurrentBasePosition(), expected);
    QCOMPARE(facts.property("canSaveCurrentBasePosition").toBool(), expected);
    survey.valid = false;
    worker->survey(survey);
    QVERIFY(!facts.canSaveCurrentBasePosition());
}

void GPSReceiverFactGroupTest::_integrityFacts()
{
    ScriptedGPSReceiver harness;
    GPSReceiver& receiver = harness.receiver;
    QVERIFY(connectOverTcp(receiver));
    ScriptedReceiverWorker* const worker = harness.workers.current();
    GPSReceiverFactGroup& facts = *receiver.facts();
    GPSPositionReport report = fixReport(GPSFixQuality::Fix3D);
    report.integrity.jamming.state = GPSIntegrityReport::JammingState::Warning;
    report.integrity.spoofing.state = GPSIntegrityReport::SpoofingState::Indicated;
    worker->position(report);
    QCOMPARE(solution(facts), (Solution{.fixType = GPSFixQuality::Fix3D, .jamming = 2, .spoofing = 2}));
    QCOMPARE(facts.jammingState()->enumStringValue(), QStringLiteral("Warning"));
    QVERIFY(facts.receiverWarning());
    report.integrity = {};
    worker->position(report);
    QCOMPARE(facts.jammingState()->rawValue().toInt(), 0);
    QVERIFY(!facts.receiverWarning());
    report.integrity.jamming.state = GPSIntegrityReport::JammingState::Ok;
    report.integrity.spoofing.state = GPSIntegrityReport::SpoofingState::None;
    report.integrity.antenna.state = GPSIntegrityReport::AntennaState::Ok;
    worker->position(report);
    QCOMPARE(facts.antennaState()->enumStringValue(), QStringLiteral("OK"));
    QVERIFY(!facts.receiverWarning());
    QSignalSpy warning(&facts, &GPSReceiverFactGroup::receiverWarningChanged);
    report.integrity.jamming.state = GPSIntegrityReport::JammingState::Critical;
    worker->position(report);
    QVERIFY(facts.receiverWarning());
    QCOMPARE(warning.size(), 1);
    // An open or shorted antenna warns on its own.
    for (const auto& [state, name] : {std::pair{GPSIntegrityReport::AntennaState::Open, QStringLiteral("Open")},
                                      std::pair{GPSIntegrityReport::AntennaState::Short, QStringLiteral("Short")}}) {
        report.integrity.antenna.state = state;
        worker->position(report);
        report.integrity.jamming.state = GPSIntegrityReport::JammingState::Ok;
        worker->position(report);
        QCOMPARE(facts.antennaState()->enumStringValue(), name);
        QVERIFY(facts.receiverWarning());
    }
    QCOMPARE(warning.size(), 1);
    receiver.disconnectReceiver();
    QCOMPARE(solution(facts), Solution{});
    QVERIFY(!facts.receiverWarning());
}

void GPSReceiverFactGroupTest::_silentReceiverClearsSolution()
{
    ManualScheduler scheduler;
    ScriptedGPSReceiver harness(&scheduler);
    QVERIFY(connectOverTcp(harness.receiver));
    ScriptedReceiverWorker* const worker = harness.workers.current();
    GPSReceiverFactGroup& facts = *harness.receiver.facts();
    GPSSatelliteReport satellites;
    satellites.inView = 12;
    satellites.used = 9;
    worker->satellites(satellites);
    auto report = fixReport(GPSFixQuality::Fix3D);
    report.integrity.jamming.state = GPSIntegrityReport::JammingState::Warning;
    report.integrity.antenna.state = GPSIntegrityReport::AntennaState::Open;
    worker->position(report);
    QCOMPARE(solution(facts), (Solution{.fixType = GPSFixQuality::Fix3D,
                                        .inView = 12,
                                        .used = 9,
                                        .jamming = static_cast<int>(GPSIntegrityReport::JammingState::Warning),
                                        .antenna = static_cast<int>(GPSIntegrityReport::AntennaState::Open)}));

    // Passive and position-only links stay connected while silent; their last solution must not linger.
    QVERIFY(scheduler.advanceBy(std::chrono::seconds(6)));
    QCOMPARE(solution(facts), Solution{});
}

void GPSReceiverFactGroupTest::_summaryLabel()
{
    ScriptedGPSReceiver harness;
    GPSReceiver& receiver = harness.receiver;
    GPSReceiverFactGroup& facts = *receiver.facts();
    QSignalSpy changed(&receiver, &GPSReceiver::summaryLabelChanged);
    QVERIFY(receiver.summaryLabel().isEmpty());
    facts.fixType()->setRawValue(static_cast<int>(GPSFixQuality::RTKFixed));
    QCOMPARE(receiver.summaryLabel(), GPSReceiver::tr("Fixed", "RTK fixed fix"));
    facts.fixType()->setRawValue(static_cast<int>(GPSFixQuality::NoFix));
    QCOMPARE(receiver.property("summaryLabel").toString(), GPSReceiver::tr("No fix"));
    QCOMPARE(changed.size(), 2);

    // A configured base reports its survey state instead of its fix.
    QVERIFY(connectOverTcp(receiver));
    QCOMPARE(receiver.activeRole(), RTKSettings::ConfiguredBase);
    QCOMPARE(receiver.summaryLabel(), GPSReceiver::tr("Base"));
    facts.active()->setRawValue(true);
    QCOMPARE(receiver.summaryLabel(), GPSReceiver::tr("Survey", "Base survey-in in progress"));
}

UT_REGISTER_TEST(GPSReceiverFactGroupTest, TestLabel::Unit)
