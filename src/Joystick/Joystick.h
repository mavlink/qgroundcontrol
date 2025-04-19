/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "MAVLinkLib.h"

#include <QtCore/QLoggingCategory>
#include <QtCore/QObject>
#include <QtCore/QThread>
#include <QtQmlIntegration/QtQmlIntegration>

Q_DECLARE_LOGGING_CATEGORY(JoystickLog)
Q_DECLARE_LOGGING_CATEGORY(JoystickValuesLog)

class MavlinkActionManager;
class QmlObjectListModel;
class Vehicle;

/*===========================================================================*/

class AssignedButtonAction
{
public:
    AssignedButtonAction(const QString &name);

    QString action;
    QElapsedTimer buttonTime;
    bool repeat = false;
};

// TODO: Q_GADGET
class AssignableButtonAction : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString  action      READ action     CONSTANT)
    Q_PROPERTY(bool     canRepeat   READ canRepeat  CONSTANT)

public:
    AssignableButtonAction(const QString &action_, bool canRepeat_ = false, QObject *parent = nullptr);

    const QString &action() const { return _action; }
    bool canRepeat() const { return _repeat; }

private:
    const QString _action;
    const bool _repeat = false;
};

/*===========================================================================*/

class Joystick : public QThread
{
    Q_OBJECT
    // QML_ELEMENT
    // QML_UNCREATABLE("")
    Q_MOC_INCLUDE("QmlObjectListModel.h")
    Q_MOC_INCLUDE("Vehicle.h")
    Q_PROPERTY(bool                     accumulator             READ    accumulator             WRITE setAccumulator        NOTIFY accumulatorChanged)
    Q_PROPERTY(bool                     calibrated              MEMBER  _calibrated                                         NOTIFY calibratedChanged)
    Q_PROPERTY(bool                     circleCorrection        READ    circleCorrection        WRITE setCircleCorrection   NOTIFY circleCorrectionChanged)
    Q_PROPERTY(bool                     negativeThrust          READ    negativeThrust          WRITE setNegativeThrust     NOTIFY negativeThrustChanged)
    Q_PROPERTY(bool                     requiresCalibration     READ    requiresCalibration                                 CONSTANT)
    Q_PROPERTY(float                    axisFrequencyHz         READ    axisFrequencyHz         WRITE setAxisFrequency      NOTIFY axisFrequencyHzChanged)
    Q_PROPERTY(float                    buttonFrequencyHz       READ    buttonFrequencyHz       WRITE setButtonFrequency    NOTIFY buttonFrequencyHzChanged)
    Q_PROPERTY(float                    exponential             READ    exponential             WRITE setExponential        NOTIFY exponentialChanged)
    Q_PROPERTY(float                    maxAxisFrequencyHz      MEMBER  _maxAxisFrequencyHz                                 CONSTANT)
    Q_PROPERTY(float                    maxButtonFrequencyHz    MEMBER  _maxButtonFrequencyHz                               CONSTANT)
    Q_PROPERTY(float                    minAxisFrequencyHz      MEMBER  _minAxisFrequencyHz                                 CONSTANT)
    Q_PROPERTY(float                    minButtonFrequencyHz    MEMBER  _minButtonFrequencyHz                               CONSTANT)
    Q_PROPERTY(int                      axisCount               READ    axisCount                                           CONSTANT)
    Q_PROPERTY(int                      throttleMode            READ    throttleMode            WRITE setThrottleMode       NOTIFY throttleModeChanged)
    Q_PROPERTY(int                      totalButtonCount        READ    totalButtonCount                                    CONSTANT)
    Q_PROPERTY(const QmlObjectListModel *assignableActions      READ    assignableActions                                   NOTIFY assignableActionsChanged)
    Q_PROPERTY(QString                  disabledActionName      READ    disabledActionName                                  CONSTANT)
    Q_PROPERTY(QString                  name                    READ    name                                                CONSTANT)
    Q_PROPERTY(QStringList              assignableActionTitles  READ    assignableActionTitles                              NOTIFY assignableActionsChanged)
    Q_PROPERTY(QStringList              buttonActions           READ    buttonActions                                       NOTIFY buttonActionsChanged)

    enum ButtonEvent_t {
        BUTTON_UP,
        BUTTON_DOWN,
        BUTTON_REPEAT
    };

public:
    Joystick(const QString &name, int axisCount, int buttonCount, int hatCount, QObject *parent = nullptr);
    virtual ~Joystick();

    struct Calibration_t {
        int min = -32767;
        int max = 32767;
        int center = 0;
        int deadband = 0;
        bool reversed = false;
    };

    enum AxisFunction_t {
        rollFunction,
        pitchFunction,
        yawFunction,
        throttleFunction,
        gimbalPitchFunction,
        gimbalYawFunction,
        maxFunction
    };

    enum ThrottleMode_t {
        ThrottleModeCenterZero,
        ThrottleModeDownZero,
        ThrottleModeMax
    };

    Q_INVOKABLE void setButtonRepeat(int button, bool repeat);
    Q_INVOKABLE bool getButtonRepeat(int button);
    Q_INVOKABLE void setButtonAction(int button, const QString &action);
    Q_INVOKABLE QString getButtonAction(int button) const;

    QString name() const { return _name; }
    int totalButtonCount() const { return _totalButtonCount; }
    int axisCount() const { return _axisCount; }
    QStringList buttonActions() const;
    const QmlObjectListModel *assignableActions() const { return _assignableButtonActions; }
    QStringList assignableActionTitles() const { return _availableActionTitles; }
    QString disabledActionName() const { return _buttonActionNone; }

    void stop();

    /// Start the polling thread which will in turn emit joystick signals
    void startPolling(Vehicle *vehicle);
    void stopPolling();

    void setCalibration(int axis, const Calibration_t &calibration);
    Calibration_t getCalibration(int axis) const;

    void setFunctionAxis(AxisFunction_t function, int axis);
    int getFunctionAxis(AxisFunction_t function) const;

    // Joystick index used by sdl library
    // Settable because sdl library remaps indices after certain events
    // virtual int index(void) = 0;
    // virtual void setIndex(int index) = 0;

	virtual bool requiresCalibration() const { return true; }

    int throttleMode() const { return _throttleMode; }
    void setThrottleMode(int mode);

    bool negativeThrust() const { return _negativeThrust; }
    void setNegativeThrust(bool allowNegative);

    float exponential() const { return _exponential; }
    void setExponential(float expo);

    bool accumulator() const { return _accumulator; }
    void setAccumulator(bool accu);

    bool deadband() const { return _deadband; }
    void setDeadband(bool accu);

    bool circleCorrection() const { return _circleCorrection; }
    void setCircleCorrection(bool circleCorrection);

    int getTXMode() const { return _transmitterMode; }
    void setTXMode(int mode);

    /// Set the current calibration mode
    void setCalibrationMode(bool calibrating);

    /// Get joystick message rate (in Hz)
    float axisFrequencyHz() const { return _axisFrequencyHz; }
    /// Set joystick message rate (in Hz)
    void setAxisFrequency(float val);

    /// Get joystick button repeat rate (in Hz)
    float buttonFrequencyHz() const { return _buttonFrequencyHz; }
    /// Set joystick button repeat rate (in Hz)
    void setButtonFrequency(float val);

signals:
    // The raw signals are only meant for use by calibration
    void rawAxisValueChanged(int index, int value);
    void rawButtonPressedChanged(int index, int pressed);
    void calibratedChanged(bool calibrated);
    void buttonActionsChanged();
    void assignableActionsChanged();
    void throttleModeChanged(int mode);
    void negativeThrustChanged(bool allowNegative);
    void exponentialChanged(float exponential);
    void accumulatorChanged(bool accumulator);
    void enabledChanged(bool enabled);
    void circleCorrectionChanged(bool circleCorrection);
    void axisValues(float roll, float pitch, float yaw, float throttle);
    void axisFrequencyHzChanged();
    void buttonFrequencyHzChanged();
    void startContinuousZoom(int direction);
    void stopContinuousZoom();
    void stepZoom(int direction);
    void stepCamera(int direction);
    void stepStream(int direction);
    void triggerCamera();
    void startVideoRecord();
    void stopVideoRecord();
    void toggleVideoRecord();
    void gimbalPitchStart(int direction);
    void gimbalYawStart(int direction);
    void gimbalPitchStop();
    void gimbalYawStop();
    void centerGimbal();
    void gimbalYawLock(bool lock);
    void setArmed(bool arm);
    void setVtolInFwdFlight(bool set);
    void setFlightMode(const QString &flightMode);
    void emergencyStop();
    void gripperAction(GRIPPER_ACTIONS gripperAction);
    void landingGearDeploy();
    void landingGearRetract();
    void unknownAction(const QString &action);

protected:
    void _setDefaultCalibration();

    QString _name;
    int _axisCount = 0;
    int _buttonCount = 0;
    int _hatCount = 0;

private slots:
    void _activeVehicleChanged(Vehicle *activeVehicle);
    void _vehicleCountChanged(int count);
    void _flightModesChanged() { _buildActionList(_activeVehicle); }

private:
    virtual bool _open() = 0;
    virtual void _close() = 0;
    virtual bool _update() = 0;

    virtual bool _getButton(int i) const = 0;
    virtual int _getAxis(int i) const = 0;
    virtual bool _getHat(int hat, int i) const = 0;

    void run() override;

    void _saveSettings();
    void _saveButtonSettings();
    void _loadSettings();

    /// Adjust the raw axis value to the -1:1 range given calibration information
    float _adjustRange(int value, const Calibration_t &calibration, bool withDeadbands);
    void _executeButtonAction(const QString &action, bool buttonDown);
    int  _findAssignableButtonAction(const QString &action);
    bool _validAxis(int axis) const;
    bool _validButton(int button) const;
    void _handleAxis();
    void _handleButtons();
    void _buildActionList(Vehicle *activeVehicle);

    void _updateTXModeSettingsKey(Vehicle *activeVehicle);

    /// Relative mappings of axis functions between different TX modes
    int _mapFunctionMode(int mode, int function);

    /// Remap current axis functions from current TX mode to new TX mode
    void _remapAxes(int currentMode, int newMode, int (&newMapping)[maxFunction]);

    int _hatButtonCount = 0;
    int _totalButtonCount = 0;
    int *_rgAxisValues = nullptr;
    Calibration_t *_rgCalibration = nullptr;
    uint8_t *_rgButtonValues = nullptr;
    MavlinkActionManager *_mavlinkActionManager = nullptr;
    QmlObjectListModel *_assignableButtonActions = nullptr;

    bool _accumulator = false;
    bool _calibrated = false;
    bool _calibrationMode = false;
    bool _circleCorrection = true;
    bool _deadband = false;
    bool _negativeThrust = false;
    bool _pollingStartedForCalibration = false;
    float _axisFrequencyHz = _defaultAxisFrequencyHz;
    float _buttonFrequencyHz = _defaultButtonFrequencyHz;
    float _exponential = 0;
    int _rgFunctionAxis[maxFunction] = {};
    QElapsedTimer _axisTime;
    QList<AssignedButtonAction*> _buttonActionArray;
    QStringList _availableActionTitles;
    std::atomic<bool> _exitThread = false;    ///< true: signal thread to exit
    ThrottleMode_t _throttleMode = ThrottleModeDownZero;
    Vehicle *_activeVehicle = nullptr;
    const char *_txModeSettingsKey = nullptr;

    static int _transmitterMode;

    static constexpr float _defaultAxisFrequencyHz = 25.0f;
    static constexpr float _defaultButtonFrequencyHz = 5.0f;
    // Arbitrary Limits
    static constexpr float _minAxisFrequencyHz = 0.25f;
    static constexpr float _maxAxisFrequencyHz = 200.0f;
    static constexpr float _minButtonFrequencyHz = 0.25f;
    static constexpr float _maxButtonFrequencyHz = 50.0f;

    static constexpr const char *_rgFunctionSettingsKey[maxFunction] = {
        "RollAxis",
        "PitchAxis",
        "YawAxis",
        "ThrottleAxis",
        "GimbalPitchAxis",
        "GimbalYawAxis"
    };

    static constexpr const char *_settingsGroup =                  "Joysticks";
    static constexpr const char *_calibratedSettingsKey =          "Calibrated4"; // Increment number to force recalibration
    static constexpr const char *_buttonActionNameKey =            "ButtonActionName%1";
    static constexpr const char *_buttonActionRepeatKey =          "ButtonActionRepeat%1";
    static constexpr const char *_throttleModeSettingsKey =        "ThrottleMode";
    static constexpr const char *_negativeThrustSettingsKey =      "NegativeThrust";
    static constexpr const char *_exponentialSettingsKey =         "Exponential";
    static constexpr const char *_accumulatorSettingsKey =         "Accumulator";
    static constexpr const char *_deadbandSettingsKey =            "Deadband";
    static constexpr const char *_circleCorrectionSettingsKey =    "Circle_Correction";
    static constexpr const char *_axisFrequencySettingsKey =       "AxisFrequency";
    static constexpr const char *_buttonFrequencySettingsKey =     "ButtonFrequency";
    static constexpr const char *_fixedWingTXModeSettingsKey =     "TXMode_FixedWing";
    static constexpr const char *_multiRotorTXModeSettingsKey =    "TXMode_MultiRotor";
    static constexpr const char *_roverTXModeSettingsKey =         "TXMode_Rover";
    static constexpr const char *_vtolTXModeSettingsKey =          "TXMode_VTOL";
    static constexpr const char *_submarineTXModeSettingsKey =     "TXMode_Submarine";

    static constexpr const char *_buttonActionNone =               QT_TR_NOOP("No Action");
    static constexpr const char *_buttonActionArm =                QT_TR_NOOP("Arm");
    static constexpr const char *_buttonActionDisarm =             QT_TR_NOOP("Disarm");
    static constexpr const char *_buttonActionToggleArm =          QT_TR_NOOP("Toggle Arm");
    static constexpr const char *_buttonActionVTOLFixedWing =      QT_TR_NOOP("VTOL: Fixed Wing");
    static constexpr const char *_buttonActionVTOLMultiRotor =     QT_TR_NOOP("VTOL: Multi-Rotor");
    static constexpr const char *_buttonActionContinuousZoomIn =   QT_TR_NOOP("Continuous Zoom In");
    static constexpr const char *_buttonActionContinuousZoomOut =  QT_TR_NOOP("Continuous Zoom Out");
    static constexpr const char *_buttonActionStepZoomIn =         QT_TR_NOOP("Step Zoom In");
    static constexpr const char *_buttonActionStepZoomOut =        QT_TR_NOOP("Step Zoom Out");
    static constexpr const char *_buttonActionNextStream =         QT_TR_NOOP("Next Video Stream");
    static constexpr const char *_buttonActionPreviousStream =     QT_TR_NOOP("Previous Video Stream");
    static constexpr const char *_buttonActionNextCamera =         QT_TR_NOOP("Next Camera");
    static constexpr const char *_buttonActionPreviousCamera =     QT_TR_NOOP("Previous Camera");
    static constexpr const char *_buttonActionTriggerCamera =      QT_TR_NOOP("Trigger Camera");
    static constexpr const char *_buttonActionStartVideoRecord =   QT_TR_NOOP("Start Recording Video");
    static constexpr const char *_buttonActionStopVideoRecord =    QT_TR_NOOP("Stop Recording Video");
    static constexpr const char *_buttonActionToggleVideoRecord =  QT_TR_NOOP("Toggle Recording Video");
    static constexpr const char *_buttonActionGimbalDown =         QT_TR_NOOP("Gimbal Down");
    static constexpr const char *_buttonActionGimbalUp =           QT_TR_NOOP("Gimbal Up");
    static constexpr const char *_buttonActionGimbalLeft =         QT_TR_NOOP("Gimbal Left");
    static constexpr const char *_buttonActionGimbalRight =        QT_TR_NOOP("Gimbal Right");
    static constexpr const char *_buttonActionGimbalCenter =       QT_TR_NOOP("Gimbal Center");
    static constexpr const char *_buttonActionGimbalYawLock =      QT_TR_NOOP("Gimbal Yaw Lock");
    static constexpr const char *_buttonActionGimbalYawFollow =    QT_TR_NOOP("Gimbal Yaw Follow");
    static constexpr const char *_buttonActionEmergencyStop =      QT_TR_NOOP("Emergency Stop");
    static constexpr const char *_buttonActionGripperGrab =        QT_TR_NOOP("Gripper Close");
    static constexpr const char *_buttonActionGripperRelease =     QT_TR_NOOP("Gripper Open");
    static constexpr const char *_buttonActionLandingGearDeploy=   QT_TR_NOOP("Landing gear deploy");
    static constexpr const char *_buttonActionLandingGearRetract=  QT_TR_NOOP("Landing gear retract");
};
