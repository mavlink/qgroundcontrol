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
#include <SDL/SDL.h>

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

    const QString& getName();

    const double sdlJoystickMin;
    const double sdlJoystickMax;

protected:
    int defaultIndex;
    double calibrationPositive[10];
    double calibrationNegative[10];
    SDL_Joystick* joystick;
    UASInterface* uas;
    QList<int> uasButtonList;
    bool done;
	QMutex m_doneMutex;

    // Axis 3 is thrust (CALIBRATION!)
    int thrustAxis;
    int xAxis;
    int yAxis;
    int yawAxis;
    SDL_Event event;
    QString joystickName;

    void init();

signals:

    /**
     * @brief Signal containing all joystick raw positions
     *
     * @param roll forward / pitch / x axis, front: 1, center: 0, back: -1
     * @param pitch left / roll / y axis, left: -1, middle: 0, right: 1
     * @param yaw turn axis, left-turn: -1, centered: 0, right-turn: 1
     * @param thrust Thrust, 0%: 0, 100%: 1
     * @param xHat hat vector in forward-backward direction, +1 forward, 0 center, -1 backward
     * @param yHat hat vector in left-right direction, -1 left, 0 center, +1 right
     */
    void joystickChanged(double roll, double pitch, double yaw, double thrust, int xHat, int yHat);

    /**
     * @brief Thrust lever of the joystick has changed
     *
     * @param thrust Thrust, 0%: 0, 100%: 1.0
     */
    void thrustChanged(int thrust);

    /**
      * @brief X-Axis / forward-backward axis has changed
      *
      * @param x forward / pitch / x axis, front: +1.0, center: 0.0, back: -1.0
      */
    void xChanged(int x);

    /**
      * @brief Y-Axis / left-right axis has changed
      *
      * @param y left / roll / y axis, left: -1.0, middle: 0.0, right: +1.0
      */
    void yChanged(int y);

    /**
      * @brief Yaw / left-right turn has changed
      *
      * @param yaw turn axis, left-turn: -1.0, middle: 0.0, right-turn: +1.0
      */
    void yawChanged(int yaw);

    /**
      * @brief Joystick button has been pressed
      *
      * @param key index of the pressed key
      */
    void buttonPressed(int key);

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
};

#endif // _JOYSTICKINPUT_H_
