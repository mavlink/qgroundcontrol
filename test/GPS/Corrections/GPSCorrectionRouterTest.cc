#include <QtCore/QPointer>
#include <QtTest/QAbstractItemModelTester>
#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

#include <utility>

#include "../GpsTestHelpers.h"
#include "GPSCorrectionEventModel.h"
#include "GPSCorrectionRouter.h"

namespace {
GPSCorrectionFrame frame(GPSCorrectionSource source, quint64 session, qint64 now, const QString& instance = {})
{
    return {source, session, now, GpsTestHelpers::buildRtcmFrame(1005, 20), 1005, true, false, instance};
}
}  // namespace

class GPSCorrectionRouterTest : public QObject
{
    Q_OBJECT

private slots:
    void automaticSelectionAndFailover();
    void peerSelectionDoesNotInterleave();
    void sessionAndReceiptValidation();
    void explicitAllAndSinkResults();
    void sourceChangePrecedesSubmission();
    void endingSelectedSessionInvalidatesOutput();
    void teardownAndReentrancy();
    void boundedPeerHistory();
    void diagnosticStagesStayDistinct_data();
    void diagnosticStagesStayDistinct();
    void terminalDeliveryAccounting_data();
    void terminalDeliveryAccounting();
    void lateDeliveryDoesNotCreditReplacementSource();
    void boundedDiagnosticsAndUnconfirmedRetirement();
    void rejectedCandidateHasNoValidatedCredit();
    void uncertainDeliveryIsNotDropped();
    void sourceSpecificSinkPreservesNtripForwarding();
    void eventHistoryUsesIncrementalRows();
    void eventHistoryAllowsReentrantUpdates();
};

void GPSCorrectionRouterTest::automaticSelectionAndFailover()
{
    qint64 now = 100000;
    GPSCorrectionRouter router(nullptr, [&now]() { return now; });
    QList<GPSCorrectionFrame> output;
    router.setSink(QStringLiteral("capture"), [&output](const GPSCorrectionFrame& update) {
        output.append(update);
        return update.data.size();
    });
    const quint64 udp = router.beginSourceSession(GPSCorrectionSource::Udp);
    const quint64 ntrip = router.beginSourceSession(GPSCorrectionSource::Ntrip, QStringLiteral("caster/mount"));
    const quint64 local = router.beginSourceSession(GPSCorrectionSource::LocalReceiver, QStringLiteral("usb-device"));
    QVERIFY(router.acceptFrame(frame(GPSCorrectionSource::Udp, udp, now, QStringLiteral("peer-a"))));
    QVERIFY(!router.acceptFrame(frame(GPSCorrectionSource::Ntrip, ntrip, now)));
    QCOMPARE(router.activeSource(), GPSCorrectionSource::Udp);
    now += GPSCorrectionRouter::SWITCH_HOLD_DOWN_MS - 1;
    QVERIFY(!router.acceptFrame(frame(GPSCorrectionSource::Ntrip, ntrip, now)));
    ++now;
    QVERIFY(router.acceptFrame(frame(GPSCorrectionSource::Ntrip, ntrip, now)));
    QCOMPARE(router.activeInstance(), QStringLiteral("caster/mount"));
    QVERIFY(!router.acceptFrame(frame(GPSCorrectionSource::Udp, udp, now, QStringLiteral("peer-a"))));
    QVERIFY(!router.acceptFrame(frame(GPSCorrectionSource::LocalReceiver, local, now)));
    now += GPSCorrectionRouter::SWITCH_HOLD_DOWN_MS;
    QVERIFY(router.acceptFrame(frame(GPSCorrectionSource::LocalReceiver, local, now)));
    QCOMPARE(router.activeSource(), GPSCorrectionSource::LocalReceiver);
    now += GPSCorrectionRouter::FRESHNESS_TIMEOUT_MS;
    QVERIFY(router.acceptFrame(frame(GPSCorrectionSource::Udp, udp, now, QStringLiteral("peer-a"))));
    QCOMPARE(router.activeSource(), GPSCorrectionSource::Udp);
    QCOMPARE(output.size(), 4);
}

void GPSCorrectionRouterTest::peerSelectionDoesNotInterleave()
{
    qint64 now = 100000;
    GPSCorrectionRouter router(nullptr, [&now]() { return now; });
    const quint64 session = router.beginSourceSession(GPSCorrectionSource::Udp);
    QVERIFY(router.acceptFrame(frame(GPSCorrectionSource::Udp, session, now, QStringLiteral("first"))));
    for (int i = 0; i < 20; ++i) {
        now += 100;
        QVERIFY(!router.acceptFrame(frame(GPSCorrectionSource::Udp, session, now, QStringLiteral("second"))));
        QVERIFY(router.acceptFrame(frame(GPSCorrectionSource::Udp, session, now, QStringLiteral("first"))));
    }
    router.setSelectedSource(GPSCorrectionSource::Udp, QStringLiteral("second"));
    router.setPolicy(GPSCorrectionRouter::Policy::Manual);
    QVERIFY(!router.acceptFrame(frame(GPSCorrectionSource::Udp, session, now, QStringLiteral("first"))));
    QVERIFY(router.acceptFrame(frame(GPSCorrectionSource::Udp, session, now, QStringLiteral("second"))));
    QCOMPARE(router.activeInstance(), QStringLiteral("second"));
    router.setSelectedSource(GPSCorrectionSource::Udp, QStringLiteral("absent"));
    QVERIFY(!router.acceptFrame(frame(GPSCorrectionSource::Udp, session, now, QStringLiteral("first"))));
    QVERIFY(!router.acceptFrame(frame(GPSCorrectionSource::Udp, session, now, QStringLiteral("second"))));
    QCOMPARE(router.sources().size(), 2);
}

void GPSCorrectionRouterTest::sessionAndReceiptValidation()
{
    qint64 now = 100000;
    GPSCorrectionRouter router(nullptr, [&now]() { return now; });
    const quint64 old = router.beginSourceSession(GPSCorrectionSource::Ntrip, QStringLiteral("one"));
    QVERIFY(router.acceptFrame(frame(GPSCorrectionSource::Ntrip, old, now)));
    router.endSourceSession(GPSCorrectionSource::Ntrip);
    QVERIFY(!router.acceptFrame(frame(GPSCorrectionSource::Ntrip, old, now)));
    const quint64 current = router.beginSourceSession(GPSCorrectionSource::Ntrip, QStringLiteral("two"));
    QVERIFY(!router.acceptFrame(frame(GPSCorrectionSource::Ntrip, old, now)));
    QVERIFY(!router.acceptFrame(frame(GPSCorrectionSource::Ntrip, current, now, QStringLiteral("one"))));
    QVERIFY(!router.acceptFrame(frame(GPSCorrectionSource::Ntrip, current, now + 1)));
    QVERIFY(!router.acceptFrame(
        frame(GPSCorrectionSource::Ntrip, current, now - GPSCorrectionRouter::FRESHNESS_TIMEOUT_MS)));
    auto filtered = frame(GPSCorrectionSource::Ntrip, current, now);
    filtered.filtered = true;
    QVERIFY(!router.acceptFrame(filtered));
    QVERIFY(router.activeInstance().isEmpty());
    QVERIFY(router.acceptFrame(frame(GPSCorrectionSource::Ntrip, current, now)));
    const auto& stats = router.statistics().at(static_cast<int>(GPSCorrectionSource::Ntrip));
    QCOMPARE(stats.filteredFrames, quint64(3));
    QCOMPARE(stats.routedFrames, quint64(1));
    QCOMPARE(stats.lastValidMs, now);
}

void GPSCorrectionRouterTest::explicitAllAndSinkResults()
{
    qint64 now = 100000;
    GPSCorrectionRouter router(nullptr, [&now]() { return now; });
    router.setPolicy(GPSCorrectionRouter::Policy::All);
    quint64 bytes = 0;
    router.setSink(QStringLiteral("accepted"), [&bytes](const GPSCorrectionFrame& update) {
        bytes += update.data.size();
        return update.data.size();
    });
    router.setSink(QStringLiteral("unavailable"), [](const GPSCorrectionFrame&) { return quint64(0); });
    for (const auto source :
         {GPSCorrectionSource::LocalReceiver, GPSCorrectionSource::Ntrip, GPSCorrectionSource::Udp}) {
        const quint64 session = router.beginSourceSession(source);
        const auto update = frame(source, session, now);
        QVERIFY(router.acceptFrame(update));
        QCOMPARE(router.statistics().at(static_cast<int>(source)).submittedBytes, quint64(update.data.size()));
    }
    QCOMPARE(bytes, quint64(frame(GPSCorrectionSource::Unknown, 0, now).data.size() * 3));
    router.removeSink(QStringLiteral("accepted"));
    const auto update = frame(GPSCorrectionSource::Udp, router.sourceSession(GPSCorrectionSource::Udp), now);
    QVERIFY(router.acceptFrame(update));
    QCOMPARE(router.statistics().at(static_cast<int>(GPSCorrectionSource::Udp)).submittedBytes,
             quint64(update.data.size()));
}

void GPSCorrectionRouterTest::sourceChangePrecedesSubmission()
{
    qint64 now = 100000;
    GPSCorrectionRouter router(nullptr, [&now]() { return now; });
    QStringList calls;
    connect(&router, &GPSCorrectionRouter::sourceSelected, this,
            [&calls](GPSCorrectionSource, const QString&) { calls.append(QStringLiteral("clear")); });
    router.setSink(QStringLiteral("receiver"), [&calls](const GPSCorrectionFrame& update) {
        calls.append(QStringLiteral("submit"));
        return update.data.size();
    });
    quint64 session = router.beginSourceSession(GPSCorrectionSource::Udp);
    router.setPolicy(GPSCorrectionRouter::Policy::Manual);
    router.setSelectedSource(GPSCorrectionSource::Udp, QStringLiteral("first"));
    QVERIFY(calls.isEmpty());
    QVERIFY(router.acceptFrame(frame(GPSCorrectionSource::Udp, session, now, QStringLiteral("first"))));
    QVERIFY(router.acceptFrame(frame(GPSCorrectionSource::Udp, session, now, QStringLiteral("first"))));
    router.setSelectedSource(GPSCorrectionSource::Udp, QStringLiteral("second"));
    QVERIFY(router.acceptFrame(frame(GPSCorrectionSource::Udp, session, now, QStringLiteral("second"))));
    session = router.beginSourceSession(GPSCorrectionSource::Udp);
    QVERIFY(router.acceptFrame(frame(GPSCorrectionSource::Udp, session, now, QStringLiteral("second"))));
    QCOMPARE(calls, QStringList({QStringLiteral("clear"), QStringLiteral("submit"), QStringLiteral("submit"),
                                 QStringLiteral("clear"), QStringLiteral("submit"), QStringLiteral("clear"),
                                 QStringLiteral("submit")}));
}

void GPSCorrectionRouterTest::teardownAndReentrancy()
{
    qint64 now = 100000;
    auto* router = new GPSCorrectionRouter(nullptr, [&now]() { return now; });
    const QPointer<GPSCorrectionRouter> guard(router);
    const quint64 session = router->beginSourceSession(GPSCorrectionSource::Udp);
    const auto update = frame(GPSCorrectionSource::Udp, session, now);
    bool recursiveAccepted = true;
    bool lateSinkCalled = false;
    router->setSink(QStringLiteral("a-teardown"), [router, &recursiveAccepted](const GPSCorrectionFrame& received) {
        recursiveAccepted = router->acceptFrame(received);
        delete router;
        return quint64(0);
    });
    router->setSink(QStringLiteral("z-retired"), [&lateSinkCalled](const GPSCorrectionFrame&) {
        lateSinkCalled = true;
        return quint64(0);
    });
    QVERIFY(!router->acceptFrame(update));
    QVERIFY(!guard);
    QVERIFY(!recursiveAccepted);
    QVERIFY(!lateSinkCalled);
}

void GPSCorrectionRouterTest::endingSelectedSessionInvalidatesOutput()
{
    qint64 now = 100000;
    GPSCorrectionRouter router(nullptr, [&now]() { return now; });
    const quint64 session = router.beginSourceSession(GPSCorrectionSource::Ntrip);
    QSignalSpy invalidated(&router, &GPSCorrectionRouter::sourceInvalidated);
    QVERIFY(router.acceptFrame(frame(GPSCorrectionSource::Ntrip, session, now)));
    router.beginSourceSession(GPSCorrectionSource::Udp);
    router.endSourceSession(GPSCorrectionSource::Udp);
    QVERIFY(invalidated.isEmpty());
    router.endSourceSession(GPSCorrectionSource::Ntrip);
    QCOMPARE(invalidated.size(), 1);
    QVERIFY(!router.acceptFrame(frame(GPSCorrectionSource::Ntrip, session, now)));
    router.endSourceSession(GPSCorrectionSource::Ntrip);
    QCOMPARE(invalidated.size(), 1);
}

void GPSCorrectionRouterTest::boundedPeerHistory()
{
    qint64 now = 100000;
    GPSCorrectionRouter router(nullptr, [&now]() { return now; });
    const quint64 session = router.beginSourceSession(GPSCorrectionSource::Udp);
    QVERIFY(router.acceptFrame(frame(GPSCorrectionSource::Udp, session, now, QStringLiteral("selected"))));
    for (int i = 0; i < 1000; ++i) {
        QVERIFY(!router.acceptFrame(frame(GPSCorrectionSource::Udp, session, now, QString::number(i))));
        QVERIFY(router.sources().size() <= GPSCorrectionRouter::MAX_SOURCE_INSTANCES);
    }
    QCOMPARE(router.activeInstance(), QStringLiteral("selected"));
    router.shutdown();
    QVERIFY(!router.acceptFrame(frame(GPSCorrectionSource::Udp, session, now, QStringLiteral("selected"))));
    QVERIFY(router.sources().isEmpty());
}

void GPSCorrectionRouterTest::diagnosticStagesStayDistinct_data()
{
    QTest::addColumn<bool>("validated");
    QTest::addColumn<bool>("filtered");
    QTest::addColumn<bool>("queueAccepted");
    QTest::newRow("valid-queued") << true << false << true;
    QTest::newRow("unvalidated-passthrough") << false << false << true;
    QTest::newRow("whitelist-filtered") << true << true << true;
    QTest::newRow("queue-full") << true << false << false;
}

void GPSCorrectionRouterTest::diagnosticStagesStayDistinct()
{
    QFETCH(bool, validated);
    QFETCH(bool, filtered);
    QFETCH(bool, queueAccepted);
    qint64 now = 100000;
    GPSCorrectionRouter router(nullptr, [&now]() { return now; });
    const auto session = router.beginSourceSession(GPSCorrectionSource::Udp);
    router.setDetailedSink(QStringLiteral("receiver"), [queueAccepted](const GPSCorrectionFrame& received) {
        return GPSCorrectionRouter::Submission{
            queueAccepted ? quint64(received.data.size()) : 0, 7,
            queueAccepted ? GPSCorrectionReason::None : GPSCorrectionReason::QueueFull};
    });
    auto update = frame(GPSCorrectionSource::Udp, session, now, QStringLiteral("peer"));
    update.validated = validated;
    update.filtered = filtered;
    QCOMPARE(router.acceptFrame(update), !filtered);
    const auto& stats = router.statistics().at(static_cast<int>(GPSCorrectionSource::Udp));
    const quint64 size = update.data.size();
    QCOMPARE(stats.receivedFrames, quint64(1));
    QCOMPARE(stats.receivedBytes, size);
    QCOMPARE(stats.validatedFrames, quint64(validated));
    QCOMPARE(stats.validatedBytes, validated ? size : 0);
    QCOMPARE(stats.selectedFrames, quint64(!filtered));
    QCOMPARE(stats.selectedBytes, filtered ? 0 : size);
    QCOMPARE(stats.queuedFrames, quint64(!filtered && queueAccepted));
    QCOMPARE(stats.queuedBytes, !filtered && queueAccepted ? size : 0);
    QCOMPARE(stats.writtenFrames, quint64(0));
    QCOMPARE(stats.writtenBytes, quint64(0));
    const auto& last = router.events().last();
    QCOMPARE(last.stage, !filtered && queueAccepted ? GPSCorrectionStage::Queued : GPSCorrectionStage::Dropped);
    QCOMPARE(last.reason, filtered        ? GPSCorrectionReason::MessageFiltered
                          : queueAccepted ? GPSCorrectionReason::None
                                          : GPSCorrectionReason::QueueFull);
    QCOMPARE(last.sourceSession, session);
    QCOMPARE(last.sourceInstance, QStringLiteral("peer"));
    QVERIFY(last.deliveryId > 0);
}

void GPSCorrectionRouterTest::terminalDeliveryAccounting_data()
{
    QTest::addColumn<GPSCorrectionOutcome>("outcome");
    QTest::addColumn<qint64>("written");
    QTest::newRow("written") << GPSCorrectionOutcome::Written << qint64(-1);
    QTest::newRow("partial-write-error") << GPSCorrectionOutcome::WriteFailed << qint64(7);
    QTest::newRow("expired-in-queue") << GPSCorrectionOutcome::Expired << qint64(0);
    QTest::newRow("source-switched") << GPSCorrectionOutcome::Cleared << qint64(0);
}

void GPSCorrectionRouterTest::terminalDeliveryAccounting()
{
    QFETCH(GPSCorrectionOutcome, outcome);
    QFETCH(qint64, written);
    qint64 now = 100000;
    GPSCorrectionRouter router(nullptr, [&now]() { return now; });
    const auto session = router.beginSourceSession(GPSCorrectionSource::Ntrip, QStringLiteral("caster/mount"));
    GPSCorrectionFrame queued;
    router.setDetailedSink(QStringLiteral("receiver"), [&queued](const GPSCorrectionFrame& received) {
        queued = received;
        return GPSCorrectionRouter::Submission{quint64(received.data.size()), 7, GPSCorrectionReason::None};
    });
    QVERIFY(router.acceptFrame(frame(GPSCorrectionSource::Ntrip, session, now)));
    const quint64 size = queued.data.size();
    const quint64 actual = written < 0 ? size : quint64(written);
    GPSCorrectionDelivery result{queued.deliveryId,
                                 queued.source,
                                 queued.sourceInstance,
                                 queued.session,
                                 QStringLiteral("receiver"),
                                 7,
                                 size,
                                 actual,
                                 outcome};
    result.acceptedBytes = actual;
    auto wrongSession = result;
    ++wrongSession.destinationSession;
    QVERIFY(!router.recordDelivery(wrongSession));
    QCOMPARE(router.destinations().first().pendingFrames, quint64(1));
    QVERIFY(router.recordDelivery(result));
    QVERIFY(!router.recordDelivery(result));
    const auto& source = router.statistics().at(static_cast<int>(GPSCorrectionSource::Ntrip));
    QCOMPARE(source.queuedBytes, size);
    QCOMPARE(source.writtenBytes, actual);
    QCOMPARE(source.writtenFrames, quint64(outcome == GPSCorrectionOutcome::Written));
    QCOMPARE(source.droppedBytes, size - actual);
    QCOMPARE(source.droppedFrames, quint64(outcome != GPSCorrectionOutcome::Written));
    const auto destination = router.destinations().first();
    QCOMPARE(destination.writtenBytes, actual);
    QCOMPARE(destination.pendingFrames, quint64(0));
    QCOMPARE(destination.pendingBytes, quint64(0));
    QCOMPARE(destination.unconfirmedFrames, quint64(0));
}

void GPSCorrectionRouterTest::lateDeliveryDoesNotCreditReplacementSource()
{
    qint64 now = 100000;
    GPSCorrectionRouter router(nullptr, [&now]() { return now; });
    GPSCorrectionFrame queued;
    router.setDetailedSink(QStringLiteral("receiver"), [&queued](const GPSCorrectionFrame& received) {
        queued = received;
        return GPSCorrectionRouter::Submission{quint64(received.data.size()), 7, GPSCorrectionReason::None};
    });
    const auto oldSession = router.beginSourceSession(GPSCorrectionSource::Ntrip, QStringLiteral("old-caster"));
    QVERIFY(router.acceptFrame(frame(GPSCorrectionSource::Ntrip, oldSession, now)));
    router.beginSourceSession(GPSCorrectionSource::Ntrip, QStringLiteral("new-caster"));
    QVERIFY(
        router.recordDelivery({queued.deliveryId, queued.source, queued.sourceInstance, queued.session,
                               QStringLiteral("receiver"), 7, quint64(queued.data.size()), quint64(queued.data.size()),
                               GPSCorrectionOutcome::Written, quint64(queued.data.size())}));
    QCOMPARE(router.statistics().at(static_cast<int>(GPSCorrectionSource::Ntrip)).writtenBytes, quint64(0));
    QCOMPARE(router.destinations().first().writtenBytes, quint64(queued.data.size()));
    QCOMPARE(router.events().last().sourceInstance, QStringLiteral("old-caster"));
    QCOMPARE(router.events().last().sourceSession, oldSession);
}

void GPSCorrectionRouterTest::boundedDiagnosticsAndUnconfirmedRetirement()
{
    qint64 now = 100000;
    GPSCorrectionRouter router(nullptr, [&now]() { return now; });
    GPSCorrectionFrame queued;
    quint64 submissions = 0;
    router.setDetailedSink(QStringLiteral("receiver"), [&queued, &submissions](const GPSCorrectionFrame& received) {
        ++submissions;
        queued = received;
        return GPSCorrectionRouter::Submission{quint64(received.data.size()), 7, GPSCorrectionReason::None};
    });
    const auto session = router.beginSourceSession(GPSCorrectionSource::Udp);
    for (qsizetype i = 0; i < GPSCorrectionRouter::MAX_PENDING_DELIVERIES + 20; ++i) {
        QVERIFY(router.acceptFrame(frame(GPSCorrectionSource::Udp, session, now)));
        QVERIFY(router.events().size() <= GPSCorrectionRouter::MAX_EVENTS);
    }
    QCOMPARE(submissions, quint64(GPSCorrectionRouter::MAX_PENDING_DELIVERIES));
    QCOMPARE(router.events().last().reason, GPSCorrectionReason::DiagnosticsBackpressure);
    router.invalidateDestination(QStringLiteral("receiver"), 6);
    QCOMPARE(router.destinations().first().pendingFrames, submissions);
    router.invalidateDestination(QStringLiteral("receiver"), 7);
    const auto destination = router.destinations().first();
    QCOMPARE(destination.pendingFrames, quint64(0));
    QCOMPARE(destination.writtenBytes, quint64(0));
    QCOMPARE(destination.unconfirmedFrames, submissions);
    QCOMPARE(destination.unconfirmedBytes, submissions * queued.data.size());
    QCOMPARE(router.events().last().stage, GPSCorrectionStage::Unconfirmed);
    QVERIFY(
        !router.recordDelivery({queued.deliveryId, queued.source, queued.sourceInstance, queued.session,
                                QStringLiteral("receiver"), 7, quint64(queued.data.size()), quint64(queued.data.size()),
                                GPSCorrectionOutcome::Written, quint64(queued.data.size())}));
    GPSCorrectionEventModel model;
    QAbstractItemModelTester tester(&model, QAbstractItemModelTester::FailureReportingMode::QtTest);
    model.setEvents(router.events());
    QCOMPARE(model.rowCount(), int(GPSCorrectionRouter::MAX_EVENTS));
    const auto last = model.index(model.rowCount() - 1);
    QCOMPARE(model.data(last, GPSCorrectionEventModel::StageRole).toInt(), int(GPSCorrectionStage::Unconfirmed));
    QCOMPARE(model.data(last, GPSCorrectionEventModel::DestinationSessionRole).toULongLong(), quint64(7));
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
    const auto session = router.beginSourceSession(GPSCorrectionSource::Udp);
    auto rejected = frame(GPSCorrectionSource::Udp, session, now, QStringLiteral("peer"));
    router.recordRejectedFrame(rejected, GPSCorrectionReason::InvalidFrame);
    const auto& stats = router.statistics().at(static_cast<int>(GPSCorrectionSource::Udp));
    QCOMPARE(stats.receivedFrames, quint64(1));
    QCOMPARE(stats.validatedFrames, quint64(0));
    QCOMPARE(stats.selectedFrames, quint64(0));
    QCOMPARE(stats.droppedBytes, quint64(rejected.data.size()));
    QCOMPARE(router.events().last().reason, GPSCorrectionReason::InvalidFrame);
}

void GPSCorrectionRouterTest::uncertainDeliveryIsNotDropped()
{
    qint64 now = 100000;
    GPSCorrectionRouter router(nullptr, [&]() { return now; });
    const auto session = router.beginSourceSession(GPSCorrectionSource::Ntrip);
    GPSCorrectionFrame queued;
    router.setDetailedSink(QStringLiteral("receiver"), [&](const GPSCorrectionFrame& value) {
        queued = value;
        return GPSCorrectionRouter::Submission{quint64(value.data.size()), 7, GPSCorrectionReason::None};
    });
    QVERIFY(router.acceptFrame(frame(GPSCorrectionSource::Ntrip, session, now)));
    const quint64 size = queued.data.size();
    GPSCorrectionDelivery delivery{queued.deliveryId,
                                   queued.source,
                                   queued.sourceInstance,
                                   queued.session,
                                   QStringLiteral("receiver"),
                                   7,
                                   size,
                                   4,
                                   GPSCorrectionOutcome::WriteFailed};
    delivery.acceptedBytes = size - 3;
    delivery.uncertainBytes = size - 7;
    auto inconsistent = delivery;
    inconsistent.acceptedBytes = delivery.writtenBytes;
    QVERIFY(!router.recordDelivery(inconsistent));
    QCOMPARE(router.destinations().first().pendingFrames, quint64(1));
    QVERIFY(router.recordDelivery(delivery));
    const auto destination = router.destinations().first();
    QCOMPARE(destination.transportAcceptedBytes, size - 3);
    QCOMPARE(destination.writtenBytes, quint64(4));
    QCOMPARE(destination.writtenFrames, quint64(0));
    QCOMPARE(destination.unconfirmedBytes, size - 7);
    QCOMPARE(destination.unconfirmedFrames, quint64(1));
    QCOMPARE(destination.droppedBytes, quint64(3));
    QCOMPARE(destination.pendingBytes, quint64(0));
    QVERIFY(!router.recordDelivery(delivery));
}

void GPSCorrectionRouterTest::sourceSpecificSinkPreservesNtripForwarding()
{
    qint64 now = 100000;
    GPSCorrectionRouter router(nullptr, [&]() { return now; });
    const auto ntrip = router.beginSourceSession(GPSCorrectionSource::Ntrip);
    const auto local = router.beginSourceSession(GPSCorrectionSource::LocalReceiver);
    QList<GPSCorrectionFrame> selected;
    QList<GPSCorrectionFrame> forwarded;
    router.setSink(QStringLiteral("selected"), [&](const GPSCorrectionFrame& value) {
        selected.append(value);
        return value.data.size();
    });
    router.setSourceSink(QStringLiteral("ntripUdp"), GPSCorrectionSource::Ntrip, [&](const GPSCorrectionFrame& value) {
        forwarded.append(value);
        return value.data.size();
    });
    QVERIFY(router.acceptFrame(frame(GPSCorrectionSource::LocalReceiver, local, now)));
    QVERIFY(!router.acceptFrame(frame(GPSCorrectionSource::Ntrip, ntrip, now)));
    QCOMPARE(selected.size(), 1);
    QCOMPARE(forwarded.size(), 1);
    QCOMPARE(forwarded.first().source, GPSCorrectionSource::Ntrip);
    auto filtered = frame(GPSCorrectionSource::Ntrip, ntrip, now);
    filtered.filtered = true;
    QVERIFY(!router.acceptFrame(filtered));
    now += GPSCorrectionRouter::FRESHNESS_TIMEOUT_MS;
    filtered.filtered = false;
    QVERIFY(!router.acceptFrame(filtered));
    QCOMPARE(forwarded.size(), 1);
    router.removeSink(QStringLiteral("ntripUdp"));
    router.acceptFrame(frame(GPSCorrectionSource::Ntrip, ntrip, now));
    QCOMPARE(forwarded.size(), 1);
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

QTEST_GUILESS_MAIN(GPSCorrectionRouterTest)

#include "GPSCorrectionRouterTest.moc"
