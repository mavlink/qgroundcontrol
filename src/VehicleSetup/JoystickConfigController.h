/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/



/// @file
///     @brief Radio Config Qml Controller
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
    
    friend class RadioConfigTest; ///< This allows our unit test to access internal information needed.
    
public:
    JoystickConfigController(void);
    ~JoystickConfigController();
    
    Q_PROPERTY(QQuickItem* statusText   MEMBER _statusText)
    Q_PROPERTY(QQuickItem* cancelButton MEMBER _cancelButton)
    Q_PROPERTY(QQuickItem* nextButton   MEMBER _nextButton)
    Q_PROPERTY(QQuickItem* skipButton   MEMBER _skipButton)
    
    Q_PROPERTY(bool rollAxisMapped      READ rollAxisMapped     NOTIFY rollAxisMappedChanged)
    Q_PROPERTY(bool pitchAxisMapped     READ pitchAxisMapped    NOTIFY pitchAxisMappedChanged)
    Q_PROPERTY(bool yawAxisMapped       READ yawAxisMapped      NOTIFY yawAxisMappedChanged)
    Q_PROPERTY(bool throttleAxisMapped  READ throttleAxisMapped NOTIFY throttleAxisMappedChanged)

    Q_PROPERTY(int rollAxisReversed     READ rollAxisReversed       NOTIFY rollAxisReversedChanged)
    Q_PROPERTY(int pitchAxisReversed    READ pitchAxisReversed      NOTIFY pitchAxisReversedChanged)
    Q_PROPERTY(int yawAxisReversed      READ yawAxisReversed        NOTIFY yawAxisReversedChanged)
    Q_PROPERTY(int throttleAxisReversed READ throttleAxisReversed   NOTIFY throttleAxisReversedChanged)
    
    Q_PROPERTY(bool deadbandToggle            READ getDeadbandToggle        WRITE setDeadbandToggle    NOTIFY deadbandToggled)

    Q_PROPERTY(int transmitterMode READ transmitterMode WRITE setTransmitterMode NOTIFY transmitterModeChanged)
    Q_PROPERTY(QString imageHelp MEMBER _imageHelp NOTIFY imageHelpChanged)
    Q_PROPERTY(bool calibrating READ calibrating NOTIFY calibratingChanged)
    
    Q_INVOKABLE void cancelButtonClicked(void);
    Q_INVOKABLE void skipButtonClicked(void);
    Q_INVOKABLE void nextButtonClicked(void);
    Q_INVOKABLE void start(void);
    Q_INVOKABLE void setDeadbandValue(int axis, int value);

    bool rollAxisMapped(void);
    bool pitchAxisMapped(void);
    bool yawAxisMapped(void);
    bool throttleAxisMapped(void);
    
    bool rollAxisReversed(void);
    bool pitchAxisReversed(void);
    bool yawAxisReversed(void);
    bool throttleAxisReversed(void);
    
    bool getDeadbandToggle(void);
    void setDeadbandToggle(bool);

    int axisCount(void);

    int transmitterMode(void) { return _transmitterMode; }
    void setTransmitterMode(int mode);

    bool calibrating(void) { return _currentStep != -1; }
    
signals:
    void axisValueChanged(int axis, int value);
    void axisDeadbandChanged(int axis, int value);

    void rollAxisMappedChanged(bool mapped);
    void pitchAxisMappedChanged(bool mapped);
    void yawAxisMappedChanged(bool mapped);
    void throttleAxisMappedChanged(bool mapped);

    void rollAxisReversedChanged(bool reversed);
    void pitchAxisReversedChanged(bool reversed);
    void yawAxisReversedChanged(bool reversed);
    void throttleAxisReversedChanged(bool reversed);

    void deadbandToggled(bool value);
    
    void imageHelpChanged(QString source);
    void transmitterModeChanged(int mode);
    void calibratingChanged(void);
    
    // @brief Signalled when in unit test mode and a message box should be displayed by the next button
    void nextButtonMessageBoxDisplayed(void);

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
        const char*                 image;
        inputFn                     rcInputFn;
        buttonFn                    nextFn;
        buttonFn                    skipFn;
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
    
    Joystick* _activeJoystick;
    
    int _transmitterMode;
    int _currentStep;  ///< Current step of state machine
    
    const struct stateMachineEntry* _getStateMachineEntry(int step);
    
    void _advanceState(void);
    void _setupCurrentState(void);
    
    bool _validAxis(int axis);

    void _inputCenterWaitBegin  (Joystick::AxisFunction_t function, int axis, int value);
    void _inputStickDetect      (Joystick::AxisFunction_t function, int axis, int value);
    void _inputStickMin         (Joystick::AxisFunction_t function, int axis, int value);
    void _inputCenterWait       (Joystick::AxisFunction_t function, int axis, int value);
    
    void _switchDetect(Joystick::AxisFunction_t function, int axis, int value, bool moveToNextStep);
    
    void _saveFlapsDown(void);
    void _skipFlaps(void);
    void _saveAllTrims(void);
    
    bool _stickSettleComplete(int axis, int value);
    
    void _validateCalibration(void);
    void _writeCalibration(void);
    void _resetInternalCalibrationValues(void);
    void _setInternalCalibrationValuesFromSettings(void);
    
    void _startCalibration(void);
    void _stopCalibration(void);
    void _calSave(void);

    void _calSaveCurrentValues(void);
    
    void _setHelpImage(const char* imageFile);
    
    void _signalAllAttitudeValueChanges(void);
    
    // Member variables

    static const char* _imageFileMode1Dir;
    static const char* _imageFileMode2Dir;
    static const char* _imageFileMode3Dir;
    static const char* _imageFileMode4Dir;
    static const char* _imageFilePrefix;
    static const char* _imageCenter;
    static const char* _imageThrottleUp;
    static const char* _imageThrottleDown;
    static const char* _imageYawLeft;
    static const char* _imageYawRight;
    static const char* _imageRollLeft;
    static const char* _imageRollRight;
    static const char* _imagePitchUp;
    static const char* _imagePitchDown;
    
    int _rgFunctionAxisMapping[Joystick::maxFunction]; ///< Maps from joystick function to axis index. _axisMax indicates axis not set for this function.

    static const int _attitudeControls = 5;
    
    int                 _axisCount;         ///< Number of actual joystick axes available
    static const int    _axisNoAxis = -1;   ///< Signals no axis set
    static const int    _axisMinimum = 4;   ///< Minimum numner of joystick axes required to run PX4
    struct AxisInfo*    _rgAxisInfo;        ///< Information associated with each axis
    int*                _axisValueSave;     ///< Saved values prior to detecting axis movement
    int*                _axisRawValue;      ///< Current set of raw axis values

    enum calStates _calState;               ///< Current calibration state
    int     _calStateCurrentAxis;           ///< Current axis being worked on in calStateIdentify and calStateDetectInversion
    bool    _calStateAxisComplete;          ///< Work associated with current axis is complete
    int     _calStateIdentifyOldMapping;    ///< Previous mapping for axis being currently identified
    int     _calStateReverseOldMapping;     ///< Previous mapping for axis being currently used to detect inversion
    
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
    
    QQuickItem* _statusText;
    QQuickItem* _cancelButton;
    QQuickItem* _nextButton;
    QQuickItem* _skipButton;
    
    QString _imageHelp;

    JoystickManager*    _joystickManager;
};

#endif // JoystickConfigController_H
