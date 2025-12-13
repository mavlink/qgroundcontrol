#pragma once

#include <QtCore/QLoggingCategory>
#include <QtCore/QElapsedTimer>
#include <QtQmlIntegration/QtQmlIntegration>
#include <QtQuick/QQuickItem>

#include "FactPanelController.h"
#include "QGCMAVLink.h"

Q_DECLARE_LOGGING_CATEGORY(RemoteControlCalibrationControllerLog)
Q_DECLARE_LOGGING_CATEGORY(RemoteControlCalibrationControllerVerboseLog)

/// Abstract base class for calibrating RC and Joystick controller.
class RemoteControlCalibrationController : public FactPanelController
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
    Q_PROPERTY(int rollChannelValue READ rollChannelValue NOTIFY rollChannelValueChanged)
    Q_PROPERTY(int pitchChannelValue READ pitchChannelValue NOTIFY pitchChannelValueChanged)
    Q_PROPERTY(int yawChannelValue READ yawChannelValue NOTIFY yawChannelValueChanged)
    Q_PROPERTY(int throttleChannelValue READ throttleChannelValue NOTIFY throttleChannelValueChanged)
    Q_PROPERTY(int rollChannelReversed READ rollChannelReversed NOTIFY rollChannelReversedChanged)
    Q_PROPERTY(int pitchChannelReversed READ pitchChannelReversed NOTIFY pitchChannelReversedChanged)
    Q_PROPERTY(int yawChannelReversed READ yawChannelReversed NOTIFY yawChannelReversedChanged)
    Q_PROPERTY(int throttleChannelReversed READ throttleChannelReversed NOTIFY throttleChannelReversedChanged)
    Q_PROPERTY(int transmitterMode READ transmitterMode WRITE setTransmitterMode NOTIFY transmitterModeChanged)
    Q_PROPERTY(QString imageHelp MEMBER _imageHelp NOTIFY imageHelpChanged)

public:
    RemoteControlCalibrationController(QObject *parent = nullptr);
    ~RemoteControlCalibrationController();

    Q_INVOKABLE void cancelButtonClicked();
    Q_INVOKABLE void skipButtonClicked();
    Q_INVOKABLE void nextButtonClicked();
    Q_INVOKABLE void start();
    Q_INVOKABLE void copyTrims();

    int rollChannelValue();
    int pitchChannelValue();
    int yawChannelValue();
    int throttleChannelValue();

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
    void channelValueChanged(int channel, int value);

    void rollChannelMappedChanged(bool mapped);
    void pitchChannelMappedChanged(bool mapped);
    void yawChannelMappedChanged(bool mapped);
    void throttleChannelMappedChanged(bool mapped);

    void rollChannelValueChanged(int rcValue);
    void pitchChannelValueChanged(int rcValue);
    void yawChannelValueChanged(int rcValue);
    void throttleChannelValueChanged(int rcValue);

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

public slots:
    /// Super class must call this when the channel values change
    ///     @param channelCount The number of channels available
    ///     @param channelValues The current values of the channels, -1 if not available
    void channelValuesChanged(int channelCount, int channelValues[QGCMAVLink::maxRcChannels]);

protected:
    // Pure virtuals which must be provided by the sub class
    virtual void _saveStoredCalibrationValues() = 0;
    virtual void _readStoredCalibrationValues() = 0;

    /// These identify the various controls functions. They are also used as indices into the _rgFunctioInfo array.
    enum StickFunction {
        stickFunctionRoll,
        stickFunctionPitch,
        stickFunctionYaw,
        stickFunctionThrottle,
        stickFunctionMax,
    };

    // These values must be provided by the super class
    int _calValidMinValue;    ///< Largest valid minimum channel range value
    int _calValidMaxValue;    ///< Smallest valid maximum channel range value
    int _calCenterPoint;      ///< Center point for calibration
    int _calDefaultMinValue;  ///< Default value for Min if not set
    int _calDefaultMaxValue;  ///< Default value for Max if not set
    int _calRoughCenterDelta; ///< Delta around center point which is considered to be roughly centered
    int _calMoveDelta;        ///< Amount of delta past center which is considered stick movement
    int _calSettleDelta;      ///< Amount of delta which is considered no stick movement
    int _calMinDelta;         ///< Amount of delta allowed around min value to consider channel at min

    int _chanCount = 0;                     ///< Number of actual rc channels available
    static constexpr int _chanMax = 18;     ///< Maximum number of support rc channels by this implementation
    static constexpr int _chanMinimum = 5;  ///< Minimum numner of channels required to run

    /// A set of information associated with a radio channel.
    struct ChannelInfo {
        enum StickFunction function;   ///< Function mapped to this channel, stickFunctionMax for none
        bool reversed;                  ///< true: channel is reverse, false: not reversed
        int rcMin;                      ///< Minimum RC value
        int rcMax;                      ///< Maximum RC value
        int rcTrim;                     ///< Trim position
    };
    ChannelInfo _rgChannelInfo[_chanMax];    ///< Information associated with each rc channel

    /// Maps from StickFunction to channel index. _chanMax indicates channel not set for this function.
    int _rgFunctionChannelMapping[stickFunctionMax];

    void _validateAndAdjustCalibrationValues();
    QString _stickFunctionToString(StickFunction function);
    void _stopCalibration();
    void _signalAllAttitudeValueChanges();

private:
    typedef void (RemoteControlCalibrationController::*inputFn)(enum StickFunction function, int chan, int value);
    typedef void (RemoteControlCalibrationController::*buttonFn)(void);
    struct stateMachineEntry {
        enum StickFunction function;
        const char *instructions;
        const char *image;
        inputFn channelInputFn;
        buttonFn nextButtonFn;
        buttonFn skipButtonFn;
    };
    /// Returns the state machine entry for the specified state.
    const stateMachineEntry *_getStateMachineEntry(int step) const;

    void _advanceState();
    /// Sets up the state machine according to the current step from _currentStep.
    void _setupCurrentState();

    void _inputCenterWaitBegin(StickFunction function, int channel, int value);
    void _inputStickDetect(StickFunction function, int channel, int value);
    void _inputStickMin(StickFunction function, int channel, int value);
    void _inputCenterWait(StickFunction function, int channel, int value);
    /// Saves min/max for non-mapped channels
    void _inputSwitchMinMax(StickFunction function, int channel, int value);
    void _inputSwitchDetect(StickFunction function, int channel, int value);

    void _switchDetect(StickFunction function, int channel, int value, bool moveToNextStep);

    void _saveAllTrims();

    bool _stickSettleComplete(int value);

    /// Resets internal calibration values to their initial state in preparation for a new calibration sequence.
    void _resetInternalCalibrationValues();

    /// Starts the calibration process
    void _startCalibration();
    /// Set up the Save state of calibration.
    void _rcCalSave();

    void _writeParameters();

    /// Saves the current channel values, so that we can detect when the use moves an input.
    void _rcCalSaveCurrentValues();

    void _setHelpImage(const char *imageFile);

    void _loadSettings();
    void _storeSettings();

    int _currentStep = -1; ///< Current step of state machine
    int _transmitterMode = 2; ///< 1: transmitter is mode 1, 2: transmitted is mode 2

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

    int _rcValueSave[_chanMax]{}; ///< Saved values prior to detecting channel movement
    int _rcRawValue[_chanMax]{}; ///< Current set of raw channel values

    int _stickDetectChannel = 0;
    int _stickDetectValue = 0;
    bool _stickDetectSettleStarted = false;
    QElapsedTimer _stickDetectSettleElapsed;

    QQuickItem *_statusText = nullptr;
    QQuickItem *_cancelButton = nullptr;
    QQuickItem *_nextButton = nullptr;
    QQuickItem *_skipButton = nullptr;

    QString _imageHelp;

    static constexpr int _updateInterval = 150;             ///< Interval for timer which updates radio channel widgets

    static constexpr int _stickDetectSettleMSecs = 500;

    static constexpr const char *_settingsGroup = "RadioCalibration";
    static constexpr const char *_settingsKeyTransmitterMode = "TransmitterMode";

    static constexpr const char *_imageCenter = "radioCenter.png";
};
