/*=====================================================================

PIXHAWK Micro Air Vehicle Flying Robotics Toolkit

(c) 2009, 2010 PIXHAWK PROJECT  <http://pixhawk.ethz.ch>

This file is part of the PIXHAWK project

    PIXHAWK is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    PIXHAWK is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with PIXHAWK. If not, see <http://www.gnu.org/licenses/>.

======================================================================*/

/**
 * @file
 * @brief Definition of joystick interface
 *
 * This class defines a new thread to operate the reading of any joystick/controllers
 * via the Simple Directmedia Library (libsdl.org). This relies on polling of the SDL,
 * which is not their recommended method, instead they suggest use event checking. That
 * does not seem to support switching joysticks after their internal event loop has started,
 * so it was abandoned.
 *
 * All joystick-related functionality is done in this class, though the JoystickWidget provides
 * a UI around modifying its settings. Additionally controller buttons can be mapped to
 * actions defined by any UASInterface object through the `UASInterface::getActions()` function.
 *
 * @author Lorenz Meier <mavteam@student.ethz.ch>
 * @author Andreas Romer <mavteam@student.ethz.ch>
 * @author Julian Oes <julian@oes.ch>
 */

#ifndef _JOYSTICKINPUT_H_
#define _JOYSTICKINPUT_H_

#include <QThread>
#include <QList>
#include <qmutex.h>
#ifdef Q_OS_MAC
#include <SDL.h>
#else
#include <SDL/SDL.h>
#endif

#include "UASInterface.h"

struct JoystickSettings {
    QMap<int, bool> axesInverted; ///< Whether each axis should be used inverted from what was reported.
    QMap<int, bool> axesLimited; ///< Whether each axis should be limited to only the positive range. Currently this only applies to the throttle axis, but is kept generic here to possibly support other axes.
    QMap<int, float> axesMaxRange; ///< The maximum values per axis
    QMap<int, float> axesMinRange; ///< The minimum values per axis
    QMap<int, int> buttonActions; ///< The index of the action associated with every button.
};
Q_DECLARE_METATYPE(JoystickSettings)

/**
 * @brief Joystick input
 */
class JoystickInput : public QThread
{
    Q_OBJECT

public:
    JoystickInput();
    ~JoystickInput();
    void run();
    void shutdown();

    /**
     * @brief The JOYSTICK_INPUT_MAPPING enum storing the values for each item in the mapping combobox.
     * This should match the order of items in the mapping combobox in JoystickAxis.ui.
     */
    enum JOYSTICK_INPUT_MAPPING
    {
        JOYSTICK_INPUT_MAPPING_NONE     = 0,
        JOYSTICK_INPUT_MAPPING_YAW      = 1,
        JOYSTICK_INPUT_MAPPING_PITCH    = 2,
        JOYSTICK_INPUT_MAPPING_ROLL     = 3,
        JOYSTICK_INPUT_MAPPING_THROTTLE = 4
    };

    /**
     * @brief The JOYSTICK_MODE enum stores the values for each item in the mode combobox.
     * This should match the order of items in the mode combobox in JoystickWidget.ui.
     */
    enum JOYSTICK_MODE
    {
        JOYSTICK_MODE_ATTITUDE     = 0,
        JOYSTICK_MODE_POSITION     = 1,
        JOYSTICK_MODE_FORCE        = 2,
        JOYSTICK_MODE_VELOCITY     = 3,
        JOYSTICK_MODE_MANUAL       = 4
    };

    /**
     * @brief Load joystick-specific settings.
     */
    void loadJoystickSettings();
    /**
     * @brief Load joystick-independent settings.
     */
    void loadGeneralSettings();

    /**
     * @brief Store joystick-specific settings.
     */
    void storeJoystickSettings() const;
    /**
     * @brief Store joystick-independent settings.
     */
    void storeGeneralSettings() const;

    bool enabled() const
    {
        return isEnabled;
    }

    bool calibrating() const
    {
        return isCalibrating;
    }

    int getMappingThrottleAxis() const
    {
        return throttleAxis;
    }

    int getMappingRollAxis() const
    {
        return rollAxis;
    }

    int getMappingPitchAxis() const
    {
        return pitchAxis;
    }

    int getMappingYawAxis() const
    {
        return yawAxis;
    }

    int getJoystickNumButtons() const
    {
        return joystickNumButtons;
    }

    int getJoystickNumAxes() const
    {
        return joystickNumAxes;
    }

    int getJoystickID() const
    {
        return joystickID;
    }

    const QString& getName() const
    {
        return joystickName;
    }

    int getNumJoysticks() const
    {
        return numJoysticks;
    }

    JOYSTICK_MODE getMode() const
    {
        return mode;
    }

    QString getJoystickNameById(int id) const
    {
        return QString(SDL_JoystickName(id));
    }

    float getCurrentValueForAxis(int axis) const;
    bool getInvertedForAxis(int axis) const;
    bool getRangeLimitForAxis(int axis) const;
    float getAxisRangeLimitMinForAxis(int axis) const;
    float getAxisRangeLimitMaxForAxis(int axis) const;
    int getActionForButton(int button) const;

    const double sdlJoystickMin;
    const double sdlJoystickMax;

protected:

    bool isEnabled; ///< Track whether the system should emit the higher-level signals: joystickChanged & actionTriggered.
    bool isCalibrating; ///< Track if calibration in progress
    bool done;

    SDL_Joystick* joystick;
    UASInterface* uas; ///< Track the current UAS.
    int autopilotType; ///< Cache the autopilotType
    int systemType; ///< Cache the systemType
    bool uasCanReverse; ///< Track whether the connect UAS can drive a reverse speed.

    // Store the mapping between axis numbers and the roll/pitch/yaw/throttle configuration.
    // Value is one of JoystickAxis::JOYSTICK_INPUT_MAPPING.
    int rollAxis;
    int pitchAxis;
    int yawAxis;
    int throttleAxis;

    // Cache information on the joystick instead of polling the SDL everytime.
    int numJoysticks; ///< Total number of joysticks detected by the SDL.
    QString joystickName;
    int joystickID;
    int joystickNumAxes;
    int joystickNumButtons;

    // mode of joystick (attitude, position, force, ... (see JOYSTICK_MODE enum))
    JOYSTICK_MODE mode;

    // Track axis/button settings based on a Joystick/AutopilotType/SystemType triplet.
    // This is only a double-map, because settings are stored/loaded based on joystick
    // name first, so only the settings for the current joystick need to be stored at any given time.
    // Pointers are kept to the various settings field to reduce lookup times.
    // Note that the mapping (0,0) corresponds to when no UAS is connected. Since this corresponds
    // to a generic vehicle type and a generic autopilot, this is a pretty safe default.
    QMap<int, QMap<int, JoystickSettings> > joystickSettings;

    // Track the last state of the axes, buttons, and hats for only emitting change signals.
    QList<float> joystickAxes; ///< The values of every axes during the last sample.
    quint16 joystickButtons;   ///< The state of every button. Bitfield supporting 16 buttons with 1s indicating that the button is down.
    qint8 xHat, yHat;            ///< The horizontal/vertical hat directions. Values are -1, 0, 1, with (-1,-1) indicating bottom-left.

    /**
     * @brief Called before main run() event loop starts. Waits for joysticks to be connected.
     */
    void init();

signals:

    /**
     * @brief Signal containing all joystick raw positions
     *
     * @param roll forward / pitch / x axis, front: 1, center: 0, back: -1. If the roll axis isn't defined, NaN is transmit instead.
     * @param pitch left / roll / y axis, left: -1, middle: 0, right: 1. If the roll axis isn't defined, NaN is transmit instead.
     * @param yaw turn axis, left-turn: -1, centered: 0, right-turn: 1. If the roll axis isn't defined, NaN is transmit instead.
     * @param throttle Throttle, -100%:-1.0, 0%: 0.0, 100%: 1.0. If the roll axis isn't defined, NaN is transmit instead.
     * @param xHat hat vector in forward-backward direction, +1 forward, 0 center, -1 backward
     * @param yHat hat vector in left-right direction, -1 left, 0 center, +1 right
     * @param mode (setpoint type) see JOYSTICK_MODE enum
     */
    void joystickChanged(float roll, float pitch, float yaw, float throttle, qint8 xHat, qint8 yHat, quint16 buttons, quint8 mode);

    /**
      * @brief Emit a new value for an axis
      *
      * @param value Value of the axis, between -1.0 and 1.0.
      */
    void axisValueChanged(int axis, float value);

    /**
      * @brief Joystick button has changed state from unpressed to pressed.
      * @param key index of the pressed key
      */
    void buttonPressed(int key);

    /**
      * @brief Joystick button has changed state from pressed to unpressed.
      *
      * @param key index of the released key
      */
    void buttonReleased(int key);

    /**
      * @brief A joystick button was pressed that had a corresponding action.
      * @param action The index of the action to trigger. Dependent on UAS.
      */
    void actionTriggered(int action);

    /**
      * @brief Hat (8-way switch on the top) has changed position
      *
      * Coordinate frame for joystick hat:
      *
      *    y
      *    ^
      *    |
      *    |
      *    0 ----> x
      *
      *
      * @param x vector in left-right direction
      * @param y vector in forward-backward direction
      */
    void hatDirectionChanged(qint8 x, qint8 y);

    /** @brief Signal that the UAS has been updated for this JoystickInput
     * Note that any UI updates should NOT query this object for joystick details. That should be done in response to the joystickSettingsChanged signal.
     */
    void activeUASSet(UASInterface*);

    /** @brief Signals that new joystick-specific settings were changed. Useful for triggering updates that at dependent on the current joystick. */
    void joystickSettingsChanged();

    /** @brief The JoystickInput has switched to a different joystick. UI should be adjusted accordingly. */
    void newJoystickSelected();

public slots:
    /** @brief Enable or disable emitting the high-level control signals from the joystick. */
    void setEnabled(bool enable);
    /** @brief Specify the UAS that this input should forward joystickChanged signals and buttonPresses to. */
    void setActiveUAS(UASInterface* uas);
    /** @brief Switch to a new joystick by ID number. Both buttons and axes are updated with the proper signals emitted. */
    void setActiveJoystick(int id);
    /** @brief Switch calibration mode active */
    void setCalibrating(bool active);
    /**
     * @brief Change the control mapping for a given joystick axis.
     * @param axisID The axis to modify (0-indexed)
     * @param newMapping The mapping to use.
     * @see JOYSTICK_INPUT_MAPPING
     */
    void setAxisMapping(int axis, JoystickInput::JOYSTICK_INPUT_MAPPING newMapping);
    /**
     * @brief Specify if an axis should be inverted.
     * @param axis The ID of the axis.
     * @param inverted True indicates inverted from normal. Varies by controller.
     */
    void setAxisInversion(int axis, bool inverted);

    /**
     * @brief Specify that an axis should only transmit the positive values. Useful for controlling throttle from auto-centering axes.
     * @param axis Which axis has its range limited.
     * @param limitRange If true only the positive half of this axis will be read.
     */
    void setAxisRangeLimit(int axis, bool limitRange);

    /**
     * @brief Specify minimum value for axis.
     * @param axis Which axis should be set.
     * @param min Value to be set.
     */
    void setAxisRangeLimitMin(int axis, float min);

    /**
     * @brief Specify maximum value for axis.
     * @param axis Which axis should be set.
     * @param max Value to be set.
     */
    void setAxisRangeLimitMax(int axis, float max);

    /**
     * @brief Specify a button->action mapping for the given uas.
     * This mapping is applied based on UAS autopilot type and UAS system type.
     * Connects the buttonEmitted signal for the corresponding button to the corresponding action for the current UAS.
     * @param button The numeric ID for the button
     * @param action The numeric ID of the action for this UAS to map to.
     */
    void setButtonAction(int button, int action);

    /**
     * @brief Specify which setpoints should be sent to the UAS when moving the joystick
     * @param newMode the mode (setpoint type) see the JOYSTICK_MODE enum
     */
    void setMode(int newMode)
    {
        mode = (JOYSTICK_MODE)newMode;
    }
};

#endif // _JOYSTICKINPUT_H_
