/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "VehicleEstimatorStatusFactGroup.h"
#include "Vehicle.h"

const char* VehicleEstimatorStatusFactGroup::_goodAttitudeEstimateFactName =        "goodAttitudeEsimate";
const char* VehicleEstimatorStatusFactGroup::_goodHorizVelEstimateFactName =        "goodHorizVelEstimate";
const char* VehicleEstimatorStatusFactGroup::_goodVertVelEstimateFactName =         "goodVertVelEstimate";
const char* VehicleEstimatorStatusFactGroup::_goodHorizPosRelEstimateFactName =     "goodHorizPosRelEstimate";
const char* VehicleEstimatorStatusFactGroup::_goodHorizPosAbsEstimateFactName =     "goodHorizPosAbsEstimate";
const char* VehicleEstimatorStatusFactGroup::_goodVertPosAbsEstimateFactName =      "goodVertPosAbsEstimate";
const char* VehicleEstimatorStatusFactGroup::_goodVertPosAGLEstimateFactName =      "goodVertPosAGLEstimate";
const char* VehicleEstimatorStatusFactGroup::_goodConstPosModeEstimateFactName =    "goodConstPosModeEstimate";
const char* VehicleEstimatorStatusFactGroup::_goodPredHorizPosRelEstimateFactName = "goodPredHorizPosRelEstimate";
const char* VehicleEstimatorStatusFactGroup::_goodPredHorizPosAbsEstimateFactName = "goodPredHorizPosAbsEstimate";
const char* VehicleEstimatorStatusFactGroup::_gpsGlitchFactName =                   "gpsGlitch";
const char* VehicleEstimatorStatusFactGroup::_accelErrorFactName =                  "accelError";
const char* VehicleEstimatorStatusFactGroup::_velRatioFactName =                    "velRatio";
const char* VehicleEstimatorStatusFactGroup::_horizPosRatioFactName =               "horizPosRatio";
const char* VehicleEstimatorStatusFactGroup::_vertPosRatioFactName =                "vertPosRatio";
const char* VehicleEstimatorStatusFactGroup::_magRatioFactName =                    "magRatio";
const char* VehicleEstimatorStatusFactGroup::_haglRatioFactName =                   "haglRatio";
const char* VehicleEstimatorStatusFactGroup::_tasRatioFactName =                    "tasRatio";
const char* VehicleEstimatorStatusFactGroup::_horizPosAccuracyFactName =            "horizPosAccuracy";
const char* VehicleEstimatorStatusFactGroup::_vertPosAccuracyFactName =             "vertPosAccuracy";

VehicleEstimatorStatusFactGroup::VehicleEstimatorStatusFactGroup(QObject* parent)
    : FactGroup                         (500, ":/json/Vehicle/EstimatorStatusFactGroup.json", parent)
    , _goodAttitudeEstimateFact         (0, _goodAttitudeEstimateFactName,          FactMetaData::valueTypeBool)
    , _goodHorizVelEstimateFact         (0, _goodHorizVelEstimateFactName,          FactMetaData::valueTypeBool)
    , _goodVertVelEstimateFact          (0, _goodVertVelEstimateFactName,           FactMetaData::valueTypeBool)
    , _goodHorizPosRelEstimateFact      (0, _goodHorizPosRelEstimateFactName,       FactMetaData::valueTypeBool)
    , _goodHorizPosAbsEstimateFact      (0, _goodHorizPosAbsEstimateFactName,       FactMetaData::valueTypeBool)
    , _goodVertPosAbsEstimateFact       (0, _goodVertPosAbsEstimateFactName,        FactMetaData::valueTypeBool)
    , _goodVertPosAGLEstimateFact       (0, _goodVertPosAGLEstimateFactName,        FactMetaData::valueTypeBool)
    , _goodConstPosModeEstimateFact     (0, _goodConstPosModeEstimateFactName,      FactMetaData::valueTypeBool)
    , _goodPredHorizPosRelEstimateFact  (0, _goodPredHorizPosRelEstimateFactName,   FactMetaData::valueTypeBool)
    , _goodPredHorizPosAbsEstimateFact  (0, _goodPredHorizPosAbsEstimateFactName,   FactMetaData::valueTypeBool)
    , _gpsGlitchFact                    (0, _gpsGlitchFactName,                     FactMetaData::valueTypeBool)
    , _accelErrorFact                   (0, _accelErrorFactName,                    FactMetaData::valueTypeBool)
    , _velRatioFact                     (0, _velRatioFactName,                      FactMetaData::valueTypeFloat)
    , _horizPosRatioFact                (0, _horizPosRatioFactName,                 FactMetaData::valueTypeFloat)
    , _vertPosRatioFact                 (0, _vertPosRatioFactName,                  FactMetaData::valueTypeFloat)
    , _magRatioFact                     (0, _magRatioFactName,                      FactMetaData::valueTypeFloat)
    , _haglRatioFact                    (0, _haglRatioFactName,                     FactMetaData::valueTypeFloat)
    , _tasRatioFact                     (0, _tasRatioFactName,                      FactMetaData::valueTypeFloat)
    , _horizPosAccuracyFact             (0, _horizPosAccuracyFactName,              FactMetaData::valueTypeFloat)
    , _vertPosAccuracyFact              (0, _vertPosAccuracyFactName,               FactMetaData::valueTypeFloat)
{
    _addFact(&_goodAttitudeEstimateFact,        _goodAttitudeEstimateFactName);
    _addFact(&_goodHorizVelEstimateFact,        _goodHorizVelEstimateFactName);
    _addFact(&_goodVertVelEstimateFact,         _goodVertVelEstimateFactName);
    _addFact(&_goodHorizPosRelEstimateFact,     _goodHorizPosRelEstimateFactName);
    _addFact(&_goodHorizPosAbsEstimateFact,     _goodHorizPosAbsEstimateFactName);
    _addFact(&_goodVertPosAbsEstimateFact,      _goodVertPosAbsEstimateFactName);
    _addFact(&_goodVertPosAGLEstimateFact,      _goodVertPosAGLEstimateFactName);
    _addFact(&_goodConstPosModeEstimateFact,    _goodConstPosModeEstimateFactName);
    _addFact(&_goodPredHorizPosRelEstimateFact, _goodPredHorizPosRelEstimateFactName);
    _addFact(&_goodPredHorizPosAbsEstimateFact, _goodPredHorizPosAbsEstimateFactName);
    _addFact(&_gpsGlitchFact,                   _gpsGlitchFactName);
    _addFact(&_accelErrorFact,                  _accelErrorFactName);
    _addFact(&_velRatioFact,                    _velRatioFactName);
    _addFact(&_horizPosRatioFact,               _horizPosRatioFactName);
    _addFact(&_vertPosRatioFact,                _vertPosRatioFactName);
    _addFact(&_magRatioFact,                    _magRatioFactName);
    _addFact(&_haglRatioFact,                   _haglRatioFactName);
    _addFact(&_tasRatioFact,                    _tasRatioFactName);
    _addFact(&_horizPosAccuracyFact,            _horizPosAccuracyFactName);
    _addFact(&_vertPosAccuracyFact,             _vertPosAccuracyFactName);
}

void VehicleEstimatorStatusFactGroup::handleMessage(Vehicle* /* vehicle */, mavlink_message_t& message)
{
    if (message.msgid != MAVLINK_MSG_ID_ESTIMATOR_STATUS) {
        return;
    }

    mavlink_estimator_status_t estimatorStatus;
    mavlink_msg_estimator_status_decode(&message, &estimatorStatus);

    goodAttitudeEstimate()->setRawValue         (!!(estimatorStatus.flags & ESTIMATOR_ATTITUDE));
    goodHorizVelEstimate()->setRawValue         (!!(estimatorStatus.flags & ESTIMATOR_VELOCITY_HORIZ));
    goodVertVelEstimate()->setRawValue          (!!(estimatorStatus.flags & ESTIMATOR_VELOCITY_VERT));
    goodHorizPosRelEstimate()->setRawValue      (!!(estimatorStatus.flags & ESTIMATOR_POS_HORIZ_REL));
    goodHorizPosAbsEstimate()->setRawValue      (!!(estimatorStatus.flags & ESTIMATOR_POS_HORIZ_ABS));
    goodVertPosAbsEstimate()->setRawValue       (!!(estimatorStatus.flags & ESTIMATOR_POS_VERT_ABS));
    goodVertPosAGLEstimate()->setRawValue       (!!(estimatorStatus.flags & ESTIMATOR_POS_VERT_AGL));
    goodConstPosModeEstimate()->setRawValue     (!!(estimatorStatus.flags & ESTIMATOR_CONST_POS_MODE));
    goodPredHorizPosRelEstimate()->setRawValue  (!!(estimatorStatus.flags & ESTIMATOR_PRED_POS_HORIZ_REL));
    goodPredHorizPosAbsEstimate()->setRawValue  (!!(estimatorStatus.flags & ESTIMATOR_PRED_POS_HORIZ_ABS));
    gpsGlitch()->setRawValue                    (estimatorStatus.flags & ESTIMATOR_GPS_GLITCH ? true : false);
    accelError()->setRawValue                   (!!(estimatorStatus.flags & ESTIMATOR_ACCEL_ERROR));
    velRatio()->setRawValue                     (estimatorStatus.vel_ratio);
    horizPosRatio()->setRawValue                (estimatorStatus.pos_horiz_ratio);
    vertPosRatio()->setRawValue                 (estimatorStatus.pos_vert_ratio);
    magRatio()->setRawValue                     (estimatorStatus.mag_ratio);
    haglRatio()->setRawValue                    (estimatorStatus.hagl_ratio);
    tasRatio()->setRawValue                     (estimatorStatus.tas_ratio);
    horizPosAccuracy()->setRawValue             (estimatorStatus.pos_horiz_accuracy);
    vertPosAccuracy()->setRawValue              (estimatorStatus.pos_vert_accuracy);

    _setTelemetryAvailable(true);
}
