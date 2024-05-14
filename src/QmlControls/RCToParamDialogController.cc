/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "RCToParamDialogController.h"
#include "QGCApplication.h"
#include "QGCToolbox.h"
#include "ParameterManager.h"
#include "MultiVehicleManager.h"
#include "Vehicle.h"
#include "FactSystem.h"

QMap<QString, FactMetaData*> RCToParamDialogController::_metaDataMap;

RCToParamDialogController::RCToParamDialogController(void)
    : _scaleFact    (0, _scaleFactName,     FactMetaData::valueTypeDouble)
    , _centerFact   (0, _centerFactName,    FactMetaData::valueTypeDouble)
    , _minFact      (0, _minFactName,       FactMetaData::valueTypeDouble)
    , _maxFact      (0, _maxFactName,       FactMetaData::valueTypeDouble)
{
    if (_metaDataMap.isEmpty()) {
        _metaDataMap = FactMetaData::createMapFromJsonFile(QStringLiteral(":/json/RCToParamDialog.FactMetaData.json"), nullptr /* QObject parent */);
    }

    _scaleFact.setMetaData  (_metaDataMap[_scaleFactName],  true /* setDefaultFromMetaData */);
    _centerFact.setMetaData (_metaDataMap[_centerFactName]);
    _minFact.setMetaData    (_metaDataMap[_minFactName]);
    _maxFact.setMetaData    (_metaDataMap[_maxFactName]);
}

void RCToParamDialogController::setTuningFact(Fact* tuningFact)
{
    _tuningFact = tuningFact;
    emit tuningFactChanged(tuningFact);

    _centerFact.setRawValue(_tuningFact->rawValue().toDouble());
    _minFact.setRawValue(_tuningFact->rawMin().toDouble());
    _maxFact.setRawValue(_tuningFact->rawMax().toDouble());

    connect(_tuningFact, &Fact::vehicleUpdated, this, &RCToParamDialogController::_parameterUpdated);
    qgcApp()->toolbox()->multiVehicleManager()->activeVehicle()->parameterManager()->refreshParameter(FactSystem::defaultComponentId, _tuningFact->name());
}

void RCToParamDialogController::_parameterUpdated(void)
{
    _ready = true;
    emit readyChanged(true);
}
