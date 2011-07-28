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
 *   @brief Joystick interface
 *
 *   @author Lorenz Meier <mavteam@student.ethz.ch>
 *   @author Andreas Romer <mavteam@student.ethz.ch>
 *
 */

#include "JoystickInput.h"

#include <QDebug>
#include <limits.h>
#include "UAS.h"
#include "UASManager.h"
#include "QGC.h"

/**
 * The coordinate frame of the joystick axis is the aeronautical frame like shown on this image:
 * @image html http://pixhawk.ethz.ch/wiki/_media/standards/body-frame.png Aeronautical frame
 */
JoystickInput::JoystickInput() :
    sdlJoystickMin(-32768.0f),
    sdlJoystickMax(32767.0f),
    defaultIndex(0),
    uas(NULL),
    uasButtonList(QList<int>()),
    done(false),
    thrustAxis(3),
    xAxis(1),
    yAxis(0),
    yawAxis(2),
    joystickName(tr("Unitinialized"))
{
    for (int i = 0; i < 10; i++) {
        calibrationPositive[i] = sdlJoystickMax;
        calibrationNegative[i] = sdlJoystickMin;
    }

    connect(UASManager::instance(), SIGNAL(activeUASSet(UASInterface*)), this, SLOT(setActiveUAS(UASInterface*)));

    // Enter main loop
    //start();
}

void JoystickInput::setActiveUAS(UASInterface* uas)
{
    // Only connect / disconnect is the UAS is of a controllable UAS class
    UAS* tmp = 0;
    if (this->uas) {
        tmp = dynamic_cast<UAS*>(this->uas);
        if(tmp) {
            disconnect(this, SIGNAL(joystickChanged(double,double,double,double,int,int)), tmp, SLOT(setManualControlCommands(double,double,double,double)));
            disconnect(this, SIGNAL(buttonPressed(int)), tmp, SLOT(receiveButton(int)));
        }
    }

    this->uas = uas;

    tmp = dynamic_cast<UAS*>(this->uas);
    if(tmp) {
        connect(this, SIGNAL(joystickChanged(double,double,double,double,int,int)), tmp, SLOT(setManualControlCommands(double,double,double,double)));
        connect(this, SIGNAL(buttonPressed(int)), tmp, SLOT(receiveButton(int)));
    }
    if (!isRunning()) {
        start();
    }
}

void JoystickInput::init()
{
    // INITIALIZE SDL Joystick support
    if (SDL_InitSubSystem(SDL_INIT_JOYSTICK | SDL_INIT_NOPARACHUTE) < 0) {
        printf("Couldn't initialize SimpleDirectMediaLayer: %s\n", SDL_GetError());
    }

    // Enumerate joysticks and select one
    int numJoysticks = SDL_NumJoysticks();

    // Wait for joysticks if none is connected
    while (numJoysticks == 0) {
        MG::SLEEP::msleep(200);
        // INITIALIZE SDL Joystick support
        if (SDL_InitSubSystem(SDL_INIT_JOYSTICK | SDL_INIT_NOPARACHUTE) < 0) {
            printf("Couldn't initialize SimpleDirectMediaLayer: %s\n", SDL_GetError());
        }
        numJoysticks = SDL_NumJoysticks();
    }

    printf("%d Input devices found:\n", numJoysticks);
    for(int i=0; i < SDL_NumJoysticks(); i++ ) {
        printf("\t- %s\n", SDL_JoystickName(i));
        joystickName = QString(SDL_JoystickName(i));
    }

    printf("\nOpened %s\n", SDL_JoystickName(defaultIndex));

    SDL_JoystickEventState(SDL_ENABLE);

    joystick = SDL_JoystickOpen(defaultIndex);

    // Make sure active UAS is set
    setActiveUAS(UASManager::instance()->getActiveUAS());
}

/**
 * @brief Execute the Joystick process
 */
void JoystickInput::run()
{

    init();

    while(!done) {
        while(SDL_PollEvent(&event)) {

            SDL_JoystickUpdate();

            // Todo check if it would be more beneficial to use the event structure
            switch(event.type) {
            case SDL_KEYDOWN:
                /* handle keyboard stuff here */
                //qDebug() << "KEY PRESSED!";
                break;

            case SDL_QUIT:
                /* Set whatever flags are necessary to */
                /* end the main loop here */
                break;

            case SDL_JOYBUTTONDOWN:  /* Handle Joystick Button Presses */
                if ( event.jbutton.button == 0 ) {
                    //qDebug() << "BUTTON PRESSED!";
                }
                break;

            case SDL_JOYAXISMOTION:  /* Handle Joystick Motion */
                if ( ( event.jaxis.value < -3200 ) || (event.jaxis.value > 3200 ) ) {
                    if( event.jaxis.axis == 0) {
                        /* Left-right movement code goes here */
                    }

                    if( event.jaxis.axis == 1) {
                        /* Up-Down movement code goes here */
                    }
                }
                break;

            default:
                //qDebug() << "SDL event occured";
                break;
            }
        }

        // Display all axes
        for(int i = 0; i < SDL_JoystickNumAxes(joystick); i++) {
            //qDebug() << "\rAXIS" << i << "is: " << SDL_JoystickGetAxis(joystick, i);
        }

        // THRUST
        double thrust = ((double)SDL_JoystickGetAxis(joystick, thrustAxis) - calibrationNegative[thrustAxis]) / (calibrationPositive[thrustAxis] - calibrationNegative[thrustAxis]);
        // Has to be inverted for Logitech Wingman
        thrust = 1.0f - thrust;
        // Bound rounding errors
        if (thrust > 1) thrust = 1;
        if (thrust < 0) thrust = 0;
        emit thrustChanged((float)thrust);

        // X Axis
        double x = ((double)SDL_JoystickGetAxis(joystick, xAxis) - calibrationNegative[xAxis]) / (calibrationPositive[xAxis] - calibrationNegative[xAxis]);
        x = 1.0f - x;
        x = x * 2.0f - 1.0f;
        // Bound rounding errors
        if (x > 1.0f) x = 1.0f;
        if (x < -1.0f) x = -1.0f;
        emit xChanged((float)x);

        // Y Axis
        double y = ((double)SDL_JoystickGetAxis(joystick, yAxis) - calibrationNegative[yAxis]) / (calibrationPositive[yAxis] - calibrationNegative[yAxis]);
        y = 1.0f - y;
        y = y * 2.0f - 1.0f;
        // Bound rounding errors
        if (y > 1.0f) y = 1.0f;
        if (y < -1.0f) y = -1.0f;
        emit yChanged((float)y);

        // Yaw Axis

        double yaw = ((double)SDL_JoystickGetAxis(joystick, yawAxis) - calibrationNegative[yawAxis]) / (calibrationPositive[yawAxis] - calibrationNegative[yawAxis]);
        yaw = 1.0f - yaw;
        yaw = yaw * 2.0f - 1.0f;
        // Bound rounding errors
        if (yaw > 1.0f) yaw = 1.0f;
        if (yaw < -1.0f) yaw = -1.0f;
        emit yawChanged((float)yaw);

        // Get joystick hat position, convert it to vector
        int hatPosition = SDL_JoystickGetHat(joystick, 0);

        int xHat,yHat;
        xHat = 0;
        yHat = 0;

        // Build up vectors describing the hat position
        //
        // Coordinate frame for joystick hat:
        //
        //    y
        //    ^
        //    |
        //    |
        //    0 ----> x
        //

        if ((SDL_HAT_UP & hatPosition) > 0) yHat = 1;
        if ((SDL_HAT_DOWN & hatPosition) > 0) yHat = -1;

        if ((SDL_HAT_LEFT & hatPosition) > 0) xHat = -1;
        if ((SDL_HAT_RIGHT & hatPosition) > 0) xHat = 1;

        // Send new values to rest of groundstation
        emit hatDirectionChanged(xHat, yHat);
        emit joystickChanged(y, x, yaw, thrust, xHat, yHat);


        // Display all buttons
        for(int i = 0; i < SDL_JoystickNumButtons(joystick); i++) {
            //qDebug() << "BUTTON" << i << "is: " << SDL_JoystickGetAxis(joystick, i);
            if(SDL_JoystickGetButton(joystick, i)) {
                emit buttonPressed(i);
                // Check if button is a UAS select button

                if (uasButtonList.contains(i)) {
                    UASInterface* uas = UASManager::instance()->getUASForId(i);
                    if (uas) {
                        UASManager::instance()->setActiveUAS(uas);
                    }
                }
            }

        }

        // Sleep, update rate of joystick is approx. 50 Hz (1000 ms / 50 = 20 ms)
        QGC::SLEEP::msleep(20);

    }

}

const QString& JoystickInput::getName()
{
    return joystickName;
}
