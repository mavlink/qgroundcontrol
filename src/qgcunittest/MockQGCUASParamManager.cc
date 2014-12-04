/*=====================================================================
 
 QGroundControl Open Source Ground Control Station
 
 (c) 2009 - 2014 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 
 This file is part of the QGROUNDCONTROL project
 
 QGROUNDCONTROL is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 QGROUNDCONTROL is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.
 
 ======================================================================*/

#include "MockQGCUASParamManager.h"
#include <QTest>
#include <QDebug>

Q_LOGGING_CATEGORY(MockQGCUASParamManagerLog, "MockQGCUASParamManagerLog")

MockQGCUASParamManager::MockQGCUASParamManager(void)
{
    
}

bool MockQGCUASParamManager::getParameterValue(int component, const QString& parameter, QVariant& value) const
{
    Q_UNUSED(component);
    
    if (_mapParams.contains(parameter)) {
        value = _mapParams[parameter];
        return true;
    }
    
    qCDebug(MockQGCUASParamManagerLog) << QString("getParameterValue: parameter not found %1").arg(parameter);
    return false;
}

void MockQGCUASParamManager::setParameter(int component, QString parameterName, QVariant value)
{
    qCDebug(MockQGCUASParamManagerLog) << QString("setParameter: component(%1) parameter(%2) value(%3)").arg(component).arg(parameterName).arg(value.toString());
    
    _mapParams[parameterName] = value;
    emit parameterUpdated(_defaultComponentId, parameterName, value);
}
