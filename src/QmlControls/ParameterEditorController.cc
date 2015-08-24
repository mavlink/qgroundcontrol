/*=====================================================================
 
 QGroundControl Open Source Ground Control Station
 
 (c) 2009, 2015 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 
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

/// @file
///     @author Don Gagne <don@thegagnes.com>

#include "ParameterEditorController.h"
#include "UASManager.h"
#include "AutoPilotPluginManager.h"
#include "QGCFileDialog.h"
#include "QGCMessageBox.h"
#include "QGCMapRCToParamDialog.h"
#include "MainWindow.h"

/// @Brief Constructs a new ParameterEditorController Widget. This widget is used within the PX4VehicleConfig set of screens.
ParameterEditorController::ParameterEditorController(void)
{
    const QMap<int, QMap<QString, QStringList> >& groupMap = _autopilot->getGroupMap();
    
    foreach (int componentId, groupMap.keys()) {
		_componentIds += QString("%1").arg(componentId);
	}
}

ParameterEditorController::~ParameterEditorController()
{
    
}

QStringList ParameterEditorController::getGroupsForComponent(int componentId)
{
	const QMap<int, QMap<QString, QStringList> >& groupMap = _autopilot->getGroupMap();

	return groupMap[componentId].keys();
}

QStringList ParameterEditorController::getFactsForGroup(int componentId, QString group)
{
	const QMap<int, QMap<QString, QStringList> >& groupMap = _autopilot->getGroupMap();
	
	return groupMap[componentId][group];
}

void ParameterEditorController::clearRCToParam(void)
{
	Q_ASSERT(_uas);
	_uas->unsetRCToParameterMap();
}

void ParameterEditorController::saveToFile(void)
{
	Q_ASSERT(_autopilot);
	
    QString msgTitle("Save Parameters");
    
	QString fileName = QGCFileDialog::getSaveFileName(NULL,
                                                      msgTitle,
                                                      qgcApp()->savedParameterFilesLocation(),
                                                      "Parameter Files (*.params)",
                                                      "params",
                                                      true);
	if (!fileName.isEmpty()) {
		QFile file(fileName);
        
		if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QGCMessageBox::critical(msgTitle, "Unable to create file");
			return;
		}
        
		QTextStream stream(&file);
		_autopilot->writeParametersToStream(stream);
		file.close();
	}
}

void ParameterEditorController::loadFromFile(void)
{
    QString errors;
    
    Q_ASSERT(_autopilot);
    
    QString msgTitle("Load Parameters");
    
	QString fileName = QGCFileDialog::getOpenFileName(NULL,
                                                      msgTitle,
                                                      qgcApp()->savedParameterFilesLocation(),
													  "Parameter Files (*.params);;All Files (*)");
    if (!fileName.isEmpty()) {
        QFile file(fileName);
        
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QGCMessageBox::critical(msgTitle, "Unable to open file");
            return;
        }
        
        QTextStream stream(&file);
        errors = _autopilot->readParametersFromStream(stream);
        file.close();
        
        if (!errors.isEmpty()) {
            emit showErrorMessage(errors);
        }
    }
}

void ParameterEditorController::refresh(void)
{
	_autopilot->refreshAllParameters();
}

void ParameterEditorController::resetAllToDefaults(void)
{
    _autopilot->resetAllParametersToDefaults();
    refresh();
}

void ParameterEditorController::setRCToParam(const QString& paramName)
{
	Q_ASSERT(_uas);
	QGCMapRCToParamDialog * d = new QGCMapRCToParamDialog(paramName, _uas, MainWindow::instance());
	d->exec();
}
