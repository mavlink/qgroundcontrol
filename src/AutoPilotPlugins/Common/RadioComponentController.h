/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/



/// @file
///     @brief Radio Config Qml Controller
///     @author Don Gagne <don@thegagnes.com

#pragma once

#include "FactPanelController.h"
#include "QGCMAVLink.h"

#include <QtCore/QLoggingCategory>
#include <QtCore/QElapsedTimer>
#include <QtQuick/QQuickItem>

Q_DECLARE_LOGGING_CATEGORY(RadioComponentControllerLog)
Q_DECLARE_LOGGING_CATEGORY(RadioComponentControllerVerboseLog)

class RadioConfigest;

namespace Ui {
    class RadioComponentController;
}


class RadioComponentController : public FactPanelController
{
    Q_OBJECT

    //friend class RadioConfigTest; ///< This allows our unit test to access internal information needed.

public:
    RadioComponentController(void);
    ~RadioComponentController();

    Q_PROPERTY(int minChannelCount MEMBER _chanMinimum CONSTANT)
    Q_PROPERTY(int channelCount READ channelCount NOTIFY channelCountChanged)

    Q_PROPERTY(QQuickItem* statusText   MEMBER _statusText      NOTIFY statusTextChanged)
    Q_PROPERTY(QQuickItem* cancelButton MEMBER _cancelButton    NOTIFY cancelButtonChanged)
    Q_PROPERTY(QQuickItem* nextButton   MEMBER _nextButton      NOTIFY nextButtonChanged)
    Q_PROPERTY(QQuickItem* skipButton   MEMBER _skipButton      NOTIFY skipButtonChanged)

    Q_PROPERTY(bool rollChannelMapped READ rollChannelMapped NOTIFY rollChannelMappedChanged)
    Q_PROPERTY(bool pitchChannelMapped READ pitchChannelMapped NOTIFY pitchChannelMappedChanged)
    Q_PROPERTY(bool yawChannelMapped READ yawChannelMapped NOTIFY yawChannelMappedChanged)
    Q_PROPERTY(bool throttleChannelMapped READ throttleChannelMapped NOTIFY throttleChannelMappedChanged)

    Q_PROPERTY(int rollChannelRCValue READ rollChannelRCValue NOTIFY rollChannelRCValueChanged)
    Q_PROPERTY(int pitchChannelRCValue READ pitchChannelRCValue NOTIFY pitchChannelRCValueChanged)
    Q_PROPERTY(int yawChannelRCValue READ yawChannelRCValue NOTIFY yawChannelRCValueChanged)
    Q_PROPERTY(int throttleChannelRCValue READ throttleChannelRCValue NOTIFY throttleChannelRCValueChanged)

    Q_PROPERTY(int rollChannelReversed READ rollChannelReversed NOTIFY rollChannelReversedChanged)
    Q_PROPERTY(int pitchChannelReversed READ pitchChannelReversed NOTIFY pitchChannelReversedChanged)
    Q_PROPERTY(int yawChannelReversed READ yawChannelReversed NOTIFY yawChannelReversedChanged)
    Q_PROPERTY(int throttleChannelReversed READ throttleChannelReversed NOTIFY throttleChannelReversedChanged)

    Q_PROPERTY(int transmitterMode READ transmitterMode WRITE setTransmitterMode NOTIFY transmitterModeChanged)
    Q_PROPERTY(QString imageHelp MEMBER _imageHelp NOTIFY imageHelpChanged)

    enum BindModes {
        DSM2,
        DSMX7,
        DSMX8
    };
    Q_ENUM(BindModes)

    Q_INVOKABLE void spektrumBindMode(int mode);
    Q_INVOKABLE void cancelButtonClicked(void);
    Q_INVOKABLE void skipButtonClicked(void);
    Q_INVOKABLE void nextButtonClicked(void);
    Q_INVOKABLE void start(void);
    Q_INVOKABLE void copyTrims(void);

    int rollChannelRCValue(void);
    int pitchChannelRCValue(void);
    int yawChannelRCValue(void);
    int throttleChannelRCValue(void);

    bool rollChannelMapped(void);
    bool pitchChannelMapped(void);
    bool yawChannelMapped(void);
    bool throttleChannelMapped(void);

    bool rollChannelReversed(void);
    bool pitchChannelReversed(void);
    bool yawChannelReversed(void);
    bool throttleChannelReversed(void);

    int channelCount(void) const;

    int transmitterMode(void) const{ return _transmitterMode; }
    void setTransmitterMode(int mode);

signals:
    void statusTextChanged(void);
    void cancelButtonChanged(void);
    void nextButtonChanged(void);
    void skipButtonChanged(void);

    void channelCountChanged(int channelCount);
    void channelRCValueChanged(int channel, int rcValue);

    void rollChannelMappedChanged(bool mapped);
    void pitchChannelMappedChanged(bool mapped);
    void yawChannelMappedChanged(bool mapped);
    void throttleChannelMappedChanged(bool mapped);

    void rollChannelRCValueChanged(int rcValue);
    void pitchChannelRCValueChanged(int rcValue);
    void yawChannelRCValueChanged(int rcValue);
    void throttleChannelRCValueChanged(int rcValue);

    void rollChannelReversedChanged(bool reversed);
    void pitchChannelReversedChanged(bool reversed);
    void yawChannelReversedChanged(bool reversed);
    void throttleChannelReversedChanged(bool reversed);

    void imageHelpChanged(QString source);
    void transmitterModeChanged(int mode);

    /// Signalled when in unit test mode and a message box should be displayed by the next button
    void nextButtonMessageBoxDisplayed(void);

    /// Signalled to QML to indicate reboot is required
    void functionMappingChangedAPMReboot(void);

    /// Signalled to Qml to indicate cal failure due to reversed throttle
    void throttleReversedCalFailure(void);

private slots:
    void _rcChannelsChanged(int channelCount, int pwmValues[QGCMAVLink::maxRcChannels]);

private:
    /// @brief These identify the various controls functions. They are also used as indices into the _rgFunctioInfo
    /// array.
    enum rcCalFunctions {
        rcCalFunctionRoll,
        rcCalFunctionPitch,
        rcCalFunctionYaw,
        rcCalFunctionThrottle,
        rcCalFunctionMax,
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

    typedef void (RadioComponentController::*inputFn)(enum rcCalFunctions function, int chan, int value);
    typedef void (RadioComponentController::*buttonFn)(void);
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

    int _currentStep;  ///< Current step of state machine

    const struct stateMachineEntry* _getStateMachineEntry(int step) const;
    const struct FunctionInfo* _functionInfo(void) const;
    bool _px4Vehicle(void) const;

    void _advanceState(void);
    void _setupCurrentState(void);

    void _inputCenterWaitBegin(enum rcCalFunctions function, int channel, int value);
    void _inputStickDetect(enum rcCalFunctions function, int channel, int value);
    void _inputStickMin(enum rcCalFunctions function, int channel, int value);
    void _inputCenterWait(enum rcCalFunctions function, int channel, int value);
    void _inputSwitchMinMax(enum rcCalFunctions function, int channel, int value);
    void _inputSwitchDetect(enum rcCalFunctions function, int channel, int value);

    void _switchDetect(enum rcCalFunctions function, int channel, int value, bool moveToNextStep);

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

    void _setHelpImage(const char* imageFile);

    void _loadSettings(void);
    void _storeSettings(void);

    void _signalAllAttitudeValueChanges(void);

    bool _channelReversedParamValue(int channel);
    void _setChannelReversedParamValue(int channel, bool reversed);

    // @brief Called by unit test code to set the mode to unit testing
    void _setUnitTestMode(void){ _unitTestMode = true; }

    // Member variables

    int _transmitterMode;   ///< 1: transmitter is mode 1, 2: transmitted is mode 2

    int _rgFunctionChannelMapping[rcCalFunctionMax];                    ///< Maps from rcCalFunctions to channel index. _chanMax indicates channel not set for this function.

    static const int _attitudeControls = 5;

    int _chanCount;                     ///< Number of actual rc channels available
    static const int _chanMax = 18;     ///< Maximum number of support rc channels by this implementation
    static const int _chanMinimum = 5;  ///< Minimum numner of channels required to run

    struct ChannelInfo _rgChannelInfo[_chanMax];    ///< Information associated with each rc channel

    enum rcCalStates _rcCalState;       ///< Current calibration state
    int _rcCalStateCurrentChannel;      ///< Current channel being worked on in rcCalStateIdentify and rcCalStateDetectInversion
    bool _rcCalStateChannelComplete;    ///< Work associated with current channel is complete
    int _rcCalStateIdentifyOldMapping;  ///< Previous mapping for channel being currently identified
    int _rcCalStateReverseOldMapping;   ///< Previous mapping for channel being currently used to detect inversion

    QString             _revParamFormat;
    bool                _revParamIsBool;

    int _rcValueSave[_chanMax];        ///< Saved values prior to detecting channel movement

    int _rcRawValue[_chanMax];         ///< Current set of raw channel values

    int     _stickDetectChannel;
    int     _stickDetectValue;
    bool    _stickDetectSettleStarted;
    QElapsedTimer   _stickDetectSettleElapsed;

    bool        _unitTestMode   = false;

    QQuickItem* _statusText     = nullptr;
    QQuickItem* _cancelButton   = nullptr;
    QQuickItem* _nextButton     = nullptr;
    QQuickItem* _skipButton     = nullptr;

    QString _imageHelp;

#ifdef UNITTEST_BUILD
    // Nasty hack to expose controller to unit test code
    static RadioComponentController*    _unitTestController;
#endif

    static constexpr int _updateInterval = 150;              ///< Interval for timer which updates radio channel widgets
    static constexpr int _rcCalPWMValidMinValue =    1300;   ///< Largest valid minimum PWM Min range value
    static constexpr int _rcCalPWMValidMaxValue =    1700;   ///< Smallest valid maximum PWM Max range value
    static constexpr int _rcCalPWMCenterPoint = ((_rcCalPWMValidMaxValue - _rcCalPWMValidMinValue) / 2.0f) + _rcCalPWMValidMinValue;
    static constexpr int _rcCalPWMDefaultMinValue =  1000;   ///< Default value for Min if not set
    static constexpr int _rcCalPWMDefaultMaxValue =  2000;   ///< Default value for Max if not set
    static constexpr int _rcCalRoughCenterDelta =    50;     ///< Delta around center point which is considered to be roughly centered
    static constexpr int _rcCalMoveDelta =           300;    ///< Amount of delta past center which is considered stick movement
    static constexpr int _rcCalSettleDelta =         20;     ///< Amount of delta which is considered no stick movement
    static constexpr int _rcCalMinDelta =            100;    ///< Amount of delta allowed around min value to consider channel at min

    static constexpr int _stickDetectSettleMSecs = 500;

    static constexpr const char*  _imageFilePrefix =   "calibration/";
    static constexpr const char*  _imageFileMode1Dir = "mode1/";
    static constexpr const char*  _imageFileMode2Dir = "mode2/";
    static constexpr const char*  _imageCenter =       "radioCenter.png";
    static constexpr const char*  _imageHome =         "radioHome.png";
    static constexpr const char*  _imageThrottleUp =   "radioThrottleUp.png";
    static constexpr const char*  _imageThrottleDown = "radioThrottleDown.png";
    static constexpr const char*  _imageYawLeft =      "radioYawLeft.png";
    static constexpr const char*  _imageYawRight =     "radioYawRight.png";
    static constexpr const char*  _imageRollLeft =     "radioRollLeft.png";
    static constexpr const char*  _imageRollRight =    "radioRollRight.png";
    static constexpr const char*  _imagePitchUp =      "radioPitchUp.png";
    static constexpr const char*  _imagePitchDown =    "radioPitchDown.png";
    static constexpr const char*  _imageSwitchMinMax = "radioSwitchMinMax.png";

    static constexpr const char* _settingsGroup =              "RadioCalibration";
    static constexpr const char* _settingsKeyTransmitterMode = "TransmitterMode";

    static constexpr const char* _px4RevParamFormat =      "RC%1_REV";
    static constexpr const char* _apmNewRevParamFormat =   "RC%1_REVERSED";

    static constexpr const struct FunctionInfo _rgFunctionInfoPX4[rcCalFunctionMax] = {
        { "RC_MAP_ROLL" },
        { "RC_MAP_PITCH" },
        { "RC_MAP_YAW" },
        { "RC_MAP_THROTTLE" }
    };

    static constexpr const struct FunctionInfo _rgFunctionInfoAPM[rcCalFunctionMax] = {
        { "RCMAP_ROLL" },
        { "RCMAP_PITCH" },
        { "RCMAP_YAW" },
        { "RCMAP_THROTTLE" }
    };
};
