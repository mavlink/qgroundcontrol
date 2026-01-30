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

    Q_PROPERTY(QQuickItem*  statusText                              MEMBER  _statusText                                         REQUIRED)
    Q_PROPERTY(QQuickItem*  cancelButton                            MEMBER  _cancelButton                                       REQUIRED)
    Q_PROPERTY(QQuickItem*  nextButton                              MEMBER  _nextButton                                         REQUIRED)

    Q_PROPERTY(int          minChannelCount                         MEMBER  _chanMinimum                                        CONSTANT)
    Q_PROPERTY(int          channelCount                            READ    channelCount                                        NOTIFY channelCountChanged)
    Q_PROPERTY(int          channelValueMin                         MEMBER  _calDefaultMinValue                                 CONSTANT)
    Q_PROPERTY(int          channelValueMax                         MEMBER  _calDefaultMaxValue                                 CONSTANT)
    Q_PROPERTY(QList<int>   stickDisplayPositions                   READ    stickDisplayPositions                               NOTIFY stickDisplayPositionsChanged)
    Q_PROPERTY(bool         centeredThrottle                        READ    centeredThrottle                    WRITE setCenteredThrottle   NOTIFY centeredThrottleChanged)
    Q_PROPERTY(int          transmitterMode                         READ    transmitterMode                     WRITE setTransmitterMode    NOTIFY transmitterModeChanged)
    Q_PROPERTY(bool         joystickMode                            READ    joystickMode                        WRITE setJoystickMode       NOTIFY joystickModeChanged REQUIRED)
    Q_PROPERTY(bool         calibrating                             READ    calibrating                                         NOTIFY calibratingChanged)
    Q_PROPERTY(bool         singleStickDisplay                      READ    singleStickDisplay                                  NOTIFY singleStickDisplayChanged)

    Q_PROPERTY(bool         rollChannelMapped                       READ    rollChannelMapped                                   NOTIFY rollChannelMappedChanged)
    Q_PROPERTY(bool         pitchChannelMapped                      READ    pitchChannelMapped                                  NOTIFY pitchChannelMappedChanged)
    Q_PROPERTY(bool         yawChannelMapped                        READ    yawChannelMapped                                    NOTIFY yawChannelMappedChanged)
    Q_PROPERTY(bool         throttleChannelMapped                   READ    throttleChannelMapped                               NOTIFY throttleChannelMappedChanged)
    Q_PROPERTY(bool         rollExtensionChannelMapped              READ    rollExtensionChannelMapped                          NOTIFY rollExtensionChannelMappedChanged)
    Q_PROPERTY(bool         pitchExtensionChannelMapped             READ    pitchExtensionChannelMapped                         NOTIFY pitchExtensionChannelMappedChanged)
    Q_PROPERTY(bool         aux1ExtensionChannelMapped              READ    aux1ExtensionChannelMapped                          NOTIFY aux1ExtensionChannelMappedChanged)
    Q_PROPERTY(bool         aux2ExtensionChannelMapped              READ    aux2ExtensionChannelMapped                          NOTIFY aux2ExtensionChannelMappedChanged)
    Q_PROPERTY(bool         aux3ExtensionChannelMapped              READ    aux3ExtensionChannelMapped                          NOTIFY aux3ExtensionChannelMappedChanged)
    Q_PROPERTY(bool         aux4ExtensionChannelMapped              READ    aux4ExtensionChannelMapped                          NOTIFY aux4ExtensionChannelMappedChanged)
    Q_PROPERTY(bool         aux5ExtensionChannelMapped              READ    aux5ExtensionChannelMapped                          NOTIFY aux5ExtensionChannelMappedChanged)
    Q_PROPERTY(bool         aux6ExtensionChannelMapped              READ    aux6ExtensionChannelMapped                          NOTIFY aux6ExtensionChannelMappedChanged)

    Q_PROPERTY(bool         pitchExtensionEnabled                   READ    pitchExtensionEnabled                               NOTIFY pitchExtensionEnabledChanged)
    Q_PROPERTY(bool         rollExtensionEnabled                    READ    rollExtensionEnabled                                NOTIFY rollExtensionEnabledChanged)
    Q_PROPERTY(bool         aux1ExtensionEnabled                    READ    aux1ExtensionEnabled                                NOTIFY aux1ExtensionEnabledChanged)
    Q_PROPERTY(bool         aux2ExtensionEnabled                    READ    aux2ExtensionEnabled                                NOTIFY aux2ExtensionEnabledChanged)
    Q_PROPERTY(bool         aux3ExtensionEnabled                    READ    aux3ExtensionEnabled                                NOTIFY aux3ExtensionEnabledChanged)
    Q_PROPERTY(bool         aux4ExtensionEnabled                    READ    aux4ExtensionEnabled                                NOTIFY aux4ExtensionEnabledChanged)
    Q_PROPERTY(bool         aux5ExtensionEnabled                    READ    aux5ExtensionEnabled                                NOTIFY aux5ExtensionEnabledChanged)
    Q_PROPERTY(bool         aux6ExtensionEnabled                    READ    aux6ExtensionEnabled                                NOTIFY aux6ExtensionEnabledChanged)
    Q_PROPERTY(bool         anyExtensionEnabled                     READ    anyExtensionEnabled                                 NOTIFY anyExtensionEnabledChanged)

    Q_PROPERTY(int          adjustedRollChannelValue                READ    adjustedRollChannelValue                            NOTIFY adjustedRollChannelValueChanged)
    Q_PROPERTY(int          adjustedPitchChannelValue               READ    adjustedPitchChannelValue                           NOTIFY adjustedPitchChannelValueChanged)
    Q_PROPERTY(int          adjustedYawChannelValue                 READ    adjustedYawChannelValue                             NOTIFY adjustedYawChannelValueChanged)
    Q_PROPERTY(int          adjustedThrottleChannelValue            READ    adjustedThrottleChannelValue                        NOTIFY adjustedThrottleChannelValueChanged)
    Q_PROPERTY(int          adjustedRollExtensionChannelValue       READ    adjustedRollExtensionChannelValue                   NOTIFY adjustedRollExtensionChannelValueChanged)
    Q_PROPERTY(int          adjustedPitchExtensionChannelValue      READ    adjustedPitchExtensionChannelValue                  NOTIFY adjustedPitchExtensionChannelValueChanged)
    Q_PROPERTY(int          adjustedAux1ExtensionChannelValue       READ    adjustedAux1ExtensionChannelValue                   NOTIFY adjustedAux1ExtensionChannelValueChanged)
    Q_PROPERTY(int          adjustedAux2ExtensionChannelValue       READ    adjustedAux2ExtensionChannelValue                   NOTIFY adjustedAux2ExtensionChannelValueChanged)
    Q_PROPERTY(int          adjustedAux3ExtensionChannelValue       READ    adjustedAux3ExtensionChannelValue                   NOTIFY adjustedAux3ExtensionChannelValueChanged)
    Q_PROPERTY(int          adjustedAux4ExtensionChannelValue       READ    adjustedAux4ExtensionChannelValue                   NOTIFY adjustedAux4ExtensionChannelValueChanged)
    Q_PROPERTY(int          adjustedAux5ExtensionChannelValue       READ    adjustedAux5ExtensionChannelValue                   NOTIFY adjustedAux5ExtensionChannelValueChanged)
    Q_PROPERTY(int          adjustedAux6ExtensionChannelValue       READ    adjustedAux6ExtensionChannelValue                   NOTIFY adjustedAux6ExtensionChannelValueChanged)

    Q_PROPERTY(int          rollChannelReversed                     READ    rollChannelReversed                                 NOTIFY rollChannelReversedChanged)
    Q_PROPERTY(int          pitchChannelReversed                    READ    pitchChannelReversed                                NOTIFY pitchChannelReversedChanged)
    Q_PROPERTY(int          yawChannelReversed                      READ    yawChannelReversed                                  NOTIFY yawChannelReversedChanged)
    Q_PROPERTY(int          throttleChannelReversed                 READ    throttleChannelReversed                             NOTIFY throttleChannelReversedChanged)
    Q_PROPERTY(int          rollExtensionChannelReversed            READ    rollExtensionChannelReversed                        NOTIFY rollExtensionChannelReversedChanged)
    Q_PROPERTY(int          pitchExtensionChannelReversed           READ    pitchExtensionChannelReversed                       NOTIFY pitchExtensionChannelReversedChanged)
    Q_PROPERTY(int          aux1ExtensionChannelReversed            READ    aux1ExtensionChannelReversed                        NOTIFY aux1ExtensionChannelReversedChanged)
    Q_PROPERTY(int          aux2ExtensionChannelReversed            READ    aux2ExtensionChannelReversed                        NOTIFY aux2ExtensionChannelReversedChanged)
    Q_PROPERTY(int          aux3ExtensionChannelReversed            READ    aux3ExtensionChannelReversed                        NOTIFY aux3ExtensionChannelReversedChanged)
    Q_PROPERTY(int          aux4ExtensionChannelReversed            READ    aux4ExtensionChannelReversed                        NOTIFY aux4ExtensionChannelReversedChanged)
    Q_PROPERTY(int          aux5ExtensionChannelReversed            READ    aux5ExtensionChannelReversed                        NOTIFY aux5ExtensionChannelReversedChanged)
    Q_PROPERTY(int          aux6ExtensionChannelReversed            READ    aux6ExtensionChannelReversed                        NOTIFY aux6ExtensionChannelReversedChanged)

    Q_PROPERTY(int          rollDeadband                            READ    rollDeadband                                        NOTIFY rollDeadbandChanged)
    Q_PROPERTY(int          pitchDeadband                           READ    pitchDeadband                                       NOTIFY pitchDeadbandChanged)
    Q_PROPERTY(int          rollExtensionDeadband                   READ    rollExtensionDeadband                               NOTIFY rollExtensionDeadbandChanged)
    Q_PROPERTY(int          yawDeadband                             READ    yawDeadband                                         NOTIFY yawDeadbandChanged)
    Q_PROPERTY(int          throttleDeadband                        READ    throttleDeadband                                    NOTIFY throttleDeadbandChanged)
    Q_PROPERTY(int          pitchExtensionDeadband                  READ    pitchExtensionDeadband                              NOTIFY pitchExtensionDeadbandChanged)
    Q_PROPERTY(int          aux1ExtensionDeadband                   READ    aux1ExtensionDeadband                               NOTIFY aux1ExtensionDeadbandChanged)
    Q_PROPERTY(int          aux2ExtensionDeadband                   READ    aux2ExtensionDeadband                               NOTIFY aux2ExtensionDeadbandChanged)
    Q_PROPERTY(int          aux3ExtensionDeadband                   READ    aux3ExtensionDeadband                               NOTIFY aux3ExtensionDeadbandChanged)
    Q_PROPERTY(int          aux4ExtensionDeadband                   READ    aux4ExtensionDeadband                               NOTIFY aux4ExtensionDeadbandChanged)
    Q_PROPERTY(int          aux5ExtensionDeadband                   READ    aux5ExtensionDeadband                               NOTIFY aux5ExtensionDeadbandChanged)
    Q_PROPERTY(int          aux6ExtensionDeadband                   READ    aux6ExtensionDeadband                               NOTIFY aux6ExtensionDeadbandChanged)

public:
    RemoteControlCalibrationController(QObject *parent = nullptr);
    ~RemoteControlCalibrationController();

    /// These identify the various controls functions. They are also used as indices into the _rgFunctioInfo array.
    enum StickFunction {
        stickFunctionRoll,
        stickFunctionPitch,
        stickFunctionYaw,
        stickFunctionThrottle,
        stickFunctionMaxRadio,
        stickFunctionPitchExtension = stickFunctionMaxRadio,
        stickFunctionRollExtension,
        stickFunctionAux1Extension,
        stickFunctionAux2Extension,
        stickFunctionAux3Extension,
        stickFunctionAux4Extension,
        stickFunctionAux5Extension,
        stickFunctionAux6Extension,
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
    int adjustedRollExtensionChannelValue();
    int adjustedPitchExtensionChannelValue();
    int adjustedAux1ExtensionChannelValue();
    int adjustedAux2ExtensionChannelValue();
    int adjustedAux3ExtensionChannelValue();
    int adjustedAux4ExtensionChannelValue();
    int adjustedAux5ExtensionChannelValue();
    int adjustedAux6ExtensionChannelValue();
    bool rollChannelMapped();
    bool pitchChannelMapped();
    bool rollExtensionChannelMapped();
    bool pitchExtensionChannelMapped();
    bool aux1ExtensionChannelMapped();
    bool aux2ExtensionChannelMapped();
    bool aux3ExtensionChannelMapped();
    bool aux4ExtensionChannelMapped();
    bool aux5ExtensionChannelMapped();
    bool aux6ExtensionChannelMapped();
    bool yawChannelMapped();
    bool throttleChannelMapped();
    bool pitchExtensionEnabled();
    bool rollExtensionEnabled();
    bool aux1ExtensionEnabled();
    bool aux2ExtensionEnabled();
    bool aux3ExtensionEnabled();
    bool aux4ExtensionEnabled();
    bool aux5ExtensionEnabled();
    bool aux6ExtensionEnabled();
    bool anyExtensionEnabled();
    bool rollChannelReversed();
    bool pitchChannelReversed();
    bool rollExtensionChannelReversed();
    bool pitchExtensionChannelReversed();
    bool aux1ExtensionChannelReversed();
    bool aux2ExtensionChannelReversed();
    bool aux3ExtensionChannelReversed();
    bool aux4ExtensionChannelReversed();
    bool aux5ExtensionChannelReversed();
    bool aux6ExtensionChannelReversed();
    bool yawChannelReversed();
    bool throttleChannelReversed();
    int rollDeadband();
    int pitchDeadband();
    int rollExtensionDeadband();
    int pitchExtensionDeadband();
    int aux1ExtensionDeadband();
    int aux2ExtensionDeadband();
    int aux3ExtensionDeadband();
    int aux4ExtensionDeadband();
    int aux5ExtensionDeadband();
    int aux6ExtensionDeadband();
    int yawDeadband();
    int throttleDeadband();
    int channelCount() const { return _chanCount; }
    int transmitterMode() const { return _transmitterMode; }
    QList<int> stickDisplayPositions() const { return _stickDisplayPositions; }
    bool centeredThrottle() const { return _centeredThrottle; }
    bool joystickMode() const { return _joystickMode; }
    bool calibrating() const { return _calibrating; }
    bool singleStickDisplay() const { return _singleStickDisplay; }

    void setTransmitterMode(int mode);
    void setCenteredThrottle(bool centered);
    void setJoystickMode(bool joystickMode);

signals:
    void channelCountChanged(int channelCount);
    void rawChannelValueChanged(int channel, int value);
    void rollChannelMappedChanged(bool mapped);
    void pitchChannelMappedChanged(bool mapped);
    void rollExtensionChannelMappedChanged(bool mapped);
    void pitchExtensionChannelMappedChanged(bool mapped);
    void aux1ExtensionChannelMappedChanged(bool mapped);
    void aux2ExtensionChannelMappedChanged(bool mapped);
    void aux3ExtensionChannelMappedChanged(bool mapped);
    void aux4ExtensionChannelMappedChanged(bool mapped);
    void aux5ExtensionChannelMappedChanged(bool mapped);
    void aux6ExtensionChannelMappedChanged(bool mapped);
    void yawChannelMappedChanged(bool mapped);
    void throttleChannelMappedChanged(bool mapped);
    void pitchExtensionEnabledChanged(bool enabled);
    void rollExtensionEnabledChanged(bool enabled);
    void aux1ExtensionEnabledChanged(bool enabled);
    void aux2ExtensionEnabledChanged(bool enabled);
    void aux3ExtensionEnabledChanged(bool enabled);
    void aux4ExtensionEnabledChanged(bool enabled);
    void aux5ExtensionEnabledChanged(bool enabled);
    void aux6ExtensionEnabledChanged(bool enabled);
    void anyExtensionEnabledChanged(bool enabled);
    void adjustedRollChannelValueChanged(int rcValue);
    void adjustedPitchChannelValueChanged(int rcValue);
    void adjustedYawChannelValueChanged(int rcValue);
    void adjustedThrottleChannelValueChanged(int rcValue);
    void adjustedRollExtensionChannelValueChanged(int rcValue);
    void adjustedPitchExtensionChannelValueChanged(int rcValue);
    void adjustedAux1ExtensionChannelValueChanged(int rcValue);
    void adjustedAux2ExtensionChannelValueChanged(int rcValue);
    void adjustedAux3ExtensionChannelValueChanged(int rcValue);
    void adjustedAux4ExtensionChannelValueChanged(int rcValue);
    void adjustedAux5ExtensionChannelValueChanged(int rcValue);
    void adjustedAux6ExtensionChannelValueChanged(int rcValue);
    void rollChannelReversedChanged(bool reversed);
    void pitchChannelReversedChanged(bool reversed);
    void rollExtensionChannelReversedChanged(bool reversed);
    void pitchExtensionChannelReversedChanged(bool reversed);
    void aux1ExtensionChannelReversedChanged(bool reversed);
    void aux2ExtensionChannelReversedChanged(bool reversed);
    void aux3ExtensionChannelReversedChanged(bool reversed);
    void aux4ExtensionChannelReversedChanged(bool reversed);
    void aux5ExtensionChannelReversedChanged(bool reversed);
    void aux6ExtensionChannelReversedChanged(bool reversed);
    void yawChannelReversedChanged(bool reversed);
    void throttleChannelReversedChanged(bool reversed);
    void rollDeadbandChanged(int deadband);
    void pitchDeadbandChanged(int deadband);
    void rollExtensionDeadbandChanged(int deadband);
    void pitchExtensionDeadbandChanged(int deadband);
    void aux1ExtensionDeadbandChanged(int deadband);
    void aux2ExtensionDeadbandChanged(int deadband);
    void aux3ExtensionDeadbandChanged(int deadband);
    void aux4ExtensionDeadbandChanged(int deadband);
    void aux5ExtensionDeadbandChanged(int deadband);
    void aux6ExtensionDeadbandChanged(int deadband);
    void yawDeadbandChanged(int deadband);
    void throttleDeadbandChanged(int deadband);
    void transmitterModeChanged();
    void stickDisplayPositionsChanged();
    void centeredThrottleChanged(bool centeredThrottle);
    void joystickModeChanged(bool joystickMode);
    void calibratingChanged(bool calibrating);
    void singleStickDisplayChanged(bool singleStickDisplay);
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
    int _stickDetectSettleMSecs = 1000;      ///< Time in ms stick must be stable before detection completes

    int _chanCount = 0;                     ///< Number of actual rc channels available
    static constexpr int _chanMinimum = 4;  ///< Minimum numner of channels required to run

    /// Maps from StickFunction to channel index. _chanMax indicates channel not set for this function.
    int _rgFunctionChannelMapping[stickFunctionMax];

    void _validateAndAdjustCalibrationValues();
    QString _stickFunctionToString(StickFunction stickFunction);
    void _signalAllAttitudeValueChanges();
    void _resetInternalCalibrationValues(); ///< Resets internal calibration values to their initial state in preparation for a new calibration sequence.

    virtual bool _stickFunctionEnabled(StickFunction stickFunction); ///< Returns true if the stick function is enabled

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
        StateMachineStepExtensionHighHorz,
        StateMachineStepExtensionHighVert,
        StateMachineStepExtensionLowHorz,
        StateMachineStepExtensionLowVert,
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
    void _setSingleStickDisplay(bool singleStickDisplay);
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
    bool _singleStickDisplay = false;
    QMap<StateMachineStepFunction, QString> _stepFunctionToMsgStringMap;
    QMap<StateMachineStepFunction, QMap<int, BothSticksDisplayPositions>> _bothStickDisplayPositionThrottleCenteredMap;
    QMap<StateMachineStepFunction, QMap<int, BothSticksDisplayPositions>> _bothStickDisplayPositionThrottleDownMap;

    QList<StateMachineEntry> _stateMachine;

    static constexpr int _updateInterval = 150;             ///< Interval for timer which updates radio channel widgets

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
