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

#include "UASInterface.h"
#include "RCValueWidget.h"
#include "QGCLoggingCategory.h"
#include "AutoPilotPlugin.h"

#include "ui_PX4RCCalibration.h"

Q_DECLARE_LOGGING_CATEGORY(PX4RCCalibrationLog)

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
    ~ PX4RCCalibration();
    
signals:
    // @brief Signalled when in unit test mode and a message box should be displayed by the next button
    void nextButtonMessageBoxDisplayed(void);

private slots:
    void _nextButton(void);
    void _skipButton(void);
    void _spektrumBind(void);
    
    void _mode1Toggled(bool checked);
    void _mode2Toggled(bool checked);
    
    void _trimNYI(void);
    
    void _updateView(void);
    
    void _remoteControlChannelRawChanged(int chan, float val);
    
private:
    /// @brief These identify the various controls functions. They are also used as indices into the _rgFunctioInfo
    /// array.
    enum rcCalFunctions {
        rcCalFunctionRoll,
        rcCalFunctionPitch,
        rcCalFunctionYaw,
        rcCalFunctionThrottle,
        rcCalFunctionModeSwitch,
        rcCalFunctionPosCtlSwitch,
        rcCalFunctionLoiterSwitch,
        rcCalFunctionReturnSwitch,
        rcCalFunctionAcroSwitch,
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
    
    typedef void (PX4RCCalibration::*inputFn)(enum rcCalFunctions function, int chan, int value);
    typedef void (PX4RCCalibration::*buttonFn)(void);
    struct stateMachineEntry {
        enum rcCalFunctions function;
        const char*         instructions;
        const char*         image;
        inputFn             rcInputFn;
        buttonFn            nextFn;
        buttonFn            skipFn;
    };
    
    /// @brief A set of information associated with a function.
    struct FunctionInfo {
        const char* parameterName;  ///< Parameter name for function mapping
    };
    
    /// @brief A set of information associated with a radio channel.
    struct ChannelInfo {
        enum rcCalFunctions function;   ///< Function mapped to this channel, rcCalFunctionMax for none
        bool                reversed;   ///< true: channel is reverse, false: not reversed
        int                 rcMin;      ///< Minimum RC value
        int                 rcMax;      ///< Maximum RC value
        int                 rcTrim;     ///< Trim position
    };
    
    /// @brief Information to relate a function to it's value widget.
    struct AttitudeInfo {
        enum rcCalFunctions function;
        RCValueWidget*      valueWidget;
    };
    
    // Methods - see source code for documentation
    
    int _currentStep;  ///< Current step of state machine
    
    const struct stateMachineEntry* _getStateMachineEntry(int step);
    
    void _nextStep(void);
    void _setupCurrentState(void);
    
    void _inputCenterWaitBegin(enum rcCalFunctions function, int channel, int value);
    void _inputStickDetect(enum rcCalFunctions function, int channel, int value);
    void _inputStickMin(enum rcCalFunctions function, int channel, int value);
    void _inputCenterWait(enum rcCalFunctions function, int channel, int value);
    void _inputSwitchMinMax(enum rcCalFunctions function, int channel, int value);
    void _inputFlapsDown(enum rcCalFunctions function, int channel, int value);
    void _inputFlapsUp(enum rcCalFunctions function, int channel, int value);
    void _inputSwitchDetect(enum rcCalFunctions function, int channel, int value);
    void _inputFlapsDetect(enum rcCalFunctions function, int channel, int value);
    
    void _switchDetect(enum rcCalFunctions function, int channel, int value, bool moveToNextStep);
    
    void _saveFlapsDown(void);
    void _skipFlaps(void);
    void _saveAllTrims(void);
    
    bool _stickSettleComplete(int value);
    
    void _validateCalibration(void);
    void _writeCalibration(void);
    void _resetInternalCalibrationValues(void);
    void _setInternalCalibrationValuesFromParameters(void);
    
    void _startCalibration(void);
    void _stopCalibration(void);
    void _rcCalSave(void);

    void _writeParameters(void);
    
    void _rcCalSaveCurrentValues(void);
    
    void _showMinMaxOnRadioWidgets(bool show);
    void _showTrimOnRadioWidgets(bool show);
    
    void _setHelpImage(const char* imageFile);
    
    void _loadSettings(void);
    void _storeSettings(void);
    
    // @brief Called by unit test code to set the mode to unit testing
    void _setUnitTestMode(void){ _unitTestMode = true; }
    
    // Member variables

    static const char* _imageFileMode1Dir;
    static const char* _imageFileMode2Dir;
    static const char* _imageFilePrefix;
    static const char* _imageCenter;
    static const char* _imageHome;
    static const char* _imageThrottleUp;
    static const char* _imageThrottleDown;
    static const char* _imageYawLeft;
    static const char* _imageYawRight;
    static const char* _imageRollLeft;
    static const char* _imageRollRight;
    static const char* _imagePitchUp;
    static const char* _imagePitchDown;
    static const char* _imageSwitchMinMax;
    
    static const char* _settingsGroup;
    static const char* _settingsKeyTransmitterMode;
    
    int _transmitterMode;   ///< 1: transmitter is mode 1, 2: transmitted is mode 2
    
    static const int _updateInterval;   ///< Interval for ui update timer
    
    static const struct FunctionInfo _rgFunctionInfo[rcCalFunctionMax]; ///< Information associated with each function.
    int _rgFunctionChannelMapping[rcCalFunctionMax];                    ///< Maps from rcCalFunctions to channel index. _chanMax indicates channel not set for this function.

    static const int _attitudeControls = 5;
    struct AttitudeInfo _rgAttitudeControl[_attitudeControls];
    
    int _chanCount;                     ///< Number of actual rc channels available
    static const int _chanMax = 18;     ///< Maximum number of supported rc channels
    static const int _chanMinimum = 5;  ///< Minimum numner of channels required to run PX4
    
    struct ChannelInfo _rgChannelInfo[_chanMax];    ///< Information associated with each rc channel
    
    enum rcCalStates _rcCalState;       ///< Current calibration state
    int _rcCalStateCurrentChannel;      ///< Current channel being worked on in rcCalStateIdentify and rcCalStateDetectInversion
    bool _rcCalStateChannelComplete;    ///< Work associated with current channel is complete
    int _rcCalStateIdentifyOldMapping;  ///< Previous mapping for channel being currently identified
    int _rcCalStateReverseOldMapping;   ///< Previous mapping for channel being currently used to detect inversion
    
    static const int _rcCalPWMCenterPoint;
    static const int _rcCalPWMValidMinValue;
    static const int _rcCalPWMValidMaxValue;
    static const int _rcCalPWMDefaultMinValue;
    static const int _rcCalPWMDefaultMaxValue;
    static const int _rcCalRoughCenterDelta;
    static const int _rcCalMoveDelta;
    static const int _rcCalSettleDelta;
    static const int _rcCalMinDelta;
    
    int _rcValueSave[_chanMax];        ///< Saved values prior to detecting channel movement
    
    int _rcRawValue[_chanMax];         ///< Current set of raw channel values
    
    RCValueWidget* _rgRCValueMonitorWidget[_chanMax];   ///< Array of radio channel value widgets
    QLabel* _rgRCValueMonitorLabel[_chanMax];           ///< Array of radio channel value labels

    UASInterface*                   _uas;
    QSharedPointer<AutoPilotPlugin> _autopilot;
    
    Ui::PX4RCCalibration* _ui;
    
    QTimer _updateTimer;    ///< Timer used to update widgete ui
    
    int     _stickDetectChannel;
    int     _stickDetectInitialValue;
    int     _stickDetectValue;
    bool    _stickDetectSettleStarted;
    QTime   _stickDetectSettleElapsed;
    static const int _stickDetectSettleMSecs;
    
    bool _unitTestMode;
};

#endif // PX4RCCalibration_H
