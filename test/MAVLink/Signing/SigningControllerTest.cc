#include "SigningControllerTest.h"

#include <chrono>

#include <QtCore/QByteArrayView>
#include <QtCore/QRegularExpression>
#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

#include "MAVLinkLib.h"
#include "MAVLinkSigning.h"
#include "MAVLinkSigningKeys.h"
#include "SigningController.h"

namespace {

constexpr mavlink_channel_t kTestChannel = MAVLINK_COMM_3;
constexpr uint8_t kTestSysId = 7;
constexpr int kTimeoutWaitMs = 6500;

MAVLinkSigning::SigningKey makeKey(uint8_t fill)
{
    MAVLinkSigning::SigningKey key{};
    key.fill(fill);
    return key;
}

struct EnableOutcome
{
    bool succeeded = false;
    bool failed = false;
    QString keyName;
    SigningController::FailReason reason{};
    QString detail;
};

struct DisableOutcome
{
    bool succeeded = false;
    bool failed = false;
    SigningController::FailReason reason{};
    QString detail;
};

void wireEnable(SigningController& ctrl, EnableOutcome& out)
{
    QObject::connect(&ctrl, &SigningController::signingConfirmed, &ctrl, [&out](const QString& keyName) {
        out.succeeded = true;
        out.keyName = keyName;
    });
    QObject::connect(&ctrl, &SigningController::signingFailed, &ctrl, [&out](const SigningFailure& f) {
        out.failed = true;
        out.reason = f.reason;
        out.detail = f.detail;
    });
}

void wireDisable(SigningController& ctrl, DisableOutcome& out)
{
    QObject::connect(&ctrl, &SigningController::signingConfirmed, &ctrl,
                     [&out](const QString&) { out.succeeded = true; });
    QObject::connect(&ctrl, &SigningController::signingFailed, &ctrl, [&out](const SigningFailure& f) {
        out.failed = true;
        out.reason = f.reason;
        out.detail = f.detail;
    });
}

mavlink_message_t makeSignedHeartbeat(mavlink_channel_t channel, const MAVLinkSigning::SigningKey& keyBytes)
{
    const QByteArrayView kv(reinterpret_cast<const char*>(keyBytes.data()), keyBytes.size());
    SigningController scratch(channel);
    (void)scratch.initSigningImmediate(kv, MAVLinkSigning::UnsignedAcceptancePolicy::Strict);
    const mavlink_heartbeat_t heartbeat{};
    mavlink_message_t message;
    (void)mavlink_msg_heartbeat_encode_chan(1, MAV_COMP_ID_AUTOPILOT1, channel, &message, &heartbeat);
    (void)scratch.clearSigning();
    return message;
}

}  // namespace

void SigningControllerTest::initTestCase()
{
    UnitTest::initTestCase();
    MAVLinkSigningKeys::setPbkdf2IterationsForTesting(1);
    // Exercise the real timeout FSM path without the production 5s wall-clock wait.
    SigningController::setTimeoutForTesting(std::chrono::milliseconds(150));
}

void SigningControllerTest::cleanup()
{
    SigningController ctrl(kTestChannel);
    ctrl.clearSigning();
    ctrl.clearDetectCooldown();
    ctrl.resetBadSigBurst();
    MAVLinkSigningKeys::instance()->removeAllKeys();
    UnitTest::cleanup();
}

void SigningControllerTest::_testInitialState()
{
    SigningController ctrl(kTestChannel);
    QCOMPARE(ctrl.state(), SigningController::State::Off);
    QVERIFY(!ctrl.status().pending());
}

void SigningControllerTest::_testBeginEnableTransitionsToEnabling()
{
    SigningController ctrl(kTestChannel);
    QSignalSpy stateSpy(&ctrl, &SigningController::stateChanged);

    const auto key = makeKey(0xAB);
    (void)ctrl.tryBeginEnable(kTestSysId, QStringLiteral("k1"), key);

    QCOMPARE(ctrl.state(), SigningController::State::Enabling);
    QVERIFY(ctrl.status().pending());
    QVERIFY(stateSpy.count() >= 1);
}

void SigningControllerTest::_testBeginDisableTransitionsToDisabling()
{
    SigningController ctrl(kTestChannel);
    const auto key = makeKey(0xCD);
    const QByteArrayView kv(reinterpret_cast<const char*>(key.data()), key.size());
    QVERIFY(ctrl.initSigningImmediate(kv, MAVLinkSigning::UnsignedAcceptancePolicy::Strict));

    QSignalSpy stateSpy(&ctrl, &SigningController::stateChanged);
    (void)ctrl.tryBeginDisable(kTestSysId);

    QCOMPARE(ctrl.state(), SigningController::State::Disabling);
    QVERIFY(ctrl.status().pending());
    QVERIFY(stateSpy.count() >= 1);
}

void SigningControllerTest::_testReentryRejected()
{
    SigningController ctrl(kTestChannel);
    const auto key = makeKey(0x11);

    QVERIFY(!ctrl.tryBeginEnable(kTestSysId, QStringLiteral("a"), key));

    QSignalSpy failSpy(&ctrl, &SigningController::signingFailed);

    expectLogMessage("MAVLink.SigningController", QtWarningMsg, QRegularExpression("enable rejected: operation already pending"));
    auto reentryEnableFail = ctrl.tryBeginEnable(kTestSysId, QStringLiteral("b"), key);
    verifyExpectedLogMessage();
    QVERIFY(reentryEnableFail.has_value());
    QCOMPARE(reentryEnableFail->reason, SigningController::FailReason::VehicleUnreachable);

    expectLogMessage("MAVLink.SigningController", QtWarningMsg, QRegularExpression("disable rejected: operation already pending"));
    auto reentryDisableFail = ctrl.tryBeginDisable(kTestSysId);
    verifyExpectedLogMessage();
    QVERIFY(reentryDisableFail.has_value());
    QCOMPARE(reentryDisableFail->reason, SigningController::FailReason::VehicleUnreachable);

    QCOMPARE(failSpy.count(), 0);
    QCOMPARE(ctrl.state(), SigningController::State::Enabling);
}

void SigningControllerTest::_testEnableTimeoutFails()
{
    SigningController ctrl(kTestChannel);
    EnableOutcome outcome;
    wireEnable(ctrl, outcome);
    const auto key = makeKey(0x33);
    (void)ctrl.tryBeginEnable(kTestSysId, QStringLiteral("k"), key);

    expectLogMessage("MAVLink.SigningController", QtWarningMsg, QRegularExpression("Signing setup not confirmed by vehicle"));
    QTRY_VERIFY_WITH_TIMEOUT(outcome.failed, kTimeoutWaitMs);
    verifyExpectedLogMessage();
    QCOMPARE(outcome.reason, SigningController::FailReason::Timeout);
    QVERIFY(!outcome.succeeded);
    QCOMPARE(ctrl.state(), SigningController::State::Off);
    QVERIFY(!ctrl.isEnabled());
}

void SigningControllerTest::_testDisableTimeoutVehicleUnreachable()
{
    SigningController ctrl(kTestChannel);
    const auto key = makeKey(0x55);
    const QByteArrayView kv(reinterpret_cast<const char*>(key.data()), key.size());
    QVERIFY(ctrl.initSigningImmediate(kv, MAVLinkSigning::UnsignedAcceptancePolicy::Strict,
                                      QStringLiteral("active")));

    DisableOutcome outcome;
    wireDisable(ctrl, outcome);
    (void)ctrl.tryBeginDisable(kTestSysId);

    expectLogMessage("MAVLink.SigningController", QtWarningMsg, QRegularExpression("Signing disable not confirmed"));
    QTRY_VERIFY_WITH_TIMEOUT(outcome.failed, kTimeoutWaitMs);
    verifyExpectedLogMessage();
    QCOMPARE(outcome.reason, SigningController::FailReason::VehicleUnreachable);
    QVERIFY(!outcome.succeeded);
    QVERIFY(ctrl.isEnabled());
}

void SigningControllerTest::_testDestructorDuringPendingDoesNotCrash()
{
    auto* ctrl = new SigningController(kTestChannel);
    const auto key = makeKey(0x77);
    (void)ctrl->tryBeginEnable(kTestSysId, QStringLiteral("k"), key);
    QCOMPARE(ctrl->state(), SigningController::State::Enabling);
    delete ctrl;
    SigningController probe(kTestChannel);
    QVERIFY(!probe.isEnabled());
}

void SigningControllerTest::_testDestructorDuringEnableRestoresAutoDetect()
{
    auto* ctrl = new SigningController(kTestChannel);
    const auto key = makeKey(0xBB);
    (void)ctrl->tryBeginEnable(kTestSysId, QStringLiteral("k"), key);
    QVERIFY(ctrl->channel().detectSnapshot().autoDetectSuspended);

    delete ctrl;

    SigningController probe(kTestChannel);
    QVERIFY(!probe.channel().detectSnapshot().autoDetectSuspended);
}

void SigningControllerTest::_testDestructorDuringDisableRestoresStrictCallback()
{
    auto* ctrl = new SigningController(kTestChannel);
    const auto key = makeKey(0xDD);
    const QByteArrayView kv(reinterpret_cast<const char*>(key.data()), key.size());
    QVERIFY(ctrl->initSigningImmediate(kv, MAVLinkSigning::UnsignedAcceptancePolicy::Strict,
                                       QStringLiteral("active")));

    (void)ctrl->tryBeginDisable(kTestSysId);
    QCOMPARE(ctrl->state(), SigningController::State::Disabling);

    const mavlink_status_t* status = mavlink_get_channel_status(kTestChannel);
    QVERIFY(status && status->signing);
    QVERIFY(status->signing->accept_unsigned_callback ==
            MAVLinkSigning::callbackForPolicy(MAVLinkSigning::UnsignedAcceptancePolicy::Pending));

    delete ctrl;

    // Destructor must detach the signing pointers so the next parser call doesn't dangle.
    status = mavlink_get_channel_status(kTestChannel);
    QVERIFY(status);
    QVERIFY(status->signing == nullptr);
    QVERIFY(status->signing_streams == nullptr);
}

void SigningControllerTest::_testCancelDuringPendingEnable()
{
    SigningController ctrl(kTestChannel);
    EnableOutcome outcome;
    wireEnable(ctrl, outcome);
    const auto key = makeKey(0x99);
    (void)ctrl.tryBeginEnable(kTestSysId, QStringLiteral("k"), key);

    QCOMPARE(ctrl.state(), SigningController::State::Enabling);
    expectLogMessage("MAVLink.SigningController", QtWarningMsg, QRegularExpression("Signing operation cancelled"));
    ctrl.cancelPending();

    QTRY_VERIFY(outcome.failed);
    verifyExpectedLogMessage();
    QCOMPARE(outcome.reason, SigningController::FailReason::VehicleUnreachable);
    QVERIFY(!outcome.succeeded);
    QCOMPARE(ctrl.state(), SigningController::State::Off);
    QVERIFY(!ctrl.isEnabled());
}

void SigningControllerTest::_testCancelDuringPendingDisable()
{
    SigningController ctrl(kTestChannel);
    const auto key = makeKey(0xAA);
    const QByteArrayView kv(reinterpret_cast<const char*>(key.data()), key.size());
    QVERIFY(ctrl.initSigningImmediate(kv, MAVLinkSigning::UnsignedAcceptancePolicy::Strict,
                                      QStringLiteral("active")));

    DisableOutcome outcome;
    wireDisable(ctrl, outcome);
    (void)ctrl.tryBeginDisable(kTestSysId);
    expectLogMessage("MAVLink.SigningController", QtWarningMsg, QRegularExpression("Signing disable not confirmed"));
    ctrl.cancelPending();

    QTRY_VERIFY(outcome.failed);
    verifyExpectedLogMessage();
    QCOMPARE(outcome.reason, SigningController::FailReason::VehicleUnreachable);
    QVERIFY(!outcome.succeeded);
    QVERIFY(ctrl.isEnabled());
}

namespace {
constexpr mavlink_channel_t kMgrTestChannel = MAVLINK_COMM_5;

mavlink_message_t makeUnsignedHeartbeat(mavlink_channel_t channel)
{
    const mavlink_heartbeat_t heartbeat{};
    mavlink_message_t message;
    (void)mavlink_msg_heartbeat_encode_chan(1, MAV_COMP_ID_AUTOPILOT1, channel, &message, &heartbeat);
    return message;
}
}  // namespace

void SigningControllerTest::_testBadSignatureAlertThreshold()
{
    SigningController ctrl(kMgrTestChannel);
    QSignalSpy alertSpy(&ctrl, &SigningController::alertRaised);
    const auto msg = makeUnsignedHeartbeat(kMgrTestChannel);

    ctrl.processFrame(/*framingOk=*/false, msg);
    ctrl.processFrame(/*framingOk=*/false, msg);
    QCOMPARE(alertSpy.count(), 0);
    ctrl.processFrame(/*framingOk=*/false, msg);
    QCOMPARE(alertSpy.count(), 1);
}

void SigningControllerTest::_testBadSignatureAlertResetOnOk()
{
    SigningController ctrl(kMgrTestChannel);
    QSignalSpy alertSpy(&ctrl, &SigningController::alertRaised);
    const auto msg = makeUnsignedHeartbeat(kMgrTestChannel);

    ctrl.processFrame(false, msg);
    ctrl.processFrame(false, msg);
    ctrl.processFrame(true, msg);
    ctrl.processFrame(false, msg);
    ctrl.processFrame(false, msg);
    QCOMPARE(alertSpy.count(), 0);
    ctrl.processFrame(false, msg);
    QCOMPARE(alertSpy.count(), 1);
}

void SigningControllerTest::_testBadSignatureAlertEdgeTriggered()
{
    SigningController ctrl(kMgrTestChannel);
    QSignalSpy alertSpy(&ctrl, &SigningController::alertRaised);
    const auto msg = makeUnsignedHeartbeat(kMgrTestChannel);

    for (int i = 0; i < 10; ++i) {
        ctrl.processFrame(false, msg);
    }
    QCOMPARE(alertSpy.count(), 1);
}

void SigningControllerTest::_testResetForLinkClearsBurstState()
{
    SigningController ctrl(kMgrTestChannel);
    QSignalSpy alertSpy(&ctrl, &SigningController::alertRaised);
    const auto msg = makeUnsignedHeartbeat(kMgrTestChannel);

    for (int i = 0; i < 5; ++i) {
        ctrl.processFrame(false, msg);
    }
    QCOMPARE(alertSpy.count(), 1);

    ctrl.resetBadSigBurst();
    ctrl.processFrame(false, msg);
    ctrl.processFrame(false, msg);
    QCOMPARE(alertSpy.count(), 1);
    ctrl.processFrame(false, msg);
    QCOMPARE(alertSpy.count(), 2);
}

void SigningControllerTest::_testKeyAutoDetectedEmittedOnMatch()
{
    SigningController ctrl(kMgrTestChannel);
    auto* keys = MAVLinkSigningKeys::instance();

    QVERIFY(keys->addKey(QStringLiteral("DetectKey"), QStringLiteral("auto-detect-passphrase")));
    const auto* entry = keys->keyAt(0);
    QVERIFY(entry);
    const auto& keyBytes = entry->keyBytes();

    const auto signedMsg = makeSignedHeartbeat(MAVLINK_COMM_4, keyBytes);
    QVERIFY(MAVLinkSigning::isMessageSigned(signedMsg));
    QVERIFY(!ctrl.isEnabled());

    QSignalSpy detectSpy(&ctrl, &SigningController::keyAutoDetected);
    const QString detected = MAVLinkSigningKeys::instance()->tryDetectKey(&ctrl, signedMsg);
    if (!detected.isEmpty()) {
        emit ctrl.keyAutoDetected(detected);
    }

    QCOMPARE(detectSpy.count(), 1);
    QCOMPARE(detectSpy.first().at(0).toString(), QStringLiteral("DetectKey"));
}

void SigningControllerTest::_testKeyAutoDetectedNotEmittedWhenChannelSigning()
{
    SigningController ctrl(kMgrTestChannel);
    auto* keys = MAVLinkSigningKeys::instance();

    QVERIFY(keys->addKey(QStringLiteral("AlreadyOn"), QStringLiteral("passpass")));
    const auto* entry = keys->keyAt(0);
    QVERIFY(entry);
    const auto& keyBytes = entry->keyBytes();

    const auto signedMsg = makeSignedHeartbeat(MAVLINK_COMM_4, keyBytes);
    const QByteArrayView kv(reinterpret_cast<const char*>(keyBytes.data()), keyBytes.size());
    QVERIFY(ctrl.initSigningImmediate(kv, MAVLinkSigning::UnsignedAcceptancePolicy::Strict));

    const QString detected = MAVLinkSigningKeys::instance()->tryDetectKey(&ctrl, signedMsg);
    QVERIFY(detected.isEmpty());
}

void SigningControllerTest::_testKeyAutoDetectedNotEmittedOnUnsigned()
{
    SigningController ctrl(kMgrTestChannel);
    auto* keys = MAVLinkSigningKeys::instance();
    QVERIFY(keys->addKey(QStringLiteral("Unused"), QStringLiteral("passpass")));

    const mavlink_heartbeat_t heartbeat{};
    mavlink_message_t message;
    (void)mavlink_msg_heartbeat_encode_chan(1, MAV_COMP_ID_AUTOPILOT1, kMgrTestChannel, &message, &heartbeat);
    QVERIFY(!MAVLinkSigning::isMessageSigned(message));

    const QString detected = MAVLinkSigningKeys::instance()->tryDetectKey(&ctrl, message);
    QVERIFY(detected.isEmpty());
}

void SigningControllerTest::_testStateOff()
{
    SigningController ctrl(kMgrTestChannel);
    QCOMPARE(ctrl.state(), SigningController::State::Off);
    QVERIFY(ctrl.status().keyName.isEmpty());
}

void SigningControllerTest::_testStateOn()
{
    SigningController ctrl(kMgrTestChannel);
    QByteArray rawKey(32, '\x42');
    const auto key = MAVLinkSigning::makeSigningKey(rawKey).value();
    const QByteArrayView kv(reinterpret_cast<const char*>(key.data()), key.size());
    QVERIFY(ctrl.initSigningImmediate(kv, MAVLinkSigning::UnsignedAcceptancePolicy::Strict,
                                      QStringLiteral("named-key")));

    QCOMPARE(ctrl.state(), SigningController::State::On);
    QCOMPARE(ctrl.status().keyName, QStringLiteral("named-key"));
}

void SigningControllerTest::_testStatusTextOff()
{
    SigningController ctrl(kMgrTestChannel);
    QCOMPARE(ctrl.statusText(), tr("Off"));
}

void SigningControllerTest::_testStatusTextOn()
{
    SigningController ctrl(kMgrTestChannel);
    QByteArray rawKey(32, '\x42');
    const auto key = MAVLinkSigning::makeSigningKey(rawKey).value();
    const QByteArrayView kv(reinterpret_cast<const char*>(key.data()), key.size());
    QVERIFY(ctrl.initSigningImmediate(kv, MAVLinkSigning::UnsignedAcceptancePolicy::Strict,
                                      QStringLiteral("named-key")));

    QCOMPARE(ctrl.statusText(), tr("On"));
}

void SigningControllerTest::_testStateChangedFiresOnEnableThenCancel()
{
    SigningController ctrl(kTestChannel);
    QSignalSpy stateSpy(&ctrl, &SigningController::stateChanged);

    const auto key = makeKey(0x42);
    (void)ctrl.tryBeginEnable(kTestSysId, QStringLiteral("k"), key);
    QCOMPARE(ctrl.state(), SigningController::State::Enabling);
    QCOMPARE(stateSpy.count(), 1);

    expectLogMessage("MAVLink.SigningController", QtWarningMsg, QRegularExpression("Signing operation cancelled"));
    ctrl.cancelPending();
    QTRY_COMPARE(ctrl.state(), SigningController::State::Off);
    verifyExpectedLogMessage();
    QVERIFY(stateSpy.count() >= 2);
}

void SigningControllerTest::_testStateChangedFiresOnDisableThenCancel()
{
    SigningController ctrl(kTestChannel);
    const auto key = makeKey(0x99);
    const QByteArrayView kv(reinterpret_cast<const char*>(key.data()), key.size());
    QVERIFY(ctrl.initSigningImmediate(kv, MAVLinkSigning::UnsignedAcceptancePolicy::Strict,
                                      QStringLiteral("active")));
    QCOMPARE(ctrl.state(), SigningController::State::On);

    QSignalSpy stateSpy(&ctrl, &SigningController::stateChanged);
    (void)ctrl.tryBeginDisable(kTestSysId);
    QCOMPARE(ctrl.state(), SigningController::State::Disabling);
    QCOMPARE(stateSpy.count(), 1);

    expectLogMessage("MAVLink.SigningController", QtWarningMsg, QRegularExpression("Signing disable not confirmed"));
    ctrl.cancelPending();
    QTRY_COMPARE(ctrl.state(), SigningController::State::On);
    verifyExpectedLogMessage();
    QVERIFY(stateSpy.count() >= 2);
}

void SigningControllerTest::_testPermanentListenerSurvivesMultipleCycles()
{
    SigningController ctrl(kTestChannel);
    int failureCount = 0;
    QObject::connect(&ctrl, &SigningController::signingFailed, &ctrl, [&](const SigningFailure&) { ++failureCount; });

    const auto key = makeKey(0x33);
    for (int i = 0; i < 3; ++i) {
        (void)ctrl.tryBeginEnable(kTestSysId, QStringLiteral("k"), key);
        QCOMPARE(ctrl.state(), SigningController::State::Enabling);
        ignoreLogMessage("MAVLink.SigningController", QtWarningMsg, QRegularExpression("Signing operation cancelled"));
        ctrl.cancelPending();
        QCOMPARE(ctrl.state(), SigningController::State::Off);
    }
    QCOMPARE(failureCount, 3);
}

void SigningControllerTest::_testCancelOnIdleNoOp()
{
    SigningController ctrl(kTestChannel);
    QSignalSpy failedSpy(&ctrl, &SigningController::signingFailed);
    QSignalSpy stateSpy(&ctrl, &SigningController::stateChanged);

    ctrl.cancelPending();

    QCOMPARE(failedSpy.count(), 0);
    QCOMPARE(stateSpy.count(), 0);
    QCOMPARE(ctrl.state(), SigningController::State::Off);
}

void SigningControllerTest::_testStatusTextDuringPending()
{
    SigningController ctrl(kTestChannel);
    QCOMPARE(ctrl.statusText(), tr("Off"));

    const auto key = makeKey(0x55);
    (void)ctrl.tryBeginEnable(kTestSysId, QStringLiteral("k"), key);
    QCOMPARE(ctrl.statusText(), tr("Configuring…"));

    expectLogMessage("MAVLink.SigningController", QtWarningMsg, QRegularExpression("Signing operation cancelled"));
    ctrl.cancelPending();
    verifyExpectedLogMessage();
    QCOMPARE(ctrl.statusText(), tr("Off"));

    const QByteArrayView kv(reinterpret_cast<const char*>(key.data()), key.size());
    QVERIFY(ctrl.initSigningImmediate(kv, MAVLinkSigning::UnsignedAcceptancePolicy::Strict, QStringLiteral("k")));
    (void)ctrl.tryBeginDisable(kTestSysId);
    QCOMPARE(ctrl.statusText(), tr("Disabling…"));
    expectLogMessage("MAVLink.SigningController", QtWarningMsg, QRegularExpression("Signing disable not confirmed"));
    ctrl.cancelPending();
    verifyExpectedLogMessage();
}

void SigningControllerTest::_testExpectedSysIdScopedToPendingOp()
{
    auto encodeSigned = [&](uint8_t sysid) {
        const auto scratchKey = makeKey(0x77);  // bind temporary so kv doesn't dangle
        const QByteArrayView kv(reinterpret_cast<const char*>(scratchKey.data()), scratchKey.size());
        SigningController scratch(kTestChannel);
        (void)scratch.initSigningImmediate(kv, MAVLinkSigning::UnsignedAcceptancePolicy::Strict);
        const mavlink_heartbeat_t heartbeat{};
        mavlink_message_t message;
        (void)mavlink_msg_heartbeat_encode_chan(sysid, MAV_COMP_ID_AUTOPILOT1, kTestChannel, &message, &heartbeat);
        (void)scratch.clearSigning();
        return message;
    };

    SigningController ctrl(kTestChannel);
    const auto key = makeKey(0x77);
    (void)ctrl.tryBeginEnable(kTestSysId, QStringLiteral("k"), key);

    mavlink_message_t fromOtherSys = encodeSigned(static_cast<uint8_t>(kTestSysId + 1));
    ctrl.processFrame(true, fromOtherSys);
    QCOMPARE(ctrl.state(), SigningController::State::Enabling);

    mavlink_message_t fromExpectedSys = encodeSigned(kTestSysId);
    ctrl.processFrame(true, fromExpectedSys);
    QCOMPARE(ctrl.state(), SigningController::State::On);
}

// Regression: mavlink/qgroundcontrol#14375 — the wall-clock refresh timer must run once the channel is signing so an
// idle outbound path doesn't drift behind wall clock. Guards the timer-gating that replaced the always-on ctor timer.
void SigningControllerTest::_testWallClockTimerRefreshesAfterEnable()
{
    SigningController ctrl(kTestChannel);
    QVERIFY(!ctrl.wallClockRefreshActiveForTesting());

    const auto key = makeKey(0x5A);
    const QByteArrayView kv(reinterpret_cast<const char*>(key.data()), key.size());
    QVERIFY(ctrl.initSigningImmediate(kv, MAVLinkSigning::UnsignedAcceptancePolicy::Strict, QStringLiteral("wc")));
    QVERIFY(ctrl.wallClockRefreshActiveForTesting());

    auto* const signing = mavlink_get_channel_status(kTestChannel)->signing;
    QVERIFY(signing);

    // Stall 3 minutes behind wall clock; the 1Hz timer must catch the timestamp back up on its own.
    constexpr uint64_t kThreeMinutesTicks = 3ULL * 60 * 100'000;
    const uint64_t stale = MAVLinkSigning::currentSigningTimestampTicks() - kThreeMinutesTicks;
    signing->timestamp = stale;

    QTRY_VERIFY_WITH_TIMEOUT(signing->timestamp > stale, 3000);
    QVERIFY(signing->timestamp >= MAVLinkSigning::currentSigningTimestampTicks() - (2ULL * 100'000));

    QVERIFY(ctrl.clearSigning());
    QVERIFY(!ctrl.wallClockRefreshActiveForTesting());
}

void SigningControllerTest::_testWallClockTimerStoppedWhenIdle()
{
    SigningController ctrl(kTestChannel);
    QVERIFY(!ctrl.wallClockRefreshActiveForTesting());

    // Pending-enable installs the channel (signOutgoing flips on confirm) → timer runs meanwhile.
    const auto key = makeKey(0x6B);
    (void)ctrl.tryBeginEnable(kTestSysId, QStringLiteral("k"), key);
    QVERIFY(ctrl.wallClockRefreshActiveForTesting());

    // Aborting back to Idle disables the channel and must stop the timer.
    expectLogMessage("MAVLink.SigningController", QtWarningMsg, QRegularExpression("Signing operation cancelled"));
    ctrl.cancelPending();
    QTRY_VERIFY(!ctrl.wallClockRefreshActiveForTesting());
    verifyExpectedLogMessage();
    QCOMPARE(ctrl.state(), SigningController::State::Off);
}

UT_REGISTER_TEST(SigningControllerTest, TestLabel::Unit, TestLabel::Comms, TestLabel::Slow)
