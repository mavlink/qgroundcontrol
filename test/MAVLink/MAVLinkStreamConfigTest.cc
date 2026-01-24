#include "MAVLinkStreamConfigTest.h"
#include "MAVLinkStreamConfig.h"
#include "MAVLinkLib.h"

#include <QtTest/QTest>

void MAVLinkStreamConfigTest::init()
{
    _messageIntervalCalls.clear();
}

void MAVLinkStreamConfigTest::_recordMessageInterval(int messageId, int rate)
{
    _messageIntervalCalls.append({messageId, rate});
}

void MAVLinkStreamConfigTest::_initialStateTest()
{
    MAVLinkStreamConfig config([this](int id, int rate) {
        _recordMessageInterval(id, rate);
    });

    // No calls should be made on construction
    QVERIFY(_messageIntervalCalls.isEmpty());
}

void MAVLinkStreamConfigTest::_setHighRateRateAndAttitudeTest()
{
    MAVLinkStreamConfig config([this](int id, int rate) {
        _recordMessageInterval(id, rate);
    });

    config.setHighRateRateAndAttitude();

    // Should start configuring - first call should be a configuration call
    QVERIFY(!_messageIntervalCalls.isEmpty());

    // Simulate ACKs to complete the configuration
    // Guard against infinite loops with maxIterations
    int maxIterations = 20;
    while (maxIterations-- > 0 && !_messageIntervalCalls.isEmpty()) {
        _messageIntervalCalls.clear();
        config.gotSetMessageIntervalAck();
    }
    QVERIFY2(maxIterations >= 0, "Configuration took too many iterations");

    // After all ACKs, should be idle (no more calls)
    config.gotSetMessageIntervalAck();
    QVERIFY(_messageIntervalCalls.isEmpty());
}

void MAVLinkStreamConfigTest::_setHighRateVelAndPosTest()
{
    MAVLinkStreamConfig config([this](int id, int rate) {
        _recordMessageInterval(id, rate);
    });

    config.setHighRateVelAndPos();

    // Should start configuring
    QVERIFY(!_messageIntervalCalls.isEmpty());

    // Check that LOCAL_POSITION_NED and POSITION_TARGET_LOCAL_NED are configured
    bool hasLocalPos = false;
    bool hasPosTarget = false;

    // Process all ACKs
    int maxIterations = 10;
    while (maxIterations-- > 0) {
        for (const auto& call : _messageIntervalCalls) {
            if (call.first == MAVLINK_MSG_ID_LOCAL_POSITION_NED && call.second > 0) {
                hasLocalPos = true;
            }
            if (call.first == MAVLINK_MSG_ID_POSITION_TARGET_LOCAL_NED && call.second > 0) {
                hasPosTarget = true;
            }
        }
        _messageIntervalCalls.clear();
        config.gotSetMessageIntervalAck();
        if (_messageIntervalCalls.isEmpty()) break;
    }

    QVERIFY(hasLocalPos);
    QVERIFY(hasPosTarget);
}

void MAVLinkStreamConfigTest::_setHighRateAltAirspeedTest()
{
    MAVLinkStreamConfig config([this](int id, int rate) {
        _recordMessageInterval(id, rate);
    });

    config.setHighRateAltAirspeed();

    // Should start configuring
    QVERIFY(!_messageIntervalCalls.isEmpty());

    // Check that NAV_CONTROLLER_OUTPUT and VFR_HUD are configured
    bool hasNavController = false;
    bool hasVfrHud = false;

    // Process all ACKs
    int maxIterations = 10;
    while (maxIterations-- > 0) {
        for (const auto& call : _messageIntervalCalls) {
            if (call.first == MAVLINK_MSG_ID_NAV_CONTROLLER_OUTPUT && call.second > 0) {
                hasNavController = true;
            }
            if (call.first == MAVLINK_MSG_ID_VFR_HUD && call.second > 0) {
                hasVfrHud = true;
            }
        }
        _messageIntervalCalls.clear();
        config.gotSetMessageIntervalAck();
        if (_messageIntervalCalls.isEmpty()) break;
    }

    QVERIFY(hasNavController);
    QVERIFY(hasVfrHud);
}

void MAVLinkStreamConfigTest::_restoreDefaultsTest()
{
    MAVLinkStreamConfig config([this](int id, int rate) {
        _recordMessageInterval(id, rate);
    });

    // First configure something
    config.setHighRateRateAndAttitude();

    // Process ACKs to complete configuration
    int maxIterations = 10;
    while (maxIterations-- > 0 && !_messageIntervalCalls.isEmpty()) {
        _messageIntervalCalls.clear();
        config.gotSetMessageIntervalAck();
    }

    // Now restore defaults
    config.restoreDefaults();

    // Should see restore calls (rate = 0)
    bool hasRestoreCall = false;
    maxIterations = 10;
    while (maxIterations-- > 0) {
        for (const auto& call : _messageIntervalCalls) {
            if (call.second == 0) {
                hasRestoreCall = true;
            }
        }
        if (_messageIntervalCalls.isEmpty()) break;
        _messageIntervalCalls.clear();
        config.gotSetMessageIntervalAck();
    }

    QVERIFY(hasRestoreCall);
}

void MAVLinkStreamConfigTest::_stateInterruptionTest()
{
    MAVLinkStreamConfig config([this](int id, int rate) {
        _recordMessageInterval(id, rate);
    });

    // Start one configuration
    config.setHighRateRateAndAttitude();

    // Before completing, start another configuration (should interrupt)
    config.setHighRateVelAndPos();

    // Process ACKs - should eventually configure the second request
    bool hasLocalPos = false;
    int maxIterations = 20;
    while (maxIterations-- > 0) {
        for (const auto& call : _messageIntervalCalls) {
            if (call.first == MAVLINK_MSG_ID_LOCAL_POSITION_NED && call.second > 0) {
                hasLocalPos = true;
            }
        }
        if (_messageIntervalCalls.isEmpty()) break;
        _messageIntervalCalls.clear();
        config.gotSetMessageIntervalAck();
    }

    QVERIFY(hasLocalPos);
}
