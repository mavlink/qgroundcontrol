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

#include <QObject>
#include <QThread>

#include "QGCLoggingCategory.h"
#include "Vehicle.h"
#include "MultiVehicleManager.h"

Q_DECLARE_LOGGING_CATEGORY(JoystickLog)
Q_DECLARE_LOGGING_CATEGORY(JoystickValuesLog)

/// Action assigned to button
class AssignedButtonAction : public QObject {
    Q_OBJECT
public:
    AssignedButtonAction(QObject* parent, const QString name);
    QString action;
    QTime   buttonTime;
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
    bool    canRepeat   () { return _repeat; }
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

    ~Joystick();

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

    Q_PROPERTY(bool     gimbalEnabled           READ gimbalEnabled          WRITE setGimbalEnabled      NOTIFY gimbalEnabledChanged)
    Q_PROPERTY(int      throttleMode            READ throttleMode           WRITE setThrottleMode       NOTIFY throttleModeChanged)
    Q_PROPERTY(float    axisFrequency           READ axisFrequency          WRITE setAxisFrequency      NOTIFY axisFrequencyChanged)
    Q_PROPERTY(float    buttonFrequency         READ buttonFrequency        WRITE setButtonFrequency    NOTIFY buttonFrequencyChanged)
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
    int         totalButtonCount    () { return _totalButtonCount; }
    int         axisCount           () { return _axisCount; }
    bool        gimbalEnabled       () { return _gimbalEnabled; }
    QStringList buttonActions       ();

    QmlObjectListModel* assignableActions   () { return &_assignableButtonActions; }
    QStringList assignableActionTitles      () { return _availableActionTitles; }
    QString     disabledActionName          () { return _buttonActionNone; }

    void setGimbalEnabled           (bool set);

    /// Start the polling thread which will in turn emit joystick signals
    void startPolling(Vehicle* vehicle);
    void stopPolling(void);

    void setCalibration(int axis, Calibration_t& calibration);
    Calibration_t getCalibration(int axis);

    void setFunctionAxis(AxisFunction_t function, int axis);
    int getFunctionAxis(AxisFunction_t function);


/*
    // Joystick index used by sdl library
    // Settable because sdl library remaps indices after certain events
    virtual int index(void) = 0;
    virtual void setIndex(int index) = 0;
*/
	virtual bool requiresCalibration(void) { return true; }

    int   throttleMode      ();
    void  setThrottleMode   (int mode);

    bool  negativeThrust    ();
    void  setNegativeThrust (bool allowNegative);

    float exponential       ();
    void  setExponential    (float expo);

    bool  accumulator       ();
    void  setAccumulator    (bool accu);

    bool  deadband          ();
    void  setDeadband       (bool accu);

    bool  circleCorrection  ();
    void  setCircleCorrection(bool circleCorrection);

    void  setTXMode         (int mode);
    int   getTXMode         () { return _transmitterMode; }

    /// Set the current calibration mode
    void  setCalibrationMode (bool calibrating);

    /// Get joystick message rate (in Hz)
    float axisFrequency     () { return _axisFrequency; }
    /// Set joystick message rate (in Hz)
    void  setAxisFrequency  (float val);

    /// Get joystick button repeat rate (in Hz)
    float buttonFrequency   () { return _buttonFrequency; }
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

    /// Signal containing new joystick information
    ///     @param roll:            Range is -1:1, negative meaning roll left, positive meaning roll right
    ///     @param pitch:           Range i -1:1, negative meaning pitch down, positive meaning pitch up
    ///     @param yaw:             Range is -1:1, negative meaning yaw left, positive meaning yaw right
    ///     @param throttle:        Range is 0:1, 0 meaning no throttle, 1 meaning full throttle
    ///     @param buttons:         Button bitmap
    ///     @param joystickMmode:   Current joystick mode
    void manualControl              (float roll, float pitch, float yaw, float throttle, quint16 buttons, int joystickMmode);
    void manualControlGimbal        (float gimbalPitch, float gimbalYaw);

    void buttonActionTriggered      (int action);

    void gimbalEnabledChanged       ();
    void axisFrequencyChanged       ();
    void buttonFrequencyChanged     ();
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
    void gimbalControlValue         (double pitch, double yaw);
    void setArmed                   (bool arm);
    void setVtolInFwdFlight         (bool set);
    void setFlightMode              (const QString& flightMode);

protected:
    void    _setDefaultCalibration  ();
    void    _saveSettings           ();
    void    _saveButtonSettings     ();
    void    _loadSettings           ();
    float   _adjustRange            (int value, Calibration_t calibration, bool withDeadbands);
    void    _executeButtonAction    (const QString& action, bool buttonDown);
    int     _findAssignableButtonAction(const QString& action);
    bool    _validAxis              (int axis);
    bool    _validButton            (int button);
    void    _handleAxis             ();
    void    _handleButtons          ();
    void    _buildActionList        (Vehicle* activeVehicle);

    void    _pitchStep              (int direction);
    void    _yawStep                (int direction);
    double  _localYaw       = 0.0;
    double  _localPitch     = 0.0;

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

    uint8_t*_rgButtonValues         = nullptr;

    bool    _exitThread             = false;    ///< true: signal thread to exit
    bool    _calibrationMode        = false;
    int*    _rgAxisValues           = nullptr;
    Calibration_t* _rgCalibration   = nullptr;
    ThrottleMode_t _throttleMode    = ThrottleModeDownZero;
    bool    _negativeThrust         = false;
    float   _exponential            = 0;
    bool    _accumulator            = false;
    bool    _deadband               = false;
    bool    _circleCorrection       = true;
    float   _axisFrequency          = 25.0f;
    float   _buttonFrequency        = 5.0f;
    Vehicle* _activeVehicle         = nullptr;
    bool    _gimbalEnabled          = false;

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
    QTime               _axisTime;

    QmlObjectListModel              _assignableButtonActions;
    QList<AssignedButtonAction*>    _buttonActionArray;
    QStringList                     _availableActionTitles;
    MultiVehicleManager*            _multiVehicleManager = nullptr;


private:
    static const char*  _rgFunctionSettingsKey[maxFunction];

    static const char* _settingsGroup;
    static const char* _calibratedSettingsKey;
    static const char* _buttonActionNameKey;
    static const char* _buttonActionRepeatKey;
    static const char* _throttleModeSettingsKey;
    static const char* _exponentialSettingsKey;
    static const char* _accumulatorSettingsKey;
    static const char* _deadbandSettingsKey;
    static const char* _circleCorrectionSettingsKey;
    static const char* _axisFrequencySettingsKey;
    static const char* _buttonFrequencySettingsKey;
    static const char* _txModeSettingsKey;
    static const char* _fixedWingTXModeSettingsKey;
    static const char* _multiRotorTXModeSettingsKey;
    static const char* _roverTXModeSettingsKey;
    static const char* _vtolTXModeSettingsKey;
    static const char* _submarineTXModeSettingsKey;
    static const char* _gimbalSettingsKey;

    static const char* _buttonActionNone;
    static const char* _buttonActionArm;
    static const char* _buttonActionDisarm;
    static const char* _buttonActionToggleArm;
    static const char* _buttonActionVTOLFixedWing;
    static const char* _buttonActionVTOLMultiRotor;
    static const char* _buttonActionStepZoomIn;
    static const char* _buttonActionStepZoomOut;
    static const char* _buttonActionContinuousZoomIn;
    static const char* _buttonActionContinuousZoomOut;
    static const char* _buttonActionNextStream;
    static const char* _buttonActionPreviousStream;
    static const char* _buttonActionNextCamera;
    static const char* _buttonActionPreviousCamera;
    static const char* _buttonActionTriggerCamera;
    static const char* _buttonActionStartVideoRecord;
    static const char* _buttonActionStopVideoRecord;
    static const char* _buttonActionToggleVideoRecord;
    static const char* _buttonActionGimbalDown;
    static const char* _buttonActionGimbalUp;
    static const char* _buttonActionGimbalLeft;
    static const char* _buttonActionGimbalRight;
    static const char* _buttonActionGimbalCenter;

private slots:
    void _activeVehicleChanged(Vehicle* activeVehicle);
};
