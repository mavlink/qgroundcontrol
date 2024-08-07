/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

/// @file
/// @brief  Joystick Controller

#pragma once

#include "QGCMAVLink.h"
#include "CustomActionManager.h"

#include <QtCore/QObject>
#include <QtCore/QThread>
#include <QtCore/QLoggingCategory>

// JoystickLog Category declaration moved to QGCLoggingCategory.cc to allow access in Vehicle
Q_DECLARE_LOGGING_CATEGORY(JoystickValuesLog)
Q_DECLARE_METATYPE(GRIPPER_ACTIONS)

class MultiVehicleManager;
class Vehicle;

/// Action assigned to button
class AssignedButtonAction : public QObject {
    Q_OBJECT
public:
    AssignedButtonAction(QObject* parent, const QString name);
    QString action;
    QElapsedTimer buttonTime;
    bool    repeat = false;
};

/// Assignable Button Action
class AssignableButtonAction : public QObject {
    Q_OBJECT
public:
    AssignableButtonAction(QObject* parent, QString action_, bool canRepeat_ = false);
    Q_PROPERTY(QString  action      READ action     CONSTANT)
    Q_PROPERTY(bool     canRepeat   READ canRepeat  CONSTANT)
    QString action      () { return _action; }
    bool    canRepeat   () const{ return _repeat; }
private:
    QString _action;
    bool    _repeat = false;
};

/// Joystick Controller
class Joystick : public QThread
{
    Q_OBJECT
public:
    Joystick(const QString& name, int axisCount, int buttonCount, int hatCount, MultiVehicleManager* multiVehicleManager);

    virtual ~Joystick();

    typedef struct Calibration_t {
        int     min;
        int     max;
        int     center;
        int     deadband;
        bool    reversed;
        Calibration_t()
            : min(-32767)
            , max(32767)
            , center(0)
            , deadband(0)
            , reversed(false) {}
    } Calibration_t;

    typedef enum {
        rollFunction,
        pitchFunction,
        yawFunction,
        throttleFunction,
        gimbalPitchFunction,
        gimbalYawFunction,
        maxFunction
    } AxisFunction_t;

    typedef enum {
        ThrottleModeCenterZero,
        ThrottleModeDownZero,
        ThrottleModeMax
    } ThrottleMode_t;

    Q_PROPERTY(QString  name                    READ name                   CONSTANT)
    Q_PROPERTY(bool     calibrated              MEMBER _calibrated          NOTIFY calibratedChanged)
    Q_PROPERTY(int      totalButtonCount        READ totalButtonCount       CONSTANT)
    Q_PROPERTY(int      axisCount               READ axisCount              CONSTANT)
    Q_PROPERTY(bool     requiresCalibration     READ requiresCalibration    CONSTANT)

    //-- Actions assigned to buttons
    Q_PROPERTY(QStringList buttonActions        READ buttonActions          NOTIFY buttonActionsChanged)

    //-- Actions that can be assigned to buttons
    Q_PROPERTY(QmlObjectListModel* assignableActions    READ assignableActions          NOTIFY      assignableActionsChanged)
    Q_PROPERTY(QStringList assignableActionTitles       READ assignableActionTitles     NOTIFY      assignableActionsChanged)
    Q_PROPERTY(QString  disabledActionName              READ disabledActionName         CONSTANT)

    Q_PROPERTY(int      throttleMode            READ throttleMode           WRITE setThrottleMode       NOTIFY throttleModeChanged)
    Q_PROPERTY(float    axisFrequencyHz         READ axisFrequencyHz        WRITE setAxisFrequency      NOTIFY axisFrequencyHzChanged)
    Q_PROPERTY(float    minAxisFrequencyHz      MEMBER _minAxisFrequencyHz                              CONSTANT)
    Q_PROPERTY(float    maxAxisFrequencyHz      MEMBER _maxAxisFrequencyHz                              CONSTANT)
    Q_PROPERTY(float    buttonFrequencyHz       READ buttonFrequencyHz      WRITE setButtonFrequency    NOTIFY buttonFrequencyHzChanged)
    Q_PROPERTY(float    minButtonFrequencyHz    MEMBER _minButtonFrequencyHz                            CONSTANT)
    Q_PROPERTY(float    maxButtonFrequencyHz    MEMBER _maxButtonFrequencyHz                            CONSTANT)
    Q_PROPERTY(bool     negativeThrust          READ negativeThrust         WRITE setNegativeThrust     NOTIFY negativeThrustChanged)
    Q_PROPERTY(float    exponential             READ exponential            WRITE setExponential        NOTIFY exponentialChanged)
    Q_PROPERTY(bool     accumulator             READ accumulator            WRITE setAccumulator        NOTIFY accumulatorChanged)
    Q_PROPERTY(bool     circleCorrection        READ circleCorrection       WRITE setCircleCorrection   NOTIFY circleCorrectionChanged)

    Q_INVOKABLE void    setButtonRepeat     (int button, bool repeat);
    Q_INVOKABLE bool    getButtonRepeat     (int button);
    Q_INVOKABLE void    setButtonAction     (int button, const QString& action);
    Q_INVOKABLE QString getButtonAction     (int button);

    // Property accessors

    QString     name                () { return _name; }
    int         totalButtonCount    () const{ return _totalButtonCount; }
    int         axisCount           () const{ return _axisCount; }
    QStringList buttonActions       ();

    QmlObjectListModel* assignableActions   () { return &_assignableButtonActions; }
    QStringList assignableActionTitles      () { return _availableActionTitles; }
    QString     disabledActionName          () { return _buttonActionNone; }

    /// Start the polling thread which will in turn emit joystick signals
    void startPolling(Vehicle* vehicle);
    void stopPolling(void);

    void setCalibration(int axis, Calibration_t& calibration);
    Calibration_t getCalibration(int axis);

    void setFunctionAxis(AxisFunction_t function, int axis);
    int getFunctionAxis(AxisFunction_t function);

    void stop();

/*
    // Joystick index used by sdl library
    // Settable because sdl library remaps indices after certain events
    virtual int index(void) = 0;
    virtual void setIndex(int index) = 0;
*/
	virtual bool requiresCalibration(void) { return true; }

    int   throttleMode      ();
    void  setThrottleMode   (int mode);

    bool  negativeThrust    () const;
    void  setNegativeThrust (bool allowNegative);

    float exponential       () const;
    void  setExponential    (float expo);

    bool  accumulator       () const;
    void  setAccumulator    (bool accu);

    bool  deadband          () const;
    void  setDeadband       (bool accu);

    bool  circleCorrection  () const;
    void  setCircleCorrection(bool circleCorrection);

    void  setTXMode         (int mode);
    int   getTXMode         () { return _transmitterMode; }

    /// Set the current calibration mode
    void  setCalibrationMode (bool calibrating);

    /// Get joystick message rate (in Hz)
    float axisFrequencyHz     () const{ return _axisFrequencyHz; }
    /// Set joystick message rate (in Hz)
    void  setAxisFrequency  (float val);

    /// Get joystick button repeat rate (in Hz)
    float buttonFrequencyHz   () const{ return _buttonFrequencyHz; }
    /// Set joystick button repeat rate (in Hz)
    void  setButtonFrequency(float val);

signals:
    // The raw signals are only meant for use by calibration
    void rawAxisValueChanged        (int index, int value);
    void rawButtonPressedChanged    (int index, int pressed);
    void calibratedChanged          (bool calibrated);
    void buttonActionsChanged       ();
    void assignableActionsChanged   ();
    void throttleModeChanged        (int mode);
    void negativeThrustChanged      (bool allowNegative);
    void exponentialChanged         (float exponential);
    void accumulatorChanged         (bool accumulator);
    void enabledChanged             (bool enabled);
    void circleCorrectionChanged    (bool circleCorrection);
    void axisValues                 (float roll, float pitch, float yaw, float throttle);

    void axisFrequencyHzChanged     ();
    void buttonFrequencyHzChanged   ();
    void startContinuousZoom        (int direction);
    void stopContinuousZoom         ();
    void stepZoom                   (int direction);
    void stepCamera                 (int direction);
    void stepStream                 (int direction);
    void triggerCamera              ();
    void startVideoRecord           ();
    void stopVideoRecord            ();
    void toggleVideoRecord          ();
    void gimbalPitchStep            (int direction);
    void gimbalYawStep              (int direction);
    void centerGimbal               ();
    void gimbalYawLock              (bool lock);
    void setArmed                   (bool arm);
    void setVtolInFwdFlight         (bool set);
    void setFlightMode              (const QString& flightMode);
    void emergencyStop              ();
    void gripperAction              (GRIPPER_ACTIONS gripperAction);
    void landingGearDeploy          ();
    void landingGearRetract         ();

protected:
    void    _setDefaultCalibration  ();
    void    _saveSettings           ();
    void    _saveButtonSettings     ();
    void    _loadSettings           ();
    float   _adjustRange            (int value, Calibration_t calibration, bool withDeadbands);
    void    _executeButtonAction    (const QString& action, bool buttonDown);
    int     _findAssignableButtonAction(const QString& action);
    bool    _validAxis              (int axis) const;
    bool    _validButton            (int button) const;
    void    _handleAxis             ();
    void    _handleButtons          ();
    void    _buildActionList        (Vehicle* activeVehicle);

private:
    virtual bool _open      ()          = 0;
    virtual void _close     ()          = 0;
    virtual bool _update    ()          = 0;

    virtual bool _getButton (int i)      = 0;
    virtual int  _getAxis   (int i)      = 0;
    virtual bool _getHat    (int hat,int i) = 0;

    void _updateTXModeSettingsKey(Vehicle* activeVehicle);
    int _mapFunctionMode(int mode, int function);
    void _remapAxes(int currentMode, int newMode, int (&newMapping)[maxFunction]);

    // Override from QThread
    virtual void run();

protected:

    enum {
        BUTTON_UP,
        BUTTON_DOWN,
        BUTTON_REPEAT
    };

    static constexpr const float _defaultAxisFrequencyHz   = 25.0f;
    static constexpr const float _defaultButtonFrequencyHz = 5.0f;

    uint8_t*_rgButtonValues         = nullptr;

    std::atomic<bool> _exitThread{false};    ///< true: signal thread to exit
    bool    _calibrationMode        = false;
    int*    _rgAxisValues           = nullptr;
    Calibration_t* _rgCalibration   = nullptr;
    ThrottleMode_t _throttleMode    = ThrottleModeDownZero;
    bool    _negativeThrust         = false;
    float   _exponential            = 0;
    bool    _accumulator            = false;
    bool    _deadband               = false;
    bool    _circleCorrection       = true;
    float   _axisFrequencyHz        = _defaultAxisFrequencyHz;
    float   _buttonFrequencyHz      = _defaultButtonFrequencyHz;
    Vehicle* _activeVehicle         = nullptr;

    bool    _pollingStartedForCalibration = false;

    QString _name;
    bool    _calibrated;
    int     _axisCount;
    int     _buttonCount;
    int     _hatCount;
    int     _hatButtonCount;
    int     _totalButtonCount;

    static int          _transmitterMode;
    int                 _rgFunctionAxis[maxFunction] = {};
    QElapsedTimer       _axisTime;

    QmlObjectListModel              _assignableButtonActions;
    QList<AssignedButtonAction*>    _buttonActionArray;
    QStringList                     _availableActionTitles;
    MultiVehicleManager*            _multiVehicleManager = nullptr;

    CustomActionManager _customActionManager;

    static constexpr const float _minAxisFrequencyHz       = 0.25f;
    static constexpr const float _maxAxisFrequencyHz       = 200.0f;
    static constexpr const float _minButtonFrequencyHz     = 0.25f;
    static constexpr const float _maxButtonFrequencyHz     = 50.0f;

private:
    const char* _txModeSettingsKey = nullptr;

    static constexpr const char* _rgFunctionSettingsKey[maxFunction] = {
        "RollAxis",
        "PitchAxis",
        "YawAxis",
        "ThrottleAxis",
        "GimbalPitchAxis",
        "GimbalYawAxis"
    };

    static constexpr const char* _settingsGroup =                  "Joysticks";
    static constexpr const char* _calibratedSettingsKey =          "Calibrated4"; // Increment number to force recalibration
    static constexpr const char* _buttonActionNameKey =            "ButtonActionName%1";
    static constexpr const char* _buttonActionRepeatKey =          "ButtonActionRepeat%1";
    static constexpr const char* _throttleModeSettingsKey =        "ThrottleMode";
    static constexpr const char* _negativeThrustSettingsKey =      "NegativeThrust";
    static constexpr const char* _exponentialSettingsKey =         "Exponential";
    static constexpr const char* _accumulatorSettingsKey =         "Accumulator";
    static constexpr const char* _deadbandSettingsKey =            "Deadband";
    static constexpr const char* _circleCorrectionSettingsKey =    "Circle_Correction";
    static constexpr const char* _axisFrequencySettingsKey =       "AxisFrequency";
    static constexpr const char* _buttonFrequencySettingsKey =     "ButtonFrequency";
    static constexpr const char* _fixedWingTXModeSettingsKey =     "TXMode_FixedWing";
    static constexpr const char* _multiRotorTXModeSettingsKey =    "TXMode_MultiRotor";
    static constexpr const char* _roverTXModeSettingsKey =         "TXMode_Rover";
    static constexpr const char* _vtolTXModeSettingsKey =          "TXMode_VTOL";
    static constexpr const char* _submarineTXModeSettingsKey =     "TXMode_Submarine";

    static constexpr const char* _buttonActionNone =               QT_TR_NOOP("No Action");
    static constexpr const char* _buttonActionArm =                QT_TR_NOOP("Arm");
    static constexpr const char* _buttonActionDisarm =             QT_TR_NOOP("Disarm");
    static constexpr const char* _buttonActionToggleArm =          QT_TR_NOOP("Toggle Arm");
    static constexpr const char* _buttonActionVTOLFixedWing =      QT_TR_NOOP("VTOL: Fixed Wing");
    static constexpr const char* _buttonActionVTOLMultiRotor =     QT_TR_NOOP("VTOL: Multi-Rotor");
    static constexpr const char* _buttonActionContinuousZoomIn =   QT_TR_NOOP("Continuous Zoom In");
    static constexpr const char* _buttonActionContinuousZoomOut =  QT_TR_NOOP("Continuous Zoom Out");
    static constexpr const char* _buttonActionStepZoomIn =         QT_TR_NOOP("Step Zoom In");
    static constexpr const char* _buttonActionStepZoomOut =        QT_TR_NOOP("Step Zoom Out");
    static constexpr const char* _buttonActionNextStream =         QT_TR_NOOP("Next Video Stream");
    static constexpr const char* _buttonActionPreviousStream =     QT_TR_NOOP("Previous Video Stream");
    static constexpr const char* _buttonActionNextCamera =         QT_TR_NOOP("Next Camera");
    static constexpr const char* _buttonActionPreviousCamera =     QT_TR_NOOP("Previous Camera");
    static constexpr const char* _buttonActionTriggerCamera =      QT_TR_NOOP("Trigger Camera");
    static constexpr const char* _buttonActionStartVideoRecord =   QT_TR_NOOP("Start Recording Video");
    static constexpr const char* _buttonActionStopVideoRecord =    QT_TR_NOOP("Stop Recording Video");
    static constexpr const char* _buttonActionToggleVideoRecord =  QT_TR_NOOP("Toggle Recording Video");
    static constexpr const char* _buttonActionGimbalDown =         QT_TR_NOOP("Gimbal Down");
    static constexpr const char* _buttonActionGimbalUp =           QT_TR_NOOP("Gimbal Up");
    static constexpr const char* _buttonActionGimbalLeft =         QT_TR_NOOP("Gimbal Left");
    static constexpr const char* _buttonActionGimbalRight =        QT_TR_NOOP("Gimbal Right");
    static constexpr const char* _buttonActionGimbalCenter =       QT_TR_NOOP("Gimbal Center");
    static constexpr const char* _buttonActionGimbalYawLock =      QT_TR_NOOP("Gimbal Yaw Lock");
    static constexpr const char* _buttonActionGimbalYawFollow =    QT_TR_NOOP("Gimbal Yaw Follow");
    static constexpr const char* _buttonActionEmergencyStop =      QT_TR_NOOP("Emergency Stop");
    static constexpr const char* _buttonActionGripperGrab =        QT_TR_NOOP("Gripper Close");
    static constexpr const char* _buttonActionGripperRelease =     QT_TR_NOOP("Gripper Open");
    static constexpr const char* _buttonActionLandingGearDeploy=   QT_TR_NOOP("Landing gear deploy");
    static constexpr const char* _buttonActionLandingGearRetract=  QT_TR_NOOP("Landing gear retract");

private slots:
    void _activeVehicleChanged(Vehicle* activeVehicle);
    void _vehicleCountChanged(int count);
    void _flightModesChanged();
};
