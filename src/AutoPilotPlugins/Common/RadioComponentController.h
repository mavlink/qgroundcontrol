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

#ifndef RadioComponentController_H
#define RadioComponentController_H

#include <QTimer>

#include "FactPanelController.h"
#include "UASInterface.h"
#include "QGCLoggingCategory.h"
#include "AutoPilotPlugin.h"

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

    int channelCount(void);

    int transmitterMode(void) { return _transmitterMode; }
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
    void _rcChannelsChanged(int channelCount, int pwmValues[Vehicle::cMaxRcChannels]);

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

    int _chanMax(void) const;

    bool _channelReversedParamValue(int channel);
    void _setChannelReversedParamValue(int channel, bool reversed);

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

    static const struct FunctionInfo _rgFunctionInfoAPM[rcCalFunctionMax]; ///< Information associated with each function, PX4 firmware
    static const struct FunctionInfo _rgFunctionInfoPX4[rcCalFunctionMax]; ///< Information associated with each function, APM firmware

    int _rgFunctionChannelMapping[rcCalFunctionMax];                    ///< Maps from rcCalFunctions to channel index. _chanMax indicates channel not set for this function.

    static const int _attitudeControls = 5;

    int _chanCount;                     ///< Number of actual rc channels available
    static const int _chanMaxPX4 = 18;  ///< Maximum number of supported rc channels, PX4 Firmware
    static const int _chanMaxAPM = 14;  ///< Maximum number of supported rc channels, APM firmware
    static const int _chanMaxAny = 18;  ///< Maximum number of support rc channels by this implementation
    static const int _chanMinimum = 5;  ///< Minimum numner of channels required to run

    struct ChannelInfo _rgChannelInfo[_chanMaxAny];    ///< Information associated with each rc channel

    QList<int> _apmPossibleMissingRCChannelParams;  ///< List of possible missing RC*_* params for APM stack

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

    static const char*  _px4RevParamFormat;
    static const char*  _apmNewRevParamFormat;
    QString             _revParamFormat;
    bool                _revParamIsBool;

    int _rcValueSave[_chanMaxAny];        ///< Saved values prior to detecting channel movement

    int _rcRawValue[_chanMaxAny];         ///< Current set of raw channel values

    int     _stickDetectChannel;
    int     _stickDetectInitialValue;
    int     _stickDetectValue;
    bool    _stickDetectSettleStarted;
    QTime   _stickDetectSettleElapsed;
    static const int _stickDetectSettleMSecs;

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
};

#endif // RadioComponentController_H
