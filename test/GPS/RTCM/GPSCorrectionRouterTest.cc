#include <QtCore/QPointer>
#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

#include "../GpsTestHelpers.h"
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

QTEST_GUILESS_MAIN(GPSCorrectionRouterTest)

#include "GPSCorrectionRouterTest.moc"
