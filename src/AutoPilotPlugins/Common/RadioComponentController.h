/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QLoggingCategory>
#include <QtCore/QElapsedTimer>
#include <QtQuick/QQuickItem>

#include "FactPanelController.h"
#include "QGCMAVLink.h"

Q_DECLARE_LOGGING_CATEGORY(RadioComponentControllerLog)
Q_DECLARE_LOGGING_CATEGORY(RadioComponentControllerVerboseLog)

class RadioComponentController : public FactPanelController
{
    Q_OBJECT
    Q_PROPERTY(int minChannelCount MEMBER _chanMinimum CONSTANT)
    Q_PROPERTY(int channelCount READ channelCount NOTIFY channelCountChanged)
    Q_PROPERTY(QQuickItem *statusText MEMBER _statusText NOTIFY statusTextChanged)
    Q_PROPERTY(QQuickItem *cancelButton MEMBER _cancelButton NOTIFY cancelButtonChanged)
    Q_PROPERTY(QQuickItem *nextButton MEMBER _nextButton NOTIFY nextButtonChanged)
    Q_PROPERTY(QQuickItem *skipButton MEMBER _skipButton NOTIFY skipButtonChanged)
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

    friend class RadioConfigTest;

public:
    RadioComponentController(QObject *parent = nullptr);
    ~RadioComponentController();

    enum BindModes {
        DSM2,
        DSMX7,
        DSMX8
    };
    Q_ENUM(BindModes)

    Q_INVOKABLE void spektrumBindMode(int mode);
    Q_INVOKABLE void crsfBindMode();
    Q_INVOKABLE void cancelButtonClicked();
    Q_INVOKABLE void skipButtonClicked();
    Q_INVOKABLE void nextButtonClicked();
    Q_INVOKABLE void start();
    Q_INVOKABLE void copyTrims();

    int rollChannelRCValue();
    int pitchChannelRCValue();
    int yawChannelRCValue();
    int throttleChannelRCValue();

    bool rollChannelMapped();
    bool pitchChannelMapped();
    bool yawChannelMapped();
    bool throttleChannelMapped();

    bool rollChannelReversed();
    bool pitchChannelReversed();
    bool yawChannelReversed();
    bool throttleChannelReversed();

    int channelCount() const { return _chanCount; }

    int transmitterMode() const { return _transmitterMode; }
    void setTransmitterMode(int mode);

signals:
    void statusTextChanged();
    void cancelButtonChanged();
    void nextButtonChanged();
    void skipButtonChanged();

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
    void nextButtonMessageBoxDisplayed();

    /// Signalled to QML to indicate reboot is required
    void functionMappingChangedAPMReboot();

    /// Signalled to Qml to indicate cal failure due to reversed throttle
    void throttleReversedCalFailure();

private slots:
    /// Connected to Vehicle::rcChannelsChanged signal
    void _rcChannelsChanged(int channelCount, int pwmValues[QGCMAVLink::maxRcChannels]);

private:
    /// These identify the various controls functions. They are also used as indices into the _rgFunctioInfo array.
    enum rcCalFunctions {
        rcCalFunctionRoll,
        rcCalFunctionPitch,
        rcCalFunctionYaw,
        rcCalFunctionThrottle,
        rcCalFunctionMax,
    };

    typedef void (RadioComponentController::*inputFn)(enum rcCalFunctions function, int chan, int value);
    typedef void (RadioComponentController::*buttonFn)(void);
    struct stateMachineEntry {
        enum rcCalFunctions function;
        const char *instructions;
        const char *image;
        inputFn rcInputFn;
        buttonFn nextFn;
        buttonFn skipFn;
    };
    /// Returns the state machine entry for the specified state.
    const stateMachineEntry *_getStateMachineEntry(int step) const;

    /// A set of information associated with a function.
    struct FunctionInfo {
        const char *parameterName; ///< Parameter name for function mapping
    };
    const FunctionInfo *_functionInfo() const;

    bool _px4Vehicle() const;

    void _advanceState();
    /// Sets up the state machine according to the current step from _currentStep.
    void _setupCurrentState();

    void _inputCenterWaitBegin(rcCalFunctions function, int channel, int value);
    void _inputStickDetect(rcCalFunctions function, int channel, int value);
    void _inputStickMin(rcCalFunctions function, int channel, int value);
    void _inputCenterWait(rcCalFunctions function, int channel, int value);
    /// Saves min/max for non-mapped channels
    void _inputSwitchMinMax(rcCalFunctions function, int channel, int value);
    void _inputSwitchDetect(rcCalFunctions function, int channel, int value);

    void _switchDetect(rcCalFunctions function, int channel, int value, bool moveToNextStep);

    void _saveAllTrims();

    bool _stickSettleComplete(int value);

    /// Validates the current settings against the calibration rules resetting values as necessary.
    void _validateCalibration();
    /// Saves the rc calibration values to the board parameters.
    void _writeCalibration();
    /// Resets internal calibration values to their initial state in preparation for a new calibration sequence.
    void _resetInternalCalibrationValues();
    /// Sets internal calibration values from the stored parameters
    void _setInternalCalibrationValuesFromParameters();

    /// Starts the calibration process
    void _startCalibration();
    /// Cancels the calibration process, setting things back to initial state.
    void _stopCalibration();
    /// Set up the Save state of calibration.
    void _rcCalSave();

    void _writeParameters();

    /// Saves the current channel values, so that we can detect when the use moves an input.
    void _rcCalSaveCurrentValues();

    void _setHelpImage(const char *imageFile);

    void _loadSettings();
    void _storeSettings();

    void _signalAllAttitudeValueChanges();

    bool _channelReversedParamValue(int channel);
    void _setChannelReversedParamValue(int channel, bool reversed);

    /// Called by unit test code to set the mode to unit testing
    void _setUnitTestMode() { _unitTestMode = true; }

    int _currentStep = -1; ///< Current step of state machine
    int _transmitterMode = 2; ///< 1: transmitter is mode 1, 2: transmitted is mode 2
    int _rgFunctionChannelMapping[rcCalFunctionMax]; ///< Maps from rcCalFunctions to channel index. _chanMax indicates channel not set for this function.

    static constexpr int _attitudeControls = 5;

    int _chanCount = 0;                     ///< Number of actual rc channels available
    static constexpr int _chanMax = 18;     ///< Maximum number of support rc channels by this implementation
    static constexpr int _chanMinimum = 5;  ///< Minimum numner of channels required to run

    /// A set of information associated with a radio channel.
    struct ChannelInfo {
        enum rcCalFunctions function;   ///< Function mapped to this channel, rcCalFunctionMax for none
        bool reversed;                  ///< true: channel is reverse, false: not reversed
        int rcMin;                      ///< Minimum RC value
        int rcMax;                      ///< Maximum RC value
        int rcTrim;                     ///< Trim position
    };
    ChannelInfo _rgChannelInfo[_chanMax];    ///< Information associated with each rc channel

    /// The states of the calibration state machine.
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
    rcCalStates _rcCalState = rcCalStateChannelWait; ///< Current calibration state

    QString _revParamFormat;
    bool _revParamIsBool = false;

    int _rcValueSave[_chanMax]{}; ///< Saved values prior to detecting channel movement
    int _rcRawValue[_chanMax]{}; ///< Current set of raw channel values

    int _stickDetectChannel = 0;
    int _stickDetectValue = 0;
    bool _stickDetectSettleStarted = false;
    QElapsedTimer _stickDetectSettleElapsed;

    bool _unitTestMode = false;

    QQuickItem *_statusText = nullptr;
    QQuickItem *_cancelButton = nullptr;
    QQuickItem *_nextButton = nullptr;
    QQuickItem *_skipButton = nullptr;

    QString _imageHelp;

#ifdef QGC_UNITTEST_BUILD
    // Nasty hack to expose controller to unit test code
    static RadioComponentController *_unitTestController;
#endif

    static constexpr int _updateInterval = 150;             ///< Interval for timer which updates radio channel widgets
    static constexpr int _rcCalPWMValidMinValue = 1300;     ///< Largest valid minimum PWM Min range value
    static constexpr int _rcCalPWMValidMaxValue = 1700;     ///< Smallest valid maximum PWM Max range value
    static constexpr int _rcCalPWMCenterPoint = ((_rcCalPWMValidMaxValue - _rcCalPWMValidMinValue) / 2.0f) + _rcCalPWMValidMinValue;
    static constexpr int _rcCalPWMDefaultMinValue = 1000;   ///< Default value for Min if not set
    static constexpr int _rcCalPWMDefaultMaxValue = 2000;   ///< Default value for Max if not set
    static constexpr int _rcCalRoughCenterDelta = 50;       ///< Delta around center point which is considered to be roughly centered
    static constexpr int _rcCalMoveDelta = 300;             ///< Amount of delta past center which is considered stick movement
    static constexpr int _rcCalSettleDelta = 20;            ///< Amount of delta which is considered no stick movement
    static constexpr int _rcCalMinDelta = 100;              ///< Amount of delta allowed around min value to consider channel at min

    static constexpr int _stickDetectSettleMSecs = 500;

    static constexpr const char *_settingsGroup = "RadioCalibration";
    static constexpr const char *_settingsKeyTransmitterMode = "TransmitterMode";

    static constexpr const char *_imageCenter = "radioCenter.png";
};
