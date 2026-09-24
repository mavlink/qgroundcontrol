#include <algorithm>
#include <functional>
#include <utility>

#include <QtCore/QPointer>
#include <QtCore/QRegularExpression>
#include <QtTest/QAbstractItemModelTester>
#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

#include "../RTCM/RTCMTestFixtures.h"
#include "GPSCorrectionEventModel.h"
#include "GPSCorrectionRouter.h"
#include "RTCMDecodedFrame.h"
#include "UnitTest.h"

namespace {
GPSCorrectionFrame frame(GPSCorrectionSource source, quint64 session, qint64 now, const QString& instance = {})
{
    return {source, session, now, GpsTestHelpers::buildRtcmFrame(1005, 20), 1005, true, false, instance};
}

GPSCorrectionIngress ingress(const GPSCorrectionSourceRegistration& source, qint64 now, const QString& instance = {})
{
    return source.token().event(GpsTestHelpers::buildRtcmFrame(1005, 20), now, 1005, true, false,
                                GPSCorrectionReason::None, instance);
}

void setAdmissionOutput(GPSCorrectionRouter& router, const QString& id, GPSCorrectionRouter::Sink submit,
                        GPSCorrectionSource scope = GPSCorrectionSource::Unknown)
{
    router.setOutput(id, GPSCorrectionRouter::admissionOnlyOutput(id, scope, std::move(submit)));
}

QString selectedInstance(const GPSCorrectionRouter& router)
{
    for (const auto& value : router.sourceInstanceDiagnostics()) {
        const auto instance = value.toMap();
        if (instance.value(QStringLiteral("selected")).toBool()) {
            return instance.value(QStringLiteral("instanceId")).toString();
        }
    }
    return {};
}
}  // namespace

class GPSCorrectionRouterTest : public UnitTest
{
    Q_OBJECT

private slots:
    void atomicConfigurationAndReplacement();
    void scopedSourceIdentity();
    void rawInputRequiresUdp_data();
    void rawInputRequiresUdp();
    void outputRetirementDuringAdmission_data();
    void outputRetirementDuringAdmission();
    void outputReplacementKeepsRegistration();
    void partialAdmissionCompletion_data();
    void partialAdmissionCompletion();
    void claimedValidatedIngressRequiresCrc_data();
    void claimedValidatedIngressRequiresCrc();
    void registrationMoveAssignment();
    void fanoutAdmissionAccounting();
    void liveFanoutDestinationsSurviveHistoryChurn();
    void retiredDuringAdmissionPreservesEvidence_data();
    void retiredDuringAdmissionPreservesEvidence();
    void automaticSelectionAndFailover();
    void peerSelectionDoesNotInterleave();
    void sourceIdentityOrderingAndRetirement();
    void sessionAndReceiptValidation();
    void nonRoutablePeersRemainObserved_data();
    void nonRoutablePeersRemainObserved();
    void sinkResults();
    void emptyOutputRemovesRegistration_data();
    void emptyOutputRemovesRegistration();
    void sourceChangePrecedesSubmission_data();
    void sourceChangePrecedesSubmission();
    void endingSelectedSessionInvalidatesOutput();
    void teardownAndReentrancy();
    void boundedPeerHistory();
    void destinationHistoryDoesNotLimitOutputs();
    void diagnosticStagesStayDistinct_data();
    void diagnosticStagesStayDistinct();
    void boundedDiagnosticsAndEventHistory();
    void rejectedCandidateHasNoValidatedCredit();
    void sourceSpecificSinkPreservesNtripForwarding();
    void scopedAdmissionAccounting_data();
    void scopedAdmissionAccounting();
    void eventHistoryUsesIncrementalRows();
    void eventHistoryAllowsReentrantUpdates();
    void decodedIngressPreservesEvidence_data();
    void decodedIngressPreservesEvidence();
    void diagnosticsSampleClockOnce();
    void diagnosticsKeepHealthDomainsIndependent();
};

void GPSCorrectionRouterTest::decodedIngressPreservesEvidence_data()
{
    QTest::addColumn<bool>("valid");
    QTest::addColumn<bool>("filtered");
    QTest::newRow("valid") << true << false;
    QTest::newRow("filtered") << true << true;
    QTest::newRow("invalid") << false << false;
}

void GPSCorrectionRouterTest::decodedIngressPreservesEvidence()
{
    QFETCH(bool, valid);
    QFETCH(bool, filtered);
    constexpr qint64 now = 100000;
    GPSCorrectionRouter router(nullptr, [] { return now; });
    auto registration = router.registerSource(GPSCorrectionSource::Ntrip, QStringLiteral("caster"));
    RTCMDecodedFrame decoded{GpsTestHelpers::buildRtcmFrame(1005), 1005, now - 10, valid, filtered};
    const auto input = registration.token().event(decoded);
    QCOMPARE(input.frame().source, GPSCorrectionSource::Ntrip);
    QCOMPARE(input.frame().sourceInstance, QStringLiteral("caster"));
    QCOMPARE(input.frame().session, registration.token().session());
    QCOMPARE(input.frame().receivedAtMs, decoded.receivedAtMs);
    QCOMPARE(input.frame().data, decoded.data);
    QCOMPARE(input.rejection(), valid ? GPSCorrectionReason::None : GPSCorrectionReason::InvalidFrame);
    QCOMPARE(router.acceptIngress(input), valid && !filtered);
    QCOMPARE(router.statistics()[2].receivedFrames, 1ULL);
    QCOMPARE(router.statistics()[2].validatedFrames, valid ? 1ULL : 0ULL);
    auto replacement = router.registerSource(GPSCorrectionSource::Ntrip, QStringLiteral("caster"));
    QVERIFY(!router.acceptIngress(input));
    QCOMPARE(router.statistics()[2].receivedFrames, 0ULL);
}

void GPSCorrectionRouterTest::diagnosticsSampleClockOnce()
{
    qint64 now = 100000;
    int samples = 0;
    GPSCorrectionRouter router(nullptr, [&]() {
        ++samples;
        return now;
    });
    auto registration = router.registerSource(GPSCorrectionSource::Ntrip, QStringLiteral("caster"));
    setAdmissionOutput(router, QStringLiteral("accepted"),
                       [](const GPSCorrectionFrame& frame) { return quint64(frame.data.size()); });
    QVERIFY(router.acceptIngress(ingress(registration, now)));
    auto udp = router.registerSource(GPSCorrectionSource::Udp, QStringLiteral("datagram"));
    QVERIFY(!router.acceptIngress(ingress(udp, now)));
    samples = 0;
    const auto sources = router.sourceDiagnostics();
    QCOMPARE(samples, 1);
    const auto source = sources[2].toMap();
    QVERIFY(source.value(QStringLiteral("usable")).toBool());
    const auto instances = router.sourceInstanceDiagnostics();
    QCOMPARE(samples, 2);
    QCOMPARE(instances.size(), 2);
    const auto instance = instances.first().toMap();
    QCOMPARE(instance.value(QStringLiteral("instanceId")).toString(), QStringLiteral("caster"));
    QVERIFY(instance.value(QStringLiteral("selected")).toBool());
    QVERIFY(!instances.last().toMap().value(QStringLiteral("selected")).toBool());
    const auto destination = router.destinationDiagnostics().first().toMap();
    QVERIFY(destination.value(QStringLiteral("queuedBytes")).toULongLong() > 0);

    QCOMPARE(samples, 2);

    now += GPSCorrectionRouter::FRESHNESS_TIMEOUT_MS;
    QVERIFY(!router.sourceDiagnostics()[2].toMap().value(QStringLiteral("usable")).toBool());
    QVERIFY(!router.sourceInstanceDiagnostics().first().toMap().value(QStringLiteral("selected")).toBool());
    QVERIFY(instance.value(QStringLiteral("selected")).toBool());
    now = 99999;
    const auto future = router.sourceDiagnostics()[2].toMap();
    QVERIFY(!future.value(QStringLiteral("usable")).toBool());
    QVERIFY(!router.sourceInstanceDiagnostics().first().toMap().value(QStringLiteral("selected")).toBool());
}

void GPSCorrectionRouterTest::diagnosticsKeepHealthDomainsIndependent()
{
    constexpr qint64 now = 100000;
    GPSCorrectionRouter router(nullptr, [] { return now; });
    auto ntrip = router.registerSource(GPSCorrectionSource::Ntrip);
    const RTCMDecodedFrame filtered{GpsTestHelpers::buildRtcmFrame(1005), 1005, now, true, true};
    QVERIFY(!router.acceptIngress(ntrip.token().event(filtered)));
    auto udp = router.registerSource(GPSCorrectionSource::Udp);
    QVERIFY(router.acceptIngress(udp.token().event(QByteArrayLiteral("raw"), now, 0, false)));
    const auto sources = router.sourceDiagnostics();
    QVERIFY(sources[2].toMap().value(QStringLiteral("usable")).toBool());
    QVERIFY(!sources[3].toMap().value(QStringLiteral("usable")).toBool());
    for (const auto& row : router.sourceInstanceDiagnostics()) {
        const auto instance = row.toMap();
        const bool isUdp = instance.value(QStringLiteral("source")).toInt() == int(GPSCorrectionSource::Udp);
        QCOMPARE(instance.value(QStringLiteral("usable")).toBool(), isUdp);
        QCOMPARE(instance.value(QStringLiteral("selected")).toBool(), isUdp);
    }
}

void GPSCorrectionRouterTest::destinationHistoryDoesNotLimitOutputs()
{
    qint64 now = 100000;
    GPSCorrectionRouter router(nullptr, [&]() { return now; });
    auto source = router.registerSource(GPSCorrectionSource::Ntrip);
    const auto data = GpsTestHelpers::buildRtcmFrame(1005, 20);
    const int outputCount = GPSCorrectionRouter::MAX_DESTINATION_HISTORY + 4;
    int submissions = 0;
    for (int index = 0; index < outputCount; ++index) {
        setAdmissionOutput(router, QString::number(index), [&](const GPSCorrectionFrame& submitted) {
            ++submissions;
            return quint64(submitted.data.size());
        });
    }
    QVERIFY(router.acceptIngress(source.token().event(data, now, 1005, true)));
    QCOMPARE(submissions, outputCount);
    QCOMPARE(router.destinations().size(), outputCount);
    for (const auto& destination : router.destinations()) {
        QCOMPARE(destination.queuedBytes, quint64(data.size()));
    }
    for (int index = 0; index < outputCount; ++index) {
        router.removeSink(QString::number(index));
    }
    QCOMPARE(router.destinations().size(), GPSCorrectionRouter::MAX_DESTINATION_HISTORY);

    setAdmissionOutput(router, QStringLiteral("receiver"),
                       [&](const GPSCorrectionFrame& submitted) { return quint64(submitted.data.size()); });
    QVERIFY(router.acceptIngress(source.token().event(data, ++now, 1005, true)));
    router.setOutput(QStringLiteral("vehicles"), {.admit = [&](const GPSCorrectionFrame& submitted) {
                         return QList<GPSCorrectionRouter::Admission>{
                             {QString::number(now),
                              {quint64(submitted.data.size()), quint64(now), GPSCorrectionReason::None},
                              true}};
                     }});
    for (int index = 0; index < outputCount; ++index) {
        QVERIFY(router.acceptIngress(source.token().event(data, ++now, 1005, true)));
        QVERIFY(router.destinations().size() <= GPSCorrectionRouter::MAX_DESTINATION_HISTORY + 3);
    }
    const auto destinations = router.destinations();
    const auto receiver = std::find_if(destinations.cbegin(), destinations.cend(), [](const auto& destination) {
        return destination.id == QStringLiteral("receiver");
    });
    QVERIFY(receiver != destinations.cend());
    QCOMPARE(receiver->queuedBytes, quint64((outputCount + 1) * data.size()));
}

void GPSCorrectionRouterTest::atomicConfigurationAndReplacement()
{
    qint64 now = 100000;
    GPSCorrectionRouter router(nullptr, [&]() { return now; });
    auto source = router.registerSource(GPSCorrectionSource::Ntrip, QStringLiteral("caster"));
    QVERIFY(router.acceptIngress(source.token().event(GpsTestHelpers::buildRtcmFrame(1005, 20), now, 1005, true)));
    QSignalSpy changes(&router, &GPSCorrectionRouter::sourceInvalidated);
    const GPSCorrectionRouter::Configuration requested{GPSCorrectionRouter::Policy::Manual, GPSCorrectionSource::Udp,
                                                       QStringLiteral("peer")};
    const GPSCorrectionRouter::Configuration replacement{GPSCorrectionRouter::Policy::Manual,
                                                         GPSCorrectionSource::Ntrip, QStringLiteral("caster")};
    connect(&router, &GPSCorrectionRouter::sourceInvalidated, &router, [&]() {
        QVERIFY(router.configuration() == requested);
        router.applyConfiguration(replacement);
    });
    router.applyConfiguration(requested);
    QCOMPARE(changes.size(), 1);
    QVERIFY(router.configuration() == replacement);
    QPointer<GPSCorrectionRouter> destroyed = new GPSCorrectionRouter;
    auto registration = destroyed->registerSource(GPSCorrectionSource::Ntrip);
    destroyed->acceptIngress(registration.token().event(GpsTestHelpers::buildRtcmFrame(1005, 20),
                                                        GPSCorrectionFrame::monotonicNowMs(), 1005, true));
    connect(destroyed, &GPSCorrectionRouter::sourceInvalidated, destroyed, [&]() { delete destroyed.data(); });
    destroyed->applyConfiguration(requested);
    QVERIFY(destroyed.isNull());
    QVERIFY(!registration.token().valid());
}

void GPSCorrectionRouterTest::scopedSourceIdentity()
{
    qint64 now = 100000;
    GPSCorrectionRouter router(nullptr, [&]() { return now; });
    GPSCorrectionRouter other(nullptr, [&]() { return now; });
    const auto bytes = GpsTestHelpers::buildRtcmFrame(1005, 20);
    auto original = router.registerSource(GPSCorrectionSource::Ntrip, QStringLiteral("caster"));
    const auto old = original.token();
    auto replacement = router.registerSource(GPSCorrectionSource::Ntrip, QStringLiteral("caster"));
    original.reset();
    QVERIFY(replacement.token().valid());
    QVERIFY(!old.valid());
    const auto receivedBefore = router.statistics()[2].receivedFrames;
    QVERIFY(!router.acceptIngress(old.event(bytes, now, 1005, true)));
    QVERIFY(!router.acceptIngress(old.event(bytes, now, 1005, false, false, GPSCorrectionReason::InvalidFrame)));
    QCOMPARE(router.statistics()[2].receivedFrames, receivedBefore);
    QVERIFY(!other.acceptIngress(replacement.token().event(bytes, now, 1005, true)));
    QVERIFY(!router.acceptIngress(replacement.token().event(bytes, now, 1005, true, false, GPSCorrectionReason::None,
                                                            QStringLiteral("imposter"))));
    QVERIFY(router.acceptIngress(replacement.token().event(bytes, now, 1005, true)));
    auto udp = router.registerSource(GPSCorrectionSource::Udp);
    router.applyConfiguration({GPSCorrectionRouter::Policy::Manual, GPSCorrectionSource::Udp, QStringLiteral("peer")});
    QVERIFY(router.acceptIngress(
        udp.token().event(bytes, now, 1005, true, false, GPSCorrectionReason::None, QStringLiteral("peer"))));
    const auto token = udp.token();
    udp.reset();
    QVERIFY(!token.valid());
    QVERIFY(!router.acceptIngress(token.event(bytes, now, 1005, true)));
}

void GPSCorrectionRouterTest::rawInputRequiresUdp_data()
{
    QTest::addColumn<GPSCorrectionSource>("category");
    QTest::newRow("local") << GPSCorrectionSource::LocalReceiver;
    QTest::newRow("ntrip") << GPSCorrectionSource::Ntrip;
    QTest::newRow("udp-passthrough") << GPSCorrectionSource::Udp;
}

void GPSCorrectionRouterTest::rawInputRequiresUdp()
{
    QFETCH(GPSCorrectionSource, category);
    qint64 now = 100000;
    GPSCorrectionRouter router(nullptr, [&]() { return now; });
    auto udp = router.registerSource(GPSCorrectionSource::Udp);
    auto source =
        category == GPSCorrectionSource::Udp ? GPSCorrectionSourceRegistration{} : router.registerSource(category);
    const auto token = category == GPSCorrectionSource::Udp ? udp.token() : source.token();
    int submissions = 0;
    setAdmissionOutput(router, QStringLiteral("capture"), [&](const GPSCorrectionFrame& received) {
        ++submissions;
        return quint64(received.data.size());
    });
    QVERIFY(router.acceptIngress(udp.token().event(GpsTestHelpers::buildRtcmFrame(1005, 20), now, 1005, true)));
    const bool rawAllowed = category == GPSCorrectionSource::Udp;
    for (int index = 0; index < 2; ++index) {
        QCOMPARE(router.acceptIngress(token.event(QByteArrayLiteral("bad"), now, 0, false)), rawAllowed);
        QCOMPARE(router.activeSource(), GPSCorrectionSource::Udp);
        now += GPSCorrectionRouter::SWITCH_HOLD_DOWN_MS;
    }
    QCOMPARE(submissions, rawAllowed ? 3 : 1);
    QCOMPARE(router.sources().size(), 1);
    const auto& stats = router.statistics().at(static_cast<int>(category));
    QCOMPARE(stats.validatedFrames, rawAllowed ? 1ULL : 0ULL);
    QCOMPARE(stats.selectedFrames, rawAllowed ? 3ULL : 0ULL);
    QCOMPARE(stats.droppedFrames, rawAllowed ? 0ULL : 2ULL);
    if (!rawAllowed) {
        QCOMPARE(router.events().last().reason, GPSCorrectionReason::InvalidFrame);
    }
}

void GPSCorrectionRouterTest::outputRetirementDuringAdmission_data()
{
    QTest::addColumn<bool>("shutdown");
    QTest::addColumn<bool>("retireBeforeReturn");
    QTest::newRow("remove-before-return") << false << true;
    QTest::newRow("remove-after-return") << false << false;
    QTest::newRow("shutdown-before-return") << true << true;
    QTest::newRow("shutdown-after-return") << true << false;
}

void GPSCorrectionRouterTest::outputRetirementDuringAdmission()
{
    QFETCH(bool, shutdown);
    QFETCH(bool, retireBeforeReturn);
    constexpr qint64 now = 100000;
    GPSCorrectionRouter router(nullptr, [] { return now; });
    auto source = router.registerSource(GPSCorrectionSource::Ntrip);
    const auto bytes = GpsTestHelpers::buildRtcmFrame(1005, 20);
    router.setOutput(QStringLiteral("a-earlier"), {.admit = [](const GPSCorrectionFrame& received) {
                         const quint64 size = received.data.size();
                         return QList<GPSCorrectionRouter::Admission>{
                             {QStringLiteral("earlier/receiver"), {size, 7, GPSCorrectionReason::None}, true}};
                     }});
    const auto retire = [&] {
        if (shutdown) {
            router.shutdown();
        } else {
            router.removeSink(QStringLiteral("a-earlier"));
        }
    };
    router.setOutput(
        QStringLiteral("b-later"), {.admit = [&](const GPSCorrectionFrame& received) {
            if (retireBeforeReturn) {
                retire();
            }
            if (!retireBeforeReturn) {
                retire();
            }
            return QList<GPSCorrectionRouter::Admission>{{QStringLiteral("later/receiver"),
                                                          {quint64(received.data.size()), 8, GPSCorrectionReason::None},
                                                          true}};
        }});
    QVERIFY(router.acceptIngress(source.token().event(bytes, now, 1005, true)));
    bool earlierFound = false;
    bool laterFound = false;
    for (const auto& destination : router.destinations()) {
        if (destination.id == QStringLiteral("earlier/receiver")) {
            earlierFound = true;
            QCOMPARE(destination.queuedBytes, quint64(bytes.size()));
        } else if (destination.id == QStringLiteral("later/receiver")) {
            laterFound = true;
            QCOMPARE(destination.queuedBytes, quint64(bytes.size()));
        }
    }
    QVERIFY(earlierFound);
    QVERIFY(laterFound);
}

void GPSCorrectionRouterTest::partialAdmissionCompletion_data()
{
    QTest::addColumn<int>("queuedBytes");
    QTest::addColumn<bool>("admissionComplete");
    QTest::addColumn<quint64>("dropEvents");
    QTest::newRow("complete-frame") << -1 << true << quint64(0);
    QTest::newRow("prefix-only") << 7 << true << quint64(1);
    QTest::newRow("all-bytes-incomplete-protocol") << -1 << false << quint64(1);
}

void GPSCorrectionRouterTest::outputReplacementKeepsRegistration()
{
    constexpr qint64 now = 100000;
    GPSCorrectionRouter router(nullptr, [] { return now; });
    auto source = router.registerSource(GPSCorrectionSource::Ntrip);
    router.setOutput(
        QStringLiteral("a-receiver"), {.admit = [](const GPSCorrectionFrame& received) {
            return QList<GPSCorrectionRouter::Admission>{
                {QStringLiteral("old/receiver"), {quint64(received.data.size()), 7, GPSCorrectionReason::None}, true}};
        }});
    bool replacementQueued = false;
    bool replace = true;
    setAdmissionOutput(router, QStringLiteral("b-replace"), [&](const GPSCorrectionFrame&) {
        if (std::exchange(replace, false)) {
            router.removeSink(QStringLiteral("a-receiver"));
            setAdmissionOutput(router, QStringLiteral("a-receiver"), [&](const GPSCorrectionFrame& received) {
                replacementQueued = true;
                return quint64(received.data.size());
            });
        }
        return quint64(0);
    });
    QVERIFY(router.acceptIngress(ingress(source, now)));
    router.setOutput(QStringLiteral("z-history"), {.admit = [](const GPSCorrectionFrame&) {
                         QList<GPSCorrectionRouter::Admission> admissions;
                         for (qsizetype index = 0; index < GPSCorrectionRouter::MAX_DESTINATION_HISTORY + 2; ++index) {
                             admissions.append({QStringLiteral("history/%1").arg(index), {}, false});
                         }
                         return admissions;
                     }});
    QVERIFY(router.acceptIngress(ingress(source, now)));
    QVERIFY(replacementQueued);
    const auto destinations = router.destinations();
    const auto replacement = std::find_if(destinations.cbegin(), destinations.cend(), [](const auto& destination) {
        return destination.id == QStringLiteral("a-receiver");
    });
    QVERIFY(replacement != destinations.cend());
    QCOMPARE(replacement->queuedFrames, 1ULL);
}

void GPSCorrectionRouterTest::partialAdmissionCompletion()
{
    QFETCH(int, queuedBytes);
    QFETCH(bool, admissionComplete);
    QFETCH(quint64, dropEvents);
    constexpr qint64 now = 100000;
    GPSCorrectionRouter router(nullptr, [] { return now; });
    auto source = router.registerSource(GPSCorrectionSource::Ntrip);
    const auto bytes = GpsTestHelpers::buildRtcmFrame(1005, 20);
    const quint64 size = bytes.size();
    const quint64 queued = queuedBytes < 0 ? size : quint64(queuedBytes);
    router.setOutput(QStringLiteral("receiver"), {.admit = [&](const GPSCorrectionFrame&) {
                         return QList<GPSCorrectionRouter::Admission>{
                             {QStringLiteral("receiver"), {queued, 7, GPSCorrectionReason::None}, admissionComplete}};
                     }});
    QVERIFY(router.acceptIngress(source.token().event(bytes, now, 1005, true)));
    const bool complete = admissionComplete && queued == size;
    const auto& stats = router.statistics()[2];
    QCOMPARE(stats.queuedFrames, quint64(complete));
    QCOMPARE(stats.queuedBytes, queued);
    QCOMPARE(stats.droppedFrames, dropEvents);
    QCOMPARE(stats.droppedBytes, complete ? 0 : size - queued);
    const auto destination = router.destinations().first();
    QCOMPARE(destination.queuedFrames, quint64(complete));
    QCOMPARE(destination.queuedBytes, queued);
    QCOMPARE(destination.droppedFrames, dropEvents);
    QCOMPARE(destination.droppedBytes, complete ? 0 : size - queued);
    QList<quint64> lossBytes;
    for (const auto& event : router.events()) {
        if (event.stage == GPSCorrectionStage::Dropped && event.destinationId == QStringLiteral("receiver")) {
            QCOMPARE(event.destinationSession, 7ULL);
            lossBytes.append(event.bytes);
        }
    }
    QList<quint64> expectedLossBytes;
    if (!complete) {
        expectedLossBytes.append(size - queued);
    }
    QCOMPARE(lossBytes, expectedLossBytes);
}

void GPSCorrectionRouterTest::claimedValidatedIngressRequiresCrc_data()
{
    rawInputRequiresUdp_data();
}

void GPSCorrectionRouterTest::claimedValidatedIngressRequiresCrc()
{
    QFETCH(GPSCorrectionSource, category);
    constexpr qint64 now = 100000;
    GPSCorrectionRouter router(nullptr, [] { return now; });
    auto source = router.registerSource(category);
    auto bytes = GpsTestHelpers::buildRtcmFrame(1005, 20);
    bytes[bytes.size() - 1] ^= 1;
    QVERIFY(!router.acceptIngress(source.token().event(bytes, now, 1005, true)));
    const auto& stats = router.statistics().at(static_cast<int>(category));
    QCOMPARE(stats.receivedFrames, 1ULL);
    QCOMPARE(stats.validatedFrames, 0ULL);
    QCOMPARE(stats.selectedFrames, 0ULL);
    QCOMPARE(stats.droppedFrames, 1ULL);
    QCOMPARE(router.events().last().reason, GPSCorrectionReason::InvalidFrame);
    QVERIFY(router.sources().isEmpty());
    QVERIFY(router.acceptIngress(ingress(source, now)));
    QCOMPARE(stats.validatedFrames, 1ULL);
}

void GPSCorrectionRouterTest::registrationMoveAssignment()
{
    constexpr qint64 now = 100000;
    GPSCorrectionRouter router(nullptr, [] { return now; });
    auto first = router.registerSource(GPSCorrectionSource::Ntrip);
    auto second = router.registerSource(GPSCorrectionSource::Udp);
    const auto retained = first.token();
    const auto retired = second.token();
    QVERIFY(router.acceptIngress(ingress(second, now)));
    bool replacementVisibleDuringRetirement = false;
    connect(&router, &GPSCorrectionRouter::sourceInvalidated, &router, [&] {
        replacementVisibleDuringRetirement =
            second.token().source() == GPSCorrectionSource::Ntrip && second.token().valid() && !first.token().valid();
    });
    second = std::move(first);
    QVERIFY(replacementVisibleDuringRetirement);
    QVERIFY(!first.token().valid());
    QVERIFY(second.token().valid());
    QVERIFY(retained.valid());
    QVERIFY(!retired.valid());
    first.reset();
    QVERIFY(retained.valid());
    GPSCorrectionSourceRegistration moved(std::move(second));
    QVERIFY(!second.token().valid());
    QVERIFY(moved.token().valid());
    moved.reset();
    QVERIFY(!retained.valid());
}

void GPSCorrectionRouterTest::fanoutAdmissionAccounting()
{
    qint64 now = 100000;
    GPSCorrectionRouter router(nullptr, [&]() { return now; });
    auto source = router.registerSource(GPSCorrectionSource::Ntrip);
    const auto bytes = GpsTestHelpers::buildRtcmFrame(1077, 500);
    router.setOutput(
        QStringLiteral("mavlink"), {.admit = [](const GPSCorrectionFrame& frame) {
            return QList<GPSCorrectionRouter::Admission>{
                {QStringLiteral("mavlink/1"),
                 {static_cast<quint64>(frame.data.size()), 1, GPSCorrectionReason::None},
                 true},
                {QStringLiteral("mavlink/2"), {180, 2, GPSCorrectionReason::DestinationUnavailable}, false}};
        }});
    QVERIFY(router.acceptIngress(source.token().event(bytes, now, 1077, true)));
    const auto& stats = router.statistics()[2];
    QCOMPARE(stats.receivedFrames, 1ULL);
    QCOMPARE(stats.queuedFrames, 1ULL);
    QCOMPARE(stats.queuedBytes, quint64(bytes.size()));
    QCOMPARE(stats.droppedFrames, 0ULL);
    const auto destinations = router.destinations();
    const auto first = std::find_if(destinations.cbegin(), destinations.cend(), [](const auto& destination) {
        return destination.id == QStringLiteral("mavlink/1");
    });
    const auto second = std::find_if(destinations.cbegin(), destinations.cend(), [](const auto& destination) {
        return destination.id == QStringLiteral("mavlink/2");
    });
    QVERIFY(first != destinations.cend());
    QVERIFY(second != destinations.cend());
    QCOMPARE(first->queuedFrames, 1ULL);
    QCOMPARE(second->queuedFrames, 0ULL);
    QCOMPARE(second->queuedBytes, 180ULL);
    QCOMPARE(second->droppedBytes, quint64(bytes.size() - 180));
}

void GPSCorrectionRouterTest::liveFanoutDestinationsSurviveHistoryChurn()
{
    qint64 now = 100000;
    GPSCorrectionRouter router(nullptr, [&]() { return now; });
    auto source = router.registerSource(GPSCorrectionSource::Ntrip);
    int destinations = GPSCorrectionRouter::MAX_DESTINATION_HISTORY + 4;
    router.setOutput(
        QStringLiteral("mavlink"), {.admit = [&](const GPSCorrectionFrame& frame) {
            QList<GPSCorrectionRouter::Admission> admissions;
            for (int index = 0; index < destinations; ++index) {
                admissions.append({QStringLiteral("mavlink/%1").arg(index),
                                   {quint64(frame.data.size()), quint64(index + 1), GPSCorrectionReason::None},
                                   true});
            }
            return admissions;
        }});
    const auto frame = GpsTestHelpers::buildRtcmFrame(1005, 20);
    for (int iteration = 1; iteration <= 3; ++iteration) {
        QVERIFY(router.acceptIngress(source.token().event(frame, ++now, 1005, true)));
        QCOMPARE(router.destinations().size(), destinations + 1);
        for (const auto& destination : router.destinations()) {
            if (destination.id != QStringLiteral("mavlink")) {
                QCOMPARE(destination.queuedFrames, quint64(iteration));
                QCOMPARE(destination.queuedBytes, quint64(iteration * frame.size()));
            }
        }
    }
    destinations = 0;
    QVERIFY(router.acceptIngress(source.token().event(frame, ++now, 1005, true)));
    QCOMPARE(router.destinations().size(), GPSCorrectionRouter::MAX_DESTINATION_HISTORY + 1);
}

void GPSCorrectionRouterTest::retiredDuringAdmissionPreservesEvidence_data()
{
    QTest::addColumn<bool>("replace");
    QTest::newRow("retired") << false;
    QTest::newRow("replaced") << true;
}

void GPSCorrectionRouterTest::retiredDuringAdmissionPreservesEvidence()
{
    QFETCH(bool, replace);
    qint64 now = 100000;
    GPSCorrectionRouter router(nullptr, [&]() { return now; });
    auto source = router.registerSource(GPSCorrectionSource::Ntrip);
    const quint64 originalSession = source.token().session();
    const auto bytes = GpsTestHelpers::buildRtcmFrame(1005, 20);
    router.setOutput(QStringLiteral("mavlink"), {.admit = [&](const GPSCorrectionFrame& frame) {
                         source.reset();
                         if (replace) {
                             source = router.registerSource(GPSCorrectionSource::Ntrip, QStringLiteral("replacement"));
                         }
                         return QList<GPSCorrectionRouter::Admission>{
                             {QStringLiteral("mavlink/old"),
                              {static_cast<quint64>(frame.data.size()), 1, GPSCorrectionReason::None},
                              true}};
                     }});
    router.acceptIngress(source.token().event(bytes, now, 1005, true));
    QCOMPARE(source.token().valid(), replace);
    const auto destinations = router.destinations();
    const auto admitted = std::find_if(destinations.cbegin(), destinations.cend(), [](const auto& destination) {
        return destination.id == QStringLiteral("mavlink/old");
    });
    QVERIFY(admitted != destinations.cend());
    QCOMPARE(admitted->queuedBytes, quint64(bytes.size()));
    if (replace) {
        QVERIFY(source.token().session() != originalSession);
        QCOMPARE(router.statistics()[2].queuedBytes, 0ULL);
        QCOMPARE(router.statistics()[2].submittedBytes, 0ULL);
    }
}

void GPSCorrectionRouterTest::automaticSelectionAndFailover()
{
    qint64 now = 100000;
    GPSCorrectionRouter router(nullptr, [&now]() { return now; });
    QList<GPSCorrectionFrame> output;
    setAdmissionOutput(router, QStringLiteral("capture"), [&output](const GPSCorrectionFrame& update) {
        output.append(update);
        return update.data.size();
    });
    auto udp = router.registerSource(GPSCorrectionSource::Udp);
    auto ntrip = router.registerSource(GPSCorrectionSource::Ntrip, QStringLiteral("caster/mount"));
    auto local = router.registerSource(GPSCorrectionSource::LocalReceiver, QStringLiteral("usb-device"));
    QVERIFY(router.acceptIngress(ingress(udp, now, QStringLiteral("peer-a"))));
    QVERIFY(!router.acceptIngress(ingress(ntrip, now)));
    QCOMPARE(router.activeSource(), GPSCorrectionSource::Udp);
    now += GPSCorrectionRouter::SWITCH_HOLD_DOWN_MS - 1;
    QVERIFY(!router.acceptIngress(ingress(ntrip, now)));
    ++now;
    QVERIFY(router.acceptIngress(ingress(ntrip, now)));
    QCOMPARE(selectedInstance(router), QStringLiteral("caster/mount"));
    QVERIFY(!router.acceptIngress(ingress(udp, now, QStringLiteral("peer-a"))));
    QVERIFY(!router.acceptIngress(ingress(local, now)));
    now += GPSCorrectionRouter::SWITCH_HOLD_DOWN_MS;
    QVERIFY(router.acceptIngress(ingress(local, now)));
    QCOMPARE(router.activeSource(), GPSCorrectionSource::LocalReceiver);
    now += GPSCorrectionRouter::FRESHNESS_TIMEOUT_MS;
    QVERIFY(router.acceptIngress(ingress(udp, now, QStringLiteral("peer-a"))));
    QCOMPARE(router.activeSource(), GPSCorrectionSource::Udp);
    QCOMPARE(output.size(), 4);
}

void GPSCorrectionRouterTest::peerSelectionDoesNotInterleave()
{
    qint64 now = 100000;
    GPSCorrectionSelector selector;
    const auto observe = [&](const QString& instance) {
        const auto update = frame(GPSCorrectionSource::Udp, 1, now, instance);
        selector.observe(update, true, now);
        return selector.selected(update, now);
    };
    QVERIFY(observe(QStringLiteral("first")));
    for (int i = 0; i < 20; ++i) {
        now += 100;
        QVERIFY(!observe(QStringLiteral("second")));
        QVERIFY(observe(QStringLiteral("first")));
    }
    selector.configure({GPSCorrectionSelector::Policy::Manual, GPSCorrectionSource::Udp, QStringLiteral("second")},
                       now);
    QVERIFY(!observe(QStringLiteral("first")));
    QVERIFY(observe(QStringLiteral("second")));
    QVERIFY(selector.activeIdentity(now));
    QCOMPARE(selector.activeIdentity(now)->instance, QStringLiteral("second"));
    selector.configure({GPSCorrectionSelector::Policy::Manual, GPSCorrectionSource::Udp, QStringLiteral("absent")},
                       now);
    QVERIFY(!observe(QStringLiteral("first")));
    QVERIFY(!observe(QStringLiteral("second")));
    QCOMPARE(selector.sources().size(), 2);
}

void GPSCorrectionRouterTest::sessionAndReceiptValidation()
{
    qint64 now = 100000;
    GPSCorrectionRouter router(nullptr, [&now]() { return now; });
    auto registration = router.registerSource(GPSCorrectionSource::Ntrip, QStringLiteral("one"));
    const auto retired = ingress(registration, now);
    QVERIFY(router.acceptIngress(retired));
    registration.reset();
    QVERIFY(!router.acceptIngress(retired));
    registration = router.registerSource(GPSCorrectionSource::Ntrip, QStringLiteral("two"));
    QVERIFY(!router.acceptIngress(retired));
    QVERIFY(!router.acceptIngress(ingress(registration, now, QStringLiteral("one"))));
    QVERIFY(!router.acceptIngress(ingress(registration, now + 1)));
    QVERIFY(!router.acceptIngress(ingress(registration, now - GPSCorrectionRouter::FRESHNESS_TIMEOUT_MS)));
    QVERIFY(!router.acceptIngress(
        registration.token().event(GpsTestHelpers::buildRtcmFrame(1005, 20), now, 1005, true, true)));
    QVERIFY(selectedInstance(router).isEmpty());
    QVERIFY(router.acceptIngress(ingress(registration, now)));
    const auto& stats = router.statistics().at(static_cast<int>(GPSCorrectionSource::Ntrip));
    QCOMPARE(stats.filteredFrames, quint64(3));
    QCOMPARE(stats.selectedFrames, quint64(1));
    QCOMPARE(stats.lastValidMs, now);
}

void GPSCorrectionRouterTest::sourceIdentityOrderingAndRetirement()
{
    constexpr qint64 now = 100000;
    GPSCorrectionSelector selector;
    for (const QString& instance : {QStringLiteral("peer/2"), QStringLiteral("peer#2"), QString()}) {
        selector.observe(frame(GPSCorrectionSource::Udp, 1, now, instance), true, now);
    }
    const QString sharedInstance = QStringLiteral("peer/2");
    selector.observe(frame(GPSCorrectionSource::Ntrip, 2, now, sharedInstance), true, now);
    selector.observe(frame(GPSCorrectionSource::LocalReceiver, 3, now, sharedInstance), true, now);
    selector.configure({}, now);
    QCOMPARE(selector.activeSource(now), GPSCorrectionSource::LocalReceiver);
    const auto sources = selector.sources();
    QCOMPARE(sources.size(), 5);
    QCOMPARE(sources[0].identity.category, GPSCorrectionSource::LocalReceiver);
    QCOMPARE(sources[1].identity.category, GPSCorrectionSource::Ntrip);
    QVERIFY(sources[2].identity.instance.isEmpty());
    QCOMPARE(sources[3].identity.instance, QStringLiteral("peer#2"));
    QCOMPARE(sources[4].identity.instance, sharedInstance);

    selector.retire(GPSCorrectionSource::LocalReceiver, now);
    QCOMPARE(selector.activeSource(now), GPSCorrectionSource::Ntrip);
    selector.retire(GPSCorrectionSource::Ntrip, now);
    QCOMPARE(selector.activeSource(now), GPSCorrectionSource::Udp);
    QVERIFY(selector.activeIdentity(now)->instance.isEmpty());
    QVERIFY(selector.selected(frame(GPSCorrectionSource::Udp, 1, now), now));
    QVERIFY(!selector.selected(frame(GPSCorrectionSource::Udp, 1, now, sharedInstance), now));
}

void GPSCorrectionRouterTest::nonRoutablePeersRemainObserved_data()
{
    QTest::addColumn<bool>("filtered");
    QTest::addColumn<qint64>("age");
    QTest::newRow("filtered") << true << qint64(0);
    QTest::newRow("expired") << false << GPSCorrectionRouter::FRESHNESS_TIMEOUT_MS;
    QTest::newRow("filtered-and-expired") << true << GPSCorrectionRouter::FRESHNESS_TIMEOUT_MS;
}

void GPSCorrectionRouterTest::nonRoutablePeersRemainObserved()
{
    QFETCH(bool, filtered);
    QFETCH(qint64, age);
    constexpr qint64 now = 100000;
    GPSCorrectionRouter router(nullptr, [] { return now; });
    auto source = router.registerSource(GPSCorrectionSource::Udp);
    const QString instance = QStringLiteral("peer/#2");
    const auto update = source.token().event(GpsTestHelpers::buildRtcmFrame(1005, 20), now - age, 1005, true, filtered,
                                             GPSCorrectionReason::None, instance);
    QVERIFY(!router.acceptIngress(update));
    const auto peers = router.sources();
    QCOMPARE(peers.size(), 1);
    QCOMPARE(peers.first().identity.category, GPSCorrectionSource::Udp);
    QCOMPARE(peers.first().identity.instance, instance);
    QCOMPARE(peers.first().lastReceivedMs, now - age);
    QCOMPARE(peers.first().lastRoutableMs, qint64(0));
    QCOMPARE(router.statistics()[3].lastValidMs, now - age);
    QCOMPARE(router.activeSource(), GPSCorrectionSource::Unknown);
    QCOMPARE(router.sourceDiagnostics()[3].toMap().value(QStringLiteral("usable")).toBool(), age == 0);
    QVERIFY(!router.sourceInstanceDiagnostics().first().toMap().value(QStringLiteral("usable")).toBool());
    QCOMPARE(router.events().last().reason,
             filtered ? GPSCorrectionReason::MessageFiltered : GPSCorrectionReason::Expired);
}

void GPSCorrectionRouterTest::sinkResults()
{
    qint64 now = 100000;
    GPSCorrectionRouter router(nullptr, [&now]() { return now; });
    quint64 bytes = 0;
    setAdmissionOutput(router, QStringLiteral("accepted"), [&bytes](const GPSCorrectionFrame& update) {
        bytes += update.data.size();
        return update.data.size();
    });
    setAdmissionOutput(router, QStringLiteral("unavailable"), [](const GPSCorrectionFrame&) { return quint64(0); });
    const auto source = router.registerSource(GPSCorrectionSource::Udp);
    const auto submittedBytes = [&router]() {
        return router.statistics().at(static_cast<int>(GPSCorrectionSource::Udp)).submittedBytes;
    };
    const auto update = ingress(source, now);
    const auto frameBytes = quint64(update.frame().data.size());
    QVERIFY(router.acceptIngress(update));
    QCOMPARE(submittedBytes(), frameBytes);
    QCOMPARE(bytes, frameBytes);
    router.removeSink(QStringLiteral("accepted"));
    QVERIFY(router.acceptIngress(ingress(source, now)));
    QCOMPARE(submittedBytes(), frameBytes);
    QCOMPARE(bytes, frameBytes);
    const auto rows = router.sourceInstanceDiagnostics();
    QCOMPARE(rows.size(), 1);
    QVERIFY(rows.first().toMap().value(QStringLiteral("selected")).toBool());
    now += GPSCorrectionRouter::FRESHNESS_TIMEOUT_MS;
    const auto expired = router.sourceInstanceDiagnostics().first().toMap();
    QVERIFY(!expired.value(QStringLiteral("usable")).toBool());
    QVERIFY(!expired.value(QStringLiteral("selected")).toBool());
}

void GPSCorrectionRouterTest::emptyOutputRemovesRegistration_data()
{
    QTest::addColumn<bool>("emptySink");
    QTest::newRow("empty-output") << false;
    QTest::newRow("empty-admission-sink") << true;
}

void GPSCorrectionRouterTest::emptyOutputRemovesRegistration()
{
    QFETCH(bool, emptySink);
    constexpr qint64 now = 100000;
    GPSCorrectionRouter router(nullptr, [] { return now; });
    auto source = router.registerSource(GPSCorrectionSource::Ntrip);
    const QString id = QStringLiteral("receiver");
    int submissions = 0;
    setAdmissionOutput(router, id, [&](const GPSCorrectionFrame& received) {
        ++submissions;
        return quint64(received.data.size());
    });
    const auto update = ingress(source, now);
    QVERIFY(router.acceptIngress(update));
    QCOMPARE(router.destinations().first().queuedBytes, quint64(update.frame().data.size()));
    router.setOutput(id, emptySink ? GPSCorrectionRouter::admissionOnlyOutput(id, GPSCorrectionSource::Unknown, {})
                                   : GPSCorrectionRouter::Output{});
    QVERIFY(router.acceptIngress(update));
    QCOMPARE(submissions, 1);
}

void GPSCorrectionRouterTest::sourceChangePrecedesSubmission_data()
{
    QTest::addColumn<QString>("first");
    QTest::addColumn<QString>("second");
    QTest::newRow("named-peers") << QStringLiteral("first") << QStringLiteral("second");
    QTest::newRow("empty-and-delimiters") << QString() << QStringLiteral("3/peer#2");
    QTest::newRow("case-sensitive") << QStringLiteral("Peer") << QStringLiteral("peer");
}

void GPSCorrectionRouterTest::sourceChangePrecedesSubmission()
{
    QFETCH(QString, first);
    QFETCH(QString, second);
    qint64 now = 100000;
    GPSCorrectionRouter router(nullptr, [&now]() { return now; });
    QStringList calls;
    connect(&router, &GPSCorrectionRouter::sourceSelected, this,
            [&calls](GPSCorrectionSource, const QString&) { calls.append(QStringLiteral("clear")); });
    setAdmissionOutput(router, QStringLiteral("receiver"), [&calls](const GPSCorrectionFrame& update) {
        calls.append(QStringLiteral("submit"));
        return update.data.size();
    });
    auto source = router.registerSource(GPSCorrectionSource::Udp);
    router.applyConfiguration({GPSCorrectionRouter::Policy::Manual, GPSCorrectionSource::Udp, first});
    QVERIFY(calls.isEmpty());
    QVERIFY(router.acceptIngress(ingress(source, now, first)));
    QVERIFY(router.acceptIngress(ingress(source, now, first)));
    router.applyConfiguration({GPSCorrectionRouter::Policy::Manual, GPSCorrectionSource::Udp, second});
    QVERIFY(router.acceptIngress(ingress(source, now, second)));
    source = router.registerSource(GPSCorrectionSource::Udp);
    QVERIFY(router.acceptIngress(ingress(source, now, second)));
    QCOMPARE(calls, QStringList({QStringLiteral("clear"), QStringLiteral("submit"), QStringLiteral("submit"),
                                 QStringLiteral("clear"), QStringLiteral("submit"), QStringLiteral("clear"),
                                 QStringLiteral("submit")}));
}

void GPSCorrectionRouterTest::teardownAndReentrancy()
{
    qint64 now = 100000;
    auto* router = new GPSCorrectionRouter(nullptr, [&now]() { return now; });
    const QPointer<GPSCorrectionRouter> guard(router);
    auto source = router->registerSource(GPSCorrectionSource::Udp);
    const auto update = ingress(source, now);
    bool recursiveAccepted = true;
    bool lateSinkCalled = false;
    setAdmissionOutput(*router, QStringLiteral("a-teardown"),
                       [router, &recursiveAccepted, update](const GPSCorrectionFrame&) {
                           recursiveAccepted = router->acceptIngress(update);
                           delete router;
                           return quint64(0);
                       });
    setAdmissionOutput(*router, QStringLiteral("z-retired"), [&lateSinkCalled](const GPSCorrectionFrame&) {
        lateSinkCalled = true;
        return quint64(0);
    });
    QVERIFY(!router->acceptIngress(update));
    QVERIFY(!guard);
    QVERIFY(!recursiveAccepted);
    QVERIFY(!lateSinkCalled);
}

void GPSCorrectionRouterTest::endingSelectedSessionInvalidatesOutput()
{
    qint64 now = 100000;
    GPSCorrectionRouter router(nullptr, [&now]() { return now; });
    auto source = router.registerSource(GPSCorrectionSource::Ntrip);
    const auto update = ingress(source, now);
    QSignalSpy invalidated(&router, &GPSCorrectionRouter::sourceInvalidated);
    QVERIFY(router.acceptIngress(update));
    auto udp = router.registerSource(GPSCorrectionSource::Udp);
    udp.reset();
    QVERIFY(invalidated.isEmpty());
    source.reset();
    QCOMPARE(invalidated.size(), 1);
    QVERIFY(!router.acceptIngress(update));
    source.reset();
    QCOMPARE(invalidated.size(), 1);
}

void GPSCorrectionRouterTest::boundedPeerHistory()
{
    qint64 now = 100000;
    GPSCorrectionRouter router(nullptr, [&now]() { return now; });
    auto source = router.registerSource(GPSCorrectionSource::Udp);
    QVERIFY(router.acceptIngress(ingress(source, now, QStringLiteral("selected"))));
    for (int i = 0; i < 1000; ++i) {
        QVERIFY(!router.acceptIngress(ingress(source, now, QString::number(i))));
        QVERIFY(router.sources().size() <= GPSCorrectionRouter::MAX_SOURCE_INSTANCES);
    }
    QCOMPARE(selectedInstance(router), QStringLiteral("selected"));
    router.shutdown();
    QVERIFY(!router.acceptIngress(ingress(source, now, QStringLiteral("selected"))));
    QVERIFY(router.sources().isEmpty());
}

void GPSCorrectionRouterTest::diagnosticStagesStayDistinct_data()
{
    using Stage = GPSCorrectionStage;
    QTest::addColumn<bool>("validated");
    QTest::addColumn<bool>("filtered");
    QTest::addColumn<bool>("queueAccepted");
    QTest::addColumn<QList<Stage>>("stages");
    QTest::newRow("valid-queued") << true << false << true
                                  << QList{Stage::Received, Stage::Validated, Stage::Selected, Stage::Queued};
    QTest::newRow("unvalidated-passthrough")
        << false << false << true << QList{Stage::Received, Stage::Selected, Stage::Queued};
    QTest::newRow("whitelist-filtered") << true << true << true
                                        << QList{Stage::Received, Stage::Validated, Stage::Dropped};
    QTest::newRow("queue-full") << true << false << false
                                << QList{Stage::Received, Stage::Validated, Stage::Selected, Stage::Dropped,
                                         Stage::Dropped};
}

void GPSCorrectionRouterTest::diagnosticStagesStayDistinct()
{
    QFETCH(bool, validated);
    QFETCH(bool, filtered);
    QFETCH(bool, queueAccepted);
    QFETCH(QList<GPSCorrectionStage>, stages);
    qint64 now = 100000;
    GPSCorrectionRouter router(nullptr, [&now]() { return now; });
    auto source = router.registerSource(GPSCorrectionSource::Udp);
    GPSCorrectionFrame admitted;
    router.setOutput(QStringLiteral("receiver"), {.admit = [&](const GPSCorrectionFrame& received) {
                         admitted = received;
                         return QList<GPSCorrectionRouter::Admission>{
                             {QStringLiteral("receiver"),
                              {queueAccepted ? quint64(received.data.size()) : 0, 7,
                               queueAccepted ? GPSCorrectionReason::None : GPSCorrectionReason::QueueFull},
                              true}};
                     }});
    const auto update = source.token().event(GpsTestHelpers::buildRtcmFrame(1005, 20), now, 1005, validated, filtered,
                                             GPSCorrectionReason::None, QStringLiteral("peer"));
    QCOMPARE(router.acceptIngress(update), !filtered);
    QCOMPARE(admitted.data, filtered ? QByteArray() : update.frame().data);
    QCOMPARE(router.activeSource(), filtered ? GPSCorrectionSource::Unknown : GPSCorrectionSource::Udp);
    const auto& stats = router.statistics().at(static_cast<int>(GPSCorrectionSource::Udp));
    const quint64 size = update.frame().data.size();
    QCOMPARE(stats.receivedFrames, quint64(1));
    QCOMPARE(stats.receivedBytes, size);
    QCOMPARE(stats.validatedFrames, quint64(validated));
    QCOMPARE(stats.validatedBytes, validated ? size : 0);
    QCOMPARE(stats.selectedFrames, quint64(!filtered));
    QCOMPARE(stats.selectedBytes, filtered ? 0 : size);
    QCOMPARE(stats.queuedFrames, quint64(!filtered && queueAccepted));
    QCOMPARE(stats.queuedBytes, !filtered && queueAccepted ? size : 0);
    GPSCorrectionEventModel events;
    events.setEvents(router.events());
    QCOMPARE(events.rowCount(), stages.size());
    for (int index = 0; index < events.rowCount(); ++index) {
        QCOMPARE(events.index(index).data(GPSCorrectionEventModel::StageRole).toInt(), int(stages[index]));
    }
    const auto& last = router.events().last();
    QCOMPARE(last.stage, !filtered && queueAccepted ? GPSCorrectionStage::Queued : GPSCorrectionStage::Dropped);
    QCOMPARE(last.reason, filtered        ? GPSCorrectionReason::MessageFiltered
                          : queueAccepted ? GPSCorrectionReason::None
                                          : GPSCorrectionReason::QueueFull);
    QCOMPARE(last.sourceSession, source.token().session());
    QCOMPARE(last.sourceInstance, QStringLiteral("peer"));
}

void GPSCorrectionRouterTest::boundedDiagnosticsAndEventHistory()
{
    qint64 now = 100000;
    GPSCorrectionRouter router(nullptr, [&now]() { return now; });
    quint64 submissions = 0;
    setAdmissionOutput(router, QStringLiteral("receiver"), [&submissions](const GPSCorrectionFrame& received) {
        ++submissions;
        return quint64(received.data.size());
    });
    auto source = router.registerSource(GPSCorrectionSource::Udp);
    for (qsizetype i = 0; i < GPSCorrectionRouter::MAX_EVENTS + 20; ++i) {
        QVERIFY(router.acceptIngress(ingress(source, now)));
        QVERIFY(router.events().size() <= GPSCorrectionRouter::MAX_EVENTS);
    }
    QCOMPARE(submissions, quint64(GPSCorrectionRouter::MAX_EVENTS + 20));
    const auto destination = router.destinations().first();
    QCOMPARE(destination.queuedFrames, submissions);
    GPSCorrectionEventModel model;
    QAbstractItemModelTester tester(&model, QAbstractItemModelTester::FailureReportingMode::QtTest);
    model.setEvents(router.events());
    QCOMPARE(model.rowCount(), int(GPSCorrectionRouter::MAX_EVENTS));
    const auto last = model.index(model.rowCount() - 1);
    QCOMPARE(model.data(last, GPSCorrectionEventModel::StageRole).toInt(), int(GPSCorrectionStage::Queued));
    QCOMPARE(model.data(last, GPSCorrectionEventModel::DestinationIdRole).toString(), QStringLiteral("receiver"));
    QVERIFY(!model.roleNames().values().contains(QByteArrayLiteral("data")));
    QSignalSpy resets(&model, &QAbstractItemModel::modelReset);
    model.setEvents(router.events());
    QVERIFY(resets.isEmpty());
    model.setEvents({});
    QCOMPARE(model.rowCount(), 0);
    QVERIFY(resets.isEmpty());
}

void GPSCorrectionRouterTest::rejectedCandidateHasNoValidatedCredit()
{
    qint64 now = 100000;
    GPSCorrectionRouter router(nullptr, [&now]() { return now; });
    auto source = router.registerSource(GPSCorrectionSource::Udp);
    const auto rejected = source.token().event(GpsTestHelpers::buildRtcmFrame(1005, 20), now, 1005, false, false,
                                               GPSCorrectionReason::InvalidFrame, QStringLiteral("peer"));
    QVERIFY(!router.acceptIngress(rejected));
    const auto& stats = router.statistics().at(static_cast<int>(GPSCorrectionSource::Udp));
    QCOMPARE(stats.receivedFrames, quint64(1));
    QCOMPARE(stats.validatedFrames, quint64(0));
    QCOMPARE(stats.selectedFrames, quint64(0));
    QCOMPARE(stats.droppedBytes, quint64(rejected.frame().data.size()));
    QCOMPARE(router.events().last().reason, GPSCorrectionReason::InvalidFrame);
    QVERIFY(router.sources().isEmpty());
}

void GPSCorrectionRouterTest::sourceSpecificSinkPreservesNtripForwarding()
{
    qint64 now = 100000;
    GPSCorrectionRouter router(nullptr, [&]() { return now; });
    auto ntrip = router.registerSource(GPSCorrectionSource::Ntrip);
    auto local = router.registerSource(GPSCorrectionSource::LocalReceiver);
    QList<GPSCorrectionFrame> selected;
    QList<GPSCorrectionFrame> forwarded;
    setAdmissionOutput(router, QStringLiteral("selected"), [&](const GPSCorrectionFrame& value) {
        selected.append(value);
        return value.data.size();
    });
    setAdmissionOutput(
        router, QStringLiteral("ntripUdp"),
        [&](const GPSCorrectionFrame& value) {
            forwarded.append(value);
            return value.data.size();
        },
        GPSCorrectionSource::Ntrip);
    QVERIFY(router.acceptIngress(ingress(local, now)));
    QVERIFY(!router.acceptIngress(ingress(ntrip, now)));
    QCOMPARE(selected.size(), 1);
    QCOMPARE(forwarded.size(), 1);
    QCOMPARE(forwarded.first().source, GPSCorrectionSource::Ntrip);
    QVERIFY(
        !router.acceptIngress(ntrip.token().event(GpsTestHelpers::buildRtcmFrame(1005, 20), now, 1005, true, true)));
    const auto expired = ingress(ntrip, now);
    now += GPSCorrectionRouter::FRESHNESS_TIMEOUT_MS;
    QVERIFY(!router.acceptIngress(expired));
    QCOMPARE(forwarded.size(), 1);
    router.removeSink(QStringLiteral("ntripUdp"));
    router.acceptIngress(ingress(ntrip, now));
    QCOMPARE(forwarded.size(), 1);
}

void GPSCorrectionRouterTest::scopedAdmissionAccounting_data()
{
    QTest::addColumn<bool>("globallySelected");
    QTest::addColumn<bool>("scopedAdmitted");
    QTest::addColumn<quint64>("sourceDropEvents");
    QTest::newRow("unselected-scoped-admitted") << false << true << quint64(1);
    QTest::newRow("unselected-scoped-rejected") << false << false << quint64(2);
    QTest::newRow("selected-scoped-admitted") << true << true << quint64(0);
    QTest::newRow("selected-all-rejected") << true << false << quint64(1);
}

void GPSCorrectionRouterTest::scopedAdmissionAccounting()
{
    QFETCH(bool, globallySelected);
    QFETCH(bool, scopedAdmitted);
    QFETCH(quint64, sourceDropEvents);
    constexpr qint64 now = 100000;
    GPSCorrectionRouter router(nullptr, [] { return now; });
    auto source = router.registerSource(GPSCorrectionSource::Ntrip);
    router.applyConfiguration({GPSCorrectionRouter::Policy::Manual,
                               globallySelected ? GPSCorrectionSource::Ntrip : GPSCorrectionSource::LocalReceiver,
                               {}});
    setAdmissionOutput(router, QStringLiteral("global"), [](const GPSCorrectionFrame&) { return quint64(0); });
    setAdmissionOutput(
        router, QStringLiteral("ntripUdp"),
        [scopedAdmitted](const GPSCorrectionFrame& frame) { return scopedAdmitted ? quint64(frame.data.size()) : 0; },
        GPSCorrectionSource::Ntrip);
    const auto update = ingress(source, now);
    const quint64 size = update.frame().data.size();
    QCOMPARE(router.acceptIngress(update), globallySelected);
    const auto& stats = router.statistics()[2];
    QCOMPARE(stats.receivedFrames, 1ULL);
    QCOMPARE(stats.receivedBytes, size);
    QCOMPARE(stats.selectedFrames, quint64(globallySelected));
    QCOMPARE(stats.queuedFrames, quint64(scopedAdmitted));
    QCOMPARE(stats.queuedBytes, scopedAdmitted ? size : 0);
    QCOMPARE(stats.droppedFrames, sourceDropEvents);
    QCOMPARE(stats.droppedBytes, size * sourceDropEvents);
    const auto destinations = router.destinations();
    const auto scoped = std::find_if(destinations.cbegin(), destinations.cend(), [](const auto& destination) {
        return destination.id == QStringLiteral("ntripUdp");
    });
    QVERIFY(scoped != destinations.cend());
    QCOMPARE(scoped->queuedFrames, quint64(scopedAdmitted));
    QCOMPARE(scoped->queuedBytes, scopedAdmitted ? size : 0);
    QCOMPARE(scoped->droppedFrames, quint64(!scopedAdmitted));
    QCOMPARE(scoped->droppedBytes, scopedAdmitted ? 0 : size);
    int selectionRejections = 0;
    int scopedRejections = 0;
    for (const auto& event : router.events()) {
        if (event.stage != GPSCorrectionStage::Dropped) {
            continue;
        }
        if (event.reason == GPSCorrectionReason::NotSelected) {
            ++selectionRejections;
            QVERIFY(event.destinationId.isEmpty());
            QCOMPARE(event.bytes, size);
        } else if (event.destinationId == QStringLiteral("ntripUdp")) {
            ++scopedRejections;
            QCOMPARE(event.reason, GPSCorrectionReason::DestinationUnavailable);
            QCOMPARE(event.bytes, size);
        }
    }
    QCOMPARE(selectionRejections, int(!globallySelected));
    QCOMPARE(scopedRejections, int(!scopedAdmitted));
}

void GPSCorrectionRouterTest::eventHistoryUsesIncrementalRows()
{
    GPSCorrectionEventModel model;
    QAbstractItemModelTester tester(&model, QAbstractItemModelTester::FailureReportingMode::QtTest);
    QSignalSpy resets(&model, &QAbstractItemModel::modelReset);
    QSignalSpy inserts(&model, &QAbstractItemModel::rowsInserted);
    QSignalSpy removals(&model, &QAbstractItemModel::rowsRemoved);
    QList<GPSCorrectionEvent> events;
    for (quint64 index = 1; index <= 256; ++index) {
        events.append({.sequence = index});
    }
    model.setEvents(events);
    QCOMPARE(model.rowCount(), 256);
    QCOMPARE(inserts.size(), 1);
    QPersistentModelIndex retained = model.index(10);
    events.removeFirst();
    events.append({.sequence = 257});
    model.setEvents(events);
    QCOMPARE(removals.size(), 1);
    QCOMPARE(inserts.size(), 2);
    QVERIFY(retained.isValid());
    QCOMPARE(retained.row(), 9);
    QCOMPARE(retained.data(GPSCorrectionEventModel::EventSequenceRole).toULongLong(), quint64(11));
    model.setEvents(events);
    QCOMPARE(inserts.size(), 2);
    model.setEvents({});
    QCOMPARE(model.rowCount(), 0);
    QCOMPARE(removals.size(), 2);
    QVERIFY(resets.isEmpty());
}

void GPSCorrectionRouterTest::eventHistoryAllowsReentrantUpdates()
{
    GPSCorrectionEventModel model;
    QAbstractItemModelTester tester(&model, QAbstractItemModelTester::FailureReportingMode::QtTest);
    bool first = true;
    connect(&model, &QAbstractItemModel::rowsAboutToBeInserted, &model, [&]() {
        if (std::exchange(first, false)) {
            model.setEvents({GPSCorrectionEvent{.sequence = 2}, GPSCorrectionEvent{.sequence = 3}});
        }
    });
    model.setEvents({GPSCorrectionEvent{.sequence = 1}});
    QCOMPARE(model.rowCount(), 2);
    QCOMPARE(model.index(0).data(GPSCorrectionEventModel::EventSequenceRole).toULongLong(), quint64(2));
}

UT_REGISTER_TEST(GPSCorrectionRouterTest, TestLabel::Unit)

#include "GPSCorrectionRouterTest.moc"
