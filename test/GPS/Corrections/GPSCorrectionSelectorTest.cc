#include "GPSCorrectionSelectorTest.h"

#include <QtTest/QTest>

#include "GPSCorrectionSelector.h"

namespace {
bool submit(GPSCorrectionSelector& selector, GPSCorrectionSettings::CorrectionSource source, qint64 now,
            const QString& instance = {})
{
    return selector.submit(source, instance, now, now);
}

GPSCorrectionStream selectedStream(const GPSCorrectionSelector& selector, qint64 now)
{
    return selector.selectedStream(now).value_or(GPSCorrectionStream{});
}

GPSCorrectionSettings::CorrectionSource activeSource(const GPSCorrectionSelector& selector, qint64 now)
{
    return static_cast<GPSCorrectionSettings::CorrectionSource>(selectedStream(selector, now).source);
}
}  // namespace

void GPSCorrectionSelectorTest::_submittedFramesAreClassified_data()
{
    QTest::addColumn<bool>("expired");
    QTest::newRow("fresh") << false;
    QTest::newRow("expired") << true;
}

void GPSCorrectionSelectorTest::_submittedFramesAreClassified()
{
    QFETCH(bool, expired);
    constexpr qint64 now = 100000;
    GPSCorrectionSelector selector;
    QVERIFY(!selector.hasSources());
    selector.beginSource(GPSCorrectionSettings::Ntrip, now);
    QVERIFY(selector.hasSources());
    const qint64 receivedAtMs = expired ? now - GPSCorrectionSelector::FRESHNESS_TIMEOUT.count() : now - 10;
    QCOMPARE(selector.submit(GPSCorrectionSettings::Ntrip, QStringLiteral("caster"), receivedAtMs, now), !expired);
    const auto selected = selector.selectedStream(now);
    QCOMPARE(selected.has_value(), !expired);
    if (selected) {
        QCOMPARE(selected->source, static_cast<int>(GPSCorrectionSettings::Ntrip));
        QCOMPARE(selected->instanceId, QStringLiteral("caster"));
    }
    // Beginning the source again starts without its old stream.
    selector.beginSource(GPSCorrectionSettings::Ntrip, now);
    QVERIFY(!selector.selectedStream(now));
}

void GPSCorrectionSelectorTest::_selectionLastsWhileFresh()
{
    qint64 now = 100000;
    GPSCorrectionSelector selector;
    selector.beginSource(GPSCorrectionSettings::Ntrip, now);
    selector.beginSource(GPSCorrectionSettings::Udp, now);
    QVERIFY(submit(selector, GPSCorrectionSettings::Ntrip, now, QStringLiteral("caster")));
    QVERIFY(!submit(selector, GPSCorrectionSettings::Udp, now, QStringLiteral("datagram")));
    QCOMPARE(selectedStream(selector, now).instanceId, QStringLiteral("caster"));

    // Selection lasts only while the stream is fresh, and never for a receipt in the future.
    now += GPSCorrectionSelector::FRESHNESS_TIMEOUT.count();
    QVERIFY(!selector.selectedStream(now));
    now = 99999;
    QVERIFY(!selector.selectedStream(now));
}

void GPSCorrectionSelectorTest::_atomicConfigurationAndReplacement()
{
    constexpr qint64 now = 100000;
    GPSCorrectionSelector selector;
    selector.beginSource(GPSCorrectionSettings::Ntrip, now);
    const auto update = [&selector]() {
        return submit(selector, GPSCorrectionSettings::Ntrip, now, QStringLiteral("caster"));
    };
    QVERIFY(update());
    selector.configure(GPSCorrectionSettings::Udp, now);
    QVERIFY(!update());
    QVERIFY(!selector.selectedStream(now));
    selector.configure(GPSCorrectionSettings::Ntrip, now);
    QVERIFY(update());
}

void GPSCorrectionSelectorTest::_automaticSelectionAndFailover()
{
    qint64 now = 100000;
    GPSCorrectionSelector selector;
    selector.beginSource(GPSCorrectionSettings::Udp, now);
    selector.beginSource(GPSCorrectionSettings::Ntrip, now);
    selector.beginSource(GPSCorrectionSettings::LocalReceiver, now);
    const QString peer = QStringLiteral("peer-a");
    const QString caster = QStringLiteral("caster/mount");
    const QString device = QStringLiteral("usb-device");
    QVERIFY(submit(selector, GPSCorrectionSettings::Udp, now, peer));
    QVERIFY(!submit(selector, GPSCorrectionSettings::Ntrip, now, caster));
    QCOMPARE(activeSource(selector, now), GPSCorrectionSettings::Udp);
    now += GPSCorrectionSelector::SWITCH_HOLD_DOWN.count() - 1;
    QVERIFY(!submit(selector, GPSCorrectionSettings::Ntrip, now, caster));
    ++now;
    QVERIFY(submit(selector, GPSCorrectionSettings::Ntrip, now, caster));
    QCOMPARE(selectedStream(selector, now).instanceId, caster);
    QVERIFY(!submit(selector, GPSCorrectionSettings::Udp, now, peer));
    QVERIFY(!submit(selector, GPSCorrectionSettings::LocalReceiver, now, device));
    now += GPSCorrectionSelector::SWITCH_HOLD_DOWN.count();
    QVERIFY(submit(selector, GPSCorrectionSettings::LocalReceiver, now, device));
    QCOMPARE(activeSource(selector, now), GPSCorrectionSettings::LocalReceiver);
    now += GPSCorrectionSelector::FRESHNESS_TIMEOUT.count();
    QVERIFY(submit(selector, GPSCorrectionSettings::Udp, now, peer));
    QCOMPARE(activeSource(selector, now), GPSCorrectionSettings::Udp);
}

void GPSCorrectionSelectorTest::_peerSelectionDoesNotInterleave()
{
    qint64 now = 100000;
    GPSCorrectionSelector selector;
    selector.beginSource(GPSCorrectionSettings::Udp, now);
    const auto observe = [&](const QString& instance) {
        return submit(selector, GPSCorrectionSettings::Udp, now, instance);
    };
    QVERIFY(observe(QStringLiteral("first")));
    for (int i = 0; i < 20; ++i) {
        now += 100;
        QVERIFY(!observe(QStringLiteral("second")));
        QVERIFY(observe(QStringLiteral("first")));
    }
    // A manual choice of another category selects neither peer.
    selector.configure(GPSCorrectionSettings::Ntrip, now);
    QVERIFY(!observe(QStringLiteral("first")));
    QVERIFY(!observe(QStringLiteral("second")));
    QVERIFY(!selector.selectedStream(now));
}

void GPSCorrectionSelectorTest::_staleInstanceIsReplaced()
{
    qint64 now = 100000;
    GPSCorrectionSelector selector;
    selector.beginSource(GPSCorrectionSettings::Udp, now);
    QVERIFY(submit(selector, GPSCorrectionSettings::Udp, now, QStringLiteral("a")));
    now += GPSCorrectionSelector::FRESHNESS_TIMEOUT.count() - 1;
    QVERIFY(!submit(selector, GPSCorrectionSettings::Udp, now, QStringLiteral("b")));
    QCOMPARE(selectedStream(selector, now).instanceId, QStringLiteral("a"));
    ++now;
    QVERIFY(submit(selector, GPSCorrectionSettings::Udp, now, QStringLiteral("b")));
    QCOMPARE(selectedStream(selector, now).instanceId, QStringLiteral("b"));
}

void GPSCorrectionSelectorTest::_sessionAndReceiptValidation()
{
    constexpr qint64 now = 100000;
    GPSCorrectionSelector selector;
    selector.beginSource(GPSCorrectionSettings::Ntrip, now);
    QVERIFY(submit(selector, GPSCorrectionSettings::Ntrip, now));
    selector.endSource(GPSCorrectionSettings::Ntrip, now);
    QVERIFY(!submit(selector, GPSCorrectionSettings::Ntrip, now));
    selector.beginSource(GPSCorrectionSettings::Ntrip, now);
    QVERIFY(!selector.submit(GPSCorrectionSettings::Ntrip, {}, now + 1, now));
    QVERIFY(!selector.submit(GPSCorrectionSettings::Ntrip, {}, now - GPSCorrectionSelector::FRESHNESS_TIMEOUT.count(),
                             now));
    QVERIFY(!selector.selectedStream(now));
    QVERIFY(submit(selector, GPSCorrectionSettings::Ntrip, now));
}

void GPSCorrectionSelectorTest::_endingSourcesFailsOver()
{
    qint64 now = 100000;
    GPSCorrectionSelector selector;
    for (const GPSCorrectionSettings::CorrectionSource source :
         {GPSCorrectionSettings::Udp, GPSCorrectionSettings::Ntrip, GPSCorrectionSettings::LocalReceiver}) {
        selector.beginSource(source, now);
    }
    (void) submit(selector, GPSCorrectionSettings::Udp, now, QStringLiteral("peer"));
    (void) submit(selector, GPSCorrectionSettings::Ntrip, now, QStringLiteral("caster"));
    (void) submit(selector, GPSCorrectionSettings::LocalReceiver, now, QStringLiteral("device"));
    // After the hold-down, the highest priority takes over.
    now += GPSCorrectionSelector::SWITCH_HOLD_DOWN.count();
    (void) submit(selector, GPSCorrectionSettings::LocalReceiver, now, QStringLiteral("device"));
    QCOMPARE(activeSource(selector, now), GPSCorrectionSettings::LocalReceiver);

    // Ending a source fails over at once to the next fresh one.
    selector.endSource(GPSCorrectionSettings::LocalReceiver, now);
    QCOMPARE(activeSource(selector, now), GPSCorrectionSettings::Ntrip);
    selector.endSource(GPSCorrectionSettings::Ntrip, now);
    QCOMPARE(selectedStream(selector, now).instanceId, QStringLiteral("peer"));
    selector.endSource(GPSCorrectionSettings::Udp, now);
    QVERIFY(!selector.hasSources());
}

void GPSCorrectionSelectorTest::_endingSelectedSessionInvalidatesOutput()
{
    constexpr qint64 now = 100000;
    GPSCorrectionSelector selector;
    // Unknown and sources that never began are not routed.
    selector.beginSource(GPSCorrectionSettings::HighestPriority, now);
    QVERIFY(!submit(selector, GPSCorrectionSettings::HighestPriority, now));
    QVERIFY(!submit(selector, GPSCorrectionSettings::Ntrip, now));
    QVERIFY(!selector.hasSources());
    selector.beginSource(GPSCorrectionSettings::Ntrip, now);
    QVERIFY(submit(selector, GPSCorrectionSettings::Ntrip, now));
    selector.beginSource(GPSCorrectionSettings::Udp, now);
    selector.endSource(GPSCorrectionSettings::Udp, now);
    QVERIFY(submit(selector, GPSCorrectionSettings::Ntrip, now));
    selector.endSource(GPSCorrectionSettings::Ntrip, now);
    QVERIFY(!submit(selector, GPSCorrectionSettings::Ntrip, now));
    QVERIFY(!selector.selectedStream(now));
    selector.shutdown();
    selector.beginSource(GPSCorrectionSettings::Ntrip, now);
    QVERIFY(!submit(selector, GPSCorrectionSettings::Ntrip, now));
}

UT_REGISTER_TEST_LIGHTWEIGHT(GPSCorrectionSelectorTest, TestLabel::Unit)
