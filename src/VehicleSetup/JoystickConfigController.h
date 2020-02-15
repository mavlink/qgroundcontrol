/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

/// @file
///     @brief Joystick Config Qml Controller
///     @author Don Gagne <don@thegagnes.com

#ifndef JoystickConfigController_H
#define JoystickConfigController_H

#include <QTimer>

#include "FactPanelController.h"
#include "QGCLoggingCategory.h"
#include "Joystick.h"

Q_DECLARE_LOGGING_CATEGORY(JoystickConfigControllerLog)

class RadioConfigest;
class JoystickManager;

namespace Ui {
    class JoystickConfigController;
}

class JoystickConfigController : public FactPanelController
{
    Q_OBJECT
    
    //friend class RadioConfigTest; ///< This allows our unit test to access internal information needed.
    
public:
    JoystickConfigController(void);
    ~JoystickConfigController();
    
    Q_PROPERTY(QString statusText               READ statusText                 NOTIFY statusTextChanged)

    Q_PROPERTY(bool rollAxisMapped              READ rollAxisMapped             NOTIFY rollAxisMappedChanged)
    Q_PROPERTY(bool pitchAxisMapped             READ pitchAxisMapped            NOTIFY pitchAxisMappedChanged)
    Q_PROPERTY(bool yawAxisMapped               READ yawAxisMapped              NOTIFY yawAxisMappedChanged)
    Q_PROPERTY(bool throttleAxisMapped          READ throttleAxisMapped         NOTIFY throttleAxisMappedChanged)

    Q_PROPERTY(bool hasGimbalPitch              READ hasGimbalPitch             NOTIFY hasGimbalPitchChanged)
    Q_PROPERTY(bool hasGimbalYaw                READ hasGimbalYaw               NOTIFY hasGimbalYawChanged)

    Q_PROPERTY(bool gimbalPitchAxisMapped       READ gimbalPitchAxisMapped      NOTIFY gimbalPitchAxisMappedChanged)
    Q_PROPERTY(bool gimbalYawAxisMapped         READ gimbalYawAxisMapped        NOTIFY gimbalYawAxisMappedChanged)

    Q_PROPERTY(int  rollAxisReversed            READ rollAxisReversed           NOTIFY rollAxisReversedChanged)
    Q_PROPERTY(int  pitchAxisReversed           READ pitchAxisReversed          NOTIFY pitchAxisReversedChanged)
    Q_PROPERTY(int  yawAxisReversed             READ yawAxisReversed            NOTIFY yawAxisReversedChanged)
    Q_PROPERTY(int  throttleAxisReversed        READ throttleAxisReversed       NOTIFY throttleAxisReversedChanged)

    Q_PROPERTY(int  gimbalPitchAxisReversed     READ gimbalPitchAxisReversed    NOTIFY gimbalPitchAxisReversedChanged)
    Q_PROPERTY(int  gimbalYawAxisReversed       READ gimbalYawAxisReversed      NOTIFY gimbalYawAxisReversedChanged)

    Q_PROPERTY(bool deadbandToggle              READ getDeadbandToggle          WRITE setDeadbandToggle    NOTIFY deadbandToggled)

    Q_PROPERTY(int  transmitterMode             READ transmitterMode            WRITE setTransmitterMode NOTIFY transmitterModeChanged)
    Q_PROPERTY(bool calibrating                 READ calibrating                NOTIFY calibratingChanged)
    Q_PROPERTY(bool nextEnabled                 READ nextEnabled                NOTIFY nextEnabledChanged)
    Q_PROPERTY(bool skipEnabled                 READ skipEnabled                NOTIFY skipEnabledChanged)

    Q_PROPERTY(QList<qreal> stickPositions      READ stickPositions             NOTIFY stickPositionsChanged)
    Q_PROPERTY(QList<qreal> gimbalPositions     READ gimbalPositions            NOTIFY gimbalPositionsChanged)

    Q_INVOKABLE void cancelButtonClicked    ();
    Q_INVOKABLE void skipButtonClicked      ();
    Q_INVOKABLE void nextButtonClicked      ();
    Q_INVOKABLE void start                  ();
    Q_INVOKABLE void setDeadbandValue       (int axis, int value);

    QString statusText                      () { return _statusText; }

    bool rollAxisMapped                     () { return _rgFunctionAxisMapping[Joystick::rollFunction]          != _axisNoAxis; }
    bool pitchAxisMapped                    () { return _rgFunctionAxisMapping[Joystick::pitchFunction]         != _axisNoAxis; }
    bool yawAxisMapped                      () { return _rgFunctionAxisMapping[Joystick::yawFunction]           != _axisNoAxis; }
    bool throttleAxisMapped                 () { return _rgFunctionAxisMapping[Joystick::throttleFunction]      != _axisNoAxis; }
    bool gimbalPitchAxisMapped              () { return _rgFunctionAxisMapping[Joystick::gimbalPitchFunction]   != _axisNoAxis; }
    bool gimbalYawAxisMapped                () { return _rgFunctionAxisMapping[Joystick::gimbalYawFunction]     != _axisNoAxis; }

    bool rollAxisReversed                   ();
    bool pitchAxisReversed                  ();
    bool yawAxisReversed                    ();
    bool throttleAxisReversed               ();
    bool gimbalPitchAxisReversed            ();
    bool gimbalYawAxisReversed              ();

    bool hasGimbalPitch                     () { return _axisCount > 4; }
    bool hasGimbalYaw                       () { return _axisCount > 5; }

    bool getDeadbandToggle                  ();
    void setDeadbandToggle                  (bool);

    int  axisCount                          () { return _axisCount; }

    int  transmitterMode                    () { return _transmitterMode; }
    void setTransmitterMode                 (int mode);

    bool calibrating                        () { return _currentStep != -1; }
    bool nextEnabled                        ();
    bool skipEnabled                        ();

    QList<qreal> stickPositions             () { return _currentStickPositions; }
    QList<qreal> gimbalPositions            () { return _currentGimbalPositions; }

    struct stateStickPositions {
        qreal   leftX;
        qreal   leftY;
        qreal   rightX;
        qreal   rightY;
    };

signals:
    void axisValueChanged                   (int axis, int value);
    void axisDeadbandChanged                (int axis, int value);
    void rollAxisMappedChanged              (bool mapped);
    void pitchAxisMappedChanged             (bool mapped);
    void yawAxisMappedChanged               (bool mapped);
    void throttleAxisMappedChanged          (bool mapped);
    void gimbalPitchAxisMappedChanged       (bool mapped);
    void gimbalYawAxisMappedChanged         (bool mapped);
    void rollAxisReversedChanged            (bool reversed);
    void pitchAxisReversedChanged           (bool reversed);
    void yawAxisReversedChanged             (bool reversed);
    void throttleAxisReversedChanged        (bool reversed);
    void gimbalPitchAxisReversedChanged     (bool reversed);
    void gimbalYawAxisReversedChanged       (bool reversed);
    void deadbandToggled                    (bool value);
    void transmitterModeChanged             (int mode);
    void calibratingChanged                 ();
    void nextEnabledChanged                 ();
    void skipEnabledChanged                 ();
    void stickPositionsChanged              ();
    void gimbalPositionsChanged             ();
    void hasGimbalPitchChanged              ();
    void hasGimbalYawChanged                ();
    void statusTextChanged                  ();

    // @brief Signalled when in unit test mode and a message box should be displayed by the next button
    void nextButtonMessageBoxDisplayed      ();

private slots:
    void _activeJoystickChanged(Joystick* joystick);
    void _axisValueChanged(int axis, int value);
    void _axisDeadbandChanged(int axis, int value);
   
private:
    /// @brief The states of the calibration state machine.
    enum calStates {
        calStateAxisWait,
        calStateBegin,
        calStateIdentify,
        calStateMinMax,
        calStateCenterThrottle,
        calStateDetectInversion,
        calStateTrims,
        calStateSave
    };

    typedef void (JoystickConfigController::*inputFn)(Joystick::AxisFunction_t function, int axis, int value);
    typedef void (JoystickConfigController::*buttonFn)(void);
    struct stateMachineEntry {
        Joystick::AxisFunction_t    function;
        const char*                 instructions;
        stateStickPositions         stickPositions;
        stateStickPositions         gimbalPositions;
        inputFn                     rcInputFn;
        buttonFn                    nextFn;
        buttonFn                    skipFn;
        int                         channelID;
    };
    
    /// @brief A set of information associated with a radio axis.
    struct AxisInfo {
        Joystick::AxisFunction_t    function;   ///< Function mapped to this axis, Joystick::maxFunction for none
        bool                        reversed;   ///< true: axis is reverse, false: not reversed
        int                         axisMin;    ///< Minimum axis value
        int                         axisMax;    ///< Maximum axis value
        int                         axisTrim;   ///< Trim position
        int                         deadband;   ///< Deadband
    };
    
    Joystick* _activeJoystick = nullptr;
    
    int _transmitterMode    = 2;
    int _currentStep        = -1;  ///< Current step of state machine
    
    const struct stateMachineEntry* _getStateMachineEntry(int step);
    
    void _advanceState          ();
    void _setupCurrentState     ();
    
    bool _validAxis             (int axis);

    void _inputCenterWaitBegin  (Joystick::AxisFunction_t function, int axis, int value);
    void _inputStickDetect      (Joystick::AxisFunction_t function, int axis, int value);
    void _inputStickMin         (Joystick::AxisFunction_t function, int axis, int value);
    void _inputCenterWait       (Joystick::AxisFunction_t function, int axis, int value);
    
    void _switchDetect          (Joystick::AxisFunction_t function, int axis, int value, bool moveToNextStep);
    
    void _saveFlapsDown         ();
    void _skipFlaps             ();
    void _saveAllTrims          ();
    
    bool _stickSettleComplete   (int axis, int value);
    
    void _validateCalibration   ();
    void _writeCalibration      ();
    void _resetInternalCalibrationValues();
    void _setInternalCalibrationValuesFromSettings();
    
    void _startCalibration      ();
    void _stopCalibration       ();

    void _calSaveCurrentValues  ();
    
    void _setStickPositions     ();
    
    void _signalAllAttitudeValueChanges();

    void _setStatusText         (const QString& text);
    
    stateStickPositions _sticksCentered;
    stateStickPositions _sticksThrottleUp;
    stateStickPositions _sticksThrottleDown;
    stateStickPositions _sticksYawLeft;
    stateStickPositions _sticksYawRight;
    stateStickPositions _sticksRollLeft;
    stateStickPositions _sticksRollRight;
    stateStickPositions _sticksPitchUp;
    stateStickPositions _sticksPitchDown;

    QList<qreal> _currentStickPositions;
    QList<qreal> _currentGimbalPositions;

    int _rgFunctionAxisMapping[Joystick::maxFunction]; ///< Maps from joystick function to axis index. _axisMax indicates axis not set for this function.

    static const int _attitudeControls  = 5;
    
    int                 _axisCount      = 0;        ///< Number of actual joystick axes available
    static const int    _axisNoAxis     = -1;       ///< Signals no axis set
    static const int    _axisMinimum    = 4;        ///< Minimum numner of joystick axes required to run PX4
    struct AxisInfo*    _rgAxisInfo     = nullptr;  ///< Information associated with each axis
    int*                _axisValueSave  = nullptr;  ///< Saved values prior to detecting axis movement
    int*                _axisRawValue   = nullptr;  ///< Current set of raw axis values

    enum calStates _calState = calStateAxisWait;    ///< Current calibration state

    int     _calStateCurrentAxis;                   ///< Current axis being worked on in calStateIdentify and calStateDetectInversion
    bool    _calStateAxisComplete;                  ///< Work associated with current axis is complete
    int     _calStateIdentifyOldMapping;            ///< Previous mapping for axis being currently identified
    int     _calStateReverseOldMapping;             ///< Previous mapping for axis being currently used to detect inversion
    
    static const int _calCenterPoint;
    static const int _calValidMinValue;
    static const int _calValidMaxValue;
    static const int _calDefaultMinValue;
    static const int _calDefaultMaxValue;
    static const int _calRoughCenterDelta;
    static const int _calMoveDelta;
    static const int _calSettleDelta;
    static const int _calMinDelta;    
    
    int     _stickDetectAxis;
    int     _stickDetectInitialValue;
    int     _stickDetectValue;
    bool    _stickDetectSettleStarted;
    QTime   _stickDetectSettleElapsed;

    static const int _stickDetectSettleMSecs;

    QString             _statusText;
    JoystickManager*    _joystickManager;
};

#endif // JoystickConfigController_H
