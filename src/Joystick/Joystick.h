#pragma once

#include "QGCMAVLink.h"

#include <QtCore/QLoggingCategory>
#include <QtCore/QObject>
#include <QtCore/QThread>
#include <QtCore/QVariantMap>
#include <QtGui/QVector3D>
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

    friend class JoystickManager;
    friend class JoystickConfigController;
#ifdef QGC_UNITTEST_BUILD
    friend class JoystickTest;
#endif

public:
    Q_PROPERTY(QString                  name                    READ    name                                                CONSTANT)
    Q_PROPERTY(JoystickSettings*        settings                READ    settings                                            CONSTANT)
    Q_PROPERTY(int                      axisCount               READ    axisCount                                           CONSTANT)
    Q_PROPERTY(int                      buttonCount             READ    buttonCount                                         CONSTANT)
    Q_PROPERTY(bool                     requiresCalibration     READ    requiresCalibration                                 CONSTANT)
    Q_PROPERTY(bool                     hasRumble               READ    hasRumble                                           CONSTANT)
    Q_PROPERTY(bool                     hasRumbleTriggers       READ    hasRumbleTriggers                                   CONSTANT)
    Q_PROPERTY(bool                     hasLED                  READ    hasLED                                              CONSTANT)
    Q_PROPERTY(QString                  guid                    READ    guid                                                CONSTANT)
    Q_PROPERTY(quint16                  vendorId                READ    vendorId                                            CONSTANT)
    Q_PROPERTY(quint16                  productId               READ    productId                                           CONSTANT)
    Q_PROPERTY(QString                  serial                  READ    serial                                              CONSTANT)
    Q_PROPERTY(QString                  deviceType              READ    deviceType                                          CONSTANT)
    Q_PROPERTY(int                      playerIndex             READ    playerIndex         WRITE setPlayerIndex            NOTIFY playerIndexChanged)
    Q_PROPERTY(int                      batteryPercent          READ    batteryPercent                                      NOTIFY batteryStateChanged)
    Q_PROPERTY(QString                  powerState              READ    powerState                                          NOTIFY batteryStateChanged)
    Q_PROPERTY(bool                     isGamepad               READ    isGamepad                                           CONSTANT)
    Q_PROPERTY(QString                  gamepadType             READ    gamepadType                                         CONSTANT)
    Q_PROPERTY(QString                  path                    READ    path                                                CONSTANT)
    Q_PROPERTY(bool                     isVirtual               READ    isVirtual                                           CONSTANT)
    Q_PROPERTY(quint16                  firmwareVersion         READ    firmwareVersion                                     CONSTANT)
    Q_PROPERTY(QString                  connectionType          READ    connectionType                                      CONSTANT)
    Q_PROPERTY(int                      ballCount               READ    ballCount                                           CONSTANT)
    Q_PROPERTY(const QmlObjectListModel *assignableActions      READ    assignableActions                                   NOTIFY assignableActionsChanged)
    Q_PROPERTY(QString                  disabledActionName      READ    disabledActionName                                  CONSTANT)
    Q_PROPERTY(QStringList              assignableActionTitles  READ    assignableActionTitles                              NOTIFY assignableActionsChanged)
    Q_PROPERTY(QStringList              buttonActions           READ    buttonActions                                       NOTIFY buttonActionsChanged)
    Q_PROPERTY(QString                  buttonActionNone        READ    buttonActionNone                                    CONSTANT)
    Q_PROPERTY(QString                  linkedGroupId           READ    linkedGroupId           WRITE setLinkedGroupId      NOTIFY linkedGroupChanged)
    Q_PROPERTY(QString                  linkedGroupRole         READ    linkedGroupRole         WRITE setLinkedGroupRole    NOTIFY linkedGroupChanged)

    Joystick(const QString &name, int axisCount, int buttonCount, int hatCount, QObject *parent = nullptr);
    virtual ~Joystick();

    static constexpr int AxisMin = -32768;
    static constexpr int AxisMax = 32767;

    enum ButtonEvent_t {
        ButtonEventUpTransition,
        ButtonEventDownTransition,
        ButtonEventRepeat,
        ButtonEventNone
    };

    struct AxisCalibration_t {
        int min = AxisMin;
        int max = AxisMax;
        int center = 0;
        int deadband = 0;
        bool reversed = false;

        void reset() {
            min = AxisMin;
            max = AxisMax;
            center = 0;
            deadband = 0;
            reversed = false;
        }
    };

    enum AxisFunction_t {
        rollFunction,
        pitchFunction,
        yawFunction,
        throttleFunction,
        pitchExtensionFunction,
        rollExtensionFunction,
        aux1ExtensionFunction,
        aux2ExtensionFunction,
        aux3ExtensionFunction,
        aux4ExtensionFunction,
        aux5ExtensionFunction,
        aux6ExtensionFunction,
        maxAxisFunction
    };
    static QString axisFunctionToString(AxisFunction_t function);

    /// Standard gamepad hat/D-pad directions
    enum HatDirection : quint8 {
        HatCentered  = 0x00,
        HatUp        = 0x01,
        HatRight     = 0x02,
        HatDown      = 0x04,
        HatLeft      = 0x08,
        HatRightUp   = HatRight | HatUp,
        HatRightDown = HatRight | HatDown,
        HatLeftUp    = HatLeft | HatUp,
        HatLeftDown  = HatLeft | HatDown
    };
    Q_ENUM(HatDirection)

    /// Standard gamepad button indices
    enum GamepadButton {
        ButtonA = 0,
        ButtonB = 1,
        ButtonX = 2,
        ButtonY = 3,
        ButtonBack = 4,
        ButtonGuide = 5,
        ButtonStart = 6,
        ButtonLeftStick = 7,
        ButtonRightStick = 8,
        ButtonLeftShoulder = 9,
        ButtonRightShoulder = 10,
        ButtonDPadUp = 11,
        ButtonDPadDown = 12,
        ButtonDPadLeft = 13,
        ButtonDPadRight = 14
    };
    Q_ENUM(GamepadButton)

    /// Standard gamepad axis indices
    enum GamepadAxis {
        AxisLeftX = 0,
        AxisLeftY = 1,
        AxisRightX = 2,
        AxisRightY = 3,
        AxisTriggerLeft = 4,
        AxisTriggerRight = 5
    };
    Q_ENUM(GamepadAxis)

    Q_INVOKABLE void setButtonRepeat(int button, bool repeat);
    Q_INVOKABLE bool getButtonRepeat(int button);
    Q_INVOKABLE void setButtonAction(int button, const QString &action);
    Q_INVOKABLE QString getButtonAction(int button) const;

    JoystickSettings* settings() { return &_joystickSettings; }
    QString name() const { return _name; }
    int buttonCount() const { return _totalButtonCount; }
    int axisCount() const { return _axisCount; }
    virtual bool requiresCalibration() const { return true; }
    virtual bool hasRumble() const { return false; }
    virtual bool hasRumbleTriggers() const { return false; }
    virtual bool hasLED() const { return false; }
    virtual QString guid() const { return QString(); }
    virtual quint16 vendorId() const { return 0; }
    virtual quint16 productId() const { return 0; }
    virtual QString serial() const { return QString(); }
    virtual QString deviceType() const { return QString(); }
    virtual int playerIndex() const { return -1; }
    virtual void setPlayerIndex(int index) { Q_UNUSED(index); }
    virtual int batteryPercent() const { return -1; }
    virtual QString powerState() const { return QString(); }
    virtual bool isGamepad() const { return false; }
    virtual QString gamepadType() const { return QString(); }
    virtual QString path() const { return QString(); }
    virtual bool isVirtual() const { return false; }
    virtual quint16 firmwareVersion() const { return 0; }
    virtual QString connectionType() const { return QString(); }
    virtual int ballCount() const { return 0; }

    Q_INVOKABLE virtual void rumble(quint16 lowFreq, quint16 highFreq, quint32 durationMs) { Q_UNUSED(lowFreq); Q_UNUSED(highFreq); Q_UNUSED(durationMs); }
    Q_INVOKABLE virtual void rumbleTriggers(quint16 left, quint16 right, quint32 durationMs) { Q_UNUSED(left); Q_UNUSED(right); Q_UNUSED(durationMs); }
    Q_INVOKABLE virtual void setLED(quint8 red, quint8 green, quint8 blue) { Q_UNUSED(red); Q_UNUSED(green); Q_UNUSED(blue); }
    Q_INVOKABLE virtual QString axisLabel(int axis) const { Q_UNUSED(axis); return QString(); }
    Q_INVOKABLE virtual QString buttonLabel(int button) const { Q_UNUSED(button); return QString(); }
    Q_INVOKABLE virtual QString getMapping() const { return QString(); }
    Q_INVOKABLE virtual bool addMapping(const QString &mapping) { Q_UNUSED(mapping); return false; }

    // Sensor support (gyroscope/accelerometer)
    Q_INVOKABLE virtual bool hasGyroscope() const { return false; }
    Q_INVOKABLE virtual bool hasAccelerometer() const { return false; }
    Q_INVOKABLE virtual bool setGyroscopeEnabled(bool enabled) { Q_UNUSED(enabled); return false; }
    Q_INVOKABLE virtual bool setAccelerometerEnabled(bool enabled) { Q_UNUSED(enabled); return false; }
    Q_INVOKABLE virtual QVector3D gyroscopeData() const { return QVector3D(); }
    Q_INVOKABLE virtual QVector3D accelerometerData() const { return QVector3D(); }
    Q_INVOKABLE virtual float gyroscopeDataRate() const { return 0.0f; }
    Q_INVOKABLE virtual float accelerometerDataRate() const { return 0.0f; }

    // Touchpad support (PS4/PS5 controllers)
    Q_INVOKABLE virtual int touchpadCount() const { return 0; }
    Q_INVOKABLE virtual int touchpadFingerCount(int touchpad) const { Q_UNUSED(touchpad); return 0; }
    Q_INVOKABLE virtual QVariantMap getTouchpadFinger(int touchpad, int finger) const {
        Q_UNUSED(touchpad); Q_UNUSED(finger); return QVariantMap();
    }

    // Trackball support
    Q_INVOKABLE virtual QVariantMap getBall(int ball) const { Q_UNUSED(ball); return QVariantMap(); }

    // PS5 adaptive trigger effects
    Q_INVOKABLE virtual bool sendEffect(const QByteArray &data) { Q_UNUSED(data); return false; }

    // Binding queries (debug/UI)
    Q_INVOKABLE virtual QVariantMap getAxisBinding(int axis) const { Q_UNUSED(axis); return QVariantMap(); }
    Q_INVOKABLE virtual QVariantMap getButtonBinding(int button) const { Q_UNUSED(button); return QVariantMap(); }

    // Capability queries
    Q_INVOKABLE virtual bool hasButton(int button) const { Q_UNUSED(button); return false; }
    Q_INVOKABLE virtual bool hasAxis(int axis) const { Q_UNUSED(axis); return false; }

    // Real gamepad type (actual hardware vs mapped type)
    Q_INVOKABLE virtual QString realGamepadType() const { return QString(); }

    // Type-specific button labels (shows correct names for controller type, e.g., "Cross" vs "A")
    Q_INVOKABLE virtual QString buttonLabelForType(int button) const { Q_UNUSED(button); return QString(); }

    // Haptic/Force Feedback support
    Q_INVOKABLE virtual bool hasHaptic() const { return false; }
    Q_INVOKABLE virtual int hapticEffectsCount() const { return 0; }
    Q_INVOKABLE virtual bool hapticRumbleSupported() const { return false; }
    Q_INVOKABLE virtual bool hapticRumbleInit() { return false; }
    Q_INVOKABLE virtual bool hapticRumblePlay(float strength, quint32 durationMs) { Q_UNUSED(strength); Q_UNUSED(durationMs); return false; }
    Q_INVOKABLE virtual void hapticRumbleStop() {}

    // Mapping for GUID (static in implementation)
    Q_INVOKABLE virtual QString getMappingForGUID(const QString &guid) const { Q_UNUSED(guid); return QString(); }

    // Virtual joystick control (for software-based joystick input)
    Q_INVOKABLE virtual bool setVirtualAxis(int axis, int value) { Q_UNUSED(axis); Q_UNUSED(value); return false; }
    Q_INVOKABLE virtual bool setVirtualButton(int button, bool down) { Q_UNUSED(button); Q_UNUSED(down); return false; }
    Q_INVOKABLE virtual bool setVirtualHat(int hat, quint8 value) { Q_UNUSED(hat); Q_UNUSED(value); return false; }
    Q_INVOKABLE virtual bool setVirtualBall(int ball, int dx, int dy) { Q_UNUSED(ball); Q_UNUSED(dx); Q_UNUSED(dy); return false; }
    Q_INVOKABLE virtual bool setVirtualTouchpad(int touchpad, int finger, bool down, float x, float y, float pressure) {
        Q_UNUSED(touchpad); Q_UNUSED(finger); Q_UNUSED(down); Q_UNUSED(x); Q_UNUSED(y); Q_UNUSED(pressure); return false;
    }
    Q_INVOKABLE virtual bool sendVirtualSensorData(int sensorType, float x, float y, float z) {
        Q_UNUSED(sensorType); Q_UNUSED(x); Q_UNUSED(y); Q_UNUSED(z); return false;
    }

    // Properties/Capability detection
    Q_INVOKABLE virtual bool hasMonoLED() const { return false; }
    Q_INVOKABLE virtual bool hasRGBLED() const { return false; }
    Q_INVOKABLE virtual bool hasPlayerLED() const { return false; }

    // Connection state
    Q_INVOKABLE virtual QString connectionState() const { return QString(); }

    // Initial axis state (for drift detection) - returns {valid, value}
    Q_INVOKABLE virtual QVariantMap getAxisInitialState(int axis) const { Q_UNUSED(axis); return QVariantMap(); }

    // Per-device custom mapping
    Q_INVOKABLE virtual bool setMapping(const QString &mapping) { Q_UNUSED(mapping); return false; }

    QStringList buttonActions() const;
    QString buttonActionNone() const { return _buttonActionNone; }
    QString disabledActionName() const { return _buttonActionNone; }
    const QmlObjectListModel *assignableActions() const { return _availableButtonActions; }
    QStringList assignableActionTitles() const { return _availableActionTitles; }

    /// HOTAS/Multi-device linking (devices with same groupId act as single joystick)
    QString linkedGroupId() const { return _linkedGroupId; }
    void setLinkedGroupId(const QString &groupId);
    QString linkedGroupRole() const { return _linkedGroupRole; }
    void setLinkedGroupRole(const QString &role);

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
    void playerIndexChanged();
    void batteryStateChanged();
    void connectionStateChanged(const QString &newState);
    void linkedGroupChanged();
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

    // Sensor event signals (for event-driven updates)
    void gyroscopeDataUpdated(const QVector3D &data);
    void accelerometerDataUpdated(const QVector3D &data);
    void touchpadEvent(int touchpad, int finger, bool down, float x, float y, float pressure);

    // Additional event signals
    void mappingRemapped();
    void updateComplete();

protected:
    QString _name;
    int _axisCount = 0;
    int _buttonCount = 0;
    int _hatCount = 0;

private slots:
    void _flightModesChanged() { _buildAvailableButtonsActionList(_pollingVehicle); }

private:
    enum PollingType {
        NotPolling, ///< Not currrently polling
        PollingForConfiguration, ///< Polling for configuration/calibration display
        PollingForVehicle, ///< Normal polling for joystick output to Vehicle
    };

    using AxisFunctionMap_t = QMap<AxisFunction_t, int>;

    virtual bool _open() = 0;
    virtual void _close() = 0;
    virtual bool _update() = 0;

    virtual bool _getButton(int i) const = 0;
    virtual int _getAxisValue(int axis) const = 0;
    virtual bool _getHat(int hat, int i) const = 0;

    void run() override;

    void _startPollingForVehicle(Vehicle &vehicle);
    void _startPollingForActiveVehicle();
    void _startPollingForConfiguration();
    void _stopPollingForConfiguration();
    void _stopAllPolling();
    QString _pollingTypeToString(PollingType pollingType) const;
    PollingType _currentPollingType = NotPolling;
    PollingType _previousPollingType = NotPolling;
    Vehicle* _pollingVehicle = nullptr;

    void _resetFunctionToAxisMap();
    void _resetAxisCalibrationData();
    void _resetButtonActionData();
    void _resetButtonEventStates();

    void _foundInvalidAxisSettingsCleanup();

    void _loadButtonSettings();
    void _loadAxisSettings(bool joystickCalibrated, int transmitterMode);
    void _saveButtonSettings();
    void _saveAxisSettings(int transmitterMode);
    void _loadFromSettingsIntoCalibrationData();
    void _saveFromCalibrationDataIntoSettings();
    void _clearAxisSettings();
    void _clearButtonSettings();

    /// Adjust the raw axis value to the -1:1 range given calibration information
    float _adjustRange(int reversedAxisValue, const AxisCalibration_t &calibration, bool withDeadbands);

    void _executeButtonAction(const QString &action, const ButtonEvent_t buttonEvent);
    int  _findAvailableButtonActionIndex(const QString &action);
    bool _validAxis(int axis) const;
    bool _validButton(int button) const;
    void _handleAxis();
    void _handleButtons();
    void _buildAvailableButtonsActionList(Vehicle *vehicle);
    AxisFunction_t _getAxisFunctionForJoystickAxis(int joystickAxis) const;
    int _getJoystickAxisForAxisFunction(AxisFunction_t axisFunction) const;
    void _setJoystickAxisForAxisFunction(AxisFunction_t axisFunction, int axis);
    void _updateButtonEventState(int buttonIndex, const bool buttonPressed, ButtonEvent_t &buttonEventState);
    void _updateButtonEventStates(QVector<ButtonEvent_t> &buttonEventStates);

    /// Remap current axis functions from current TX mode to new TX mode
    void _remapFunctionsInFunctionMapToNewTransmittedMode(int fromMode, int toMode);

    int _hatButtonCount = 0;
    int _totalButtonCount = 0;
    QVector<AxisCalibration_t> _rgCalibration;
    QVector<ButtonEvent_t> _buttonEventStates;
    QVector<AssignedButtonAction*> _assignedButtonActions;
    MavlinkActionManager *_mavlinkActionManager = nullptr;
    QmlObjectListModel *_availableButtonActions = nullptr;

    JoystickManager* _joystickManager = nullptr;
    JoystickSettings _joystickSettings;

    AxisFunctionMap_t _axisFunctionToJoystickAxisMap; ///< Map from AxisFunction_t to axis index, kJoystickAxisNotAssigned if not assigned
    static constexpr const int kJoystickAxisNotAssigned = -1;

    QElapsedTimer _axisElapsedTimer;
    QStringList _availableActionTitles;
    std::atomic<bool> _exitThread = false;    ///< true: signal thread to exit

    // HOTAS/Multi-device linking
    QString _linkedGroupId;
    QString _linkedGroupRole;

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

};
