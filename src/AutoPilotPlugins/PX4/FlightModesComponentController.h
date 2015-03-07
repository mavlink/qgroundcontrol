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

#ifndef FLIGHTMODESCOMPONENTCONTROLLER_H
#define FLIGHTMODESCOMPONENTCONTROLLER_H

#include <QObject>
#include <QQuickItem>
#include <QList>

#include "UASInterface.h"
#include "AutoPilotPlugin.h"

/// MVC Controller for FlightModesComponent.qml.
class FlightModesComponentController : public QObject
{
    Q_OBJECT
    
public:
    FlightModesComponentController(QObject* parent = NULL);
    ~FlightModesComponentController();
    
    Q_PROPERTY(bool validConfiguration MEMBER _validConfiguration CONSTANT)
    Q_PROPERTY(QString configurationErrors MEMBER _configurationErrors CONSTANT)
    
    Q_PROPERTY(int channelCount MEMBER _channelCount CONSTANT)
    
    Q_PROPERTY(double modeSwitchLiveRange READ modeSwitchLiveRange NOTIFY switchLiveRangeChanged)
    Q_PROPERTY(double returnSwitchLiveRange READ returnSwitchLiveRange NOTIFY switchLiveRangeChanged)
    Q_PROPERTY(double loiterSwitchLiveRange READ loiterSwitchLiveRange NOTIFY switchLiveRangeChanged)
    Q_PROPERTY(double posCtlSwitchLiveRange READ posCtlSwitchLiveRange NOTIFY switchLiveRangeChanged)
    
    Q_PROPERTY(bool sendLiveRCSwitchRanges READ sendLiveRCSwitchRanges WRITE setSendLiveRCSwitchRanges NOTIFY liveRCSwitchRangesChanged)
    
    double modeSwitchLiveRange(void);
    double returnSwitchLiveRange(void);
    double loiterSwitchLiveRange(void);
    double posCtlSwitchLiveRange(void);
    
    bool sendLiveRCSwitchRanges(void) { return _liveRCValues; }
    void setSendLiveRCSwitchRanges(bool start);
    
signals:
    void switchLiveRangeChanged(void);
    void liveRCSwitchRangesChanged(void);
    
private slots:
    void _remoteControlChannelRawChanged(int chan, float fval);
    
private:
    double _switchLiveRange(const QString& param);
    void _initRcValues(void);
    void _validateConfiguration(void);
    
    static const int _chanMax = 18;
    
    UASInterface*                   _uas;

    QList<double>   _rcValues;
    bool            _liveRCValues;
    int             _rgRCMin[_chanMax];
    int             _rgRCMax[_chanMax];
    bool            _rgRCReversed[_chanMax];
    
    bool    _validConfiguration;
    QString _configurationErrors;
    int     _channelCount;
    
    AutoPilotPlugin*    _autoPilotPlugin;
};

#endif
