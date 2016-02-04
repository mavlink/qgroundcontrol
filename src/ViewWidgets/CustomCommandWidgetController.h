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

#ifndef CustomCommandWidgetController_H
#define CustomCommandWidgetController_H

#include <QObject>

#include "UASInterface.h"
#include "AutoPilotPlugin.h"
#include "FactPanelController.h"

class CustomCommandWidgetController : public FactPanelController
{
	Q_OBJECT
	
public:
	CustomCommandWidgetController(void);
    
    Q_PROPERTY(QString customQmlFile MEMBER _customQmlFile NOTIFY customQmlFileChanged)
	
    Q_INVOKABLE void sendCommand    (int commandId, QVariant componentId, QVariant confirm, QVariant param1, QVariant param2, QVariant param3, QVariant param4, QVariant param5, QVariant param6, QVariant param7);
    Q_INVOKABLE void selectQmlFile  (void);
    Q_INVOKABLE void clearQmlFile   (void);
    
signals:
    void customQmlFileChanged   (const QString& customQmlFile);

private slots:
    void _activeVehicleChanged  (Vehicle* activeVehicle);

private:
	UASInterface*       _uas;
    QString             _customQmlFile;
    static const char*  _settingsKey;
};

#endif
