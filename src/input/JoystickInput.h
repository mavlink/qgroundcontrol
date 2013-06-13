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
 *   @brief Definition of joystick interface
 *
 *   @author Lorenz Meier <mavteam@student.ethz.ch>
 *   @author Andreas Romer <mavteam@student.ethz.ch>
 *
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
     * @brief Load joystick settings
     */
    void loadSettings();

    /**
     * @brief Store joystick settings
     */
    void storeSettings();

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

    QString getJoystickNameById(int id) const
    {
        return QString(SDL_JoystickName(id));
    }

    float getCurrentValueForAxis(int axisID);

    const double sdlJoystickMin;
    const double sdlJoystickMax;

protected:
    double calibrationPositive[10];
    double calibrationNegative[10];
    SDL_Joystick* joystick;
    UASInterface* uas;
    bool done;

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

    QList<float> joystickAxes; ///< The values of every axes during the last sample
    quint16 joystickButtons;   ///< The state of every button. Bitfield supporting 16 buttons with 1s indicating that the button is down.
    int xHat, yHat;            ///< The horizontal/vertical hat directions. Values are -1, 0, 1, with (-1,-1) indicating bottom-left.

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
     */
    void joystickChanged(double roll, double pitch, double yaw, double throttle, int xHat, int yHat, int buttons);

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
    void hatDirectionChanged(int x, int y);

public slots:
    void setActiveUAS(UASInterface* uas);
    /** @brief Switch to a new joystick by ID number. */
    void setActiveJoystick(int id);

    void setMappingRollAxis(int axis)
    {
        rollAxis = axis;
    }

    void setMappingPitchAxis(int axis)
    {
        pitchAxis = axis;
    }

    void setMappingYawAxis(int axis)
    {
        yawAxis = axis;
    }

    void setMappingThrottleAxis(int axis)
    {
        throttleAxis = axis;
    }
};

#endif // _JOYSTICKINPUT_H_
