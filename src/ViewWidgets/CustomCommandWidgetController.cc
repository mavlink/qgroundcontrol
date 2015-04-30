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

#include "CustomCommandWidgetController.h"
#include "UASManager.h"
#include "QGCMAVLink.h"
#include "QGCFileDialog.h"

#include <QSettings>
#include <QUrl>

const char* CustomCommandWidgetController::_settingsKey = "CustomCommand.QmlFile";

CustomCommandWidgetController::CustomCommandWidgetController(void) :
	_uas(NULL)
{
    _uas = UASManager::instance()->getActiveUAS();
    Q_ASSERT(_uas);
    
    QSettings settings;
    _customQmlFile = settings.value(_settingsKey).toString();
}

void CustomCommandWidgetController::sendCommand(int commandId, QVariant componentId, QVariant confirm, QVariant param1, QVariant param2, QVariant param3, QVariant param4, QVariant param5, QVariant param6, QVariant param7)
{
    Q_UNUSED(commandId);
    Q_UNUSED(componentId);
    Q_UNUSED(confirm);
    Q_UNUSED(param1);
    Q_UNUSED(param2);
    Q_UNUSED(param3);
    Q_UNUSED(param4);
    Q_UNUSED(param5);
    Q_UNUSED(param6);
    Q_UNUSED(param7);
    _uas->executeCommand((MAV_CMD)commandId, confirm.toInt(), param1.toFloat(), param2.toFloat(), param3.toFloat(), param4.toFloat(), param5.toFloat(), param6.toFloat(), param7.toFloat(), componentId.toInt());
}

void CustomCommandWidgetController::selectQmlFile(void)
{
    QSettings settings;
    
    QString qmlFile = QGCFileDialog::getOpenFileName(NULL, "Select custom Qml file", QString(), "Qml files (*.qml)");
    if (qmlFile.isEmpty()) {
        _customQmlFile.clear();
        settings.remove(_settingsKey);
    } else {
		QUrl url = QUrl::fromLocalFile(qmlFile);
		_customQmlFile = url.toString();
        settings.setValue(_settingsKey, _customQmlFile);
    }
    
    emit customQmlFileChanged(_customQmlFile);
}

void CustomCommandWidgetController::clearQmlFile(void)
{
    _customQmlFile.clear();
    QSettings settings;
    settings.remove(_settingsKey);
    emit customQmlFileChanged(_customQmlFile);
}
