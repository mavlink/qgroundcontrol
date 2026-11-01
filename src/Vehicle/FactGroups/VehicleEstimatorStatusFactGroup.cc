#include "VehicleEstimatorStatusFactGroup.h"

#include "MAVLinkLib.h"
#include "Vehicle.h"

VehicleEstimatorStatusFactGroup::VehicleEstimatorStatusFactGroup(QObject *parent)
    : FactGroup(500, QStringLiteral(":/json/Vehicle/EstimatorStatusFactGroup.json"), parent)
{
    _addFact(&_goodAttitudeEstimateFact);
    _addFact(&_goodHorizVelEstimateFact);
    _addFact(&_goodVertVelEstimateFact);
    _addFact(&_goodHorizPosRelEstimateFact);
    _addFact(&_goodHorizPosAbsEstimateFact);
    _addFact(&_goodVertPosAbsEstimateFact);
    _addFact(&_goodVertPosAGLEstimateFact);
    _addFact(&_goodConstPosModeEstimateFact);
    _addFact(&_goodPredHorizPosRelEstimateFact);
    _addFact(&_goodPredHorizPosAbsEstimateFact);
    _addFact(&_gpsGlitchFact);
    _addFact(&_accelErrorFact);
    _addFact(&_velRatioFact);
    _addFact(&_horizPosRatioFact);
    _addFact(&_vertPosRatioFact);
    _addFact(&_magRatioFact);
    _addFact(&_haglRatioFact);
    _addFact(&_tasRatioFact);
    _addFact(&_horizPosAccuracyFact);
    _addFact(&_vertPosAccuracyFact);
}

void VehicleEstimatorStatusFactGroup::handleMessage(Vehicle *vehicle, const mavlink_message_t &message)
{
    Q_UNUSED(vehicle);

    switch (message.msgid) {
        case MAVLINK_MSG_ID_ESTIMATOR_STATUS:
            _handleEstimatorStatus(message);
            break;
        case MAVLINK_MSG_ID_EKF_STATUS_REPORT:
            _handleEkfStatusReport(message);
            break;
        default:
            break;
    }
}

void VehicleEstimatorStatusFactGroup::_handleEstimatorStatus(const mavlink_message_t& message)
{
    mavlink_estimator_status_t estimatorStatus{};
    mavlink_msg_estimator_status_decode(&message, &estimatorStatus);

    _estimatorStatusTimer.start();

    _setFlags(estimatorStatus.flags);
    _setRatios(estimatorStatus.vel_ratio, estimatorStatus.pos_horiz_ratio, estimatorStatus.pos_vert_ratio,
               estimatorStatus.mag_ratio, estimatorStatus.hagl_ratio, estimatorStatus.tas_ratio);
    horizPosAccuracy()->setRawValue(estimatorStatus.pos_horiz_accuracy);
    vertPosAccuracy()->setRawValue(estimatorStatus.pos_vert_accuracy);

    _setTelemetryAvailable(true);
}

void VehicleEstimatorStatusFactGroup::_handleEkfStatusReport(const mavlink_message_t& message)
{
    // ArduPilot's own equivalent of ESTIMATOR_STATUS, which it streams by default; older
    // ArduPilot sends only this. Prefer ESTIMATOR_STATUS when the vehicle sends both.
    if (_estimatorStatusTimer.isValid() && !_estimatorStatusTimer.hasExpired(kEstimatorStatusPreferredMs)) {
        return;
    }

    mavlink_ekf_status_report_t ekfStatusReport{};
    mavlink_msg_ekf_status_report_decode(&message, &ekfStatusReport);

    // Flags up to EKF_PRED_POS_HORIZ_ABS have the same values as the ESTIMATOR_STATUS_FLAGS;
    // the GPS glitch flag has a different value, and EKF_UNINITIALIZED (1024) has no equivalent here
    static constexpr uint16_t kCommonFlags = EKF_ATTITUDE | EKF_VELOCITY_HORIZ | EKF_VELOCITY_VERT | EKF_POS_HORIZ_REL |
                                             EKF_POS_HORIZ_ABS | EKF_POS_VERT_ABS | EKF_POS_VERT_AGL |
                                             EKF_CONST_POS_MODE | EKF_PRED_POS_HORIZ_REL | EKF_PRED_POS_HORIZ_ABS;
    static_assert(kCommonFlags == (ESTIMATOR_ATTITUDE | ESTIMATOR_VELOCITY_HORIZ | ESTIMATOR_VELOCITY_VERT |
                                   ESTIMATOR_POS_HORIZ_REL | ESTIMATOR_POS_HORIZ_ABS | ESTIMATOR_POS_VERT_ABS |
                                   ESTIMATOR_POS_VERT_AGL | ESTIMATOR_CONST_POS_MODE | ESTIMATOR_PRED_POS_HORIZ_REL |
                                   ESTIMATOR_PRED_POS_HORIZ_ABS),
                  "EKF_STATUS_FLAGS and ESTIMATOR_STATUS_FLAGS differ");
    uint16_t flags = ekfStatusReport.flags & kCommonFlags;
    if (ekfStatusReport.flags & EKF_GPS_GLITCHING) {
        flags |= ESTIMATOR_GPS_GLITCH;
    }
    _setFlags(flags);

    // Despite their names, EKF_STATUS_REPORT's *_variance fields are innovation test ratios,
    // on the same scale as ESTIMATOR_STATUS's
    _setRatios(ekfStatusReport.velocity_variance, ekfStatusReport.pos_horiz_variance, ekfStatusReport.pos_vert_variance,
               ekfStatusReport.compass_variance, ekfStatusReport.terrain_alt_variance,
               ekfStatusReport.airspeed_variance);

    // EKF_STATUS_REPORT has no accuracies
    horizPosAccuracy()->setRawValue(qQNaN());
    vertPosAccuracy()->setRawValue(qQNaN());

    _setTelemetryAvailable(true);
}

void VehicleEstimatorStatusFactGroup::_setFlags(uint16_t flags)
{
    goodAttitudeEstimate()->setRawValue(!!(flags & ESTIMATOR_ATTITUDE));
    goodHorizVelEstimate()->setRawValue(!!(flags & ESTIMATOR_VELOCITY_HORIZ));
    goodVertVelEstimate()->setRawValue(!!(flags & ESTIMATOR_VELOCITY_VERT));
    goodHorizPosRelEstimate()->setRawValue(!!(flags & ESTIMATOR_POS_HORIZ_REL));
    goodHorizPosAbsEstimate()->setRawValue(!!(flags & ESTIMATOR_POS_HORIZ_ABS));
    goodVertPosAbsEstimate()->setRawValue(!!(flags & ESTIMATOR_POS_VERT_ABS));
    goodVertPosAGLEstimate()->setRawValue(!!(flags & ESTIMATOR_POS_VERT_AGL));
    goodConstPosModeEstimate()->setRawValue(!!(flags & ESTIMATOR_CONST_POS_MODE));
    goodPredHorizPosRelEstimate()->setRawValue(!!(flags & ESTIMATOR_PRED_POS_HORIZ_REL));
    goodPredHorizPosAbsEstimate()->setRawValue(!!(flags & ESTIMATOR_PRED_POS_HORIZ_ABS));
    gpsGlitch()->setRawValue(!!(flags & ESTIMATOR_GPS_GLITCH));
    accelError()->setRawValue(!!(flags & ESTIMATOR_ACCEL_ERROR));
}

void VehicleEstimatorStatusFactGroup::_setRatios(float vel, float horizPos, float vertPos, float mag, float hagl,
                                                 float tas)
{
    velRatio()->setRawValue(vel);
    horizPosRatio()->setRawValue(horizPos);
    vertPosRatio()->setRawValue(vertPos);
    magRatio()->setRawValue(mag);
    haglRatio()->setRawValue(hagl);
    tasRatio()->setRawValue(tas);
}
