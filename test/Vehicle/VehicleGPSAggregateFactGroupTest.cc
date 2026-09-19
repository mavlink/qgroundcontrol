#include "VehicleGPSAggregateFactGroupTest.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <memory>

#include <QtTest/QSignalSpy>

#include "MAVLinkLib.h"
#include "ManualScheduler.h"
#include "VehicleGPSAggregateFactGroup.h"
#include "VehicleGPSFactGroup.h"
#include "development/mavlink_msg_gnss_integrity.h"

namespace {

using namespace std::chrono_literals;

class IntegrityReceiver : public VehicleGPSFactGroup
{
public:
    IntegrityReceiver(uint8_t id, RuntimeScheduler* scheduler)
        : VehicleGPSFactGroup(nullptr, scheduler)
    {
        _gnssIntegrityId = id;
    }
};

mavlink_message_t integrityMessage(uint8_t id, uint8_t spoofing, uint8_t jamming, uint8_t authentication)
{
    mavlink_gnss_integrity_t integrity{};
    integrity.id = id;
    integrity.spoofing_state = spoofing;
    integrity.jamming_state = jamming;
    integrity.authentication_state = authentication;
    mavlink_message_t message{};
    mavlink_msg_gnss_integrity_encode(1, 1, &message, &integrity);
    return message;
}

void verifyAggregate(VehicleGPSAggregateFactGroup& aggregate, int spoofing, int jamming, int authentication,
                     bool available)
{
    QCOMPARE(aggregate.spoofingState()->rawValue().toInt(), spoofing);
    QCOMPARE(aggregate.jammingState()->rawValue().toInt(), jamming);
    QCOMPARE(aggregate.authenticationState()->rawValue().toInt(), authentication);
    QCOMPARE(aggregate.isStale()->rawValue().toBool(), !available);
    QCOMPARE(aggregate.telemetryAvailable(), available);
}

}  // namespace

void VehicleGPSAggregateFactGroupTest::_independentExpiry_data()
{
    QTest::addColumn<bool>("silentGps2");
    QTest::addColumn<int>("silentInterference");
    QTest::addColumn<int>("silentAuthentication");
    QTest::addColumn<int>("activeInterference");
    for (bool silentGps2 : {false, true}) {
        const QByteArray prefix = silentGps2 ? "silent-gps2-" : "silent-gps1-";
        QTest::newRow((prefix + "ok-vs-unknown").constData()) << silentGps2 << 0 << 3 << 0;
        QTest::newRow((prefix + "interference-vs-clear").constData()) << silentGps2 << 3 << 0 << 1;
    }
}

void VehicleGPSAggregateFactGroupTest::_independentExpiry()
{
    QFETCH(bool, silentGps2);
    QFETCH(int, silentInterference);
    QFETCH(int, silentAuthentication);
    QFETCH(int, activeInterference);
    ManualScheduler scheduler;
    IntegrityReceiver gps1(0, &scheduler);
    IntegrityReceiver gps2(1, &scheduler);
    VehicleGPSAggregateFactGroup aggregate(nullptr, &scheduler);
    aggregate.setLiveUpdates(true);
    aggregate.bindToGps(&gps1, &gps2);
    verifyAggregate(aggregate, 255, 255, 255, false);
    const auto silent =
        integrityMessage(silentGps2 ? 1 : 0, silentInterference, silentInterference, silentAuthentication);
    const auto active = integrityMessage(silentGps2 ? 0 : 1, activeInterference, activeInterference, 0);
    gps1.handleMessage(nullptr, silent);
    gps2.handleMessage(nullptr, silent);
    for (int second = 1; second <= 4; ++second) {
        QVERIFY(scheduler.advanceBy(1s));
        gps1.handleMessage(nullptr, active);
        gps2.handleMessage(nullptr, active);
    }
    QVERIFY(scheduler.advanceBy(1s - 1us));
    verifyAggregate(aggregate, silentInterference, silentInterference, silentAuthentication, true);

    QSignalSpy spoofingSpy(aggregate.spoofingState(), &Fact::valueChanged);
    QSignalSpy jammingSpy(aggregate.jammingState(), &Fact::valueChanged);
    QSignalSpy authenticationSpy(aggregate.authenticationState(), &Fact::valueChanged);
    QSignalSpy staleSpy(aggregate.isStale(), &Fact::valueChanged);
    QSignalSpy availableSpy(&aggregate, &FactGroup::telemetryAvailableChanged);
    QVERIFY(scheduler.advanceBy(1us));
    verifyAggregate(aggregate, activeInterference, activeInterference, 0, true);
    QCOMPARE(spoofingSpy.count(), silentInterference != activeInterference ? 1 : 0);
    QCOMPARE(jammingSpy.count(), silentInterference != activeInterference ? 1 : 0);
    QCOMPARE(authenticationSpy.count(), silentAuthentication != 0 ? 1 : 0);
    QCOMPARE(staleSpy.count(), 0);
    QCOMPARE(availableSpy.count(), 0);

    QVERIFY(scheduler.advanceBy(4s - 1us));
    verifyAggregate(aggregate, activeInterference, activeInterference, 0, true);
    QVERIFY(scheduler.advanceBy(1us));
    verifyAggregate(aggregate, 255, 255, 255, false);
    QCOMPARE(staleSpy.count(), 1);
    QCOMPARE(availableSpy.count(), 1);
    QCOMPARE(availableSpy.at(0).at(0).toBool(), false);
    QCOMPARE(scheduler.pendingCount(), 0);

    gps1.handleMessage(nullptr, active);
    gps2.handleMessage(nullptr, active);
    verifyAggregate(aggregate, activeInterference, activeInterference, 0, true);
    QCOMPARE(staleSpy.count(), 2);
    QCOMPARE(availableSpy.count(), 2);
    QCOMPARE(availableSpy.at(1).at(0).toBool(), true);
}

void VehicleGPSAggregateFactGroupTest::_freshAuthenticationPrecedence_data()
{
    QTest::addColumn<int>("first");
    QTest::addColumn<int>("second");
    QTest::addColumn<int>("expected");
    constexpr std::array priority{255, 0, 4, 1, 3, 2};
    for (size_t first = 0; first < priority.size(); ++first) {
        for (size_t second = 0; second < priority.size(); ++second) {
            const QByteArray name = QByteArray::number(priority[first]) + '-' + QByteArray::number(priority[second]);
            QTest::newRow(name.constData()) << priority[first] << priority[second] << priority[std::max(first, second)];
        }
    }
}

void VehicleGPSAggregateFactGroupTest::_freshAuthenticationPrecedence()
{
    QFETCH(int, first);
    QFETCH(int, second);
    QFETCH(int, expected);
    ManualScheduler scheduler;
    IntegrityReceiver gps1(0, &scheduler);
    IntegrityReceiver gps2(1, &scheduler);
    VehicleGPSAggregateFactGroup aggregate(nullptr, &scheduler);
    aggregate.bindToGps(&gps1, &gps2);
    gps1.handleMessage(nullptr, integrityMessage(0, 3, 255, first));
    gps2.handleMessage(nullptr, integrityMessage(1, 1, 2, second));
    verifyAggregate(aggregate, 3, 2, expected, true);
}

void VehicleGPSAggregateFactGroupTest::_bindPreservesReceiptAge()
{
    ManualScheduler scheduler;
    IntegrityReceiver gps1(0, &scheduler);
    IntegrityReceiver gps2(1, &scheduler);
    VehicleGPSAggregateFactGroup aggregate(nullptr, &scheduler);
    gps1.handleMessage(nullptr, integrityMessage(0, 3, 3, 3));
    QVERIFY(scheduler.advanceBy(3s));
    aggregate.bindToGps(&gps1, nullptr);
    verifyAggregate(aggregate, 3, 3, 3, true);
    QVERIFY(scheduler.advanceBy(1s));
    aggregate.updateFromGps(&gps1, nullptr);
    QVERIFY(scheduler.advanceBy(1s));
    verifyAggregate(aggregate, 255, 255, 255, false);

    gps2.handleMessage(nullptr, integrityMessage(1, 1, 1, 0));
    aggregate.bindToGps(&gps1, &gps2);
    verifyAggregate(aggregate, 1, 1, 0, true);
    QVERIFY(scheduler.advanceBy(1s));
    aggregate.bindToGps(&gps2, &gps1);
    QVERIFY(scheduler.advanceBy(4s));
    verifyAggregate(aggregate, 255, 255, 255, false);
    QCOMPARE(scheduler.pendingCount(), 0);
}

void VehicleGPSAggregateFactGroupTest::_rebindDisconnectsPreviousReceivers()
{
    ManualScheduler scheduler;
    IntegrityReceiver previous(0, &scheduler);
    IntegrityReceiver replacement(1, &scheduler);
    VehicleGPSAggregateFactGroup aggregate(nullptr, &scheduler);
    aggregate.bindToGps(&previous, nullptr);
    previous.handleMessage(nullptr, integrityMessage(0, 3, 3, 2));
    QVERIFY(scheduler.advanceBy(4s));
    aggregate.bindToGps(nullptr, &replacement);
    verifyAggregate(aggregate, 255, 255, 255, false);
    QCOMPARE(scheduler.pendingCount(), 0);
    replacement.handleMessage(nullptr, integrityMessage(1, 1, 1, 0));
    previous.handleMessage(nullptr, integrityMessage(0, 3, 3, 2));
    QVERIFY(scheduler.advanceBy(1s));
    verifyAggregate(aggregate, 1, 1, 0, true);
    aggregate.bindToGps(nullptr, nullptr);
    verifyAggregate(aggregate, 255, 255, 255, false);
    replacement.handleMessage(nullptr, integrityMessage(1, 3, 3, 2));
    QVERIFY(scheduler.advanceBy(5s));
    verifyAggregate(aggregate, 255, 255, 255, false);
    QCOMPARE(scheduler.pendingCount(), 0);
}

void VehicleGPSAggregateFactGroupTest::_onlyIntegrityRefreshesReceipt()
{
    ManualScheduler scheduler;
    IntegrityReceiver gps(0, &scheduler);
    VehicleGPSAggregateFactGroup aggregate(nullptr, &scheduler);
    aggregate.bindToGps(&gps, nullptr);
    gps.handleMessage(nullptr, integrityMessage(1, 3, 3, 2));
    QCOMPARE(gps.gnssIntegrityTimestampUs(), quint64(0));
    verifyAggregate(aggregate, 255, 255, 255, false);
    gps.handleMessage(nullptr, integrityMessage(0, 255, 255, 255));
    verifyAggregate(aggregate, 255, 255, 255, true);
    const quint64 receipt = gps.gnssIntegrityTimestampUs();
    QVERIFY(scheduler.advanceBy(4s));
    gps.handleMessage(nullptr, integrityMessage(1, 3, 3, 2));
    mavlink_message_t message{};
    mavlink_gps_raw_int_t position{};
    mavlink_msg_gps_raw_int_encode(1, 1, &message, &position);
    gps.handleMessage(nullptr, message);
    QCOMPARE(gps.gnssIntegrityTimestampUs(), receipt);
    QVERIFY(scheduler.advanceBy(1s));
    verifyAggregate(aggregate, 255, 255, 255, false);
}

void VehicleGPSAggregateFactGroupTest::_receiverDestruction()
{
    ManualScheduler scheduler;
    auto gps1 = std::make_unique<IntegrityReceiver>(0, &scheduler);
    auto gps2 = std::make_unique<IntegrityReceiver>(1, &scheduler);
    auto aggregate = std::make_unique<VehicleGPSAggregateFactGroup>(nullptr, &scheduler);
    aggregate->bindToGps(gps1.get(), gps2.get());
    gps1->handleMessage(nullptr, integrityMessage(0, 3, 3, 2));
    gps2->handleMessage(nullptr, integrityMessage(1, 1, 1, 0));
    gps1.reset();
    verifyAggregate(*aggregate, 1, 1, 0, true);
    gps2.reset();
    verifyAggregate(*aggregate, 255, 255, 255, false);
    QCOMPARE(scheduler.pendingCount(), 0);

    IntegrityReceiver survivor(0, &scheduler);
    aggregate->bindToGps(&survivor, nullptr);
    survivor.handleMessage(nullptr, integrityMessage(0, 1, 1, 3));
    QCOMPARE(scheduler.pendingCount(), 1);
    aggregate.reset();
    QCOMPARE(scheduler.pendingCount(), 0);
    survivor.handleMessage(nullptr, integrityMessage(0, 3, 3, 2));
    QVERIFY(scheduler.advanceBy(5s));
}

void VehicleGPSAggregateFactGroupTest::_schedulerDestruction()
{
    auto scheduler = std::make_unique<ManualScheduler>();
    IntegrityReceiver gps(0, scheduler.get());
    VehicleGPSAggregateFactGroup aggregate(nullptr, scheduler.get());
    aggregate.bindToGps(&gps, nullptr);
    gps.handleMessage(nullptr, integrityMessage(0, 1, 1, 3));
    scheduler.reset();
    verifyAggregate(aggregate, 255, 255, 255, false);
    gps.handleMessage(nullptr, integrityMessage(0, 3, 3, 2));
    verifyAggregate(aggregate, 255, 255, 255, false);
}

void VehicleGPSAggregateFactGroupTest::_reentrantRebind()
{
    ManualScheduler scheduler;
    IntegrityReceiver gps(0, &scheduler);
    VehicleGPSAggregateFactGroup aggregate(nullptr, &scheduler);
    aggregate.bindToGps(&gps, nullptr);
    connect(aggregate.authenticationState(), &Fact::rawValueChanged, &aggregate, [&aggregate](const QVariant& value) {
        if (value.toInt() == 3) {
            aggregate.bindToGps(nullptr, nullptr);
        }
    });
    gps.handleMessage(nullptr, integrityMessage(0, 1, 1, 3));
    verifyAggregate(aggregate, 255, 255, 255, false);
    QCOMPARE(scheduler.pendingCount(), 0);
}

UT_REGISTER_TEST(VehicleGPSAggregateFactGroupTest, TestLabel::Unit)
