#include "MockQGCUASParamManager.h"
#include <QTest>
#include <QDebug>

MockQGCUASParamManager::MockQGCUASParamManager(void)
{
    
}

bool MockQGCUASParamManager::getParameterValue(int component, const QString& parameter, QVariant& value) const
{
    Q_UNUSED(component);
    
    if (_mapParams.contains(parameter)) {
        value = _mapParams[parameter];
    }
    return false;
}

void MockQGCUASParamManager::setParameter(int component, QString parameterName, QVariant value)
{
    Q_UNUSED(component);

    _mapParamsSet[parameterName] = value;
}
