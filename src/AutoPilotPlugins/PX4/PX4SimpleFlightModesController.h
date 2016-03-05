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

#ifndef PX4SimpleFlightModesController_H
#define PX4SimpleFlightModesController_H

#include <QObject>
#include <QQuickItem>
#include <QList>
#include <QStringList>

#include "UASInterface.h"
#include "AutoPilotPlugin.h"
#include "FactPanelController.h"
#include "Vehicle.h"

/// MVC Controller for PX4SimpleFlightModes.qml
class PX4SimpleFlightModesController : public FactPanelController
{
    Q_OBJECT
    
public:
    PX4SimpleFlightModesController(void);
    
    Q_PROPERTY(int          activeFlightMode    READ activeFlightMode       NOTIFY activeFlightModeChanged)
    Q_PROPERTY(int          channelCount        MEMBER _channelCount        CONSTANT)
    Q_PROPERTY(QVariantList rcChannelValues     MEMBER _rcChannelValues     NOTIFY rcChannelValuesChanged)

    int activeFlightMode(void) const { return _activeFlightMode; }

signals:
    void activeFlightModeChanged(int activeFlightMode);
    void channelOptionEnabledChanged(void);
    void rcChannelValuesChanged(void);
    
private slots:
    void _rcChannelsChanged(int channelCount, int pwmValues[Vehicle::cMaxRcChannels]);
    
private:
    int             _activeFlightMode;
    int             _channelCount;
    QVariantList    _rcChannelValues;
};

#endif
