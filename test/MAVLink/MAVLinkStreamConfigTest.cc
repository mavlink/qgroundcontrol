#include "MAVLinkStreamConfigTest.h"

#include "MAVLinkLib.h"
#include "MAVLinkStreamConfig.h"

#include <QtCore/QPair>
#include <QtCore/QList>

namespace {

// 100 Hz in microseconds — must match the static constexpr in MAVLinkStreamConfig.cc
constexpr int kHighRate = static_cast<int>(1000000.0 / 100.0);

} // namespace

// Construction: no callback is invoked, object starts in Idle state (verified
// indirectly by the absence of any callback and by the behaviour of subsequent calls).
void MAVLinkStreamConfigTest::_testInitialState()
{
    QList<QPair<int,int>> calls;
    MAVLinkStreamConfig config([&calls](int id, int rate){ calls.append({id, rate}); });

    // No callback should fire on construction.
    QVERIFY(calls.isEmpty());

    // Calling gotSetMessageIntervalAck() in Idle is a no-op.
    config.gotSetMessageIntervalAck();
    QVERIFY(calls.isEmpty());
}

// setHighRateRateAndAttitude() from Idle with no previously configured messages:
//   - No restore phase (changedIds is empty), transitions immediately to Configuring.
//   - First callback fires synchronously during the configure call itself.
//   - Each subsequent gotSetMessageIntervalAck() advances to the next message.
//   - Messages are emitted in last-in / first-out order from _desiredRates.
void MAVLinkStreamConfigTest::_testSetHighRateRateAndAttitude()
{
    QList<QPair<int,int>> calls;
    MAVLinkStreamConfig config([&calls](int id, int rate){ calls.append({id, rate}); });

    config.setHighRateRateAndAttitude();

    // First message fires synchronously.
    QCOMPARE(calls.size(), 1);
    QCOMPARE(calls[0].first,  MAVLINK_MSG_ID_ATTITUDE_TARGET);
    QCOMPARE(calls[0].second, kHighRate);

    config.gotSetMessageIntervalAck();

    QCOMPARE(calls.size(), 2);
    QCOMPARE(calls[1].first,  MAVLINK_MSG_ID_ATTITUDE_QUATERNION);
    QCOMPARE(calls[1].second, kHighRate);

    // All messages consumed; further ack is a no-op.
    config.gotSetMessageIntervalAck();
    QCOMPARE(calls.size(), 2);
}

// setHighRateVelAndPos() follows the same pattern with different message IDs.
void MAVLinkStreamConfigTest::_testSetHighRateVelAndPos()
{
    QList<QPair<int,int>> calls;
    MAVLinkStreamConfig config([&calls](int id, int rate){ calls.append({id, rate}); });

    config.setHighRateVelAndPos();

    QCOMPARE(calls.size(), 1);
    QCOMPARE(calls[0].first,  MAVLINK_MSG_ID_POSITION_TARGET_LOCAL_NED);
    QCOMPARE(calls[0].second, kHighRate);

    config.gotSetMessageIntervalAck();

    QCOMPARE(calls.size(), 2);
    QCOMPARE(calls[1].first,  MAVLINK_MSG_ID_LOCAL_POSITION_NED);
    QCOMPARE(calls[1].second, kHighRate);

    config.gotSetMessageIntervalAck();
    QCOMPARE(calls.size(), 2);
}

// setHighRateAltAirspeed() follows the same pattern with different message IDs.
void MAVLinkStreamConfigTest::_testSetHighRateAltAirspeed()
{
    QList<QPair<int,int>> calls;
    MAVLinkStreamConfig config([&calls](int id, int rate){ calls.append({id, rate}); });

    config.setHighRateAltAirspeed();

    QCOMPARE(calls.size(), 1);
    QCOMPARE(calls[0].first,  MAVLINK_MSG_ID_VFR_HUD);
    QCOMPARE(calls[0].second, kHighRate);

    config.gotSetMessageIntervalAck();

    QCOMPARE(calls.size(), 2);
    QCOMPARE(calls[1].first,  MAVLINK_MSG_ID_NAV_CONTROLLER_OUTPUT);
    QCOMPARE(calls[1].second, kHighRate);

    config.gotSetMessageIntervalAck();
    QCOMPARE(calls.size(), 2);
}

// After fully configuring two messages, restoreDefaults() should emit rate=0
// for each previously configured message ID (in reverse push order).
void MAVLinkStreamConfigTest::_testRestoreDefaultsAfterConfigure()
{
    QList<QPair<int,int>> calls;
    MAVLinkStreamConfig config([&calls](int id, int rate){ calls.append({id, rate}); });

    // Configure attitude messages fully.
    config.setHighRateRateAndAttitude();     // callback[0]: ATTITUDE_TARGET, kHighRate
    config.gotSetMessageIntervalAck();       // callback[1]: ATTITUDE_QUATERNION, kHighRate
    config.gotSetMessageIntervalAck();       // → Idle, _changedIds = [ATTITUDE_TARGET, ATTITUDE_QUATERNION]
    QCOMPARE(calls.size(), 2);

    calls.clear();
    config.restoreDefaults();

    // Restore fires synchronously for the last pushed ID (ATTITUDE_QUATERNION was pushed second).
    QCOMPARE(calls.size(), 1);
    QCOMPARE(calls[0].first,  MAVLINK_MSG_ID_ATTITUDE_QUATERNION);
    QCOMPARE(calls[0].second, 0);

    config.gotSetMessageIntervalAck();

    QCOMPARE(calls.size(), 2);
    QCOMPARE(calls[1].first,  MAVLINK_MSG_ID_ATTITUDE_TARGET);
    QCOMPARE(calls[1].second, 0);

    // Both restored; further ack is a no-op.
    config.gotSetMessageIntervalAck();
    QCOMPARE(calls.size(), 2);
}

// restoreDefaults() from Idle (no previously configured messages) is a no-op.
void MAVLinkStreamConfigTest::_testRestoreDefaultsFromIdle()
{
    QList<QPair<int,int>> calls;
    MAVLinkStreamConfig config([&calls](int id, int rate){ calls.append({id, rate}); });

    config.restoreDefaults();
    QVERIFY(calls.isEmpty());

    config.gotSetMessageIntervalAck();
    QVERIFY(calls.isEmpty());
}

// Calling setHighRateVelAndPos() while setHighRateRateAndAttitude() is mid-flight
// (one message already sent, one still pending) should:
//   1. Abandon the pending ATTITUDE_QUATERNION message.
//   2. Restore the already-sent ATTITUDE_TARGET (rate=0).
//   3. Then configure the vel+pos messages.
void MAVLinkStreamConfigTest::_testInterruptConfigureWithNewRequest()
{
    QList<QPair<int,int>> calls;
    MAVLinkStreamConfig config([&calls](int id, int rate){ calls.append({id, rate}); });

    // Start attitude configure — first message fires synchronously.
    config.setHighRateRateAndAttitude();
    QCOMPARE(calls.size(), 1);
    QCOMPARE(calls[0].first,  MAVLINK_MSG_ID_ATTITUDE_TARGET);
    QCOMPARE(calls[0].second, kHighRate);

    // Interrupt before sending the second message.
    config.setHighRateVelAndPos();

    // The interrupt is staged; no extra callbacks yet.
    QCOMPARE(calls.size(), 1);

    // Next ack detects a pending _nextState and begins restoring.
    // ATTITUDE_TARGET (the only changedId) gets rate=0 synchronously inside nextDesiredRate.
    config.gotSetMessageIntervalAck();
    QCOMPARE(calls.size(), 2);
    QCOMPARE(calls[1].first,  MAVLINK_MSG_ID_ATTITUDE_TARGET);
    QCOMPARE(calls[1].second, 0);

    // Ack for the restore → changedIds empty → switch to Configuring vel+pos.
    // POSITION_TARGET_LOCAL_NED fires synchronously.
    config.gotSetMessageIntervalAck();
    QCOMPARE(calls.size(), 3);
    QCOMPARE(calls[2].first,  MAVLINK_MSG_ID_POSITION_TARGET_LOCAL_NED);
    QCOMPARE(calls[2].second, kHighRate);

    // Ack → LOCAL_POSITION_NED.
    config.gotSetMessageIntervalAck();
    QCOMPARE(calls.size(), 4);
    QCOMPARE(calls[3].first,  MAVLINK_MSG_ID_LOCAL_POSITION_NED);
    QCOMPARE(calls[3].second, kHighRate);

    // Ack → Idle, no more calls.
    config.gotSetMessageIntervalAck();
    QCOMPARE(calls.size(), 4);
}

UT_REGISTER_TEST(MAVLinkStreamConfigTest, TestLabel::Unit)
