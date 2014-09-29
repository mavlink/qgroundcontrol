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

#ifndef FLIGHTMODECONFIG_H
#define FLIGHTMODECONFIG_H

#include <QWidget>
#include <QComboBox>
#include "ui_FlightModeConfig.h"
#include "AP2ConfigWidget.h"

class FlightModeConfig : public AP2ConfigWidget
{
    Q_OBJECT
    
public:
    explicit FlightModeConfig(QWidget *parent = 0);
    
private slots:
    // Overrides from AP2ConfigWidget
    virtual void activeUASSet(UASInterface *uas);
    virtual void parameterChanged(int uas, int component, QString parameterName, QVariant value);

    // Signalled from UAS
    void remoteControlChannelRawChanged(int chan,float val);

    // Signalled from FlightModeConfig UI
    void saveButtonClicked(void);
    
    
private:
    typedef struct {
        const char* label;
        int         value;
    } FlightModeInfo_t;

    static const FlightModeInfo_t   _rgModeInfoFixedWing[];
    static const FlightModeInfo_t   _rgModeInfoRotor[];
    static const FlightModeInfo_t   _rgModeInfoRover[];
    const FlightModeInfo_t*         _rgModeInfo;
    int                             _cModeInfo;
    
    static const int _cModes = 6;

    static const char*  _rgModeParamFixedWing[_cModes];
    static const char*  _rgModeParamRotor[_cModes];
    static const char*  _rgModeParamRover[_cModes];
    const char**        _rgModeParam;
    
    static const int    _rgModePWMBoundary[_cModes];

    bool                _simpleModeSupported;
    static const char*  _simpleModeBitMaskParam;

    static const char*  _modeSwitchRCChannelParamFixedWing;
    static const char*  _modeSwitchRCChannelParamRover;
    const char*         _modeSwitchRCChannelParam;
    static const int    _modeSwitchRCChannelRotor = 4;  // ArduCopter harcoded to 0-based channel 4
    static const int    _modeSwitchRCChannelInvalid = -1;
    int                 _modeSwitchRCChannel;
    
    QLabel*                 _rgLabel[_cModes];
    QComboBox*              _rgCombo[_cModes];
    QCheckBox*              _rgSimpleModeCheckBox[_cModes];
    QLabel*                 _rgPWMLabel[_cModes];
    Ui::FlightModeConfig    _ui;
};

#endif // FLIGHTMODECONFIG_H
