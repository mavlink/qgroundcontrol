/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


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
