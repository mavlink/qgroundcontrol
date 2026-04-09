#include "SigningControllerTest.h"

#include <QtCore/QByteArrayView>
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
    QObject::connect(&ctrl, &SigningController::signingConfirmed, &ctrl,
                     [&out](const QString& keyName) {
                         out.succeeded = true;
                         out.keyName = keyName;
                     });
    QObject::connect(&ctrl, &SigningController::signingFailed, &ctrl,
                     [&out](const SigningFailure& f) {
                         out.failed = true;
                         out.reason = f.reason;
                         out.detail = f.detail;
                     });
}

void wireDisable(SigningController& ctrl, DisableOutcome& out)
{
    QObject::connect(&ctrl, &SigningController::signingConfirmed, &ctrl,
                     [&out](const QString&) { out.succeeded = true; });
    QObject::connect(&ctrl, &SigningController::signingFailed, &ctrl,
                     [&out](const SigningFailure& f) {
                         out.failed = true;
                         out.reason = f.reason;
                         out.detail = f.detail;
                     });
}

mavlink_message_t makeSignedHeartbeat(mavlink_channel_t channel, const MAVLinkSigning::SigningKey& keyBytes)
{
    const QByteArrayView kv(reinterpret_cast<const char*>(keyBytes.data()), keyBytes.size());
    SigningController scratch(channel);
    (void)scratch.initSigningImmediate(kv, MAVLinkSigning::insecureConnectionAcceptUnsignedCallback);
    const mavlink_heartbeat_t heartbeat{};
    mavlink_message_t message;
    (void)mavlink_msg_heartbeat_encode_chan(1, MAV_COMP_ID_AUTOPILOT1, channel, &message, &heartbeat);
    (void)scratch.clearSigning();
    return message;
}

}  // namespace

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
    ctrl.beginEnable(kTestSysId, QStringLiteral("k1"), key);

    QCOMPARE(ctrl.state(), SigningController::State::Enabling);
    QVERIFY(ctrl.status().pending());
    QVERIFY(stateSpy.count() >= 1);
}

void SigningControllerTest::_testBeginDisableTransitionsToDisabling()
{
    SigningController ctrl(kTestChannel);
    const auto key = makeKey(0xCD);
    const QByteArrayView kv(reinterpret_cast<const char*>(key.data()), key.size());
    QVERIFY(ctrl.initSigningImmediate(kv, MAVLinkSigning::secureConnectionAcceptUnsignedCallback));

    QSignalSpy stateSpy(&ctrl, &SigningController::stateChanged);
    ctrl.beginDisable(kTestSysId);

    QCOMPARE(ctrl.state(), SigningController::State::Disabling);
    QVERIFY(ctrl.status().pending());
    QVERIFY(stateSpy.count() >= 1);
}

void SigningControllerTest::_testReentryRejected()
{
    SigningController ctrl(kTestChannel);
    const auto key = makeKey(0x11);

    ctrl.beginEnable(kTestSysId, QStringLiteral("a"), key);

    // Re-entry: any subsequent begin*() while pending must emit signingFailed synchronously.
    EnableOutcome reentryEnable;
    DisableOutcome reentryDisable;
    wireEnable(ctrl, reentryEnable);
    ctrl.beginEnable(kTestSysId, QStringLiteral("b"), key);
    wireDisable(ctrl, reentryDisable);
    ctrl.beginDisable(kTestSysId);

    QTRY_VERIFY(reentryEnable.failed);
    QTRY_VERIFY(reentryDisable.failed);
    QCOMPARE(reentryEnable.reason, SigningController::FailReason::VehicleUnreachable);
    QCOMPARE(reentryDisable.reason, SigningController::FailReason::VehicleUnreachable);
    QCOMPARE(ctrl.state(), SigningController::State::Enabling);
}

void SigningControllerTest::_testEnableTimeoutFails()
{
    SigningController ctrl(kTestChannel);
    EnableOutcome outcome;
    wireEnable(ctrl, outcome);
    const auto key = makeKey(0x33);
    ctrl.beginEnable(kTestSysId, QStringLiteral("k"), key);

    QTRY_VERIFY_WITH_TIMEOUT(outcome.failed, kTimeoutWaitMs);
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
    QVERIFY(ctrl.initSigningImmediate(kv, MAVLinkSigning::secureConnectionAcceptUnsignedCallback,
                                      QStringLiteral("active")));

    DisableOutcome outcome;
    wireDisable(ctrl, outcome);
    ctrl.beginDisable(kTestSysId);

    QTRY_VERIFY_WITH_TIMEOUT(outcome.failed, kTimeoutWaitMs);
    QCOMPARE(outcome.reason, SigningController::FailReason::VehicleUnreachable);
    QVERIFY(!outcome.succeeded);
    QVERIFY(ctrl.isEnabled());
}

void SigningControllerTest::_testDestructorDuringPendingDoesNotCrash()
{
    auto* ctrl = new SigningController(kTestChannel);
    const auto key = makeKey(0x77);
    ctrl->beginEnable(kTestSysId, QStringLiteral("k"), key);
    QCOMPARE(ctrl->state(), SigningController::State::Enabling);
    delete ctrl;
    SigningController probe(kTestChannel);
    QVERIFY(!probe.isEnabled());
}

void SigningControllerTest::_testDestructorDuringEnableRestoresAutoDetect()
{
    auto* ctrl = new SigningController(kTestChannel);
    const auto key = makeKey(0xBB);
    ctrl->beginEnable(kTestSysId, QStringLiteral("k"), key);
    QVERIFY(ctrl->detectSnapshot().autoDetectSuspended);

    delete ctrl;

    SigningController probe(kTestChannel);
    QVERIFY(!probe.detectSnapshot().autoDetectSuspended);
}

void SigningControllerTest::_testDestructorDuringDisableRestoresStrictCallback()
{
    auto* ctrl = new SigningController(kTestChannel);
    const auto key = makeKey(0xDD);
    const QByteArrayView kv(reinterpret_cast<const char*>(key.data()), key.size());
    QVERIFY(ctrl->initSigningImmediate(kv, MAVLinkSigning::secureConnectionAcceptUnsignedCallback,
                                       QStringLiteral("active")));

    ctrl->beginDisable(kTestSysId);
    QCOMPARE(ctrl->state(), SigningController::State::Disabling);

    const mavlink_status_t* status = mavlink_get_channel_status(kTestChannel);
    QVERIFY(status && status->signing);
    QVERIFY(status->signing->accept_unsigned_callback == MAVLinkSigning::pendingDisableAcceptUnsignedCallback);

    delete ctrl;

    status = mavlink_get_channel_status(kTestChannel);
    QVERIFY(status && status->signing);
    QVERIFY(status->signing->accept_unsigned_callback == MAVLinkSigning::secureConnectionAcceptUnsignedCallback);
}

void SigningControllerTest::_testCancelDuringPendingEnable()
{
    SigningController ctrl(kTestChannel);
    EnableOutcome outcome;
    wireEnable(ctrl, outcome);
    const auto key = makeKey(0x99);
    ctrl.beginEnable(kTestSysId, QStringLiteral("k"), key);

    QCOMPARE(ctrl.state(), SigningController::State::Enabling);
    ctrl.cancelPending();

    QTRY_VERIFY(outcome.failed);
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
    QVERIFY(ctrl.initSigningImmediate(kv, MAVLinkSigning::secureConnectionAcceptUnsignedCallback,
                                      QStringLiteral("active")));

    DisableOutcome outcome;
    wireDisable(ctrl, outcome);
    ctrl.beginDisable(kTestSysId);
    ctrl.cancelPending();

    QTRY_VERIFY(outcome.failed);
    QCOMPARE(outcome.reason, SigningController::FailReason::VehicleUnreachable);
    QVERIFY(!outcome.succeeded);
    QVERIFY(ctrl.isEnabled());
}

// --- Alert burst logic tests (formerly MAVLinkSigningManagerTest) ---

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

    // Threshold is 3 consecutive bad-signature frames.
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
    ctrl.processFrame(true, msg);  // OK frame clears burst.
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

    // Repeated bad frames past threshold must emit alert exactly once.
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
    QCOMPARE(alertSpy.count(), 1);  // still 1, threshold not yet reached
    ctrl.processFrame(false, msg);
    QCOMPARE(alertSpy.count(), 2);  // threshold re-reached after reset
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
    // tryDetectKey is called by MAVLinkSigningManager::processFrame; we call it directly here
    // since we can't construct a LinkInterface in pure unit tests.
    const QString detected =
        MAVLinkSigningKeys::instance()->tryDetectKey(&ctrl, signedMsg);
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

    QVERIFY(keys->addKey(QStringLiteral("AlreadyOn"), QStringLiteral("pass")));
    const auto* entry = keys->keyAt(0);
    QVERIFY(entry);
    const auto& keyBytes = entry->keyBytes();

    const auto signedMsg = makeSignedHeartbeat(MAVLINK_COMM_4, keyBytes);
    const QByteArrayView kv(reinterpret_cast<const char*>(keyBytes.data()), keyBytes.size());
    QVERIFY(ctrl.initSigningImmediate(kv, MAVLinkSigning::secureConnectionAcceptUnsignedCallback));

    // When channel is already signing, tryDetectKey returns empty — auto-detect not emitted.
    const QString detected =
        MAVLinkSigningKeys::instance()->tryDetectKey(&ctrl, signedMsg);
    QVERIFY(detected.isEmpty());
}

void SigningControllerTest::_testKeyAutoDetectedNotEmittedOnUnsigned()
{
    SigningController ctrl(kMgrTestChannel);
    auto* keys = MAVLinkSigningKeys::instance();
    QVERIFY(keys->addKey(QStringLiteral("Unused"), QStringLiteral("pass")));

    const mavlink_heartbeat_t heartbeat{};
    mavlink_message_t message;
    (void)mavlink_msg_heartbeat_encode_chan(1, MAV_COMP_ID_AUTOPILOT1, kMgrTestChannel, &message, &heartbeat);
    QVERIFY(!MAVLinkSigning::isMessageSigned(message));

    const QString detected =
        MAVLinkSigningKeys::instance()->tryDetectKey(&ctrl, message);
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
    QVERIFY(ctrl.initSigningImmediate(kv, MAVLinkSigning::secureConnectionAcceptUnsignedCallback,
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
    QVERIFY(ctrl.initSigningImmediate(kv, MAVLinkSigning::secureConnectionAcceptUnsignedCallback,
                                      QStringLiteral("named-key")));

    // Fresh init → last_status is NONE, statusText returns "On".
    QCOMPARE(ctrl.statusText(), tr("On"));
}

void SigningControllerTest::_testStateChangedFiresOnEnableThenCancel()
{
    SigningController ctrl(kTestChannel);
    QSignalSpy stateSpy(&ctrl, &SigningController::stateChanged);

    const auto key = makeKey(0x42);
    ctrl.beginEnable(kTestSysId, QStringLiteral("k"), key);
    QCOMPARE(ctrl.state(), SigningController::State::Enabling);
    QCOMPARE(stateSpy.count(), 1);

    ctrl.cancelPending();
    QTRY_COMPARE(ctrl.state(), SigningController::State::Off);
    QVERIFY(stateSpy.count() >= 2);
}

void SigningControllerTest::_testStateChangedFiresOnDisableThenCancel()
{
    SigningController ctrl(kTestChannel);
    const auto key = makeKey(0x99);
    const QByteArrayView kv(reinterpret_cast<const char*>(key.data()), key.size());
    QVERIFY(ctrl.initSigningImmediate(kv, MAVLinkSigning::secureConnectionAcceptUnsignedCallback,
                                      QStringLiteral("active")));
    QCOMPARE(ctrl.state(), SigningController::State::On);

    QSignalSpy stateSpy(&ctrl, &SigningController::stateChanged);
    ctrl.beginDisable(kTestSysId);
    QCOMPARE(ctrl.state(), SigningController::State::Disabling);
    QCOMPARE(stateSpy.count(), 1);

    ctrl.cancelPending();
    QTRY_COMPARE(ctrl.state(), SigningController::State::On);
    QVERIFY(stateSpy.count() >= 2);
}

UT_REGISTER_TEST(SigningControllerTest, TestLabel::Unit, TestLabel::Comms, TestLabel::Slow)
