#include "VehicleEstimatorStatusFactGroupTest.h"

#include "MAVLinkLib.h"
#include "VehicleEstimatorStatusFactGroup.h"

namespace {

mavlink_message_t estimatorStatusMessage(uint16_t flags, float ratio, float accuracy)
{
    mavlink_estimator_status_t estimatorStatus{};
    estimatorStatus.flags = flags;
    estimatorStatus.vel_ratio = ratio;
    estimatorStatus.pos_horiz_ratio = ratio + 0.01f;
    estimatorStatus.pos_vert_ratio = ratio + 0.02f;
    estimatorStatus.mag_ratio = ratio + 0.03f;
    estimatorStatus.hagl_ratio = ratio + 0.04f;
    estimatorStatus.tas_ratio = ratio + 0.05f;
    estimatorStatus.pos_horiz_accuracy = accuracy;
    estimatorStatus.pos_vert_accuracy = accuracy + 1.0f;

    mavlink_message_t message{};
    (void) mavlink_msg_estimator_status_encode(1, MAV_COMP_ID_AUTOPILOT1, &message, &estimatorStatus);
    return message;
}

mavlink_message_t ekfStatusReportMessage(uint16_t flags, float ratio)
{
    mavlink_ekf_status_report_t ekfStatusReport{};
    ekfStatusReport.flags = flags;
    ekfStatusReport.velocity_variance = ratio;
    ekfStatusReport.pos_horiz_variance = ratio + 0.01f;
    ekfStatusReport.pos_vert_variance = ratio + 0.02f;
    ekfStatusReport.compass_variance = ratio + 0.03f;
    ekfStatusReport.terrain_alt_variance = ratio + 0.04f;
    ekfStatusReport.airspeed_variance = ratio + 0.05f;

    mavlink_message_t message{};
    (void) mavlink_msg_ekf_status_report_encode(1, MAV_COMP_ID_AUTOPILOT1, &message, &ekfStatusReport);
    return message;
}

void verifyRatios(VehicleEstimatorStatusFactGroup& factGroup, float ratio)
{
    QCOMPARE(factGroup.velRatio()->rawValue().toFloat(), ratio);
    QCOMPARE(factGroup.horizPosRatio()->rawValue().toFloat(), ratio + 0.01f);
    QCOMPARE(factGroup.vertPosRatio()->rawValue().toFloat(), ratio + 0.02f);
    QCOMPARE(factGroup.magRatio()->rawValue().toFloat(), ratio + 0.03f);
    QCOMPARE(factGroup.haglRatio()->rawValue().toFloat(), ratio + 0.04f);
    QCOMPARE(factGroup.tasRatio()->rawValue().toFloat(), ratio + 0.05f);
}

}  // namespace

void VehicleEstimatorStatusFactGroupTest::_estimatorStatus()
{
    VehicleEstimatorStatusFactGroup factGroup;
    QVERIFY(!factGroup.telemetryAvailable());

    factGroup.handleMessage(
        nullptr, estimatorStatusMessage(ESTIMATOR_ATTITUDE | ESTIMATOR_GPS_GLITCH | ESTIMATOR_ACCEL_ERROR, 0.1f, 1.5f));

    QVERIFY(factGroup.telemetryAvailable());
    QVERIFY(factGroup.goodAttitudeEstimate()->rawValue().toBool());
    QVERIFY(!factGroup.goodHorizVelEstimate()->rawValue().toBool());
    QVERIFY(factGroup.gpsGlitch()->rawValue().toBool());
    QVERIFY(factGroup.accelError()->rawValue().toBool());
    verifyRatios(factGroup, 0.1f);
    QCOMPARE(factGroup.horizPosAccuracy()->rawValue().toFloat(), 1.5f);
    QCOMPARE(factGroup.vertPosAccuracy()->rawValue().toFloat(), 2.5f);
}

void VehicleEstimatorStatusFactGroupTest::_ekfStatusReportFallback()
{
    VehicleEstimatorStatusFactGroup factGroup;

    factGroup.handleMessage(nullptr, ekfStatusReportMessage(EKF_ATTITUDE | EKF_VELOCITY_HORIZ | EKF_POS_HORIZ_ABS |
                                                                EKF_PRED_POS_HORIZ_ABS | EKF_GPS_GLITCHING,
                                                            0.2f));

    QVERIFY(factGroup.telemetryAvailable());
    QVERIFY(factGroup.goodAttitudeEstimate()->rawValue().toBool());
    QVERIFY(factGroup.goodHorizVelEstimate()->rawValue().toBool());
    QVERIFY(!factGroup.goodVertVelEstimate()->rawValue().toBool());
    QVERIFY(!factGroup.goodHorizPosRelEstimate()->rawValue().toBool());
    QVERIFY(factGroup.goodHorizPosAbsEstimate()->rawValue().toBool());
    QVERIFY(factGroup.goodPredHorizPosAbsEstimate()->rawValue().toBool());
    QVERIFY(factGroup.gpsGlitch()->rawValue().toBool());
    QVERIFY(!factGroup.accelError()->rawValue().toBool());
    verifyRatios(factGroup, 0.2f);
    QVERIFY(qIsNaN(factGroup.horizPosAccuracy()->rawValue().toDouble()));
    QVERIFY(qIsNaN(factGroup.vertPosAccuracy()->rawValue().toDouble()));
}

void VehicleEstimatorStatusFactGroupTest::_ekfStatusReportUninitializedIsNotGpsGlitch()
{
    // EKF_UNINITIALIZED has the value of ESTIMATOR_GPS_GLITCH
    VehicleEstimatorStatusFactGroup factGroup;

    factGroup.handleMessage(nullptr, ekfStatusReportMessage(EKF_UNINITIALIZED, 0.0f));

    QVERIFY(!factGroup.gpsGlitch()->rawValue().toBool());
}

void VehicleEstimatorStatusFactGroupTest::_estimatorStatusPreferred()
{
    VehicleEstimatorStatusFactGroup factGroup;

    factGroup.handleMessage(nullptr, estimatorStatusMessage(ESTIMATOR_ATTITUDE, 0.1f, 1.5f));
    factGroup.handleMessage(nullptr, ekfStatusReportMessage(EKF_VELOCITY_HORIZ | EKF_GPS_GLITCHING, 0.9f));

    // the EKF_STATUS_REPORT is ignored while ESTIMATOR_STATUS is being received
    QVERIFY(factGroup.goodAttitudeEstimate()->rawValue().toBool());
    QVERIFY(!factGroup.goodHorizVelEstimate()->rawValue().toBool());
    QVERIFY(!factGroup.gpsGlitch()->rawValue().toBool());
    verifyRatios(factGroup, 0.1f);
    QCOMPARE(factGroup.horizPosAccuracy()->rawValue().toFloat(), 1.5f);
}

UT_REGISTER_TEST(VehicleEstimatorStatusFactGroupTest, TestLabel::Unit, TestLabel::Vehicle)
