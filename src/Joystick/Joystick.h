/*=====================================================================
 
 QGroundControl Open Source Ground Control Station
 
 (c) 2009 - 2014 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 
 This file is part of the QGROUNDCONTROL project
 
 QGROUNDCONTROL is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 QGROUNDCONTROL is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.
 
 ======================================================================*/

#ifndef Joystick_H
#define Joystick_H

#include <QObject>
#include <QThread>

#include "QGCLoggingCategory.h"
#include "Vehicle.h"

Q_DECLARE_LOGGING_CATEGORY(JoystickLog)

class Joystick : public QThread
{
    Q_OBJECT
    
public:
    Joystick(const QString& name, int axisCount, int buttonCount, int sdlIndex);
    ~Joystick();
    
    typedef struct {
        int     min;
        int     max;
        int     center;
        bool    reversed;
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
    
#ifndef __mobile__
    Q_PROPERTY(QString name READ name CONSTANT)
    
    Q_PROPERTY(bool calibrated MEMBER _calibrated NOTIFY calibratedChanged)
    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY enabledChanged)
    
    Q_PROPERTY(int buttonCount MEMBER _buttonCount CONSTANT)
    Q_PROPERTY(int axisCount MEMBER _axisCount CONSTANT)
    
    Q_PROPERTY(QStringList actions READ actions CONSTANT)
    
    Q_PROPERTY(QVariantList buttonActions READ buttonActions NOTIFY buttonActionsChanged)
    Q_INVOKABLE void setButtonAction(int button, int action);
    Q_INVOKABLE int getButtonAction(int button);
    
    Q_PROPERTY(int throttleMode READ throttleMode WRITE setThrottleMode NOTIFY throttleModeChanged)
    
    /// Start the polling thread which will in turn emit joystick signals
    void startPolling(void);
    void stopPolling(void);
    
    void setCalibration(int axis, Calibration_t& calibration);
    Calibration_t getCalibration(int axis);
    
    void setFunctionAxis(AxisFunction_t function, int axis);
    int getFunctionAxis(AxisFunction_t function);
    
    QStringList actions(void);
    QVariantList buttonActions(void);
    
    QString name(void) { return _name; }
    
    int throttleMode(void);
    void setThrottleMode(int mode);
    
    bool enabled(void);
    void setEnabled(bool enabled);
    
    bool calibrating(void) { return _calibrating; }
    void setCalibrating(bool calibrating) { _calibrating = calibrating; }
    
signals:
    void calibratedChanged(bool calibrated);
    
    // The raw signals are only meant for use by calibration
    void rawAxisValueChanged(int index, int value);
    void rawButtonPressedChanged(int index, int pressed);
    
    void buttonActionsChanged(QVariantList actions);
    
    void throttleModeChanged(int mode);
    
    void enabledChanged(bool enabled);
    
    /// Signal containing new joystick information
    ///     @param roll     Range is -1:1, negative meaning roll left, positive meaning roll right
    ///     @param pitch    Range i -1:1, negative meaning pitch down, positive meaning pitch up
    ///     @param yaw      Range is -1:1, negative meaning yaw left, positive meaning yaw right
    ///     @param throttle Range is 0:1, 0 meaning no throttle, 1 meaning full throttle
    ///     @param mode     See Vehicle::JoystickMode_t enum
    void manualControl(float roll, float pitch, float yaw, float throttle, quint16 buttons, int joystickMmode);
    
    void buttonActionTriggered(int action);
    
private:
    void _saveSettings(void);
    void _loadSettings(void);
    float _adjustRange(int value, Calibration_t calibration);
    
    // Override from QThread
    virtual void run(void);

private:
    int     _sdlIndex;      ///< Index for SDL_JoystickOpen
    
    bool    _exitThread;    ///< true: signal thread to exit
    
    QString _name;
    bool    _enabled;
    bool    _calibrated;
    bool    _calibrating;
    int     _axisCount;
    int     _buttonCount;
    
    static const int    _cAxes = 4;
    int                 _rgAxisValues[_cAxes];
    Calibration_t       _rgCalibration[_cAxes];
    int                 _rgFunctionAxis[maxFunction];
    
    static const int    _cButtons = 12;
    bool                _rgButtonValues[_cButtons];
    int                 _rgButtonActions[_cButtons];
    quint16             _lastButtonBits;
    
    ThrottleMode_t      _throttleMode;
#endif // __mobile__
    
private:
    static const char*  _rgFunctionSettingsKey[maxFunction];

    static const char* _settingsGroup;
    static const char* _calibratedSettingsKey;
    static const char* _buttonActionSettingsKey;
    static const char* _throttleModeSettingsKey;
    static const char* _enabledSettingsKey;
};
    
#endif
