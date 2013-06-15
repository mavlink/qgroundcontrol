/*=====================================================================
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
#include <QMutexLocker>
#include <QSettings>
#include <math.h>

/**
 * The coordinate frame of the joystick axis is the aeronautical frame like shown on this image:
 * @image html http://pixhawk.ethz.ch/wiki/_media/standards/body-frame.png Aeronautical frame
 */
JoystickInput::JoystickInput() :
    sdlJoystickMin(-32768.0f),
    sdlJoystickMax(32767.0f),
    uas(NULL),
    done(false),
    rollAxis(-1),
    pitchAxis(-1),
    yawAxis(-1),
    throttleAxis(-1),
    joystickName(""),
    joystickID(-1),
    joystickNumButtons(0)
{
    loadSettings();

    for (int i = 0; i < 10; i++) {
        calibrationPositive[i] = sdlJoystickMax;
        calibrationNegative[i] = sdlJoystickMin;
    }

    // Start this thread. This allows the Joystick Settings window to work correctly even w/o any UASes connected.
    start();
}

JoystickInput::~JoystickInput()
{
    storeSettings();
    done = true;
}

void JoystickInput::loadSettings()
{
//    // Load defaults from settings
//    QSettings settings;
//    settings.sync();
//    settings.beginGroup("QGC_JOYSTICK_INPUT");
//    xAxis = (settings.value("X_AXIS_MAPPING", xAxis).toInt());
//    yAxis = (settings.value("Y_AXIS_MAPPING", yAxis).toInt());
//    thrustAxis = (settings.value("THRUST_AXIS_MAPPING", thrustAxis).toInt());
//    yawAxis = (settings.value("YAW_AXIS_MAPPING", yawAxis).toInt());
//    autoButtonMapping = (settings.value("AUTO_BUTTON_MAPPING", autoButtonMapping).toInt());
//    stabilizeButtonMapping = (settings.value("STABILIZE_BUTTON_MAPPING", stabilizeButtonMapping).toInt());
//    manualButtonMapping = (settings.value("MANUAL_BUTTON_MAPPING", manualButtonMapping).toInt());
//    settings.endGroup();
}

void JoystickInput::storeSettings()
{
//    // Store settings
//    QSettings settings;
//    settings.beginGroup("QGC_JOYSTICK_INPUT");
//    settings.setValue("X_AXIS_MAPPING", xAxis);
//    settings.setValue("Y_AXIS_MAPPING", yAxis);
//    settings.setValue("THRUST_AXIS_MAPPING", thrustAxis);
//    settings.setValue("YAW_AXIS_MAPPING", yawAxis);
//    settings.setValue("AUTO_BUTTON_MAPPING", autoButtonMapping);
//    settings.setValue("STABILIZE_BUTTON_MAPPING", stabilizeButtonMapping);
//    settings.setValue("MANUAL_BUTTON_MAPPING", manualButtonMapping);
//    settings.endGroup();
//    settings.sync();
}


void JoystickInput::setActiveUAS(UASInterface* uas)
{
    // Only connect / disconnect is the UAS is of a controllable UAS class
    UAS* tmp = 0;
    if (this->uas)
    {
        tmp = dynamic_cast<UAS*>(this->uas);
        if(tmp)
        {
            disconnect(this, SIGNAL(joystickChanged(double,double,double,double,int,int,int)), tmp, SLOT(setManualControlCommands(double,double,double,double,int,int,int)));
            disconnect(this, SIGNAL(buttonPressed(int)), tmp, SLOT(receiveButton(int)));
        }
    }

    this->uas = uas;

    if (this->uas)
    {
        tmp = dynamic_cast<UAS*>(this->uas);
        if(tmp) {
            connect(this, SIGNAL(joystickChanged(double,double,double,double,int,int,int)), tmp, SLOT(setManualControlCommands(double,double,double,double,int,int,int)));
            connect(this, SIGNAL(buttonPressed(int)), tmp, SLOT(receiveButton(int)));
        }
    }
}

void JoystickInput::init()
{
    // INITIALIZE SDL Joystick support
    if (SDL_InitSubSystem(SDL_INIT_JOYSTICK | SDL_INIT_NOPARACHUTE) < 0) {
        printf("Couldn't initialize SimpleDirectMediaLayer: %s\n", SDL_GetError());
    }

    // Enumerate joysticks and select one
    numJoysticks = SDL_NumJoysticks();

    // Wait for joysticks if none are connected
    while (numJoysticks == 0 && !done)
    {
        QGC::SLEEP::msleep(400);
        // INITIALIZE SDL Joystick support
        if (SDL_InitSubSystem(SDL_INIT_JOYSTICK | SDL_INIT_NOPARACHUTE) < 0)
        {
            printf("Couldn't initialize SimpleDirectMediaLayer: %s\n", SDL_GetError());
        }
        numJoysticks = SDL_NumJoysticks();
    }
    if (done)
    {
        return;
    }

    qDebug() << QString("%1 Input devices found:").arg(numJoysticks);
    for(int i=0; i < numJoysticks; i++ )
    {
        qDebug() << QString("\t- %1").arg(SDL_JoystickName(i));
        SDL_Joystick* x = SDL_JoystickOpen(i);
        qDebug() << QString("Number of Axes: %1").arg(QString::number(SDL_JoystickNumAxes(x)));
        qDebug() << QString("Number of Buttons: %1").arg(QString::number(SDL_JoystickNumButtons(x)));
        SDL_JoystickClose(x);
    }

    // And attach to the first joystick found to start.
    setActiveJoystick(0);

    // Make sure active UAS is set
    setActiveUAS(UASManager::instance()->getActiveUAS());
}

void JoystickInput::shutdown()
{
    done = true;
}

/**
 * @brief Execute the Joystick process
 * Note that the SDL procedure is polled. This is because connecting and disconnecting while the event checker is running
 * fails as of SDL 1.2. It is therefore much easier to just poll for the joystick we want to sample.
 */
void JoystickInput::run()
{
    init();

    forever
    {
        if (done)
        {
            done = false;
            exit();
            return;
        }

        // Poll the joystick for new values.
        SDL_JoystickUpdate();

        // Emit all necessary signals for all axes.
        for (int i = 0; i < joystickNumAxes; i++)
        {
            // First emit the uncalibrated values for each axis based on their ID.
            // This is generally not used for controlling a vehicle, but a UI representation, so it being slightly off is fine.
            float axisValue = SDL_JoystickGetAxis(joystick, i);
            if (joystickAxesInverted[i])
            {
                axisValue = (axisValue - calibrationNegative[i]) / (calibrationPositive[i] - calibrationNegative[i]);
            }
            else
            {
                axisValue = (axisValue - calibrationPositive[i]) / (calibrationNegative[i] - calibrationPositive[i]);
            }
            axisValue = 1.0f - axisValue;

            // If the joystick isn't limited to [0:1.0], map it into [-1.0:1.0].
            if (!joystickAxesRangeLimited[i])
            {
                axisValue = axisValue * 2.0f - 1.0f;
            }

            // Bound rounding errors
            if (axisValue > 1.0f) axisValue = 1.0f;
            if (axisValue < -1.0f) axisValue = -1.0f;
            if (joystickAxes[i] != axisValue)
            {
                joystickAxes[i] = axisValue;
                emit axisValueChanged(i, axisValue);
            }
        }

        // Build up vectors describing the hat position
        int hatPosition = SDL_JoystickGetHat(joystick, 0);
        int newYHat = 0;
        if ((SDL_HAT_UP & hatPosition) > 0) newYHat = 1;
        if ((SDL_HAT_DOWN & hatPosition) > 0) newYHat = -1;
        int newXHat = 0;
        if ((SDL_HAT_LEFT & hatPosition) > 0) newXHat = -1;
        if ((SDL_HAT_RIGHT & hatPosition) > 0) newXHat = 1;
        if (newYHat != yHat || newXHat != xHat)
        {
            xHat = newXHat;
            yHat = newYHat;
            emit hatDirectionChanged(newXHat, newYHat);
        }

        // Emit signals for each button individually
        for (int i = 0; i < joystickNumButtons; i++)
        {
            // If the button was down, but now it's up, trigger a buttonPressed event
            quint16 lastButtonState = joystickButtons & (1 << i);
            if (SDL_JoystickGetButton(joystick, i) && !lastButtonState)
            {
                emit buttonPressed(i);
                joystickButtons |= 1 << i;
            }
            else if (!SDL_JoystickGetButton(joystick, i) && lastButtonState)
            {
                emit buttonReleased(i);
                joystickButtons &= ~(1 << i);
            }
        }

        // Now signal an update for all UI together.
        float roll = rollAxis > -1?joystickAxes[rollAxis]:NAN;
        float pitch = pitchAxis > -1?joystickAxes[pitchAxis]:NAN;
        float yaw = yawAxis > -1?joystickAxes[yawAxis]:NAN;
        float throttle = throttleAxis > -1?joystickAxes[throttleAxis]:NAN;
        emit joystickChanged(roll, pitch, yaw, throttle, xHat, yHat, joystickButtons);

        // Sleep, update rate of joystick is approx. 50 Hz (1000 ms / 50 = 20 ms)
        QGC::SLEEP::msleep(20);
    }
}

void JoystickInput::setActiveJoystick(int id)
{
    if (joystick && SDL_JoystickOpened(joystickID))
    {
        SDL_JoystickClose(joystick);
        joystick = NULL;
        joystickID = -1;
    }

    joystickID = id;
    joystick = SDL_JoystickOpen(joystickID);
    if (joystick && SDL_JoystickOpened(joystickID))
    {
        SDL_JoystickUpdate();
        // Update joystick configuration.
        joystickName = QString(SDL_JoystickName(joystickID));
        joystickNumButtons = SDL_JoystickNumButtons(joystick);
        joystickNumAxes = SDL_JoystickNumAxes(joystick);

        // Update cached joystick values
        joystickAxes.clear();
        joystickAxesInverted.clear();
        joystickAxesRangeLimited.clear();
        for (int i = 0; i < joystickNumAxes; i++)
        {
            int axisValue = SDL_JoystickGetAxis(joystick, i);
            joystickAxes.append(axisValue);
            emit axisValueChanged(i, axisValue);

            joystickAxesInverted.append(false);
            joystickAxesRangeLimited.append(false);
        }
        joystickButtons = 0;
        for (int i = 0; i < joystickNumButtons; i++)
        {
            if (SDL_JoystickGetButton(joystick, i))
            {
                emit buttonPressed(i);
                joystickButtons |= 1 << i;
            }
        }
        qDebug() << QString("Switching to joystick '%1' with %2 buttons/%3 axes").arg(joystickName, QString::number(joystickNumButtons), QString::number(joystickNumAxes));
    }
    else
    {
        joystickNumButtons = 0;
        joystickNumAxes = 0;
    }
}

void JoystickInput::setAxisMapping(int axis, JOYSTICK_INPUT_MAPPING newMapping)
{
    switch (newMapping)
    {
        case JOYSTICK_INPUT_MAPPING_ROLL:
            rollAxis = axis;
            break;
        case JOYSTICK_INPUT_MAPPING_PITCH:
            pitchAxis = axis;
            break;
        case JOYSTICK_INPUT_MAPPING_YAW:
            yawAxis = axis;
            break;
        case JOYSTICK_INPUT_MAPPING_THROTTLE:
            throttleAxis = axis;
            break;
        case JOYSTICK_INPUT_MAPPING_NONE:
        default:
            if (rollAxis == axis)
            {
                rollAxis = -1;
            }
            if (pitchAxis == axis)
            {
                pitchAxis = -1;
            }
            if (yawAxis == axis)
            {
                yawAxis = -1;
            }
            if (throttleAxis == axis)
            {
                throttleAxis = -1;
            }
            break;
    }
}

void JoystickInput::setAxisInversion(int axis, bool inverted)
{
    if (axis < joystickAxesInverted.size())
    {
        joystickAxesInverted[axis] = inverted;
    }
}

void JoystickInput::setAxisRangeLimit(int axis, bool rangeLimited)
{
    if (axis < joystickAxesRangeLimited.size())
    {
        joystickAxesRangeLimited[axis] = rangeLimited;
    }
}

float JoystickInput::getCurrentValueForAxis(int axis)
{
    if (axis < joystickAxes.size())
    {
        return joystickAxes[axis];
    }
    return 0.0f;
}
