/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#ifndef APMFlightModesComponentController_H
#define APMFlightModesComponentController_H

#include <QObject>
#include <QQuickItem>
#include <QList>
#include <QStringList>

#include "UASInterface.h"
#include "AutoPilotPlugin.h"
#include "FactPanelController.h"
#include "Vehicle.h"

/// MVC Controller for FlightModesComponent.qml.
class APMFlightModesComponentController : public FactPanelController
{
    Q_OBJECT
    
public:
    APMFlightModesComponentController(void);
    
    Q_PROPERTY(QString  modeParamPrefix             MEMBER _modeParamPrefix     CONSTANT)
    Q_PROPERTY(QString  modeChannelParam            MEMBER _modeChannelParam    CONSTANT)
    Q_PROPERTY(int      activeFlightMode            READ activeFlightMode       NOTIFY activeFlightModeChanged)
    Q_PROPERTY(int      channelCount                MEMBER _channelCount        CONSTANT)
    Q_PROPERTY(QVariantList channelOptionEnabled    READ channelOptionEnabled   NOTIFY channelOptionEnabledChanged)

    int activeFlightMode(void) const { return _activeFlightMode; }
    QVariantList channelOptionEnabled(void) const { return _rgChannelOptionEnabled; }

signals:
    void activeFlightModeChanged(int activeFlightMode);
    void channelOptionEnabledChanged(void);
    
private slots:
    void _rcChannelsChanged(int channelCount, int pwmValues[Vehicle::cMaxRcChannels]);
    
private:
    QString         _modeParamPrefix;
    QString         _modeChannelParam;
    int             _activeFlightMode;
    int             _channelCount;
    QVariantList    _rgChannelOptionEnabled;
};

#endif
