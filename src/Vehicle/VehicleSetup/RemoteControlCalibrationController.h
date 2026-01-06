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

    Q_PROPERTY(QQuickItem*  statusText                  MEMBER _statusText              REQUIRED)
    Q_PROPERTY(QQuickItem*  cancelButton                MEMBER _cancelButton            REQUIRED)
    Q_PROPERTY(QQuickItem*  nextButton                  MEMBER _nextButton              REQUIRED)

    Q_PROPERTY(int          minChannelCount             MEMBER _chanMinimum             CONSTANT)
    Q_PROPERTY(int          channelCount                READ   channelCount             NOTIFY channelCountChanged)
    Q_PROPERTY(int          channelValueMin             MEMBER _calDefaultMinValue      CONSTANT)
    Q_PROPERTY(int          channelValueMax             MEMBER _calDefaultMaxValue      CONSTANT)
    Q_PROPERTY(bool         rollChannelMapped           READ   rollChannelMapped        NOTIFY rollChannelMappedChanged)
    Q_PROPERTY(bool         pitchChannelMapped          READ   pitchChannelMapped       NOTIFY pitchChannelMappedChanged)
    Q_PROPERTY(bool         yawChannelMapped            READ   yawChannelMapped         NOTIFY yawChannelMappedChanged)
    Q_PROPERTY(bool         throttleChannelMapped       READ   throttleChannelMapped    NOTIFY throttleChannelMappedChanged)
    Q_PROPERTY(int          adjustedRollChannelValue     READ  adjustedRollChannelValue     NOTIFY adjustedRollChannelValueChanged)
    Q_PROPERTY(int          adjustedPitchChannelValue    READ  adjustedPitchChannelValue    NOTIFY adjustedPitchChannelValueChanged)
    Q_PROPERTY(int          adjustedYawChannelValue      READ  adjustedYawChannelValue      NOTIFY adjustedYawChannelValueChanged)
    Q_PROPERTY(int          adjustedThrottleChannelValue READ  adjustedThrottleChannelValue NOTIFY adjustedThrottleChannelValueChanged)
    Q_PROPERTY(int          rollChannelReversed         READ   rollChannelReversed       NOTIFY rollChannelReversedChanged)
    Q_PROPERTY(int          pitchChannelReversed        READ   pitchChannelReversed      NOTIFY pitchChannelReversedChanged)
    Q_PROPERTY(int          yawChannelReversed          READ   yawChannelReversed        NOTIFY yawChannelReversedChanged)
    Q_PROPERTY(int          throttleChannelReversed     READ   throttleChannelReversed   NOTIFY throttleChannelReversedChanged)
    Q_PROPERTY(int          rollDeadband                READ   rollDeadband             NOTIFY rollDeadbandChanged)
    Q_PROPERTY(int          pitchDeadband               READ   pitchDeadband            NOTIFY pitchDeadbandChanged)
    Q_PROPERTY(int          yawDeadband                 READ   yawDeadband              NOTIFY yawDeadbandChanged)
    Q_PROPERTY(int          throttleDeadband            READ   throttleDeadband         NOTIFY throttleDeadbandChanged)
    Q_PROPERTY(QList<int>   stickDisplayPositions       READ   stickDisplayPositions     NOTIFY stickDisplayPositionsChanged)

    Q_PROPERTY(bool         centeredThrottle            READ   centeredThrottle          WRITE setCenteredThrottle   NOTIFY centeredThrottleChanged)
    Q_PROPERTY(int          transmitterMode             READ   transmitterMode           WRITE setTransmitterMode    NOTIFY transmitterModeChanged)
    Q_PROPERTY(bool         joystickMode                READ   joystickMode              WRITE setJoystickMode       NOTIFY joystickModeChanged REQUIRED)
    Q_PROPERTY(bool         calibrating                 READ   calibrating               NOTIFY calibratingChanged)

public:
    RemoteControlCalibrationController(QObject *parent = nullptr);
    ~RemoteControlCalibrationController();

    /// These identify the various controls functions. They are also used as indices into the _rgFunctioInfo array.
    enum StickFunction {
        stickFunctionRoll,
        stickFunctionPitch,
        stickFunctionYaw,
        stickFunctionThrottle,
        stickFunctionMax,
    };

    Q_INVOKABLE void cancelButtonClicked();
    Q_INVOKABLE void nextButtonClicked();
    virtual Q_INVOKABLE void start();
    Q_INVOKABLE void copyTrims();

    int adjustedRollChannelValue();
    int adjustedPitchChannelValue();
    int adjustedYawChannelValue();
    int adjustedThrottleChannelValue();
    bool rollChannelMapped();
    bool pitchChannelMapped();
    bool yawChannelMapped();
    bool throttleChannelMapped();
    bool rollChannelReversed();
    bool pitchChannelReversed();
    bool yawChannelReversed();
    bool throttleChannelReversed();
    int rollDeadband();
    int pitchDeadband();
    int yawDeadband();
    int throttleDeadband();
    int channelCount() const { return _chanCount; }
    int transmitterMode() const { return _transmitterMode; }
    QList<int> stickDisplayPositions() const { return _stickDisplayPositions; }
    bool centeredThrottle() const { return _centeredThrottle; }
    bool joystickMode() const { return _joystickMode; }
    bool calibrating() const { return _calibrating; }

    void setTransmitterMode(int mode);
    void setCenteredThrottle(bool centered);
    void setJoystickMode(bool joystickMode);

signals:
    void channelCountChanged(int channelCount);
    void rawChannelValueChanged(int channel, int value);
    void rollChannelMappedChanged(bool mapped);
    void pitchChannelMappedChanged(bool mapped);
    void yawChannelMappedChanged(bool mapped);
    void throttleChannelMappedChanged(bool mapped);
    void adjustedRollChannelValueChanged(int rcValue);
    void adjustedPitchChannelValueChanged(int rcValue);
    void adjustedYawChannelValueChanged(int rcValue);
    void adjustedThrottleChannelValueChanged(int rcValue);
    void rollChannelReversedChanged(bool reversed);
    void pitchChannelReversedChanged(bool reversed);
    void yawChannelReversedChanged(bool reversed);
    void throttleChannelReversedChanged(bool reversed);
    void rollDeadbandChanged(int deadband);
    void pitchDeadbandChanged(int deadband);
    void yawDeadbandChanged(int deadband);
    void throttleDeadbandChanged(int deadband);
    void transmitterModeChanged();
    void stickDisplayPositionsChanged();
    void centeredThrottleChanged(bool centeredThrottle);
    void joystickModeChanged(bool joystickMode);
    void calibratingChanged(bool calibrating);
    void calibrationCompleted();

public slots:
    /// Super class must call this when the raw channel values change
    ///     @param channelValues The current channel values
    void rawChannelValuesChanged(QVector<int> channelValues);

protected:
    /// A set of information associated with a radio channel.
    static constexpr int _chanMax = QGCMAVLink::maxRcChannels; ///< Maximum number of supported channels by this implementation
    struct ChannelInfo {
        enum StickFunction stickFunction; ///< Function mapped to this channel, stickFunctionMax for none
        bool channelReversed;
        int channelMin;
        int channelMax;
        int channelTrim; ///< Trim position (usually center for sticks)
        int deadband; ///< deadband around center
    };
    ChannelInfo _rgChannelInfo[_chanMax];    ///< Information associated with each rc channel

    // These values must be provided by the super class
    int _calValidMinValue;    ///< Largest valid minimum channel range value
    int _calValidMaxValue;    ///< Smallest valid maximum channel range value
    int _calCenterPoint;      ///< Center point for calibration
    int _calDefaultMinValue;  ///< Default value for Min if not set
    int _calDefaultMaxValue;  ///< Default value for Max if not set
    int _calRoughCenterDelta; ///< Delta around center point which is considered to be roughly centered
    int _calMoveDelta;        ///< Amount of delta past center which is considered stick movement
    int _calSettleDelta;      ///< Amount of delta which is considered no stick movement

    int _chanCount = 0;                     ///< Number of actual rc channels available
    static constexpr int _chanMinimum = 4;  ///< Minimum numner of channels required to run

    /// Maps from StickFunction to channel index. _chanMax indicates channel not set for this function.
    int _rgFunctionChannelMapping[stickFunctionMax];

    void _validateAndAdjustCalibrationValues();
    QString _stickFunctionToString(StickFunction stickFunction);
    void _signalAllAttitudeValueChanges();
    void _resetInternalCalibrationValues(); ///< Resets internal calibration values to their initial state in preparation for a new calibration sequence.

private:
    virtual void _saveStoredCalibrationValues() = 0; ///< Super class must implement to save stored calibration values
    virtual void _readStoredCalibrationValues() = 0; ///< Super class must implement to read stored calibration values

    // Stick display position in calibration image
    //  horizontal - -1 indicates left, 0 indicates centered, +1 indicates right
    //  vertical   - -1 indicates down, 0 indicates centered, +1 indicates up
    struct StickDisplayPosition {
        int horizontal;
        int vertical;
    };

    struct BothSticksDisplayPositions {
        StickDisplayPosition leftStick;
        StickDisplayPosition rightStick;
    };

    enum StateMachineStepFunction {
        StateMachineStepStickNeutral,
        StateMachineStepThrottleUp,
        StateMachineStepThrottleDown,
        StateMachineStepYawRight,
        StateMachineStepYawLeft,
        StateMachineStepRollRight,
        StateMachineStepRollLeft,
        StateMachineStepPitchUp,
        StateMachineStepPitchDown,
        StateMachineStepPitchCenter,
        StateMachineStepSwitchMinMax,
        StateMachineStepComplete
    };

    typedef void (RemoteControlCalibrationController::*inputFn)(enum StickFunction stickFunction, int chan, int value);
    typedef void (RemoteControlCalibrationController::*buttonFn)(void);
    struct StateMachineEntry {
        enum StickFunction stickFunction;
        enum StateMachineStepFunction stepFunction;
        inputFn channelInputFn;
        buttonFn nextButtonFn;
    };

    const StateMachineEntry &_getStateMachineEntry(int step) const;
    void _advanceState();
    void _setupCurrentState();
    void _inputCenterWaitBegin(StickFunction stickFunction, int channel, int value);
    void _inputStickDetect(StickFunction stickFunction, int channel, int value);
    void _inputStickMin(StickFunction stickFunction, int channel, int value);
    void _inputCenterWait(StickFunction stickFunction, int channel, int value);
    void _inputSwitchMinMax(StickFunction stickFunction, int channel, int value);    ///< Saves min/max for non-mapped channels
    void _saveCalibrationValues();
    void _saveAllTrims();
    bool _stickSettleComplete(int value);
    void _startCalibration();
    void _stopCalibration();
    void _saveCurrentRawValues();   ///< Saves the current channel values, so that we can detect when the user moves an input.
    void _loadCalibrationUISettings();
    void _saveCalibrationUISettings();
    int _deadbandForFunction(StickFunction stickFunction) const;
    void _emitDeadbandChanged(StickFunction stickFunction);
    int _adjustChannelRawValue(const ChannelInfo& info, int rawValue) const;

    int _currentStep = -1;
    int _transmitterMode = 2;

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

    int _channelValueSave[_chanMax]{};  ///< Saved values prior to detecting channel movement
    int _channelRawValue[_chanMax]{};   ///< Current set of raw channel values

    int _stickDetectChannel = 0;
    int _stickDetectValue = 0;
    bool _stickDetectSettleStarted = false;
    QElapsedTimer _stickDetectSettleElapsed;

    QQuickItem *_statusText = nullptr;
    QQuickItem *_cancelButton = nullptr;
    QQuickItem *_nextButton = nullptr;

    QList<int> _stickDisplayPositions;
    bool _centeredThrottle = false;
    bool _joystickMode = false;
    bool _calibrating = false;
    QMap<StateMachineStepFunction, QString> _stepFunctionToMsgStringMap;
    QMap<StateMachineStepFunction, QMap<int, BothSticksDisplayPositions>> _bothStickDisplayPositionThrottleCenteredMap;
    QMap<StateMachineStepFunction, QMap<int, BothSticksDisplayPositions>> _bothStickDisplayPositionThrottleDownMap;

    QList<StateMachineEntry> _stateMachine;

    static constexpr int _updateInterval = 150;             ///< Interval for timer which updates radio channel widgets

    static constexpr int _stickDetectSettleMSecs = 500;

    static constexpr const char *_settingsGroup = "RadioCalibration";
    static constexpr const char *_settingsKeyTransmitterMode = "TransmitterMode";

    // All the valid single stick positions used in calibration stick display
    static constexpr StickDisplayPosition _stickDisplayPositionCentered         = {0, 0};
    static constexpr StickDisplayPosition _stickDisplayPositionXCenteredYDown   = {0, -1};
    static constexpr StickDisplayPosition _stickDisplayPositionXCenteredYUp     = {0,  1};
    static constexpr StickDisplayPosition _stickDisplayPositionXLeftYCentered   = {-1, 0};
    static constexpr StickDisplayPosition _stickDisplayPositionXRightYCentered  = { 1, 0};
    static constexpr StickDisplayPosition _stickDisplayPositionXLeftYDown       = {-1, -1};
    static constexpr StickDisplayPosition _stickDisplayPositionXRightYDown      = { 1, -1};
};
