/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "PX4GeoFenceManager.h"
#include "Vehicle.h"
#include "FirmwarePlugin.h"
#include "ParameterManager.h"

PX4GeoFenceManager::PX4GeoFenceManager(Vehicle* vehicle)
    : GeoFenceManager(vehicle)
    , _firstParamLoadComplete(false)
    , _circleRadiusFact(NULL)
{
    connect(_vehicle->getParameterManager(), &ParameterManager::parametersReady,  this, &PX4GeoFenceManager::_parametersReady);

    if (_vehicle->getParameterManager()->parametersAreReady()) {
        _parametersReady();
    }
}

PX4GeoFenceManager::~PX4GeoFenceManager()
{

}

void PX4GeoFenceManager::_parametersReady(void)
{
    if (!_firstParamLoadComplete) {
        _firstParamLoadComplete = true;

        _circleRadiusFact = _vehicle->getParameterFact(FactSystem::defaultComponentId, QStringLiteral("GF_MAX_HOR_DIST"));
        connect(_circleRadiusFact, &Fact::rawValueChanged, this, &PX4GeoFenceManager::_circleRadiusRawValueChanged);
        emit circleRadiusChanged(circleRadius());

        QStringList paramNames;
        QStringList paramLabels;

        paramNames << QStringLiteral("GF_ACTION") << QStringLiteral("GF_MAX_HOR_DIST") << QStringLiteral("GF_MAX_VER_DIST");
        paramLabels << QStringLiteral("Action:") << QStringLiteral("Radius:") << QStringLiteral("Max Altitude:");

        _params.clear();
        _paramLabels.clear();
        for (int i=0; i<paramNames.count(); i++) {
            QString paramName = paramNames[i];
            if (_vehicle->parameterExists(FactSystem::defaultComponentId, paramName)) {
                Fact* paramFact = _vehicle->getParameterFact(FactSystem::defaultComponentId, paramName);
                _params << QVariant::fromValue(paramFact);
                _paramLabels << paramLabels[i];
            }
        }
        emit paramsChanged(_params);
        emit paramLabelsChanged(_paramLabels);

        emit circleSupportedChanged(circleSupported());

        qCDebug(GeoFenceManagerLog) << "fenceSupported:circleSupported:polygonSupported:breachReturnSupported" <<
                                       fenceSupported() << circleSupported() << polygonSupported() << breachReturnSupported();
    }
}

float PX4GeoFenceManager::circleRadius(void) const
{
    if (_circleRadiusFact) {
        return _circleRadiusFact->rawValue().toFloat();
    } else {
        return 0.0;
    }
}

void PX4GeoFenceManager::_circleRadiusRawValueChanged(QVariant value)
{
    emit circleRadiusChanged(value.toFloat());
    emit circleSupportedChanged(circleSupported());
}

bool PX4GeoFenceManager::circleSupported(void) const
{
    if (_circleRadiusFact) {
        return _circleRadiusFact->rawValue().toFloat() >= 0.0;
    }

    return false;
}
