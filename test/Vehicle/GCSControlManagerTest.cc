#include "GCSControlManagerTest.h"

#include <QtCore/QRegularExpression>
#include <QtTest/QSignalSpy>

#include "MAVLinkLib.h"
#include "MAVLinkProtocol.h"
#include "Vehicle.h"

namespace {

mavlink_message_t makeControlStatusMessage(uint8_t systemId, uint8_t senderCompId, uint8_t flags, uint8_t gcsMain,
                                           const QList<int>& secondaryIds)
{
    uint8_t secondary[10] = {};
    for (int i = 0; i < secondaryIds.count() && i < 10; ++i) {
        secondary[i] = static_cast<uint8_t>(secondaryIds.at(i));
    }

    mavlink_message_t message{};
    (void) mavlink_msg_control_status_pack(systemId, senderCompId, &message, flags, gcsMain, secondary);
    return message;
}

mavlink_message_t makeForwardedRequestOperatorControlMessage(uint8_t systemId, uint8_t senderCompId, float action,
                                                             float allowTakeover, float timeoutSecs,
                                                             float requestingSysid)
{
    mavlink_message_t message{};
    (void) mavlink_msg_command_long_pack(
        systemId, senderCompId, &message, static_cast<uint8_t>(MAVLinkProtocol::instance()->getSystemId()),
        static_cast<uint8_t>(MAVLinkProtocol::getComponentId()), MAV_CMD_REQUEST_OPERATOR_CONTROL,
        0,  // confirmation
        action, allowTakeover, timeoutSecs, requestingSysid, 0, 0, 0);
    return message;
}

}  // namespace

void GCSControlManagerTest::_controlStatusUpdatesState()
{
    GCSControlManager* const controlManager = vehicle()->gcsControlManager();
    QVERIFY(controlManager);
    QCOMPARE(controlManager->firstControlStatusReceived(), false);

    QSignalSpy statusSpy(controlManager, &GCSControlManager::gcsControlStatusChanged);
    QVERIFY(statusSpy.isValid());

    const uint8_t vehicleSysId = static_cast<uint8_t>(mockLink()->vehicleId());
    const uint8_t myId = static_cast<uint8_t>(MAVLinkProtocol::instance()->getSystemId());
    mockLink()->respondWithMavlinkMessage(makeControlStatusMessage(
        vehicleSysId, MAV_COMP_ID_AUTOPILOT1, GCS_CONTROL_STATUS_FLAGS_SYSTEM_MANAGER, myId, {1, 2}));

    QVERIFY_SIGNAL_COUNT_WAIT(statusSpy, 1, TestTimeout::shortMs());

    QCOMPARE(controlManager->firstControlStatusReceived(), true);
    QCOMPARE(controlManager->sysidInControl(), myId);
    QCOMPARE(controlManager->secondaryGCSList(), QList<int>({1, 2}));
    QCOMPARE(controlManager->gcsControlStatusFlags_SystemManager(), true);
    QCOMPARE(controlManager->gcsControlStatusFlags_TakeoverAllowed(), false);
}

void GCSControlManagerTest::_controlStatusIgnoredWithoutSystemManagerFlag()
{
    GCSControlManager* const controlManager = vehicle()->gcsControlManager();
    QSignalSpy statusSpy(controlManager, &GCSControlManager::gcsControlStatusChanged);
    QVERIFY(statusSpy.isValid());

    const uint8_t vehicleSysId = static_cast<uint8_t>(mockLink()->vehicleId());

    // A component-local CONTROL_STATUS (no SYSTEM_MANAGER flag) must be ignored. Proven by
    // sending it first, then a system-level one and observing exactly one state change once
    // both have necessarily been processed (messages are delivered in order over one link).
    mockLink()->respondWithMavlinkMessage(makeControlStatusMessage(vehicleSysId, MAV_COMP_ID_AUTOPILOT1, 0, 99, {}));
    mockLink()->respondWithMavlinkMessage(
        makeControlStatusMessage(vehicleSysId, MAV_COMP_ID_AUTOPILOT1, GCS_CONTROL_STATUS_FLAGS_SYSTEM_MANAGER, 5, {}));

    QVERIFY_TRUE_WAIT(controlManager->sysidInControl() == 5, TestTimeout::shortMs());
    QCOMPARE(statusSpy.count(), 1);
}

void GCSControlManagerTest::_controlStatusTracksTakeoverAllowedFlag()
{
    GCSControlManager* const controlManager = vehicle()->gcsControlManager();
    QSignalSpy statusSpy(controlManager, &GCSControlManager::gcsControlStatusChanged);
    QVERIFY(statusSpy.isValid());

    const uint8_t vehicleSysId = static_cast<uint8_t>(mockLink()->vehicleId());
    mockLink()->respondWithMavlinkMessage(makeControlStatusMessage(
        vehicleSysId, MAV_COMP_ID_AUTOPILOT1,
        GCS_CONTROL_STATUS_FLAGS_SYSTEM_MANAGER | GCS_CONTROL_STATUS_FLAGS_TAKEOVER_ALLOWED, 0, {}));

    QVERIFY_SIGNAL_COUNT_WAIT(statusSpy, 1, TestTimeout::shortMs());
    QCOMPARE(controlManager->gcsControlStatusFlags_TakeoverAllowed(), true);
    QCOMPARE(controlManager->sysidInControl(), static_cast<uint8_t>(0));
}

void GCSControlManagerTest::_controlStatusGatesJoystickSend()
{
    GCSControlManager* const controlManager = vehicle()->gcsControlManager();
    const uint8_t vehicleSysId = static_cast<uint8_t>(mockLink()->vehicleId());
    const uint8_t myId = static_cast<uint8_t>(MAVLinkProtocol::instance()->getSystemId());
    const uint8_t otherGcsId = (myId == 1) ? 2 : 1;

    QSignalSpy statusSpy(controlManager, &GCSControlManager::gcsControlStatusChanged);
    QVERIFY(statusSpy.isValid());

    // Control held by another GCS: joystick output must be suppressed.
    mockLink()->respondWithMavlinkMessage(makeControlStatusMessage(
        vehicleSysId, MAV_COMP_ID_AUTOPILOT1, GCS_CONTROL_STATUS_FLAGS_SYSTEM_MANAGER, otherGcsId, {}));
    QVERIFY_SIGNAL_COUNT_WAIT(statusSpy, 1, TestTimeout::shortMs());

    mockLink()->clearReceivedMavlinkMessageCounts();
    vehicle()->sendJoystickDataThreadSafe(0, 0, 0, 0, 0, 0, qQNaN(), qQNaN(), qQNaN(), qQNaN(), qQNaN(), qQNaN(),
                                          qQNaN(), qQNaN());
    QTest::qWait(TestTimeout::shortMs());
    QCOMPARE(mockLink()->receivedMavlinkMessageCount(MAVLINK_MSG_ID_MANUAL_CONTROL), 0);

    // Control reverts to this GCS: joystick output must resume.
    statusSpy.clear();
    mockLink()->respondWithMavlinkMessage(makeControlStatusMessage(vehicleSysId, MAV_COMP_ID_AUTOPILOT1,
                                                                   GCS_CONTROL_STATUS_FLAGS_SYSTEM_MANAGER, myId, {}));
    QVERIFY_SIGNAL_COUNT_WAIT(statusSpy, 1, TestTimeout::shortMs());

    vehicle()->sendJoystickDataThreadSafe(0, 0, 0, 0, 0, 0, qQNaN(), qQNaN(), qQNaN(), qQNaN(), qQNaN(), qQNaN(),
                                          qQNaN(), qQNaN());
    QVERIFY_TRUE_WAIT(mockLink()->receivedMavlinkMessageCount(MAVLINK_MSG_ID_MANUAL_CONTROL) == 1,
                      TestTimeout::shortMs());
}

void GCSControlManagerTest::_controlStatusSecondaryListChangeEmitsSignal()
{
    GCSControlManager* const controlManager = vehicle()->gcsControlManager();
    const uint8_t vehicleSysId = static_cast<uint8_t>(mockLink()->vehicleId());

    QSignalSpy statusSpy(controlManager, &GCSControlManager::gcsControlStatusChanged);
    QVERIFY(statusSpy.isValid());

    mockLink()->respondWithMavlinkMessage(makeControlStatusMessage(
        vehicleSysId, MAV_COMP_ID_AUTOPILOT1, GCS_CONTROL_STATUS_FLAGS_SYSTEM_MANAGER, 0, {1, 2, 3}));
    QVERIFY_SIGNAL_COUNT_WAIT(statusSpy, 1, TestTimeout::shortMs());
    QCOMPARE(controlManager->secondaryGCSList(), QList<int>({1, 2, 3}));

    // Shrinking the secondary list (e.g. a GCS deregistering) must also be observed.
    mockLink()->respondWithMavlinkMessage(makeControlStatusMessage(vehicleSysId, MAV_COMP_ID_AUTOPILOT1,
                                                                   GCS_CONTROL_STATUS_FLAGS_SYSTEM_MANAGER, 0, {2}));
    QVERIFY_SIGNAL_COUNT_WAIT(statusSpy, 2, TestTimeout::shortMs());
    QCOMPARE(controlManager->secondaryGCSList(), QList<int>({2}));

    // Clearing it entirely must be observed too.
    mockLink()->respondWithMavlinkMessage(
        makeControlStatusMessage(vehicleSysId, MAV_COMP_ID_AUTOPILOT1, GCS_CONTROL_STATUS_FLAGS_SYSTEM_MANAGER, 0, {}));
    QVERIFY_SIGNAL_COUNT_WAIT(statusSpy, 3, TestTimeout::shortMs());
    QCOMPARE(controlManager->secondaryGCSList(), QList<int>());
}

void GCSControlManagerTest::_requestOperatorControlRejectedByVehicle()
{
    GCSControlManager* const controlManager = vehicle()->gcsControlManager();
    QVERIFY(controlManager->sendControlRequestAllowed());

    // MockLink's default result for MAV_CMD_REQUEST_OPERATOR_CONTROL is MAV_RESULT_UNSUPPORTED
    // (protocol not implemented), which the manager surfaces as a generic rejection.
    expectAppMessage(QRegularExpression("Control request rejected by vehicle"));

    mockLink()->clearReceivedMavCommandCounts();
    controlManager->requestOperatorControl(false, 5);

    QVERIFY_TRUE_WAIT(mockLink()->receivedMavCommandCount(MAV_CMD_REQUEST_OPERATOR_CONTROL) == 1,
                      TestTimeout::shortMs());
    QVERIFY_TRUE_WAIT(controlManager->sendControlRequestAllowed(), TestTimeout::shortMs());

    verifyExpectedLogMessage();
}

void GCSControlManagerTest::_requestOperatorControlPendingKeepsCountdown()
{
    GCSControlManager* const controlManager = vehicle()->gcsControlManager();
    QVERIFY(controlManager->sendControlRequestAllowed());

    // MAV_RESULT_FAILED means the owner was notified and the request is genuinely pending
    // vehicle-side: unlike DENIED, the request-lockout countdown must keep running.
    mockLink()->setRequestOperatorControlResult(MAV_RESULT_FAILED);

    QSignalSpy allowedSpy(controlManager, &GCSControlManager::sendControlRequestAllowedChanged);
    QVERIFY(allowedSpy.isValid());

    mockLink()->clearReceivedMavCommandCounts();
    controlManager->requestOperatorControl(false, 5);

    QVERIFY_TRUE_WAIT(mockLink()->receivedMavCommandCount(MAV_CMD_REQUEST_OPERATOR_CONTROL) == 1,
                      TestTimeout::shortMs());
    QTest::qWait(TestTimeout::shortMs());

    // Exactly one transition (the lockout on send); a wrongly-cancelled countdown would have
    // emitted a second, re-enabling transition after the FAILED ack.
    QCOMPARE(allowedSpy.count(), 1);
    QCOMPARE(controlManager->sendControlRequestAllowed(), false);
}

void GCSControlManagerTest::_requestOperatorControlAcceptedKeepsLockoutUntilConfirmed()
{
    GCSControlManager* const controlManager = vehicle()->gcsControlManager();
    const uint8_t vehicleSysId = static_cast<uint8_t>(mockLink()->vehicleId());
    const uint8_t myId = static_cast<uint8_t>(MAVLinkProtocol::instance()->getSystemId());
    QVERIFY(controlManager->sendControlRequestAllowed());

    // MAV_RESULT_ACCEPTED means the vehicle granted the request, but the lockout only lifts
    // once CONTROL_STATUS actually confirms this GCS is in control - the ack alone is not enough.
    mockLink()->setRequestOperatorControlResult(MAV_RESULT_ACCEPTED);

    QSignalSpy allowedSpy(controlManager, &GCSControlManager::sendControlRequestAllowedChanged);
    QVERIFY(allowedSpy.isValid());

    mockLink()->clearReceivedMavCommandCounts();
    controlManager->requestOperatorControl(false, 30);

    QVERIFY_TRUE_WAIT(mockLink()->receivedMavCommandCount(MAV_CMD_REQUEST_OPERATOR_CONTROL) == 1,
                      TestTimeout::shortMs());
    QTest::qWait(TestTimeout::shortMs());

    // Exactly one transition so far (the lockout on send); the ACCEPTED ack must not itself
    // re-enable requests.
    QCOMPARE(allowedSpy.count(), 1);
    QCOMPARE(controlManager->sendControlRequestAllowed(), false);
    QVERIFY(controlManager->requestOperatorControlRemainingMsecs() > 0);

    mockLink()->respondWithMavlinkMessage(makeControlStatusMessage(vehicleSysId, MAV_COMP_ID_AUTOPILOT1,
                                                                   GCS_CONTROL_STATUS_FLAGS_SYSTEM_MANAGER, myId, {}));

    QVERIFY_TRUE_WAIT(controlManager->sendControlRequestAllowed(), TestTimeout::shortMs());
    QCOMPARE(allowedSpy.count(), 2);
    // QTimer::remainingTime() reports -1 once the countdown has been stopped.
    QCOMPARE(controlManager->requestOperatorControlRemainingMsecs(), -1);
}

void GCSControlManagerTest::_requestOperatorControlLockoutClearedByTakeoverAllowedFlag()
{
    GCSControlManager* const controlManager = vehicle()->gcsControlManager();
    const uint8_t vehicleSysId = static_cast<uint8_t>(mockLink()->vehicleId());
    const uint8_t myId = static_cast<uint8_t>(MAVLinkProtocol::instance()->getSystemId());
    const uint8_t otherGcsId = (myId == 1) ? 2 : 1;

    mockLink()->setRequestOperatorControlResult(MAV_RESULT_ACCEPTED);

    QSignalSpy statusSpy(controlManager, &GCSControlManager::gcsControlStatusChanged);
    QVERIFY(statusSpy.isValid());

    controlManager->requestOperatorControl(false, 30);
    QVERIFY_TRUE_WAIT(mockLink()->receivedMavCommandCount(MAV_CMD_REQUEST_OPERATOR_CONTROL) == 1,
                      TestTimeout::shortMs());
    QTest::qWait(TestTimeout::shortMs());
    QCOMPARE(controlManager->sendControlRequestAllowed(), false);

    // Control is still held by another GCS, but TAKEOVER_ALLOWED means a request is no longer
    // pointless: the lockout must clear even though sysidInControl != this GCS.
    mockLink()->respondWithMavlinkMessage(makeControlStatusMessage(
        vehicleSysId, MAV_COMP_ID_AUTOPILOT1,
        GCS_CONTROL_STATUS_FLAGS_SYSTEM_MANAGER | GCS_CONTROL_STATUS_FLAGS_TAKEOVER_ALLOWED, otherGcsId, {}));

    QVERIFY_SIGNAL_COUNT_WAIT(statusSpy, 1, TestTimeout::shortMs());
    QCOMPARE(controlManager->sysidInControl(), otherGcsId);
    QVERIFY_TRUE_WAIT(controlManager->sendControlRequestAllowed(), TestTimeout::shortMs());
}

void GCSControlManagerTest::_requestOperatorControlLockoutClearedWhenUncontrolled()
{
    GCSControlManager* const controlManager = vehicle()->gcsControlManager();
    const uint8_t vehicleSysId = static_cast<uint8_t>(mockLink()->vehicleId());
    const uint8_t myId = static_cast<uint8_t>(MAVLinkProtocol::instance()->getSystemId());
    const uint8_t otherGcsId = (myId == 1) ? 2 : 1;

    mockLink()->setRequestOperatorControlResult(MAV_RESULT_ACCEPTED);

    QSignalSpy statusSpy(controlManager, &GCSControlManager::gcsControlStatusChanged);
    QVERIFY(statusSpy.isValid());

    // Another GCS holds control, takeover not allowed.
    mockLink()->respondWithMavlinkMessage(makeControlStatusMessage(
        vehicleSysId, MAV_COMP_ID_AUTOPILOT1, GCS_CONTROL_STATUS_FLAGS_SYSTEM_MANAGER, otherGcsId, {}));
    QVERIFY_SIGNAL_COUNT_WAIT(statusSpy, 1, TestTimeout::shortMs());

    controlManager->requestOperatorControl(false, 30);
    QVERIFY_TRUE_WAIT(mockLink()->receivedMavCommandCount(MAV_CMD_REQUEST_OPERATOR_CONTROL) == 1,
                      TestTimeout::shortMs());
    QTest::qWait(TestTimeout::shortMs());
    QCOMPARE(controlManager->sendControlRequestAllowed(), false);

    // The controlling GCS releases: CONTROL_STATUS reports uncontrolled with takeover still
    // not allowed. The lockout must clear -- requesting is meaningful again. This is the
    // third re-enable condition (in-control / takeover-allowed / uncontrolled) and was the
    // original stuck-countdown bug: the old check missed the sysid_in_control == 0 case.
    mockLink()->respondWithMavlinkMessage(makeControlStatusMessage(
        vehicleSysId, MAV_COMP_ID_AUTOPILOT1, GCS_CONTROL_STATUS_FLAGS_SYSTEM_MANAGER, 0, {}));
    QVERIFY_SIGNAL_COUNT_WAIT(statusSpy, 2, TestTimeout::shortMs());
    QVERIFY_TRUE_WAIT(controlManager->sendControlRequestAllowed(), TestTimeout::shortMs());
}

void GCSControlManagerTest::_requestOperatorControlLockoutExpiresAfterTimeout()
{
    GCSControlManager* const controlManager = vehicle()->gcsControlManager();

    // Accepted and never confirmed by CONTROL_STATUS: only the countdown timer should re-enable
    // requests. Uses the minimum allowed timeout (FlyViewSettings requestControlTimeout, 3s) to
    // keep this bounded.
    mockLink()->setRequestOperatorControlResult(MAV_RESULT_ACCEPTED);

    controlManager->requestOperatorControl(false, 3);
    QVERIFY(!controlManager->sendControlRequestAllowed());

    QVERIFY_TRUE_WAIT(controlManager->sendControlRequestAllowed(), TestTimeout::mediumMs());
}

void GCSControlManagerTest::_requestOperatorControlDeniedShowsAuthorizationMessageAndClearsLockout()
{
    GCSControlManager* const controlManager = vehicle()->gcsControlManager();
    QVERIFY(controlManager->sendControlRequestAllowed());

    // MAV_RESULT_DENIED means this GCS is not an authorized operator of the vehicle: unlike a
    // pending (FAILED/IN_PROGRESS) result, the lockout must clear immediately and a distinct,
    // authorization-specific message must be shown (not the generic rejection message).
    mockLink()->setRequestOperatorControlResult(MAV_RESULT_DENIED);
    expectAppMessage(QRegularExpression("not an authorized operator of vehicle"));

    mockLink()->clearReceivedMavCommandCounts();
    controlManager->requestOperatorControl(false, 5);

    QVERIFY_TRUE_WAIT(mockLink()->receivedMavCommandCount(MAV_CMD_REQUEST_OPERATOR_CONTROL) == 1,
                      TestTimeout::shortMs());
    QVERIFY_TRUE_WAIT(controlManager->sendControlRequestAllowed(), TestTimeout::shortMs());

    verifyExpectedLogMessage();
}

void GCSControlManagerTest::_requestOperatorControlDuplicateRequestShowsWaitingMessage()
{
    GCSControlManager* const controlManager = vehicle()->gcsControlManager();

    // ACCEPTED so the first request's own (later, real) ack stays silent - this test is only
    // about the second, duplicate call's immediate synchronous response.
    mockLink()->setRequestOperatorControlResult(MAV_RESULT_ACCEPTED);
    expectAppMessage(QRegularExpression("Waiting for previous operator control request"));

    mockLink()->clearReceivedMavCommandCounts();
    // Two back-to-back calls with no intervening event-loop turn: the second is guaranteed to
    // find the first still pending in Vehicle's command queue and be treated as a duplicate,
    // resolved synchronously without a second COMMAND_LONG reaching the wire.
    controlManager->requestOperatorControl(false, 5);
    controlManager->requestOperatorControl(false, 5);

    verifyExpectedLogMessage();
    QVERIFY_TRUE_WAIT(mockLink()->receivedMavCommandCount(MAV_CMD_REQUEST_OPERATOR_CONTROL) == 1,
                      TestTimeout::shortMs());
}

void GCSControlManagerTest::_startTimerRevertAllowTakeoverRequestsOnExpiry()
{
    GCSControlManager* const controlManager = vehicle()->gcsControlManager();
    const uint8_t vehicleSysId = static_cast<uint8_t>(mockLink()->vehicleId());
    const uint8_t myId = static_cast<uint8_t>(MAVLinkProtocol::instance()->getSystemId());

    // This GCS must be the current controller for the revert-to-no-takeover request to fire.
    QSignalSpy statusSpy(controlManager, &GCSControlManager::gcsControlStatusChanged);
    QVERIFY(statusSpy.isValid());
    mockLink()->respondWithMavlinkMessage(makeControlStatusMessage(vehicleSysId, MAV_COMP_ID_AUTOPILOT1,
                                                                   GCS_CONTROL_STATUS_FLAGS_SYSTEM_MANAGER, myId, {}));
    QVERIFY_SIGNAL_COUNT_WAIT(statusSpy, 1, TestTimeout::shortMs());

    // ACCEPTED so the auto-issued revert request's own ack stays silent - this test is only
    // about the timer actually firing and issuing that request.
    mockLink()->setRequestOperatorControlResult(MAV_RESULT_ACCEPTED);
    mockLink()->clearReceivedMavCommandCounts();
    controlManager->startTimerRevertAllowTakeover();

    // operatorControlTakeoverTimeoutMsecs() is a fixed 10s; wait past it for the timer to fire
    // and issue a non-override request that reverts "allow takeover" back off.
    QVERIFY_TRUE_WAIT(mockLink()->receivedMavCommandCount(MAV_CMD_REQUEST_OPERATOR_CONTROL) == 1,
                      TestTimeout::longMs());
}

void GCSControlManagerTest::_releaseOperatorControlTargetsLearnedCompId()
{
    GCSControlManager* const controlManager = vehicle()->gcsControlManager();
    const uint8_t vehicleSysId = static_cast<uint8_t>(mockLink()->vehicleId());
    constexpr uint8_t kSystemManagerCompId = MAV_COMP_ID_ONBOARD_COMPUTER;

    QSignalSpy statusSpy(controlManager, &GCSControlManager::gcsControlStatusChanged);
    QVERIFY(statusSpy.isValid());
    mockLink()->respondWithMavlinkMessage(
        makeControlStatusMessage(vehicleSysId, kSystemManagerCompId, GCS_CONTROL_STATUS_FLAGS_SYSTEM_MANAGER, 0, {}));
    QVERIFY_SIGNAL_COUNT_WAIT(statusSpy, 1, TestTimeout::shortMs());

    mockLink()->clearReceivedMavCommandCounts();
    controlManager->releaseOperatorControl();

    // The release must target the component that last sent CONTROL_STATUS, not the autopilot.
    // (MockLink always ACKs as the autopilot component regardless of target, so the resulting
    // ack is a compId mismatch from Vehicle's perspective and is not asserted on here.)
    QVERIFY_TRUE_WAIT(mockLink()->receivedMavCommandCount(MAV_CMD_REQUEST_OPERATOR_CONTROL, kSystemManagerCompId) == 1,
                      TestTimeout::shortMs());
    QCOMPARE(mockLink()->receivedMavCommandCount(MAV_CMD_REQUEST_OPERATOR_CONTROL, MAV_COMP_ID_AUTOPILOT1), 0);
}

void GCSControlManagerTest::_systemManagerCompIdChangeAdoptsNewManager()
{
    GCSControlManager* const controlManager = vehicle()->gcsControlManager();
    const uint8_t vehicleSysId = static_cast<uint8_t>(mockLink()->vehicleId());

    QSignalSpy statusSpy(controlManager, &GCSControlManager::gcsControlStatusChanged);
    QVERIFY(statusSpy.isValid());

    // First manager claimant: the autopilot.
    mockLink()->respondWithMavlinkMessage(
        makeControlStatusMessage(vehicleSysId, MAV_COMP_ID_AUTOPILOT1, GCS_CONTROL_STATUS_FLAGS_SYSTEM_MANAGER, 0, {}));
    QVERIFY_SIGNAL_COUNT_WAIT(statusSpy, 1, TestTimeout::shortMs());

    // The manager role moves to another component: the switch must be adopted (so requests
    // keep reaching whoever actually manages control) and warned about (it can also mean two
    // components are claiming SYSTEM_MANAGER, which the spec requires integrators to prevent).
    // gcs_main changes alongside so the second processing is observable via the status signal.
    expectLogMessage("Vehicle.GCSControlManager", QtWarningMsg, QRegularExpression("System manager component changed"));
    mockLink()->respondWithMavlinkMessage(makeControlStatusMessage(vehicleSysId, MAV_COMP_ID_ONBOARD_COMPUTER,
                                                                   GCS_CONTROL_STATUS_FLAGS_SYSTEM_MANAGER, 5, {}));
    QVERIFY_SIGNAL_COUNT_WAIT(statusSpy, 2, TestTimeout::shortMs());
    verifyExpectedLogMessage();

    // Subsequent commands must target the new manager, not the autopilot.
    mockLink()->clearReceivedMavCommandCounts();
    controlManager->releaseOperatorControl();
    QVERIFY_TRUE_WAIT(
        mockLink()->receivedMavCommandCount(MAV_CMD_REQUEST_OPERATOR_CONTROL, MAV_COMP_ID_ONBOARD_COMPUTER) == 1,
        TestTimeout::shortMs());
    QCOMPARE(mockLink()->receivedMavCommandCount(MAV_CMD_REQUEST_OPERATOR_CONTROL, MAV_COMP_ID_AUTOPILOT1), 0);
}

void GCSControlManagerTest::_forwardedRequestOperatorControlEmitsSignalAndAcks()
{
    GCSControlManager* const controlManager = vehicle()->gcsControlManager();
    const uint8_t vehicleSysId = static_cast<uint8_t>(mockLink()->vehicleId());

    QSignalSpy requestSpy(controlManager, &GCSControlManager::requestOperatorControlReceived);
    QVERIFY(requestSpy.isValid());

    mockLink()->clearReceivedMavlinkMessageCounts();
    mockLink()->respondWithMavlinkMessage(makeForwardedRequestOperatorControlMessage(
        vehicleSysId, MAV_COMP_ID_AUTOPILOT1, 1 /* request */, 1 /* allow takeover */, 30 /* timeout secs */,
        7 /* requesting sysid */));

    QVERIFY_SIGNAL_COUNT_WAIT(requestSpy, 1, TestTimeout::shortMs());
    const QList<QVariant> args = requestSpy.takeFirst();
    QCOMPARE(args.at(0).toInt(), 7);
    QCOMPARE(args.at(1).toInt(), 1);
    QCOMPARE(args.at(2).toInt(), 30);

    // The takeover notification must always be ACKed (command receipt, per spec; the autopilot
    // clears its own pending state on send and does not consume the ack).
    QVERIFY_TRUE_WAIT(mockLink()->receivedMavlinkMessageCount(MAVLINK_MSG_ID_COMMAND_ACK) == 1, TestTimeout::shortMs());
}

void GCSControlManagerTest::_forwardedRequestSuppressedWhileTakeoverWindowActive()
{
    GCSControlManager* const controlManager = vehicle()->gcsControlManager();
    const uint8_t vehicleSysId = static_cast<uint8_t>(mockLink()->vehicleId());
    const uint8_t myId = static_cast<uint8_t>(MAVLinkProtocol::instance()->getSystemId());

    // This GCS holds control and has just approved a takeover (revert timer running).
    QSignalSpy statusSpy(controlManager, &GCSControlManager::gcsControlStatusChanged);
    QVERIFY(statusSpy.isValid());
    mockLink()->respondWithMavlinkMessage(makeControlStatusMessage(vehicleSysId, MAV_COMP_ID_AUTOPILOT1,
                                                                   GCS_CONTROL_STATUS_FLAGS_SYSTEM_MANAGER, myId, {}));
    QVERIFY_SIGNAL_COUNT_WAIT(statusSpy, 1, TestTimeout::shortMs());
    controlManager->startTimerRevertAllowTakeover();

    QSignalSpy requestSpy(controlManager, &GCSControlManager::requestOperatorControlReceived);
    QVERIFY(requestSpy.isValid());

    // A repeated/crossed request arrives while the window is open: the vehicle grants any
    // takeover without asking during this window, so the operator must not be re-prompted
    // for consent they already gave. The notification is still ACKed (command receipt).
    mockLink()->clearReceivedMavlinkMessageCounts();
    mockLink()->respondWithMavlinkMessage(makeForwardedRequestOperatorControlMessage(
        vehicleSysId, MAV_COMP_ID_AUTOPILOT1, 1 /* request */, 0, 30, 7));

    QVERIFY_TRUE_WAIT(mockLink()->receivedMavlinkMessageCount(MAVLINK_MSG_ID_COMMAND_ACK) == 1, TestTimeout::shortMs());
    QCOMPARE(requestSpy.count(), 0);
}

void GCSControlManagerTest::_forwardedReleaseOperatorControlDoesNotEmitSignal()
{
    GCSControlManager* const controlManager = vehicle()->gcsControlManager();
    const uint8_t vehicleSysId = static_cast<uint8_t>(mockLink()->vehicleId());

    QSignalSpy requestSpy(controlManager, &GCSControlManager::requestOperatorControlReceived);
    QVERIFY(requestSpy.isValid());

    mockLink()->clearReceivedMavlinkMessageCounts();
    mockLink()->respondWithMavlinkMessage(
        makeForwardedRequestOperatorControlMessage(vehicleSysId, MAV_COMP_ID_AUTOPILOT1, 0 /* release */, 0, 0, 7));

    // A forwarded release is still ACKed, but must not raise a takeover prompt.
    QVERIFY_TRUE_WAIT(mockLink()->receivedMavlinkMessageCount(MAVLINK_MSG_ID_COMMAND_ACK) == 1, TestTimeout::shortMs());
    QCOMPARE(requestSpy.count(), 0);
}

UT_REGISTER_TEST(GCSControlManagerTest, TestLabel::Integration, TestLabel::Vehicle)
