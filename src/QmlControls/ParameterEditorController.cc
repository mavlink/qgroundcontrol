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
#include "AutoPilotPluginManager.h"
#include "QGCApplication.h"

#ifndef __mobile__
#include "QGCFileDialog.h"
#include "QGCMapRCToParamDialog.h"
#include "MainWindow.h"
#endif

#include <QStandardPaths>

/// @Brief Constructs a new ParameterEditorController Widget. This widget is used within the PX4VehicleConfig set of screens.
ParameterEditorController::ParameterEditorController(void)
{
    if (_autopilot) {
        const QMap<int, QMap<QString, QStringList> >& groupMap = _autopilot->getGroupMap();

        foreach (int componentId, groupMap.keys()) {
            _componentIds += QString("%1").arg(componentId);
        }
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

QStringList ParameterEditorController::getParametersForGroup(int componentId, QString group)
{
	const QMap<int, QMap<QString, QStringList> >& groupMap = _autopilot->getGroupMap();
	
	return groupMap[componentId][group];
}

QStringList ParameterEditorController::searchParametersForComponent(int componentId, const QString& searchText, bool searchInName, bool searchInDescriptions)
{
    QStringList list;
    
    foreach(const QString &paramName, _autopilot->parameterNames(componentId)) {
        if (searchText.isEmpty()) {
            list += paramName;
        } else {
            Fact* fact = _autopilot->getParameterFact(componentId, paramName);
            
            if (searchInName && fact->name().contains(searchText, Qt::CaseInsensitive)) {
                list += paramName;
            } else if (searchInDescriptions && (fact->shortDescription().contains(searchText, Qt::CaseInsensitive) || fact->longDescription().contains(searchText, Qt::CaseInsensitive))) {
                list += paramName;
            }
        }
    }
    list.sort();
    
    return list;
}

void ParameterEditorController::clearRCToParam(void)
{
	Q_ASSERT(_uas);
	_uas->unsetRCToParameterMap();
}

void ParameterEditorController::saveToFile(const QString& filename)
{
    if (!_autopilot) {
        qWarning() << "Internal error _autopilot==NULL";
        return;
    }

    if (!filename.isEmpty()) {
        QFile file(filename);
        
		if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            qgcApp()->showMessage(QString("Unable to create file: %1").arg(filename));
			return;
		}
        
		QTextStream stream(&file);
		_autopilot->writeParametersToStream(stream);
		file.close();
	}
}

void ParameterEditorController::saveToFilePicker(void)
{
#ifndef __mobile__
    QString fileName = QGCFileDialog::getSaveFileName(NULL,
                                                      "Save Parameters",
                                                      QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
                                                      "Parameter Files (*.params)",
                                                      "params",
                                                      true);
    saveToFile(fileName);
#endif
}

void ParameterEditorController::loadFromFile(const QString& filename)
{
    QString errors;
    
    if (!_autopilot) {
        qWarning() << "Internal error _autopilot==NULL";
        return;
    }

    if (!filename.isEmpty()) {
        QFile file(filename);
        
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            qgcApp()->showMessage(QString("Unable to open file: %1").arg(filename));
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

void ParameterEditorController::loadFromFilePicker(void)
{
#ifndef __mobile__
    QString fileName = QGCFileDialog::getOpenFileName(NULL,
                                                      "Load Parameters",
                                                      QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
                                                      "Parameter Files (*.params);;All Files (*)");
    loadFromFile(fileName);
#endif
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
#ifdef __mobile__
    Q_UNUSED(paramName)
#else
	Q_ASSERT(_uas);
    QGCMapRCToParamDialog * d = new QGCMapRCToParamDialog(paramName, _uas, qgcApp()->toolbox()->multiVehicleManager(), MainWindow::instance());
	d->exec();
#endif
}
