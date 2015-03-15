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
#include "mavlink.h"
#include "QGCLoggingCategory.h"

#include <QTest>
#include <QDebug>

QGC_LOGGING_CATEGORY(MockQGCUASParamManagerLog, "MockQGCUASParamManagerLog")

MockQGCUASParamManager::MockQGCUASParamManager(void)
{
    _loadParams();
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

void MockQGCUASParamManager::_loadParams(void)
{
    QFile paramFile(":/unittest/MockLink.param");
    
    bool success = paramFile.open(QFile::ReadOnly);
    Q_UNUSED(success);
    Q_ASSERT(success);
    
    QTextStream paramStream(&paramFile);
    
    while (!paramStream.atEnd()) {
        QString line = paramStream.readLine();
        
        if (line.startsWith("#")) {
            continue;
        }
        
        QStringList paramData = line.split("\t");
        Q_ASSERT(paramData.count() == 5);
        
        QString paramName = paramData.at(2);
        QString valStr = paramData.at(3);
        uint paramType = paramData.at(4).toUInt();
        
        QVariant paramValue;
        switch (paramType) {
            case MAV_PARAM_TYPE_REAL32:
                paramValue = QVariant(valStr.toFloat());
                break;
            case MAV_PARAM_TYPE_UINT32:
                paramValue = QVariant(valStr.toUInt());
                break;
            case MAV_PARAM_TYPE_INT32:
                paramValue = QVariant(valStr.toInt());
                break;
            case MAV_PARAM_TYPE_INT8:
                paramValue = QVariant((unsigned char)valStr.toUInt());
                break;
            default:
                Q_ASSERT(false);
                break;
        }
        
        Q_ASSERT(!_mapParams.contains(paramName));
        _mapParams[paramName] = paramValue;
    }
}

QList<int> MockQGCUASParamManager::getComponentForParam(const QString& parameter) const
{
    if (_mapParams.contains(parameter)) {
        QList<int> list;
        list << 50;
        return list;
    } else {
        return QList<int>();
    }
}
