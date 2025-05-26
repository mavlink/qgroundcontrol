/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "RCToParamDialogController.h"
#include "ParameterManager.h"
#include "MultiVehicleManager.h"
#include "Vehicle.h"
#include "QGCLoggingCategory.h"
#include "Fact.h"

QGC_LOGGING_CATEGORY(RCToParamDialogControllerLog, "qgc.qmlcontrols.rctoparamdialogcontroller")

QMap<QString, FactMetaData*> RCToParamDialogController::_metaDataMap;

RCToParamDialogController::RCToParamDialogController(QObject *parent)
    : QObject(parent)
    , _scaleFact(new Fact(0, _scaleFactName, FactMetaData::valueTypeDouble, this))
    , _centerFact(new Fact(0, _centerFactName, FactMetaData::valueTypeDouble, this))
    , _minFact(new Fact(0, _minFactName, FactMetaData::valueTypeDouble, this))
    , _maxFact(new Fact(0, _maxFactName, FactMetaData::valueTypeDouble, this))
{
    // qCDebug(RCToParamDialogControllerLog) << Q_FUNC_INFO << this;

    if (_metaDataMap.isEmpty()) {
        _metaDataMap = FactMetaData::createMapFromJsonFile(QStringLiteral(":/json/RCToParamDialog.FactMetaData.json"), nullptr /* QObject parent */);
    }

    _scaleFact->setMetaData(_metaDataMap[_scaleFactName],  true /* setDefaultFromMetaData */);
    _centerFact->setMetaData(_metaDataMap[_centerFactName]);
    _minFact->setMetaData(_metaDataMap[_minFactName]);
    _maxFact->setMetaData(_metaDataMap[_maxFactName]);
}

RCToParamDialogController::~RCToParamDialogController()
{
    // qCDebug(RCToParamDialogControllerLog) << Q_FUNC_INFO << this;`
}

void RCToParamDialogController::setTuningFact(Fact *tuningFact)
{
    _tuningFact = tuningFact;
    emit tuningFactChanged(tuningFact);

    _centerFact->setRawValue(_tuningFact->rawValue().toDouble());
    _minFact->setRawValue(_tuningFact->rawMin().toDouble());
    _maxFact->setRawValue(_tuningFact->rawMax().toDouble());

    (void) connect(_tuningFact, &Fact::vehicleUpdated, this, &RCToParamDialogController::_parameterUpdated);
    MultiVehicleManager::instance()->activeVehicle()->parameterManager()->refreshParameter(ParameterManager::defaultComponentId, _tuningFact->name());
}

void RCToParamDialogController::_parameterUpdated()
{
    _ready = true;
    emit readyChanged(_ready);
}
