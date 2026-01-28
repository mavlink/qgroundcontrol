#pragma once

#include "QGCMAVLink.h"

#include <QtCore/QLoggingCategory>
#include <QtCore/QObject>
#include <QtCore/QThread>
#include <QtQmlIntegration/QtQmlIntegration>

#include "RemoteControlCalibrationController.h"
#include "Fact.h"
#include "JoystickSettings.h"

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
    AssignedButtonAction(const QString &name, bool repeat);

    QString actionName;
    bool repeat = false;
    QElapsedTimer buttonElapsedTimer;
};

class AvailableButtonAction : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString  action      READ action     CONSTANT)
    Q_PROPERTY(bool     canRepeat   READ canRepeat  CONSTANT)

public:
    AvailableButtonAction(const QString &actionName, bool canRepeat, QObject *parent = nullptr);

    const QString &action() const { return _actionName; }
    bool canRepeat() const { return _repeat; }

private:
    const QString _actionName;
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

public:
    Q_PROPERTY(QString                  name                    READ    name                                                CONSTANT)
    Q_PROPERTY(JoystickSettings*        settings                READ    settings                                            CONSTANT)
    Q_PROPERTY(int                      axisCount               READ    axisCount                                           CONSTANT)
    Q_PROPERTY(int                      buttonCount             READ    buttonCount                                         CONSTANT)
    Q_PROPERTY(const QmlObjectListModel *assignableActions      READ    assignableActions                                   NOTIFY assignableActionsChanged)
    Q_PROPERTY(QString                  disabledActionName      READ    disabledActionName                                  CONSTANT)
    Q_PROPERTY(QStringList              assignableActionTitles  READ    assignableActionTitles                              NOTIFY assignableActionsChanged)
    Q_PROPERTY(QStringList              buttonActions           READ    buttonActions                                       NOTIFY buttonActionsChanged)
    Q_PROPERTY(QString                  buttonActionNone        READ    buttonActionNone                                    CONSTANT)

    Joystick(const QString &name, int axisCount, int buttonCount, int hatCount, QObject *parent = nullptr);
    virtual ~Joystick();

    enum ButtonEvent_t {
        ButtonEventUpTransition,
        ButtonEventDownTransition,
        ButtonEventRepeat,
        ButtonEventNone
    };

    struct AxisCalibration_t {
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
        maxAxisFunction // If the value of this is changed, be sure to update JoystickAxis.SettingsGroup.json/stickFunction metadata
    };
    static QString axisFunctionToString(AxisFunction_t function);

    Q_INVOKABLE void setButtonRepeat(int button, bool repeat);
    Q_INVOKABLE bool getButtonRepeat(int button);
    Q_INVOKABLE void setButtonAction(int button, const QString &action);
    Q_INVOKABLE QString getButtonAction(int button) const;

    JoystickSettings* settings() { return &_joystickSettings; }
    QString name() const { return _name; }
    int buttonCount() const { return _totalButtonCount; }
    int axisCount() const { return _axisCount; }
    QStringList buttonActions() const;
    QString buttonActionNone() const { return _buttonActionNone; }
    QString disabledActionName() const { return _buttonActionNone; }
    const QmlObjectListModel *assignableActions() const { return _availableButtonActions; }
    QStringList assignableActionTitles() const { return _availableActionTitles; }

    void setFunctionAxis(AxisFunction_t function, int axis);
    int getFunctionAxis(AxisFunction_t function) const;
    void setAxisCalibration(int axis, const AxisCalibration_t &calibration);
    Joystick::AxisCalibration_t getAxisCalibration(int axis) const;

    RemoteControlCalibrationController::StickFunction mapAxisFunctionToRCCStickFunction(AxisFunction_t axisFunction) const;
    AxisFunction_t mapRCCStickFunctionToAxisFunction(RemoteControlCalibrationController::StickFunction stickFunction) const;

    void setFunctionForChannel(RemoteControlCalibrationController::StickFunction stickFunction, int channel);
    int getChannelForFunction(RemoteControlCalibrationController::StickFunction stickFunction) const ;

    Q_INVOKABLE void startConfiguration(); ///< Tells the joystick that the configuration UI is being displayed so it can do any special processing required
    Q_INVOKABLE void stopConfiguration(); ///< Tells the joystick that the configuration UI is being closed so it can do any special processing required

    void stop();

signals:
    void buttonActionsChanged();
    void assignableActionsChanged();
    void axisValues(float roll, float pitch, float yaw, float throttle);
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
    void rawChannelValuesChanged(QVector<int> channelValues); ///< Signalled during PollingForConfiguration
    void rawButtonPressedChanged(int index, bool pressed); ///< Signalled during PollingForConfiguration

protected:
    QString _name;
    int _axisCount = 0;
    int _buttonCount = 0;
    int _hatCount = 0;

private slots:
    void _flightModesChanged() { _buildActionList(_pollingVehicle); }

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

    void _loadFromSettingsIntoCalibrationData();
    void _saveFromCalibrationDataIntoSettings();

    /// Adjust the raw axis value to the -1:1 range given calibration information
    float _adjustRange(int value, const AxisCalibration_t &calibration, bool withDeadbands);

    void _executeButtonAction(const QString &action, const ButtonEvent_t buttonEvent);
    int  _findAvailableButtonActionIndex(const QString &action);
    bool _validAxis(int axis) const;
    bool _validButton(int button) const;
    void _handleAxis();
    void _handleButtons();
    void _buildActionList(Vehicle *vehicle);
    AxisFunction_t _getFunctionForAxis(int axis) const;
    void _updateButtonEventState(int buttonIndex, const bool buttonPressed, ButtonEvent_t &buttonEventState);
    void _updateButtonEventStates(QVector<ButtonEvent_t> &buttonEventStates);
    void _migrateLegacySettings();


    /// Relative mappings of axis functions between different TX modes
    int _mapFunctionMode(int mode, int function);

    /// Remap current axis functions from current TX mode to new TX mode
    void _remapAxes(int fromMode, int toMode, int (&newMapping)[maxAxisFunction]);

    int _hatButtonCount = 0;
    int _totalButtonCount = 0;
    QVector<AxisCalibration_t> _rgCalibration;
    QVector<ButtonEvent_t> _buttonEventStates;
    QVector<AssignedButtonAction*> _assignedButtonActions;
    MavlinkActionManager *_mavlinkActionManager = nullptr;
    QmlObjectListModel *_availableButtonActions = nullptr;
    JoystickSettings _joystickSettings;

    int _rgFunctionAxis[maxAxisFunction] = {};
    QElapsedTimer _axisElapsedTimer;
    QStringList _availableActionTitles;
    std::atomic<bool> _exitThread = false;    ///< true: signal thread to exit

    static constexpr const char *_rgFunctionSettingsKey[maxAxisFunction] = {
        "RollAxis",
        "PitchAxis",
        "YawAxis",
        "ThrottleAxis",
        "GimbalPitchAxis",
        "GimbalYawAxis"
    };

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
    static constexpr const char *_buttonActionGripperGrab =        QT_TR_NOOP("Gripper Grab");
    static constexpr const char *_buttonActionGripperRelease =     QT_TR_NOOP("Gripper Release");
    static constexpr const char *_buttonActionGripperHold =        QT_TR_NOOP("Gripper Hold");
    static constexpr const char *_buttonActionLandingGearDeploy=   QT_TR_NOOP("Landing gear deploy");
    static constexpr const char *_buttonActionLandingGearRetract=  QT_TR_NOOP("Landing gear retract");
    static constexpr const char *_buttonActionMotorInterlockEnable=   QT_TR_NOOP("Motor Interlock enable");
    static constexpr const char *_buttonActionMotorInterlockDisable=  QT_TR_NOOP("Motor Interlock disable");

    friend class JoystickManager;
    friend class JoystickConfigController;
};
