/****************************************************************************
 *
 *   (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#ifndef Joystick_H
#define Joystick_H

#include <QObject>
#include <QThread>

#include "QGCLoggingCategory.h"
#include "Vehicle.h"
#include "MultiVehicleManager.h"

Q_DECLARE_LOGGING_CATEGORY(JoystickLog)
Q_DECLARE_LOGGING_CATEGORY(JoystickValuesLog)

//-- Action assigned to button
class AssignedButtonAction : public QObject {
    Q_OBJECT
public:
    AssignedButtonAction(QObject* parent, const QString name);
    QString action = "No Action";
    QString secondary_action = "No Action";
    QTime   buttonTime;
    bool    repeat = false; //TODO: move this to SecondaryAction_t enum
    uint8_t buttonCombo = 0;

    typedef enum {
        SECONDARY_BUTTON_PRESS_NONE,
        SECONDARY_BUTTON_PRESS_DOUBLE,
        SECONDARY_BUTTON_PRESS_LONG,
        SECONDARY_BUTTON_PRESS_COMBO,
        SECONDARY_BUTTON_PRESS_REPEAT // TODO: replace the repeat bool when changing the UI
    } SecondaryAction_t;

    SecondaryAction_t secondaryActionType = SECONDARY_BUTTON_PRESS_NONE;

    typedef enum {
        BUTTON_IDLE_STATE, //BUTTON_UP_STATE
        BUTTON_DOWN_STATE,
        BUTTON_DOWNREPEAT_STATE,
        BUTTON_DOWNUP_STATE,
        BUTTON_DONE_STATE
    } ButtonStateMachine_t;

    ButtonStateMachine_t buttonCurrentState = BUTTON_IDLE_STATE;
};


class JoystickCustomAction : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int button1                READ  button1               WRITE  setButton1               NOTIFY button1Changed)
    Q_PROPERTY(int button2                READ  button2               WRITE  setButton2               NOTIFY button2Changed)
    Q_PROPERTY(QString action             READ  action                WRITE  setAction                NOTIFY actionChanged)
    Q_PROPERTY(int secondaryActionType    READ  secondaryActionType   WRITE  setSecondaryActionType   NOTIFY secondaryActionTypeChanged)
    Q_PROPERTY(QStringList buttonOptions  READ  buttonOptions         WRITE  setButtonOptions         NOTIFY buttonOptionsChanged)
    Q_PROPERTY(int selectedButtonIndex1   READ  selectedButtonIndex1  WRITE  setSelectedButtonIndex1  NOTIFY selectedButtonIndex1Changed)
    Q_PROPERTY(int selectedButtonIndex2   READ  selectedButtonIndex2  WRITE  setSelectedButtonIndex2  NOTIFY selectedButtonIndex2Changed)

public:
    JoystickCustomAction(QObject *parent = nullptr, int buttonCount = 0, QStringList buttonOptions = {"Undefined"});

    int button1();
    int button2();
    QString action();

    int secondaryActionType();
    QStringList buttonOptions();
    int selectedButtonIndex1();
    int selectedButtonIndex2();

    void setButton1(const int &buttonNumber);
    void setButton2(const int &buttonNumber);
    void setAction(const QString &action);
    void setSecondaryActionType(int secondaryActionType);
    void setButtonOptions(QStringList options);
    void setSelectedButtonIndex1(int button);
    void setSelectedButtonIndex2(int button);

    void addButtonToOptionsList(int button);
    void removeButtonFromOptionsList(int button);
    void updateCustomActionIndex();
    void setCurrentButton1(QString button);
    void setCurrentButton2(QString button);

signals:
    void button1Changed();
    void button2Changed();
    void actionChanged();
    void secondaryActionTypeChanged();
    void buttonOptionsChanged();
    void selectedButtonIndex1Changed();
    void selectedButtonIndex2Changed();

private:
    int _button1 = 0;
    int _button2 = 0;
    QString _action = "No Action";
    int _secondaryActionType = 0;
    QStringList _buttonOptions;
    int _selectedButtonIndex1 = 0;
    int _selectedButtonIndex2 = 0;

    int _buttonCount;
    QString _currentButton1 = "Unassigned";
    QString _currentButton2 = "Unassigned";
};

//-- Assignable Button Action
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
    Q_PROPERTY(QStringList buttonActions                   READ buttonActions                   NOTIFY buttonActionsChanged)
    Q_PROPERTY(QStringList buttonSecondaryActions          READ buttonSecondaryActions          NOTIFY buttonActionsChanged)
    Q_PROPERTY(QStringList buttonSecondaryActionTypeList   READ buttonSecondaryActionTypeList   NOTIFY buttonSecondaryActionTypeListChanged)

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
    Q_PROPERTY(QList<QObject*> customActions    READ customActions                                      NOTIFY customActionsChanged)

    Q_INVOKABLE AssignableButtonAction* getAssignableAction(const QString& action);
    Q_INVOKABLE void    setButtonRepeat     (int button, bool repeat);
    Q_INVOKABLE bool    getButtonRepeat     (int button);
    Q_INVOKABLE void    setButtonAction     (int button, const QString& action, bool primaryAction);
    Q_INVOKABLE void    createAvailableButtonsList  ();
    Q_INVOKABLE void    createCustomActionsTable    ();
    Q_INVOKABLE void    newCustomAction             ();
    Q_INVOKABLE void    updateCustomActions         (int customIndex, const QString& button1, const QString& button2, const QString& action, int secondaryActionType);
    Q_INVOKABLE void    removeCustomAction          (int comboIndex);


    // Property accessors
    QString     name                () { return _name; }
    int         totalButtonCount    () { return _totalButtonCount; }
    int         axisCount           () { return _axisCount; }
    bool        gimbalEnabled       () { return _gimbalEnabled; }
    QStringList buttonActions       ();
    QStringList buttonSecondaryActions       ();
    QStringList buttonNumberingList       ();
    QStringList buttonSecondaryActionTypeList();

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

    QList<QObject*> customActions ();


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

    float axisFrequency     () { return _axisFrequency; }
    void  setAxisFrequency  (float val);

    float buttonFrequency   () { return _buttonFrequency; }
    void  setButtonFrequency(float val);

    void addButtonGlobalCustomActionsList(int button);
    void removeButtonGlobalCustomActionsList(int button);
    void addButtonLocalCustomActionsLists(int button, int customIndex);
    void removeButtonLocalCustomActionsLists(int button, int customIndex);
    QString getButtonAction     (int button, bool primary);
    void    setButtonDouble     (int button, bool doublePress);
    void    setButtonLong       (int button, bool longPress);

signals:
    // The raw signals are only meant for use by calibration
    void rawAxisValueChanged        (int index, int value);
    void rawButtonPressedChanged    (int index, int pressed);
    void calibratedChanged          (bool calibrated);
    void buttonActionsChanged       ();
    void buttonNumberingChanged();
    void buttonSecondaryActionTypeListChanged();
    void assignableActionsChanged   ();
    void throttleModeChanged        (int mode);
    void negativeThrustChanged      (bool allowNegative);
    void exponentialChanged         (float exponential);
    void accumulatorChanged         (bool accumulator);
    void enabledChanged             (bool enabled);
    void circleCorrectionChanged    (bool circleCorrection);
    void customActionsChanged        ();
    void buttonRepeatChanged(int buttonIndex, bool set);
    void buttonCustomActionChanged(int customActionIndex, bool set);

    /// Signal containing new joystick information
    ///     @param roll         Range is -1:1, negative meaning roll left, positive meaning roll right
    ///     @param pitch        Range i -1:1, negative meaning pitch down, positive meaning pitch up
    ///     @param yaw          Range is -1:1, negative meaning yaw left, positive meaning yaw right
    ///     @param throttle     Range is 0:1, 0 meaning no throttle, 1 meaning full throttle
    ///     @param mode     See Vehicle::JoystickMode_t enum
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
    void toggleCameraMode           ();
    void startVideoRecord           ();
    void stopVideoRecord            ();
    void toggleVideoRecord          ();
    void toggleThermal              ();
    void switchThermalOn            ();
    void switchThermalOff           ();
    void thermalNextPalette         ();
    void gimbalPitchStep            (int direction);
    void gimbalYawStep              (int direction);
    void startGimbalPitch           (int direction);
    void centerGimbal               ();
    void gimbalControlValue         (double pitch, double yaw);
    void startContinuousGimbalPitch (int direction);
    void stopContinuousGimbalPitch  ();
    void setArmed                   (bool arm);
    void setVtolInFwdFlight         (bool set);
    void setFlightMode              (const QString& flightMode);

private slots:
    void handleManualControlGimbal(float gimbalPitch, float gimbalYaw);

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

    void setButtonCombo(int button, int comboButton, const QString& action, bool comboPress);
    void removeButtonComboTable(int button, bool comboAction);
    void updateCustomActionsIndexes(int customIndex, const QString& button1, const QString& button2);

    // Override from QThread
    virtual void run();

protected:

    enum {
        BUTTON_UP,
        BUTTON_DOWN
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

    QList<QObject*>  _customActions;
    QList<bool>  _comboEnabled;
    QStringList _buttonOptions;

private:
    static const char*  _rgFunctionSettingsKey[maxFunction];

    static const char* _settingsGroup;
    static const char* _calibratedSettingsKey;
    static const char* _buttonActionNameKey;
    static const char* _buttonSecondaryActionNameKey;
    static const char* _buttonActionRepeatKey;
    static const char* _buttonSecondaryActionTypeKey;
    static const char* _buttonComboKey;
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
    static const char* _buttonActionToggleCameraMode;
    static const char* _buttonActionStartVideoRecord;
    static const char* _buttonActionStopVideoRecord;
    static const char* _buttonActionToggleVideoRecord;
    static const char* _buttonActionGimbalDown;
    static const char* _buttonActionGimbalUp;
    static const char* _buttonActionGimbalLeft;
    static const char* _buttonActionGimbalRight;
    static const char* _buttonActionGimbalCenter;
    static const char* _buttonActionGimbalPitchUp;
    static const char* _buttonActionGimbalPitchDown;
    static const char* _buttonActionToggleThermal;
    static const char* _buttonActionThermalOn;
    static const char* _buttonActionThermalOff;
    static const char* _buttonActionThermalNextPalette;

private slots:
    void _activeVehicleChanged(Vehicle* activeVehicle);
};

#endif
