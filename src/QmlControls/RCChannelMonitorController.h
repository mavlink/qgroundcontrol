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

#ifndef RCChannelMonitorController_H
#define RCChannelMonitorController_H

#include <QTimer>

#include "FactPanelController.h"
#include "UASInterface.h"
#include "QGCLoggingCategory.h"
#include "AutoPilotPlugin.h"

class RCChannelMonitorController : public FactPanelController
{
    Q_OBJECT
    
public:
    RCChannelMonitorController(void);
    
    Q_PROPERTY(int channelCount READ channelCount NOTIFY channelCountChanged)
        
    int channelCount(void) { return _chanCount; }
    
signals:
    void channelCountChanged(int channelCount);
    void channelRCValueChanged(int channel, int rcValue);

private slots:
    void _rcChannelsChanged(int channelCount, int pwmValues[Vehicle::cMaxRcChannels]);

private:
    int _chanMax(void) const;

    int _chanCount;

    static const int _chanMaxPX4 = 18;  ///< Maximum number of supported rc channels, PX4 Firmware
    static const int _chanMaxAPM = 14;  ///< Maximum number of supported rc channels, APM firmware
};

#endif // RCChannelMonitorController_H
