/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "VehicleEstimatorStatusFactGroup.h"
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

    if (message.msgid != MAVLINK_MSG_ID_ESTIMATOR_STATUS) {
        return;
    }

    mavlink_estimator_status_t estimatorStatus{};
    mavlink_msg_estimator_status_decode(&message, &estimatorStatus);

    goodAttitudeEstimate()->setRawValue(!!(estimatorStatus.flags & ESTIMATOR_ATTITUDE));
    goodHorizVelEstimate()->setRawValue(!!(estimatorStatus.flags & ESTIMATOR_VELOCITY_HORIZ));
    goodVertVelEstimate()->setRawValue(!!(estimatorStatus.flags & ESTIMATOR_VELOCITY_VERT));
    goodHorizPosRelEstimate()->setRawValue(!!(estimatorStatus.flags & ESTIMATOR_POS_HORIZ_REL));
    goodHorizPosAbsEstimate()->setRawValue(!!(estimatorStatus.flags & ESTIMATOR_POS_HORIZ_ABS));
    goodVertPosAbsEstimate()->setRawValue(!!(estimatorStatus.flags & ESTIMATOR_POS_VERT_ABS));
    goodVertPosAGLEstimate()->setRawValue(!!(estimatorStatus.flags & ESTIMATOR_POS_VERT_AGL));
    goodConstPosModeEstimate()->setRawValue(!!(estimatorStatus.flags & ESTIMATOR_CONST_POS_MODE));
    goodPredHorizPosRelEstimate()->setRawValue(!!(estimatorStatus.flags & ESTIMATOR_PRED_POS_HORIZ_REL));
    goodPredHorizPosAbsEstimate()->setRawValue(!!(estimatorStatus.flags & ESTIMATOR_PRED_POS_HORIZ_ABS));
    gpsGlitch()->setRawValue(!!(estimatorStatus.flags & ESTIMATOR_GPS_GLITCH));
    accelError()->setRawValue(!!(estimatorStatus.flags & ESTIMATOR_ACCEL_ERROR));
    velRatio()->setRawValue(estimatorStatus.vel_ratio);
    horizPosRatio()->setRawValue(estimatorStatus.pos_horiz_ratio);
    vertPosRatio()->setRawValue(estimatorStatus.pos_vert_ratio);
    magRatio()->setRawValue(estimatorStatus.mag_ratio);
    haglRatio()->setRawValue(estimatorStatus.hagl_ratio);
    tasRatio()->setRawValue(estimatorStatus.tas_ratio);
    horizPosAccuracy()->setRawValue(estimatorStatus.pos_horiz_accuracy);
    vertPosAccuracy()->setRawValue(estimatorStatus.pos_vert_accuracy);

    _setTelemetryAvailable(true);
}
