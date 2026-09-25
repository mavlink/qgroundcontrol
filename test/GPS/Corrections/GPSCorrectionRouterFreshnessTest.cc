#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

#include "../RTCM/RTCMTestFixtures.h"
#include "GPSCorrectionRouter.h"
#include "GPSCorrectionRouterTest.h"

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
    QVERIFY(sources[2].usable);
    QVERIFY(!sources[3].usable);
    for (const auto& row : router.sourceInstanceDiagnostics()) {
        const auto& instance = row;
        const bool isUdp = instance.source == int(GPSCorrectionSource::Udp);
        QCOMPARE(instance.usable, isUdp);
        QCOMPARE(instance.selected, isUdp);
    }
}

void GPSCorrectionRouterTest::managerSourceSelectionAndSessions()
{
    constexpr qint64 now = 100000;
    GPSCorrectionRouter router(nullptr, [] { return now; });
    QSignalSpy routed(&router, &GPSCorrectionRouter::frameRouted);
    const QByteArray data = GpsTestHelpers::buildRtcmFrame(1005, 20);
    const auto initial = router.sourceDiagnostics().at(static_cast<int>(GPSCorrectionSource::LocalReceiver));
    QVERIFY(!initial.active);
    QVERIFY(!router.acceptIngress(GPSCorrectionSourceToken().event(data, now, 1005, true)));
    QVERIFY(routed.isEmpty());

    auto local = router.registerSource(GPSCorrectionSource::LocalReceiver);
    auto ntrip = router.registerSource(GPSCorrectionSource::Ntrip);
    const auto oldToken = ntrip.token();
    auto frame = oldToken.event(data, now, 0, true);
    QVERIFY(router.acceptIngress(frame));
    QCOMPARE(routed.size(), 1);
    QCOMPARE(qvariant_cast<GPSCorrectionFrame>(routed.first().first()).messageId, 1005);

    router.applyConfiguration({GPSCorrectionRouter::Policy::Manual, GPSCorrectionSource::LocalReceiver, {}});
    QVERIFY(!router.acceptIngress(frame));
    QCOMPARE(routed.size(), 1);
    QVERIFY(router.acceptIngress(local.token().event(data, now, 1005, true)));
    QCOMPARE(routed.size(), 2);

    router.applyConfiguration({GPSCorrectionRouter::Policy::Manual, GPSCorrectionSource::Ntrip, {}});
    ntrip.reset();
    QVERIFY(!router.acceptIngress(frame));
    QCOMPARE(routed.size(), 2);
    ntrip = router.registerSource(GPSCorrectionSource::Ntrip);
    QVERIFY(ntrip.token().session() != oldToken.session());
    QVERIFY(!router.acceptIngress(frame));
    QCOMPARE(routed.size(), 2);

    frame = ntrip.token().event(data, now, 0, true);
    QVERIFY(router.acceptIngress(frame));
    QCOMPARE(routed.size(), 3);
    const auto stats = router.sourceDiagnostics().at(static_cast<int>(GPSCorrectionSource::Ntrip));
    QCOMPARE(stats.validatedFrames, quint64(1));
    QCOMPARE(stats.selectedFrames, quint64(1));
    QVERIFY(stats.usable);
}

void GPSCorrectionRouterTest::managerFilteredAndExpiredFrames()
{
    constexpr qint64 now = 100000;
    GPSCorrectionRouter router(nullptr, [] { return now; });
    QSignalSpy routed(&router, &GPSCorrectionRouter::frameRouted);
    auto ntrip = router.registerSource(GPSCorrectionSource::Ntrip);
    const auto data = GpsTestHelpers::buildRtcmFrame(1005, 20);

    QVERIFY(!router.acceptIngress(ntrip.token().event(data, now, 1005, true, true)));
    auto stats = router.sourceDiagnostics().at(static_cast<int>(GPSCorrectionSource::Ntrip));
    QVERIFY(stats.usable);
    QCOMPARE(stats.droppedFrames, quint64(1));
    QVERIFY(routed.isEmpty());

    QVERIFY(!router.acceptIngress(ntrip.token().event(data, now - 6000, 1005, true)));
    stats = router.sourceDiagnostics().at(static_cast<int>(GPSCorrectionSource::Ntrip));
    QVERIFY(stats.usable);
    QCOMPARE(stats.droppedFrames, quint64(2));
    QVERIFY(routed.isEmpty());

    QVERIFY(!router.acceptIngress(ntrip.token().event(data, now + 60000, 1005, true)));
    stats = router.sourceDiagnostics().at(static_cast<int>(GPSCorrectionSource::Ntrip));
    QCOMPARE(stats.droppedFrames, quint64(3));
    QCOMPARE(stats.validatedFrames, quint64(2));
    QVERIFY(routed.isEmpty());
}

void GPSCorrectionRouterTest::managerReceivedByteRates()
{
    qint64 now = 100000;
    GPSCorrectionRouter router(nullptr, [&]() { return now; });
    auto ntrip = router.registerSource(GPSCorrectionSource::Ntrip);
    const auto rate = [&router]() {
        return router.sourceDiagnostics().at(static_cast<int>(GPSCorrectionSource::Ntrip)).receivedBytesPerSecond;
    };
    const auto frame = GpsTestHelpers::buildRtcmFrame(1005, 20);

    router.sampleReceivedByteRates(now);
    QVERIFY(router.acceptIngress(ntrip.token().event(frame, now, 1005, true)));
    QCOMPARE(rate(), 0ULL);
    now += 500;
    router.sampleReceivedByteRates(now);
    QCOMPARE(rate(), quint64(frame.size() * 2));
    now += 1000;
    router.sampleReceivedByteRates(now);
    QCOMPARE(rate(), 0ULL);

    ntrip.reset();
    ntrip = router.registerSource(GPSCorrectionSource::Ntrip);
    QVERIFY(router.acceptIngress(ntrip.token().event(frame, now, 1005, true)));
    now += 1000;
    router.sampleReceivedByteRates(now);
    QCOMPARE(rate(), quint64(frame.size()));
}
