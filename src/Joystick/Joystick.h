/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
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
        maxFunction
    } AxisFunction_t;

    typedef enum {
        ThrottleModeCenterZero,
        ThrottleModeDownZero,
        ThrottleModeMax
    } ThrottleMode_t;

    Q_PROPERTY(QString name READ name CONSTANT)

    Q_PROPERTY(bool calibrated MEMBER _calibrated NOTIFY calibratedChanged)

    Q_PROPERTY(int totalButtonCount  READ totalButtonCount    CONSTANT)
    Q_PROPERTY(int axisCount    READ axisCount      CONSTANT)

    Q_PROPERTY(QStringList actions READ actions CONSTANT)

    Q_PROPERTY(QVariantList buttonActions READ buttonActions NOTIFY buttonActionsChanged)
    Q_INVOKABLE void setButtonAction(int button, const QString& action);
    Q_INVOKABLE QString getButtonAction(int button);

    Q_PROPERTY(int throttleMode READ throttleMode WRITE setThrottleMode NOTIFY throttleModeChanged)
    Q_PROPERTY(bool negativeThrust READ negativeThrust WRITE setNegativeThrust NOTIFY negativeThrustChanged)
    Q_PROPERTY(float exponential READ exponential WRITE setExponential NOTIFY exponentialChanged)
    Q_PROPERTY(bool accumulator READ accumulator WRITE setAccumulator NOTIFY accumulatorChanged)
    Q_PROPERTY(bool requiresCalibration READ requiresCalibration CONSTANT)
    Q_PROPERTY(bool circleCorrection READ circleCorrection WRITE setCircleCorrection NOTIFY circleCorrectionChanged)
    Q_PROPERTY(float frequency READ frequency WRITE setFrequency NOTIFY frequencyChanged)

    // Property accessors

    int axisCount(void) { return _axisCount; }
    int totalButtonCount(void) { return _totalButtonCount; }

    /// Start the polling thread which will in turn emit joystick signals
    void startPolling(Vehicle* vehicle);
    void stopPolling(void);

    void setCalibration(int axis, Calibration_t& calibration);
    Calibration_t getCalibration(int axis);

    void setFunctionAxis(AxisFunction_t function, int axis);
    int getFunctionAxis(AxisFunction_t function);

    QStringList actions(void);
    QVariantList buttonActions(void);

    QString name(void) { return _name; }
/*
    // Joystick index used by sdl library
    // Settable because sdl library remaps indices after certain events
    virtual int index(void) = 0;
    virtual void setIndex(int index) = 0;
*/
	virtual bool requiresCalibration(void) { return true; }

    int throttleMode(void);
    void setThrottleMode(int mode);

    bool negativeThrust(void);
    void setNegativeThrust(bool allowNegative);

    float exponential(void);
    void setExponential(float expo);

    bool accumulator(void);
    void setAccumulator(bool accu);

    bool deadband(void);
    void setDeadband(bool accu);

    bool circleCorrection(void);
    void setCircleCorrection(bool circleCorrection);

    void setTXMode(int mode);
    int getTXMode(void) { return _transmitterMode; }

    /// Set the current calibration mode
    void setCalibrationMode(bool calibrating);

    float frequency();
    void setFrequency(float val);

signals:
    void calibratedChanged(bool calibrated);

    // The raw signals are only meant for use by calibration
    void rawAxisValueChanged(int index, int value);
    void rawButtonPressedChanged(int index, int pressed);

    void buttonActionsChanged(QVariantList actions);

    void throttleModeChanged(int mode);

    void negativeThrustChanged(bool allowNegative);

    void exponentialChanged(float exponential);

    void accumulatorChanged(bool accumulator);

    void enabledChanged(bool enabled);

    void circleCorrectionChanged(bool circleCorrection);

    /// Signal containing new joystick information
    ///     @param roll     Range is -1:1, negative meaning roll left, positive meaning roll right
    ///     @param pitch    Range i -1:1, negative meaning pitch down, positive meaning pitch up
    ///     @param yaw      Range is -1:1, negative meaning yaw left, positive meaning yaw right
    ///     @param throttle Range is 0:1, 0 meaning no throttle, 1 meaning full throttle
    ///     @param mode     See Vehicle::JoystickMode_t enum
    void manualControl(float roll, float pitch, float yaw, float throttle, quint16 buttons, int joystickMmode);

    void buttonActionTriggered(int action);

    void frequencyChanged   ();
    void stepZoom           (int direction);
    void stepCamera         (int direction);
    void stepStream         (int direction);

protected:
    void    _setDefaultCalibration(void);
    void    _saveSettings(void);
    void    _loadSettings(void);
    float   _adjustRange(int value, Calibration_t calibration, bool withDeadbands);
    void    _buttonAction(const QString& action);
    bool    _validAxis(int axis);
    bool    _validButton(int button);

private:
    virtual bool _open() = 0;
    virtual void _close() = 0;
    virtual bool _update() = 0;

    virtual bool _getButton(int i) = 0;
    virtual int _getAxis(int i) = 0;
    virtual uint8_t _getHat(int hat,int i) = 0;

    void _updateTXModeSettingsKey(Vehicle* activeVehicle);
    int _mapFunctionMode(int mode, int function);
    void _remapAxes(int currentMode, int newMode, int (&newMapping)[maxFunction]);

    // Override from QThread
    virtual void run(void);

protected:

    bool    _exitThread;    ///< true: signal thread to exit

    QString _name;
    bool    _calibrated;
    int     _axisCount;
    int     _buttonCount;
    int     _hatCount;
    int     _hatButtonCount;
    int     _totalButtonCount;

    static int          _transmitterMode;
    bool                _calibrationMode;

    int*                _rgAxisValues;
    Calibration_t*      _rgCalibration;
    int                 _rgFunctionAxis[maxFunction];

    bool*               _rgButtonValues;
    QStringList         _rgButtonActions;
    quint16             _lastButtonBits;

    ThrottleMode_t      _throttleMode;

    bool                _negativeThrust;

    float                _exponential;
    bool                _accumulator;
    bool                _deadband;
    bool                _circleCorrection;
    float               _frequency;

    Vehicle*            _activeVehicle;
    bool                _pollingStartedForCalibration;

    MultiVehicleManager*    _multiVehicleManager;

private:
    static const char*  _rgFunctionSettingsKey[maxFunction];

    static const char* _settingsGroup;
    static const char* _calibratedSettingsKey;
    static const char* _buttonActionSettingsKey;
    static const char* _throttleModeSettingsKey;
    static const char* _exponentialSettingsKey;
    static const char* _accumulatorSettingsKey;
    static const char* _deadbandSettingsKey;
    static const char* _circleCorrectionSettingsKey;
    static const char* _frequencySettingsKey;
    static const char* _txModeSettingsKey;
    static const char* _fixedWingTXModeSettingsKey;
    static const char* _multiRotorTXModeSettingsKey;
    static const char* _roverTXModeSettingsKey;
    static const char* _vtolTXModeSettingsKey;
    static const char* _submarineTXModeSettingsKey;

    static const char* _buttonActionArm;
    static const char* _buttonActionDisarm;
    static const char* _buttonActionVTOLFixedWing;
    static const char* _buttonActionVTOLMultiRotor;
    static const char* _buttonActionZoomIn;
    static const char* _buttonActionZoomOut;
    static const char* _buttonActionNextStream;
    static const char* _buttonActionPreviousStream;
    static const char* _buttonActionNextCamera;
    static const char* _buttonActionPreviousCamera;

private slots:
    void _activeVehicleChanged(Vehicle* activeVehicle);
};

#endif
