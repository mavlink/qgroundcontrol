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

#ifndef PARAMETEREDITORCONTROLLER_H
#define PARAMETEREDITORCONTROLLER_H

#include <QObject>
#include <QList>

#include "AutoPilotPlugin.h"
#include "UASInterface.h"
#include "FactPanelController.h"

class ParameterEditorController : public FactPanelController
{
    Q_OBJECT
    
public:
    ParameterEditorController(void);
    ~ParameterEditorController();

    Q_PROPERTY(QStringList componentIds MEMBER _componentIds CONSTANT)
	
	Q_INVOKABLE QStringList getGroupsForComponent(int componentId);
	Q_INVOKABLE QStringList getParametersForGroup(int componentId, QString group);
    Q_INVOKABLE QStringList searchParametersForComponent(int componentId, const QString& searchText, bool searchInName, bool searchInDescriptions);
	
	Q_INVOKABLE void clearRCToParam(void);
    Q_INVOKABLE void saveToFilePicker(void);
    Q_INVOKABLE void loadFromFilePicker(void);
    Q_INVOKABLE void saveToFile(const QString& filename);
    Q_INVOKABLE void loadFromFile(const QString& filename);
    Q_INVOKABLE void refresh(void);
    Q_INVOKABLE void resetAllToDefaults(void);
	Q_INVOKABLE void setRCToParam(const QString& paramName);
	
	QList<QObject*> model(void);
    
signals:
    void showErrorMessage(const QString& errorMsg);
	
private:
	QStringList			_componentIds;
};

#endif
