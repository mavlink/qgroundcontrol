#include "QGCCameraManagerTest.h"

#include <QtCore/QLoggingCategory>
#include <QtCore/QRegularExpression>
#include <QtCore/QScopeGuard>

#include "CameraMetaData.h"
#include "MAVLinkLib.h"
#include "QGCCameraManager.h"
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
    ignoreLogMessage("Vehicle.MavCommandQueue", QtWarningMsg,
                     QRegularExpression("Giving up sending command after max retries:"));

    // Enable camera manager debug logging: the failure handlers log CameraStruct
    // fields, widening the use-after-free reads for ASan to catch. Scoped so the
    // process-global filter rules are restored even on early test failure.
    const QByteArray oldLoggingRules = qgetenv("QT_LOGGING_RULES");
    QLoggingCategory::setFilterRules(QStringLiteral("Camera.QGCCameraManager.debug=true"));
    const auto restoreLoggingRules = qScopeGuard([oldLoggingRules]() {
        QLoggingCategory::setFilterRules(QString::fromUtf8(oldLoggingRules));
    });

    // Ensure MockLink never responds to the camera info request so it stays pending
    // and eventually times out.
    mockLink()->setRequestMessageNoResponse(MAVLINK_MSG_ID_CAMERA_INFORMATION);

    // Inject a camera component heartbeat. The camera manager creates a CameraStruct
    // and immediately requests CAMERA_INFORMATION with the struct as handler data.
    mavlink_message_t msg{};
    (void) mavlink_msg_heartbeat_pack_chan(vehicle()->id(),
                                           MAV_COMP_ID_CAMERA,
                                           mockLink()->mavlinkChannel(),
                                           &msg,
                                           MAV_TYPE_CAMERA,
                                           MAV_AUTOPILOT_INVALID,
                                           0,   // base_mode
                                           0,   // custom_mode
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

UT_REGISTER_TEST(QGCCameraManagerTest, TestLabel::Integration, TestLabel::Vehicle)
