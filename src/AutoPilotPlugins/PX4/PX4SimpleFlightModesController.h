/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#pragma once

#include <QtCore/QVariantList>

#include "FactPanelController.h"
#include "QGCMAVLink.h"

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
    void _rcChannelsChanged(int channelCount, int pwmValues[QGCMAVLink::maxRcChannels]);
    
private:
    int             _activeFlightMode;
    int             _channelCount;
    QVariantList    _rcChannelValues;
};
