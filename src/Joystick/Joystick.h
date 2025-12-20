/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "QGCMAVLink.h"

#include <QtCore/QLoggingCategory>
#include <QtCore/QObject>
#include <QtCore/QThread>
#include <QtQmlIntegration/QtQmlIntegration>

#include "RemoteControlCalibrationController.h"
#include "Fact.h"

Q_DECLARE_LOGGING_CATEGORY(JoystickLog)
Q_DECLARE_LOGGING_CATEGORY(JoystickValuesLog)

class MavlinkActionManager;
class QmlObjectListModel;
class Vehicle;
class JoystickManager;
class JoystickConfigController;

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

// There is only one Joystick instance active in the system at a time.
// You get access to it through JoystickManager.
// It is associated with a specific Vehicle instance.
class Joystick : public QThread
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")
    Q_MOC_INCLUDE("QmlObjectListModel.h")
    Q_MOC_INCLUDE("Vehicle.h")

    Q_PROPERTY(bool                     throttleSmoothing       READ    throttleSmoothing       WRITE setThrottleSmoothing  NOTIFY throttleSmoothingChanged)
    Q_PROPERTY(bool                     calibrated              MEMBER  _calibrated                                         NOTIFY calibratedChanged)
    Q_PROPERTY(bool                     circleCorrection        READ    circleCorrection        WRITE setCircleCorrection   NOTIFY circleCorrectionChanged)
    Q_PROPERTY(bool                     useDeadband             READ    useDeadband             WRITE setUseDeadband        NOTIFY useDeadbandChanged)
    Q_PROPERTY(bool                     negativeThrust          READ    negativeThrust          WRITE setNegativeThrust     NOTIFY negativeThrustChanged)
    Q_PROPERTY(bool                     requiresCalibration     READ    requiresCalibration                                 CONSTANT)
    Q_PROPERTY(double                   axisFrequencyHz         READ    axisFrequencyHz         WRITE setAxisFrequency      NOTIFY axisFrequencyHzChanged)
    Q_PROPERTY(double                   buttonFrequencyHz       READ    buttonFrequencyHz       WRITE setButtonFrequency    NOTIFY buttonFrequencyHzChanged)
    Q_PROPERTY(Fact*                    axisFrequencyHzFact     READ    axisFrequencyHzFact                                 CONSTANT)
    Q_PROPERTY(Fact*                    buttonFrequencyHzFact   READ    buttonFrequencyHzFact                               CONSTANT)
    Q_PROPERTY(Fact*                    exponentialPctFact      READ    exponentialPctFact                                  CONSTANT)
    Q_PROPERTY(double                   maxAxisFrequencyHz      MEMBER  _maxAxisFrequencyHz                                 CONSTANT)
    Q_PROPERTY(double                   maxButtonFrequencyHz    MEMBER  _maxButtonFrequencyHz                               CONSTANT)
    Q_PROPERTY(double                   minAxisFrequencyHz      MEMBER  _minAxisFrequencyHz                                 CONSTANT)
    Q_PROPERTY(double                   minButtonFrequencyHz    MEMBER  _minButtonFrequencyHz                               CONSTANT)
    Q_PROPERTY(int                      axisCount               READ    axisCount                                           CONSTANT)
    Q_PROPERTY(bool                     throttleModeCenterZero  READ    throttleModeCenterZero  WRITE setThrottleModeCenterZero NOTIFY throttleModeCenterZeroChanged)
    Q_PROPERTY(int                      buttonCount             READ    buttonCount                                         CONSTANT)
    Q_PROPERTY(const QmlObjectListModel *assignableActions      READ    assignableActions                                   NOTIFY assignableActionsChanged)
    Q_PROPERTY(QString                  disabledActionName      READ    disabledActionName                                  CONSTANT)
    Q_PROPERTY(QString                  name                    READ    name                                                CONSTANT)
    Q_PROPERTY(QStringList              assignableActionTitles  READ    assignableActionTitles                              NOTIFY assignableActionsChanged)
    Q_PROPERTY(QStringList              buttonActions           READ    buttonActions                                       NOTIFY buttonActionsChanged)
    Q_PROPERTY(QString                  buttonActionNone        READ    buttonActionNone                                    CONSTANT)
    Q_PROPERTY(bool                     enableManualControlExtensions READ enableManualControlExtensions WRITE setEnableManualControlExtensions NOTIFY enableManualControlExtensionsChanged)
    Q_PROPERTY(int                      transmitterMode         READ    transmitterMode          WRITE setTransmitterMode   NOTIFY transmitterModeChanged)

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
        maxAxisFunction
    };
    static QString axisFunctionToString(AxisFunction_t function);

    Q_INVOKABLE void setButtonRepeat(int button, bool repeat);
    Q_INVOKABLE bool getButtonRepeat(int button);
    Q_INVOKABLE void setButtonAction(int button, const QString &action);
    Q_INVOKABLE QString getButtonAction(int button) const;

    QString name() const { return _name; }
    int buttonCount() const { return _totalButtonCount; }
    int axisCount() const { return _axisCount; }
    bool isCalibrated() const { return _calibrated; }
    QStringList buttonActions() const;
    QString buttonActionNone() const { return _buttonActionNone; }
    const QmlObjectListModel *assignableActions() const { return _assignableButtonActions; }
    QStringList assignableActionTitles() const { return _availableActionTitles; }
    QString disabledActionName() const { return _buttonActionNone; }

    void setCalibration(int axis, const Calibration_t &calibration);
    Calibration_t getCalibration(int axis) const;

    void setFunctionAxis(AxisFunction_t function, int axis);
    int getFunctionAxis(AxisFunction_t function) const;

    RemoteControlCalibrationController::StickFunction mapAxisFunctionToRCCStickFunction(AxisFunction_t axisFunction) const;
    AxisFunction_t mapRCCStickFunctionToAxisFunction(RemoteControlCalibrationController::StickFunction stickFunction) const;

    void setFunctionForChannel(RemoteControlCalibrationController::StickFunction stickFunction, int channel);
    int getChannelForFunction(RemoteControlCalibrationController::StickFunction stickFunction) const ;

    // Joystick index used by sdl library
    // Settable because sdl library remaps indices after certain events
    // virtual int index(void) = 0;
    // virtual void setIndex(int index) = 0;

	virtual bool requiresCalibration() const { return true; }

    bool throttleModeCenterZero() const { return _throttleModeCenterZero; }
    void setThrottleModeCenterZero(bool throttleCenterZero);

    bool negativeThrust() const { return _negativeThrust; }
    void setNegativeThrust(bool allowNegative);

    bool throttleSmoothing() const { return _throttleSmoothing; }
    void setThrottleSmoothing(bool enabled);

    bool useDeadband() const { return _useDeadband; }
    void setUseDeadband(bool accu);

    bool circleCorrection() const { return _circleCorrection; }
    void setCircleCorrection(bool circleCorrection);

    /// Get joystick message rate (in Hz)
    double axisFrequencyHz() const { return _axisFrequencyHzFact.rawValue().toDouble(); }
    /// Set joystick message rate (in Hz)
    void setAxisFrequency(double val);

    /// Get joystick button repeat rate (in Hz)
    double buttonFrequencyHz() const { return _buttonFrequencyHzFact.rawValue().toDouble(); }
    /// Set joystick button repeat rate (in Hz)
    void setButtonFrequency(double val);

    Fact* axisFrequencyHzFact() { return &_axisFrequencyHzFact; }
    Fact* buttonFrequencyHzFact() { return &_buttonFrequencyHzFact; }
    Fact* exponentialPctFact() { return &_exponentialPctFact; }

    bool enableManualControlExtensions() const { return _enableManualControlExtensions; }
    void setEnableManualControlExtensions(bool enable);

    int transmitterMode() const { return _transmitterMode; }
    void setTransmitterMode(int mode);

    Q_INVOKABLE void startConfiguration(); ///< Tells the joystick that the configuration UI is being displayed so it can do any special processing required
    Q_INVOKABLE void stopConfiguration(); ///< Tells the joystick that the configuration UI is being closed so it can do any special processing required

signals:
    void calibratedChanged(bool calibrated);
    void buttonActionsChanged();
    void assignableActionsChanged();
    void throttleModeCenterZeroChanged(bool throttleCenterZero);
    void negativeThrustChanged(bool allowNegative);
    void exponentialChanged(double exponential);
    void throttleSmoothingChanged(bool throttleSmoothing);
    void useDeadbandChanged(bool useDeadband);
    void enabledChanged(bool enabled);
    void circleCorrectionChanged(bool circleCorrection);
    void enableManualControlExtensionsChanged();
    void transmitterModeChanged(int mode);
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
    void gripperAction(QGCMAVLink::GripperActions gripperAction);
    void landingGearDeploy();
    void landingGearRetract();
    void motorInterlock(bool enable);
    void unknownAction(const QString &action);
    void vehicleJoystickData(float roll, float pitch, float yaw, float throttle, uint16_t buttonsLow, uint16_t buttonsHigh, float gimbalPitch, float gimbalYaw);
    void rawChannelValuesChanged(QVector<int> channelValues); ///< Used by joystick configuration UI
    void rawButtonPressedChanged(int index, int pressed); ///< Used by joystick configuration UI

protected:
    QString _name;
    int _axisCount = 0;
    int _buttonCount = 0;
    int _hatCount = 0;

private slots:
    void _flightModesChanged() { _buildActionList(_pollingVehicle); }
    void _saveExponentialPctSetting();

private:
    virtual bool _open() = 0;
    virtual void _close() = 0;
    virtual bool _update() = 0;

    virtual bool _getButton(int i) const = 0;
    virtual int _getAxisValue(int axis) const = 0;
    virtual bool _getHat(int hat, int i) const = 0;

    void run() override;

    enum PollingType {
        NotPolling, ///< Not currrently polling
        PollingForConfiguration, ///< Polling for configuration/calibration display
        PollingForVehicle, ///< Normal polling for joystick output to Vehicle
    };
    void _startPollingForVehicle(Vehicle &vehicle);
    void _startPollingForConfiguration();
    void _stopPollingForConfiguration();
    void _stopAllPolling();
    QString _pollingTypeToString(PollingType pollingType) const;
    PollingType _currentPollingType = NotPolling;
    PollingType _previousPollingType = NotPolling;
    Vehicle* _pollingVehicle = nullptr;

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
    void _buildActionList(Vehicle *vehicle);

    /// Relative mappings of axis functions between different TX modes
    int _mapFunctionMode(int mode, int function);

    /// Remap current axis functions from current TX mode to new TX mode
    void _remapAxes(int currentMode, int newMode, int (&newMapping)[maxAxisFunction]);

    int _hatButtonCount = 0;
    int _totalButtonCount = 0;
    Calibration_t *_rgCalibration = nullptr;
    uint8_t *_rgButtonValues = nullptr;
    MavlinkActionManager *_mavlinkActionManager = nullptr;
    QmlObjectListModel *_assignableButtonActions = nullptr;

    bool _throttleSmoothing = false;
    bool _calibrated = false;
    bool _circleCorrection = true;
    bool _useDeadband = false;
    bool _negativeThrust = false;
    bool _enableManualControlExtensions = false;
    int _rgFunctionAxis[maxAxisFunction] = {};
    QElapsedTimer _axisTime;
    QList<AssignedButtonAction*> _buttonActionArray;
    QStringList _availableActionTitles;
    std::atomic<bool> _exitThread = false;    ///< true: signal thread to exit
    bool _throttleModeCenterZero = false;
    int _transmitterMode = 2;

    QMap<QString, FactMetaData*> _metaDataMap;
    Fact _axisFrequencyHzFact;
    Fact _buttonFrequencyHzFact;
    Fact _exponentialPctFact;
    static constexpr const char* _joystickFactsSettingsGroup = "JoystickFacts";
    static constexpr const char* _axisFrequencyHzFactName = "axisFrequencyHz";
    static constexpr const char* _buttonFrequencyHzFactName = "buttonFrequencyHz";
    static constexpr const char* _exponentialPctFactName = "exponentialPct";

    static constexpr double _defaultAxisFrequencyHz = 25.0;
    static constexpr double _defaultButtonFrequencyHz = 5.0;

    static constexpr double _minAxisFrequencyHz = 0.25;
    static constexpr double _maxAxisFrequencyHz = 200.0;
    static constexpr double _minButtonFrequencyHz = 0.25;
    static constexpr double _maxButtonFrequencyHz = 50.0;
    static constexpr double _minExponentialPct = 0.0;
    static constexpr double _maxExponentialPct = 50.0;

    static constexpr const char *_rgFunctionSettingsKey[maxAxisFunction] = {
        "RollAxis",
        "PitchAxis",
        "YawAxis",
        "ThrottleAxis",
        "GimbalPitchAxis",
        "GimbalYawAxis"
    };

    static constexpr const char *_settingsGroup =                       "Joysticks";
    static constexpr const char *_calibratedSettingsKey =               "Calibrated5"; // Increment number to force recalibration
    static constexpr const char *_buttonActionNameSettingsKey =         "ButtonActionName%1";
    static constexpr const char *_buttonActionRepeatSettingsKey =       "ButtonActionRepeat%1";
    static constexpr const char *_throttleModeCenterZeroSettingsKey =   "ThrottleModeCenterZero";
    static constexpr const char *_negativeThrustSettingsKey =           "NegativeThrust";
    static constexpr const char *_exponentialPctSettingsKey =           "ExponentialPct";
    static constexpr const char *_throttleSmoothingSettingsKey =        "Accumulator";
    static constexpr const char *_useDeadbandSettingsKey =              "Deadband";
    static constexpr const char *_circleCorrectionSettingsKey =         "Circle_Correction";
    static constexpr const char *_axisFrequencySettingsKey =            "AxisFrequency";
    static constexpr const char *_buttonFrequencySettingsKey =          "ButtonFrequency";
    static constexpr const char *_manualControlExtensionsEnabledKey =   "ManualControlExtensionsEnabled";
    static constexpr const char *_transmitterModeSettingsKey =           "TransmitterMode";

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
    static constexpr const char *_buttonActionGripperClose =       QT_TR_NOOP("Gripper Close");
    static constexpr const char *_buttonActionGripperOpen =        QT_TR_NOOP("Gripper Open");
    static constexpr const char *_buttonActionGripperStop =        QT_TR_NOOP("Gripper Stop");
    static constexpr const char *_buttonActionLandingGearDeploy=   QT_TR_NOOP("Landing gear deploy");
    static constexpr const char *_buttonActionLandingGearRetract=  QT_TR_NOOP("Landing gear retract");
    static constexpr const char *_buttonActionMotorInterlockEnable=   QT_TR_NOOP("Motor Interlock enable");
    static constexpr const char *_buttonActionMotorInterlockDisable=  QT_TR_NOOP("Motor Interlock disable");

    friend class JoystickManager;
    friend class JoystickConfigController;
};
