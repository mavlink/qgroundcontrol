#include "QGCCameraManagerTest.h"

#include <QtCore/QLoggingCategory>
#include <QtCore/QRegularExpression>
#include <QtCore/QScopeGuard>
#include <QtTest/QSignalSpy>

#include "CameraMetaData.h"
#include "Fixtures/RAIIFixtures.h"
#include "MAVLinkLib.h"
#include "MockLink.h"
#include "QGCCameraManager.h"
#include "SimulatedCameraControl.h"
#include "Vehicle.h"

void QGCCameraManagerTest::_testCameraList()
{
    const QList<CameraMetaData*> cameraList = CameraMetaData::parseCameraMetaData();
    QVERIFY(!cameraList.isEmpty());
    qDeleteAll(cameraList);
}

/// Reproduces issue #13251 (crash 1): use-after-free of QGCCameraManager::CameraStruct.
///
/// The camera info request commands are queued in the vehicle's MavCommandQueue with
/// resultHandlerData pointing at the CameraStruct. _checkForLostCameras() deletes the
/// CameraStruct when the camera goes silent, without cancelling the pending command.
/// When the command later times out, the failure handler dereferences the freed struct.
///
/// In production this is reached when a camera reappears after a silent period (which
/// re-requests camera info with infoReceived still true) and then goes silent again for
/// kSilentTimeoutMs while the request is pending. The test shortcuts the two 5 second
/// silent periods by performing the same cleanup _checkForLostCameras() does (take from
/// _cameraInfoRequest + delete) while the request is pending.
///
/// The use-after-free is only reliably detected when running under AddressSanitizer
/// (CI ASan job). Without ASan the test exercises the path but may pass silently.
void QGCCameraManagerTest::_testLostCameraCleanupWithPendingRequest()
{
    TestFixtures::MavCommandAckTimeoutFixture shortAckTimeout;
    ignoreLogMessage("Vehicle.MavCommandQueue", QtWarningMsg,
                     QRegularExpression("Giving up sending command after max retries:"));

    // Enable camera manager debug logging: the failure handlers log CameraStruct
    // fields, widening the use-after-free reads for ASan to catch. Scoped so the
    // process-global filter rules are restored even on early test failure.
    const QByteArray oldLoggingRules = qgetenv("QT_LOGGING_RULES");
    QLoggingCategory::setFilterRules(QStringLiteral("Camera.QGCCameraManager.debug=true"));
    const auto restoreLoggingRules =
        qScopeGuard([oldLoggingRules]() { QLoggingCategory::setFilterRules(QString::fromUtf8(oldLoggingRules)); });

    // Ensure MockLink never responds to the camera info request so it stays pending
    // and eventually times out.
    mockLink()->setRequestMessageNoResponse(MAVLINK_MSG_ID_CAMERA_INFORMATION);

    // Inject a camera component heartbeat. The camera manager creates a CameraStruct
    // and immediately requests CAMERA_INFORMATION with the struct as handler data.
    mavlink_message_t msg{};
    (void) mavlink_msg_heartbeat_pack_chan(vehicle()->id(), MAV_COMP_ID_CAMERA, mockLink()->mavlinkChannel(), &msg,
                                           MAV_TYPE_CAMERA, MAV_AUTOPILOT_INVALID,
                                           0,  // base_mode
                                           0,  // custom_mode
                                           MAV_STATE_ACTIVE);
    mockLink()->respondWithMavlinkMessage(msg);

    QGCCameraManager* cameraManager = vehicle()->cameraManager();
    QVERIFY(cameraManager);

    QTRY_VERIFY_WITH_TIMEOUT(cameraManager->findCameraStruct(MAV_COMP_ID_CAMERA) != nullptr, TestTimeout::longMs());
    QTRY_VERIFY_WITH_TIMEOUT(vehicle()->isMavCommandPending(MAV_COMP_ID_CAMERA, MAV_CMD_REQUEST_MESSAGE),
                             TestTimeout::longMs());

    // Mimic the lost-camera cleanup in _checkForLostCameras() while the camera info
    // request is still pending in the MavCommandQueue.
    QGCCameraManager::CameraStruct* pInfo = cameraManager->_cameraInfoRequest.take(QString::number(MAV_COMP_ID_CAMERA));
    QVERIFY(pInfo);
    delete pInfo;

    // Let the pending request retry and give up. Before the fix the give-up failure
    // handler dereferenced the freed CameraStruct (issue #13251 crash 1) — detected
    // by the CI ASan job. With the fix, the handler resolves the compId via the
    // manager-owned request context, finds the camera gone, and bails out.
    QTRY_VERIFY_WITH_TIMEOUT(!vehicle()->isMavCommandPending(MAV_COMP_ID_CAMERA, MAV_CMD_REQUEST_MESSAGE),
                             TestTimeout::longMs());
}

/// The Fly View's camera controls read the current camera, and a camera that is deleted reads as null to them: they
/// have to be told of the camera that took its place.
void QGCCameraManagerTest::_testLostCurrentCameraIsAnnounced()
{
    // Nothing answers what the vehicle asks of the camera.
    ignoreLogMessage("Vehicle.MavCommandQueue", QtWarningMsg,
                     QRegularExpression("Giving up sending command after max retries:"));

    QGCCameraManager* const cameraManager = vehicle()->cameraManager();
    QVERIFY(cameraManager);
    MavlinkCameraControlInterface* const standIn = cameraManager->_simulatedCameraControl;
    QCOMPARE(cameraManager->currentCameraInstance(), standIn);

    // A camera appears: one heartbeat, and its answer to the request for its information.
    mavlink_message_t heartbeat{};
    (void) mavlink_msg_heartbeat_pack_chan(vehicle()->id(), MAV_COMP_ID_CAMERA, mockLink()->mavlinkChannel(),
                                           &heartbeat, MAV_TYPE_CAMERA, MAV_AUTOPILOT_INVALID, 0, 0, MAV_STATE_ACTIVE);
    mockLink()->respondWithMavlinkMessage(heartbeat);
    QTRY_VERIFY_WITH_TIMEOUT(cameraManager->findCameraStruct(MAV_COMP_ID_CAMERA) != nullptr, TestTimeout::longMs());

    mavlink_camera_information_t information{};
    (void) qstrncpy(reinterpret_cast<char*>(information.vendor_name), "QGC", sizeof(information.vendor_name));
    (void) qstrncpy(reinterpret_cast<char*>(information.model_name), "Test camera", sizeof(information.model_name));
    information.flags = CAMERA_CAP_FLAGS_CAPTURE_IMAGE;
    mavlink_message_t informationMessage{};
    (void) mavlink_msg_camera_information_encode_chan(vehicle()->id(), MAV_COMP_ID_CAMERA, mockLink()->mavlinkChannel(),
                                                      &informationMessage, &information);
    mockLink()->respondWithMavlinkMessage(informationMessage);
    QTRY_VERIFY_WITH_TIMEOUT(cameraManager->currentCameraInstance() != standIn, TestTimeout::longMs());
    QCOMPARE(cameraManager->currentCamera(), 0);

    // It sends no more heartbeats, so it is taken for lost: the stand-in camera is the current one again, at the
    // same index.
    QSignalSpy currentCameraSpy(cameraManager, &QGCCameraManager::currentCameraChanged);
    QTRY_COMPARE_WITH_TIMEOUT(cameraManager->currentCameraInstance(), standIn, TestTimeout::longMs());
    QCOMPARE(cameraManager->currentCamera(), 0);
    QCOMPARE(currentCameraSpy.count(), 1);
}

UT_REGISTER_TEST(QGCCameraManagerTest, TestLabel::Integration, TestLabel::Vehicle)
