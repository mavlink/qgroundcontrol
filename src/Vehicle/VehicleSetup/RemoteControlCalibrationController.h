#pragma once

#include <QtCore/QElapsedTimer>
#include <QtQmlIntegration/QtQmlIntegration>
#include <QtQuick/QQuickItem>

#include "FactPanelController.h"
#include "QGCMAVLink.h"

/// \brief Abstract base class for calibrating RC and Joystick controller.
///
class RemoteControlCalibrationController : public FactPanelController
{
    Q_OBJECT

    Q_PROPERTY(QQuickItem*  statusText                              MEMBER  _statusText                                 REQUIRED)
    Q_PROPERTY(QQuickItem*  cancelButton                            MEMBER  _cancelButton                               REQUIRED)
    Q_PROPERTY(QQuickItem*  nextButton                              MEMBER  _nextButton                                 REQUIRED)

    Q_PROPERTY(int          minChannelCount                         MEMBER  _chanMinimum                                CONSTANT)
    Q_PROPERTY(int          channelCount                            READ    channelCount                                NOTIFY channelCountChanged)
    Q_PROPERTY(int          channelValueMin                         MEMBER  _calDefaultMinValue                         CONSTANT)
    Q_PROPERTY(int          channelValueMax                         MEMBER  _calDefaultMaxValue                         CONSTANT)
    Q_PROPERTY(QList<int>   stickDisplayPositions                   READ    stickDisplayPositions                       NOTIFY stickDisplayPositionsChanged)
    Q_PROPERTY(bool         centeredThrottle                        READ    centeredThrottle WRITE setCenteredThrottle  NOTIFY centeredThrottleChanged)
    Q_PROPERTY(int          transmitterMode                         READ    transmitterMode WRITE setTransmitterMode    NOTIFY transmitterModeChanged)
    Q_PROPERTY(bool         joystickMode                            READ    joystickMode WRITE setJoystickMode          NOTIFY joystickModeChanged REQUIRED)
    Q_PROPERTY(bool         calibrating                             READ    calibrating                                 NOTIFY calibratingChanged)
    Q_PROPERTY(bool         singleStickDisplay                      READ    singleStickDisplay                          NOTIFY singleStickDisplayChanged)
    Q_PROPERTY(bool         oneSidedButtonVisible                   READ    oneSidedButtonVisible                       NOTIFY oneSidedButtonVisibleChanged)

    Q_PROPERTY(bool         rollChannelMapped                       READ    rollChannelMapped                           NOTIFY rollChannelMappedChanged)
    Q_PROPERTY(bool         pitchChannelMapped                      READ    pitchChannelMapped                          NOTIFY pitchChannelMappedChanged)
    Q_PROPERTY(bool         yawChannelMapped                        READ    yawChannelMapped                            NOTIFY yawChannelMappedChanged)
    Q_PROPERTY(bool         throttleChannelMapped                   READ    throttleChannelMapped                       NOTIFY throttleChannelMappedChanged)
    Q_PROPERTY(bool         rollExtensionChannelMapped              READ    rollExtensionChannelMapped                  NOTIFY rollExtensionChannelMappedChanged)
    Q_PROPERTY(bool         pitchExtensionChannelMapped             READ    pitchExtensionChannelMapped                 NOTIFY pitchExtensionChannelMappedChanged)
    Q_PROPERTY(bool         additionalAxis1ChannelMapped            READ    additionalAxis1ChannelMapped                NOTIFY additionalAxis1ChannelMappedChanged)
    Q_PROPERTY(bool         additionalAxis2ChannelMapped            READ    additionalAxis2ChannelMapped                NOTIFY additionalAxis2ChannelMappedChanged)
    Q_PROPERTY(bool         additionalAxis3ChannelMapped            READ    additionalAxis3ChannelMapped                NOTIFY additionalAxis3ChannelMappedChanged)
    Q_PROPERTY(bool         additionalAxis4ChannelMapped            READ    additionalAxis4ChannelMapped                NOTIFY additionalAxis4ChannelMappedChanged)
    Q_PROPERTY(bool         additionalAxis5ChannelMapped            READ    additionalAxis5ChannelMapped                NOTIFY additionalAxis5ChannelMappedChanged)
    Q_PROPERTY(bool         additionalAxis6ChannelMapped            READ    additionalAxis6ChannelMapped                NOTIFY additionalAxis6ChannelMappedChanged)

    Q_PROPERTY(bool         pitchExtensionEnabled                   READ    pitchExtensionEnabled                       NOTIFY pitchExtensionEnabledChanged)
    Q_PROPERTY(bool         rollExtensionEnabled                    READ    rollExtensionEnabled                        NOTIFY rollExtensionEnabledChanged)
    Q_PROPERTY(bool         additionalAxis1Enabled                  READ    additionalAxis1Enabled                      NOTIFY additionalAxis1EnabledChanged)
    Q_PROPERTY(bool         additionalAxis2Enabled                  READ    additionalAxis2Enabled                      NOTIFY additionalAxis2EnabledChanged)
    Q_PROPERTY(bool         additionalAxis3Enabled                  READ    additionalAxis3Enabled                      NOTIFY additionalAxis3EnabledChanged)
    Q_PROPERTY(bool         additionalAxis4Enabled                  READ    additionalAxis4Enabled                      NOTIFY additionalAxis4EnabledChanged)
    Q_PROPERTY(bool         additionalAxis5Enabled                  READ    additionalAxis5Enabled                      NOTIFY additionalAxis5EnabledChanged)
    Q_PROPERTY(bool         additionalAxis6Enabled                  READ    additionalAxis6Enabled                      NOTIFY additionalAxis6EnabledChanged)

    Q_PROPERTY(int          adjustedRollChannelValue                READ    adjustedRollChannelValue                    NOTIFY adjustedRollChannelValueChanged)
    Q_PROPERTY(int          adjustedPitchChannelValue               READ    adjustedPitchChannelValue                   NOTIFY adjustedPitchChannelValueChanged)
    Q_PROPERTY(int          adjustedYawChannelValue                 READ    adjustedYawChannelValue                     NOTIFY adjustedYawChannelValueChanged)
    Q_PROPERTY(int          adjustedThrottleChannelValue            READ    adjustedThrottleChannelValue                NOTIFY adjustedThrottleChannelValueChanged)
    Q_PROPERTY(int          adjustedRollExtensionChannelValue       READ    adjustedRollExtensionChannelValue           NOTIFY adjustedRollExtensionChannelValueChanged)
    Q_PROPERTY(int          adjustedPitchExtensionChannelValue      READ    adjustedPitchExtensionChannelValue          NOTIFY adjustedPitchExtensionChannelValueChanged)
    Q_PROPERTY(int          adjustedAdditionalAxis1ChannelValue     READ    adjustedAdditionalAxis1ChannelValue         NOTIFY adjustedAdditionalAxis1ChannelValueChanged)
    Q_PROPERTY(int          adjustedAdditionalAxis2ChannelValue     READ    adjustedAdditionalAxis2ChannelValue         NOTIFY adjustedAdditionalAxis2ChannelValueChanged)
    Q_PROPERTY(int          adjustedAdditionalAxis3ChannelValue     READ    adjustedAdditionalAxis3ChannelValue         NOTIFY adjustedAdditionalAxis3ChannelValueChanged)
    Q_PROPERTY(int          adjustedAdditionalAxis4ChannelValue     READ    adjustedAdditionalAxis4ChannelValue         NOTIFY adjustedAdditionalAxis4ChannelValueChanged)
    Q_PROPERTY(int          adjustedAdditionalAxis5ChannelValue     READ    adjustedAdditionalAxis5ChannelValue         NOTIFY adjustedAdditionalAxis5ChannelValueChanged)
    Q_PROPERTY(int          adjustedAdditionalAxis6ChannelValue     READ    adjustedAdditionalAxis6ChannelValue         NOTIFY adjustedAdditionalAxis6ChannelValueChanged)

    Q_PROPERTY(int          rollChannelReversed                     READ    rollChannelReversed                         NOTIFY rollChannelReversedChanged)
    Q_PROPERTY(int          pitchChannelReversed                    READ    pitchChannelReversed                        NOTIFY pitchChannelReversedChanged)
    Q_PROPERTY(int          yawChannelReversed                      READ    yawChannelReversed                          NOTIFY yawChannelReversedChanged)
    Q_PROPERTY(int          throttleChannelReversed                 READ    throttleChannelReversed                     NOTIFY throttleChannelReversedChanged)
    Q_PROPERTY(int          rollExtensionChannelReversed            READ    rollExtensionChannelReversed                NOTIFY rollExtensionChannelReversedChanged)
    Q_PROPERTY(int          pitchExtensionChannelReversed           READ    pitchExtensionChannelReversed               NOTIFY pitchExtensionChannelReversedChanged)
    Q_PROPERTY(int          additionalAxis1ChannelReversed          READ    additionalAxis1ChannelReversed              NOTIFY additionalAxis1ChannelReversedChanged)
    Q_PROPERTY(int          additionalAxis2ChannelReversed          READ    additionalAxis2ChannelReversed              NOTIFY additionalAxis2ChannelReversedChanged)
    Q_PROPERTY(int          additionalAxis3ChannelReversed          READ    additionalAxis3ChannelReversed              NOTIFY additionalAxis3ChannelReversedChanged)
    Q_PROPERTY(int          additionalAxis4ChannelReversed          READ    additionalAxis4ChannelReversed              NOTIFY additionalAxis4ChannelReversedChanged)
    Q_PROPERTY(int          additionalAxis5ChannelReversed          READ    additionalAxis5ChannelReversed              NOTIFY additionalAxis5ChannelReversedChanged)
    Q_PROPERTY(int          additionalAxis6ChannelReversed          READ    additionalAxis6ChannelReversed              NOTIFY additionalAxis6ChannelReversedChanged)

    Q_PROPERTY(int          rollDeadband                            READ    rollDeadband                                NOTIFY rollDeadbandChanged)
    Q_PROPERTY(int          pitchDeadband                           READ    pitchDeadband                               NOTIFY pitchDeadbandChanged)
    Q_PROPERTY(int          rollExtensionDeadband                   READ    rollExtensionDeadband                       NOTIFY rollExtensionDeadbandChanged)
    Q_PROPERTY(int          yawDeadband                             READ    yawDeadband                                 NOTIFY yawDeadbandChanged)
    Q_PROPERTY(int          throttleDeadband                        READ    throttleDeadband                            NOTIFY throttleDeadbandChanged)
    Q_PROPERTY(int          pitchExtensionDeadband                  READ    pitchExtensionDeadband                      NOTIFY pitchExtensionDeadbandChanged)
    Q_PROPERTY(int          additionalAxis1Deadband                 READ    additionalAxis1Deadband                     NOTIFY additionalAxis1DeadbandChanged)
    Q_PROPERTY(int          additionalAxis2Deadband                 READ    additionalAxis2Deadband                     NOTIFY additionalAxis2DeadbandChanged)
    Q_PROPERTY(int          additionalAxis3Deadband                 READ    additionalAxis3Deadband                     NOTIFY additionalAxis3DeadbandChanged)
    Q_PROPERTY(int          additionalAxis4Deadband                 READ    additionalAxis4Deadband                     NOTIFY additionalAxis4DeadbandChanged)
    Q_PROPERTY(int          additionalAxis5Deadband                 READ    additionalAxis5Deadband                     NOTIFY additionalAxis5DeadbandChanged)
    Q_PROPERTY(int          additionalAxis6Deadband                 READ    additionalAxis6Deadband                     NOTIFY additionalAxis6DeadbandChanged)

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
        stickFunctionAdditionalAxis1,
        stickFunctionAdditionalAxis2,
        stickFunctionAdditionalAxis3,
        stickFunctionAdditionalAxis4,
        stickFunctionAdditionalAxis5,
        stickFunctionAdditionalAxis6,
        stickFunctionMax,
    };

    Q_INVOKABLE void cancelButtonClicked();
    Q_INVOKABLE void nextButtonClicked();
    Q_INVOKABLE void oneSidedButtonClicked();
    virtual Q_INVOKABLE void start();
    Q_INVOKABLE void copyTrims();

    int adjustedRollChannelValue();
    int adjustedPitchChannelValue();
    int adjustedYawChannelValue();
    int adjustedThrottleChannelValue();
    int adjustedRollExtensionChannelValue();
    int adjustedPitchExtensionChannelValue();
    int adjustedAdditionalAxis1ChannelValue();
    int adjustedAdditionalAxis2ChannelValue();
    int adjustedAdditionalAxis3ChannelValue();
    int adjustedAdditionalAxis4ChannelValue();
    int adjustedAdditionalAxis5ChannelValue();
    int adjustedAdditionalAxis6ChannelValue();
    bool rollChannelMapped();
    bool pitchChannelMapped();
    bool rollExtensionChannelMapped();
    bool pitchExtensionChannelMapped();
    bool additionalAxis1ChannelMapped();
    bool additionalAxis2ChannelMapped();
    bool additionalAxis3ChannelMapped();
    bool additionalAxis4ChannelMapped();
    bool additionalAxis5ChannelMapped();
    bool additionalAxis6ChannelMapped();
    bool yawChannelMapped();
    bool throttleChannelMapped();
    bool pitchExtensionEnabled();
    bool rollExtensionEnabled();
    bool additionalAxis1Enabled();
    bool additionalAxis2Enabled();
    bool additionalAxis3Enabled();
    bool additionalAxis4Enabled();
    bool additionalAxis5Enabled();
    bool additionalAxis6Enabled();
    bool rollChannelReversed();
    bool pitchChannelReversed();
    bool rollExtensionChannelReversed();
    bool pitchExtensionChannelReversed();
    bool additionalAxis1ChannelReversed();
    bool additionalAxis2ChannelReversed();
    bool additionalAxis3ChannelReversed();
    bool additionalAxis4ChannelReversed();
    bool additionalAxis5ChannelReversed();
    bool additionalAxis6ChannelReversed();
    bool yawChannelReversed();
    bool throttleChannelReversed();
    int rollDeadband();
    int pitchDeadband();
    int rollExtensionDeadband();
    int pitchExtensionDeadband();
    int additionalAxis1Deadband();
    int additionalAxis2Deadband();
    int additionalAxis3Deadband();
    int additionalAxis4Deadband();
    int additionalAxis5Deadband();
    int additionalAxis6Deadband();
    int yawDeadband();
    int throttleDeadband();
    int channelCount() const { return _chanCount; }
    int transmitterMode() const { return _transmitterMode; }
    QList<int> stickDisplayPositions() const { return _stickDisplayPositions; }
    bool centeredThrottle() const { return _centeredThrottle; }
    bool joystickMode() const { return _joystickMode; }
    bool calibrating() const { return _calibrating; }
    bool singleStickDisplay() const { return _singleStickDisplay; }
    bool oneSidedButtonVisible() const;

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
    void additionalAxis1ChannelMappedChanged(bool mapped);
    void additionalAxis2ChannelMappedChanged(bool mapped);
    void additionalAxis3ChannelMappedChanged(bool mapped);
    void additionalAxis4ChannelMappedChanged(bool mapped);
    void additionalAxis5ChannelMappedChanged(bool mapped);
    void additionalAxis6ChannelMappedChanged(bool mapped);
    void yawChannelMappedChanged(bool mapped);
    void throttleChannelMappedChanged(bool mapped);
    void pitchExtensionEnabledChanged(bool enabled);
    void rollExtensionEnabledChanged(bool enabled);
    void additionalAxis1EnabledChanged(bool enabled);
    void additionalAxis2EnabledChanged(bool enabled);
    void additionalAxis3EnabledChanged(bool enabled);
    void additionalAxis4EnabledChanged(bool enabled);
    void additionalAxis5EnabledChanged(bool enabled);
    void additionalAxis6EnabledChanged(bool enabled);
    void adjustedRollChannelValueChanged(int rcValue);
    void adjustedPitchChannelValueChanged(int rcValue);
    void adjustedYawChannelValueChanged(int rcValue);
    void adjustedThrottleChannelValueChanged(int rcValue);
    void adjustedRollExtensionChannelValueChanged(int rcValue);
    void adjustedPitchExtensionChannelValueChanged(int rcValue);
    void adjustedAdditionalAxis1ChannelValueChanged(int rcValue);
    void adjustedAdditionalAxis2ChannelValueChanged(int rcValue);
    void adjustedAdditionalAxis3ChannelValueChanged(int rcValue);
    void adjustedAdditionalAxis4ChannelValueChanged(int rcValue);
    void adjustedAdditionalAxis5ChannelValueChanged(int rcValue);
    void adjustedAdditionalAxis6ChannelValueChanged(int rcValue);
    void rollChannelReversedChanged(bool reversed);
    void pitchChannelReversedChanged(bool reversed);
    void rollExtensionChannelReversedChanged(bool reversed);
    void pitchExtensionChannelReversedChanged(bool reversed);
    void additionalAxis1ChannelReversedChanged(bool reversed);
    void additionalAxis2ChannelReversedChanged(bool reversed);
    void additionalAxis3ChannelReversedChanged(bool reversed);
    void additionalAxis4ChannelReversedChanged(bool reversed);
    void additionalAxis5ChannelReversedChanged(bool reversed);
    void additionalAxis6ChannelReversedChanged(bool reversed);
    void yawChannelReversedChanged(bool reversed);
    void throttleChannelReversedChanged(bool reversed);
    void rollDeadbandChanged(int deadband);
    void pitchDeadbandChanged(int deadband);
    void rollExtensionDeadbandChanged(int deadband);
    void pitchExtensionDeadbandChanged(int deadband);
    void additionalAxis1DeadbandChanged(int deadband);
    void additionalAxis2DeadbandChanged(int deadband);
    void additionalAxis3DeadbandChanged(int deadband);
    void additionalAxis4DeadbandChanged(int deadband);
    void additionalAxis5DeadbandChanged(int deadband);
    void additionalAxis6DeadbandChanged(int deadband);
    void yawDeadbandChanged(int deadband);
    void throttleDeadbandChanged(int deadband);
    void transmitterModeChanged();
    void stickDisplayPositionsChanged();
    void centeredThrottleChanged(bool centeredThrottle);
    void joystickModeChanged(bool joystickMode);
    void calibratingChanged(bool calibrating);
    void singleStickDisplayChanged(bool singleStickDisplay);
    void oneSidedButtonVisibleChanged(bool visible);
    void calibrationCompleted();

public slots:
    void _rawChannelValuesChanged(QVector<int> channelValues) { _processChannelValues(channelValues); }
    void _clampedChannelValuesChanged(QVector<int> channelValues) { _processChannelValues(channelValues); }

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
    void _processChannelValues(QVector<int> channelValues);

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
    void _applyOneSidedCalibration();
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
    bool _isOneSidedCalibrationStep(int step) const;

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
    QMap<StateMachineStepFunction, const char*> _stepFunctionToMsgStringMap;
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
