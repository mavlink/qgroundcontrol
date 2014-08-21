/*=====================================================================
 
 QGroundControl Open Source Ground Control Station
 
 (c) 2009, 2014 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 
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
///     @brief PX4 RC Calibration Widget
///     @author Don Gagne <don@thegagnes.com

#ifndef PX4RCCalibration_H
#define PX4RCCalibration_H

#include <QWidget>
#include <QTimer>

#include "QGCToolWidget.h"
#include "UASInterface.h"

#include "ui_PX4RCCalibration.h"

class PX4RCCalibrationTest;

namespace Ui {
    class PX4RCCalibration;
}


class PX4RCCalibration : public QWidget
{
    Q_OBJECT
    
    friend class PX4RCCalibrationTest; ///< This allows our unit test to access internal information needed.
    
public:
    explicit PX4RCCalibration(QWidget *parent = 0);

private slots:
    void _rcCalNext(void);
    void _rcCalTryAgain(void);
    void _rcCalSkip(void);
    void _rcCalCancel(void);
    
    void _updateView(void);
    
    void _remoteControlChannelRawChanged(int chan, float val);
    void _setActiveUAS(UASInterface* uas);
    void _toggleSpektrumPairing(bool enabled);
    
private:
    /// @brief These identify the various controls functions. They are also used as indices into the _rgFunctioInfo
    /// aray.
    enum rcCalFunctions {
        rcCalFunctionRoll,
        rcCalFunctionPitch,
        rcCalFunctionYaw,
        rcCalFunctionThrottle,
        rcCalFunctionModeSwitch,
        rcCalFunctionPosCtlSwitch,
        rcCalFunctionLoiterSwitch,
        rcCalFunctionReturnSwitch,
        rcCalFunctionFlaps,
        rcCalFunctionAux1,
        rcCalFunctionAux2,
        rcCalFunctionMax,
        
        // Attitude functions are roll/pitch/yaw/throttle
        rcCalFunctionFirstAttitudeFunction = rcCalFunctionRoll,
        rcCalFunctionLastAttitudeFunction = rcCalFunctionThrottle,
        
        // Non-Attitude functions are everthing else
        rcCalFunctionFirstNonAttitudeFunction = rcCalFunctionModeSwitch,
        rcCalFunctionLastNonAttitudeFunction = rcCalFunctionAux2,
    };
    
    /// @brief The states of the calibration state machine.
    enum rcCalStates {
        rcCalStateChannelWait,
        rcCalStateBegin,
        rcCalStateIdentify,
        rcCalStateMinMax,
        rcCalStateCenterThrottle,
        rcCalStateDetectInversion,
        rcCalStateTrims,
        rcCalStateSave
    };
    
    /// @brief A set of information associated with a function.
    struct FunctionInfo {
        const char* functionName;   ///< User visible function name
        const char* inversionMsg;   ///< Message to display to user to detect inversion
        const char* parameterName;  ///< Parameter name for function mapping
        bool        required;       ///< true: function must be mapped
    };
    
    /// @brief A set of information associated with a radio channel.
    struct ChannelInfo {
        enum rcCalFunctions function;   ///< Function mapped to this channel, rcCalFunctionMax for none
        bool                reversed;   ///< true: channel is reverse, false: not reversed
        float               rcMin;      ///< Minimum RC value
        float               rcMax;      ///< Maximum RC value
        float               rcTrim;     ///< Trim position
    };
    
    // Methods - see source code for documentation
    
    void _writeCalibration(bool trimsOnly);
    void _resetInternalCalibrationValues(void);
    void _copyAndSetTrims(void);
    
    void _rcCalChannelWait(bool firstTime);
    void _rcCalBegin(void);
    void _rcCalNextIdentifyChannelMapping(void);
    void _rcCalReadChannelsMinMax(void);
    void _rcCalCenterThrottle(void);
    void _rcCalNextDetectChannelInversion(void);
    void _rcCalTrims(void);
    void _rcCalSave(void);

    void _rcCalSaveCurrentValues(void);
    
    void _showMinMaxOnRadioWidgets(bool show);
    
    void _unitTestForceCalState(enum rcCalStates state);
    
    // Member variables
    
    static const int _updateInterval;   ///< Interval for ui update timer
    
    static const struct FunctionInfo _rgFunctionInfo[rcCalFunctionMax]; ///< Information associated with each function.
    int _rgFunctionChannelMapping[rcCalFunctionMax];                    ///< Maps from rcCalFunctions to channel index. _chanMax indicates channel not set for this function.
    
    int _chanCount;                     ///< Number of actual rc channels available
    static const int _chanMax = 18;     ///< Maximum number of supported rc channels
    static const int _chanMinimum = 5;  ///< Minimum numner of channels required to run PX4
    
    struct ChannelInfo _rgChannelInfo[_chanMax];    ///< Information associated with each rc channel
    
    enum rcCalStates _rcCalState;       ///< Current calibration state
    int _rcCalStateCurrentChannel;      ///< Current channel being worked on in rcCalStateIdentify and rcCalStateDetectInversion
    bool _rcCalStateChannelComplete;    ///< Work associated with current channel is complete
    int _rcCalStateIdentifyOldMapping;  ///< Previous mapping for channel being currently identified
    int _rcCalStateReverseOldMapping;   ///< Previous mapping for channel being currently used to detect inversion
    
    static const int _rcCalPWMCenterPoint;      ///< PWM center value;
    static const int _rcCalPWMValidMinValue;    ///< Valid minimum PWM value
    static const int _rcCalPWMValidMaxValue;    ///< Valid maximum PWM value
    static const int _rcCalRoughCenterDelta;    ///< Delta around center point which is considered to be roughly centered
    static const float _rcCalMoveDelta;         ///< Amount of delta which is considered stick movement
    static const float _rcCalMinDelta;          ///< Amount of delta allowed around min value to consider channel at min
    
    float _rcValueSave[_chanMax];        ///< Saved values prior to detecting channel movement
    
    float _rcRawValue[_chanMax];         ///< Current set of raw channel values
    
    QGCRadioChannelDisplay* _rgAttitudeRadioWidget[4];  ///< Array of Attitide Function radio channel widgets
    QGCRadioChannelDisplay* _rgRadioWidget[_chanMax];   ///< Array of radio channel widgets

    UASInterface* _mav;                  ///< The current MAV
    
    Ui::PX4RCCalibration* _ui;
    
    QTimer _updateTimer;    ///< Timer used to update widgete ui
};

#endif // PX4RCCalibration_H
