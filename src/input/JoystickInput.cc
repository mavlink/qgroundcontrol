/*=====================================================================
======================================================================*/

/**
 * @file
 *   @brief Joystick interface
 *
 *   @author Lorenz Meier <mavteam@student.ethz.ch>
 *   @author Andreas Romer <mavteam@student.ethz.ch>
 *   @author Julian Oes <julian@oes.ch
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
#include <limits>

using namespace std;

/**
 * The coordinate frame of the joystick axis is the aeronautical frame like shown on this image:
 * @image html http://pixhawk.ethz.ch/wiki/_media/standards/body-frame.png Aeronautical frame
 */
JoystickInput::JoystickInput() :
    sdlJoystickMin(-32768.0f),
    sdlJoystickMax(32767.0f),
    isEnabled(false),
    isCalibrating(false),
    done(false),
    uas(NULL),
    autopilotType(0),
    systemType(0),
    uasCanReverse(false),
    rollAxis(-1),
    pitchAxis(-1),
    yawAxis(-1),
    throttleAxis(-1),
    joystickName(""),
    joystickID(-1),
    joystickNumButtons(0)
{

    // Make sure we initialize with the correct UAS.
    setActiveUAS(UASManager::instance()->getActiveUAS());

    // Start this thread. This allows the Joystick Settings window to work correctly even w/o any UASes connected.
    start();
}

JoystickInput::~JoystickInput()
{
    storeGeneralSettings();
    storeJoystickSettings();
    done = true;
}

void JoystickInput::loadGeneralSettings()
{
    // Load defaults from settings
    QSettings settings;

    // Deal with settings specific to the JoystickInput
    settings.beginGroup("JOYSTICK_INPUT");
    isEnabled = settings.value("ENABLED", false).toBool();
    joystickName = settings.value("JOYSTICK_NAME", "").toString();
    mode = (JOYSTICK_MODE)settings.value("JOYSTICK_MODE", JOYSTICK_MODE_ATTITUDE).toInt();
    settings.endGroup();
}

/**
 * @brief Restores settings for the current joystick from saved settings file.
 * Assumes that both joystickName & joystickNumAxes are correct.
 */
void JoystickInput::loadJoystickSettings()
{
    // Load defaults from settings
    QSettings settings;

    // Now for the current joystick
    settings.beginGroup(joystickName);
    rollAxis = (settings.value("ROLL_AXIS_MAPPING", -1).toInt());
    pitchAxis = (settings.value("PITCH_AXIS_MAPPING", -1).toInt());
    yawAxis = (settings.value("YAW_AXIS_MAPPING", -1).toInt());
    throttleAxis = (settings.value("THROTTLE_AXIS_MAPPING", -1).toInt());

    // Clear out and then restore the (AUTOPILOT, SYSTEM) mapping for joystick settings
    joystickSettings.clear();
    int autopilots = settings.beginReadArray("AUTOPILOTS");
    for (int i = 0; i < autopilots; i++)
    {
        settings.setArrayIndex(i);
        int autopilotType = settings.value("AUTOPILOT_TYPE", 0).toInt();
        int systems = settings.beginReadArray("SYSTEMS");
        for (int j = 0; j < systems; j++)
        {
            settings.setArrayIndex(j);
            int systemType = settings.value("SYSTEM_TYPE", 0).toInt();

            // Now that both the autopilot and system type are available, update some references.
            QMap<int, bool>* joystickAxesInverted = &joystickSettings[autopilotType][systemType].axesInverted;
            QMap<int, bool>* joystickAxesLimited = &joystickSettings[autopilotType][systemType].axesLimited;
            QMap<int, float>* joystickAxesMinRange = &joystickSettings[autopilotType][systemType].axesMaxRange;
            QMap<int, float>* joystickAxesMaxRange = &joystickSettings[autopilotType][systemType].axesMinRange;
            QMap<int, int>* joystickButtonActions = &joystickSettings[autopilotType][systemType].buttonActions;

            // Read back the joystickAxesInverted QList one element at a time.
            int axesStored = settings.beginReadArray("AXES_INVERTED");
            for (int k = 0; k < axesStored; k++)
            {
                settings.setArrayIndex(k);
                int index = settings.value("INDEX", 0).toInt();
                bool inverted = settings.value("INVERTED", false).toBool();
                joystickAxesInverted->insert(index, inverted);
            }
            settings.endArray();

            // Read back the joystickAxesLimited QList one element at a time.
            axesStored = settings.beginReadArray("AXES_LIMITED");
            for (int k = 0; k < axesStored; k++)
            {
                settings.setArrayIndex(k);
                int index = settings.value("INDEX", 0).toInt();
                bool limited = settings.value("LIMITED", false).toBool();
                joystickAxesLimited->insert(index, limited);
            }
            settings.endArray();

            // Read back the joystickAxesMinRange QList one element at a time.
            axesStored = settings.beginReadArray("AXES_MIN_RANGE");
            for (int k = 0; k < axesStored; k++)
            {
                settings.setArrayIndex(k);
                int index = settings.value("INDEX", 0).toInt();
                float min = settings.value("MIN_RANGE", false).toFloat();
                joystickAxesMinRange->insert(index, min);
            }
            settings.endArray();

            // Read back the joystickAxesMaxRange QList one element at a time.
            axesStored = settings.beginReadArray("AXES_MAX_RANGE");
            for (int k = 0; k < axesStored; k++)
            {
                settings.setArrayIndex(k);
                int index = settings.value("INDEX", 0).toInt();
                float max = settings.value("MAX_RANGE", false).toFloat();
                joystickAxesMaxRange->insert(index, max);
            }
            settings.endArray();

            // Read back the button->action mapping.
            int buttonsStored = settings.beginReadArray("BUTTONS_ACTIONS");
            for (int k = 0; k < buttonsStored; k++)
            {
                settings.setArrayIndex(k);
                int index = settings.value("INDEX", 0).toInt();
                int action = settings.value("ACTION", 0).toInt();
                joystickButtonActions->insert(index, action);
            }
            settings.endArray();
        }
        settings.endArray();
    }
    settings.endArray();

    settings.endGroup();

    emit joystickSettingsChanged();
}

void JoystickInput::storeGeneralSettings() const
{
    QSettings settings;
    settings.beginGroup("JOYSTICK_INPUT");
    settings.setValue("ENABLED", isEnabled);
    settings.setValue("JOYSTICK_NAME", joystickName);
    settings.setValue("JOYSTICK_MODE", mode);
    settings.endGroup();
}

void JoystickInput::storeJoystickSettings() const
{
    // Store settings
    QSettings settings;
    settings.beginGroup(joystickName);
    settings.setValue("ROLL_AXIS_MAPPING", rollAxis);
    settings.setValue("PITCH_AXIS_MAPPING", pitchAxis);
    settings.setValue("YAW_AXIS_MAPPING", yawAxis);
    settings.setValue("THROTTLE_AXIS_MAPPING", throttleAxis);
    settings.beginWriteArray("AUTOPILOTS");
    QMapIterator<int, QMap<int, JoystickSettings> > i(joystickSettings);
    int autopilotIndex = 0;
    while (i.hasNext())
    {
        i.next();
        settings.setArrayIndex(autopilotIndex++);

        int autopilotType = i.key();
        settings.setValue("AUTOPILOT_TYPE", autopilotType);

        settings.beginWriteArray("SYSTEMS");
        QMapIterator<int, JoystickSettings> j(i.value());
        int systemIndex = 0;
        while (j.hasNext())
        {
            j.next();
            settings.setArrayIndex(systemIndex++);

            int systemType = j.key();
            settings.setValue("SYSTEM_TYPE", systemType);

            // Now that both the autopilot and system type are available, update some references.
            QMapIterator<int, bool> joystickAxesInverted(joystickSettings[autopilotType][systemType].axesInverted);
            QMapIterator<int, bool> joystickAxesLimited(joystickSettings[autopilotType][systemType].axesLimited);
            QMapIterator<int, float> joystickAxesMinRange(joystickSettings[autopilotType][systemType].axesMaxRange);
            QMapIterator<int, float> joystickAxesMaxRange(joystickSettings[autopilotType][systemType].axesMinRange);
            QMapIterator<int, int> joystickButtonActions(joystickSettings[autopilotType][systemType].buttonActions);

            settings.beginWriteArray("AXES_INVERTED");
            int k = 0;
            while (joystickAxesInverted.hasNext())
            {
                joystickAxesInverted.next();
                int inverted = joystickAxesInverted.value();
                // Only save this axes' inversion status if it's not the default
                if (inverted)
                {
                    settings.setArrayIndex(k++);
                    int index = joystickAxesInverted.key();
                    settings.setValue("INDEX", index);
                    settings.setValue("INVERTED", inverted);
                }
            }
            settings.endArray();

            settings.beginWriteArray("AXES_MIN_RANGE");
            k = 0;
            while (joystickAxesMinRange.hasNext())
            {
                joystickAxesMinRange.next();
                float min = joystickAxesMinRange.value();
                settings.setArrayIndex(k++);
                int index = joystickAxesMinRange.key();
                settings.setValue("INDEX", index);
                settings.setValue("MIN_RANGE", min);
            }
            settings.endArray();

            settings.beginWriteArray("AXES_MAX_RANGE");
            k = 0;
            while (joystickAxesMaxRange.hasNext())
            {
                joystickAxesMaxRange.next();
                float max = joystickAxesMaxRange.value();
                settings.setArrayIndex(k++);
                int index = joystickAxesMaxRange.key();
                settings.setValue("INDEX", index);
                settings.setValue("MAX_RANGE", max);
            }
            settings.endArray();

            settings.beginWriteArray("AXES_LIMITED");
            k = 0;
            while (joystickAxesLimited.hasNext())
            {
                joystickAxesLimited.next();
                int limited = joystickAxesLimited.value();
                if (limited)
                {
                    settings.setArrayIndex(k++);
                    int index = joystickAxesLimited.key();
                    settings.setValue("INDEX", index);
                    settings.setValue("LIMITED", limited);
                }
            }
            settings.endArray();

            settings.beginWriteArray("BUTTONS_ACTIONS");
            k = 0;
            while (joystickButtonActions.hasNext())
            {
                joystickButtonActions.next();
                int action = joystickButtonActions.value();
                if (action != -1)
                {
                    settings.setArrayIndex(k++);
                    int index = joystickButtonActions.key();
                    settings.setValue("INDEX", index);
                    settings.setValue("ACTION", action);
                }
            }
            settings.endArray();
        }
        settings.endArray(); // SYSTEMS
    }
    settings.endArray(); // AUTOPILOTS
    settings.endGroup();
}

void JoystickInput::setActiveUAS(UASInterface* uas)
{
    // Do nothing if the UAS hasn't changed.
    if (uas == this->uas)
    {
        return;
    }

    // Only connect / disconnect is the UAS is of a controllable UAS class
    UAS* tmp = 0;
    if (this->uas)
    {
        tmp = dynamic_cast<UAS*>(this->uas);
        if(tmp)
        {
            disconnect(this, SIGNAL(joystickChanged(float,float,float,float,qint8,qint8,quint16,quint8)), tmp, SLOT(setExternalControlSetpoint(float,float,float,float,qint8,qint8,quint16,quint8)));
            disconnect(this, SIGNAL(actionTriggered(int)), tmp, SLOT(triggerAction(int)));
        }
        uasCanReverse = false;
    }

    // Save any settings for the last UAS
    if (joystickID > -1)
    {
        storeJoystickSettings();
    }

    this->uas = uas;

    if (this->uas && (tmp = dynamic_cast<UAS*>(this->uas)))
    {
        connect(this, SIGNAL(joystickChanged(float,float,float,float,qint8,qint8,quint16,quint8)), tmp, SLOT(setExternalControlSetpoint(float,float,float,float,qint8,qint8,quint16,quint8)));
        qDebug() << "connected joystick";
        connect(this, SIGNAL(actionTriggered(int)), tmp, SLOT(triggerAction(int)));
        uasCanReverse = tmp->systemCanReverse();

        // Update the joystick settings for a new UAS.
        autopilotType = uas->getAutopilotType();
        systemType = uas->getSystemType();
    }

    // Make sure any UI elements know we've updated the UAS. The UASManager signal is re-emitted here so that UI elements know to
    // update their UAS-specific UI.
    emit activeUASSet(uas);

    // Load any joystick-specific settings now that the UAS has changed.
    if (joystickID > -1)
    {
        loadJoystickSettings();
    }
}

void JoystickInput::setEnabled(bool enabled)
{
    this->isEnabled = enabled;
    storeJoystickSettings();
}

void JoystickInput::init()
{
    // Initialize SDL Joystick support and detect number of joysticks.
    if (SDL_InitSubSystem(SDL_INIT_JOYSTICK | SDL_INIT_NOPARACHUTE) < 0) {
        printf("Couldn't initialize SimpleDirectMediaLayer: %s\n", SDL_GetError());
    }

    // Enumerate joysticks and select one
    numJoysticks = SDL_NumJoysticks();

    // If no joysticks are connected, there's nothing we can do, so just keep
    // going back to sleep every second unless the program quits.
    while (!numJoysticks && !done)
    {
        QGC::SLEEP::sleep(1);
    }
    if (done)
    {
        return;
    }

    // Now that we've detected a joystick, load in the joystick-agnostic settings.
    loadGeneralSettings();

    // Enumerate all found devices
    qDebug() << QString("%1 Input devices found:").arg(numJoysticks);
    int activeJoystick = 0;
    for(int i=0; i < numJoysticks; i++ )
    {
        QString name = SDL_JoystickName(i);
        qDebug() << QString("\t%1").arg(name);

        // If we've matched this joystick to what was last opened, note it.
        // Note: The way this is implemented the LAST joystick of a given name will be opened.
        if (name == joystickName)
        {
            activeJoystick = i;
        }

        SDL_Joystick* x = SDL_JoystickOpen(i);
        qDebug() << QString("\tNumber of Axes: %1").arg(QString::number(SDL_JoystickNumAxes(x)));
        qDebug() << QString("\tNumber of Buttons: %1").arg(QString::number(SDL_JoystickNumButtons(x)));
        SDL_JoystickClose(x);
    }

    // Set the active joystick based on name, if a joystick was found in the saved settings, otherwise
    // default to the first one.
    setActiveJoystick(activeJoystick);

    // Now make sure we know what the current UAS is and track changes to it.
    setActiveUAS(UASManager::instance()->getActiveUAS());
    connect(UASManager::instance(), SIGNAL(activeUASSet(UASInterface*)), this, SLOT(setActiveUAS(UASInterface*)));
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
            // Here we map the joystick axis value into the initial range of [0:1].
            float axisValue = SDL_JoystickGetAxis(joystick, i);

            // during calibration save min and max values
            if (isCalibrating)
            {
                if (joystickSettings[autopilotType][systemType].axesMinRange.value(i) > axisValue)
                {
                    joystickSettings[autopilotType][systemType].axesMinRange[i] = axisValue;
                }

                if (joystickSettings[autopilotType][systemType].axesMaxRange.value(i) < axisValue)
                {
                    joystickSettings[autopilotType][systemType].axesMaxRange[i] = axisValue;
                }
            }

            if (joystickSettings[autopilotType][systemType].axesInverted[i])
            {
                axisValue = (axisValue - joystickSettings[autopilotType][systemType].axesMinRange.value(i)) /
                        (joystickSettings[autopilotType][systemType].axesMaxRange.value(i) -
                         joystickSettings[autopilotType][systemType].axesMinRange.value(i));
            }
            else
            {
                axisValue = (axisValue - joystickSettings[autopilotType][systemType].axesMaxRange.value(i)) /
                        (joystickSettings[autopilotType][systemType].axesMinRange.value(i) -
                         joystickSettings[autopilotType][systemType].axesMaxRange.value(i));
            }
            axisValue = 1.0f - axisValue;

            // For non-throttle axes or if the UAS can reverse, go ahead and convert this into the range [-1:1].

            //if (uasCanReverse || throttleAxis != i)
            // don't take into account if UAS can reverse. This means to reverse position but not throttle
            // therefore deactivated for now
            if (throttleAxis != i)
            {
                axisValue = axisValue * 2.0f - 1.0f;
            }
            // Otherwise if this vehicle can only go forward, scale it to [0:1].
            else if (throttleAxis == i && joystickSettings[autopilotType][systemType].axesLimited.value(i))
            {
                if (axisValue < 0.0f)
                {
                    axisValue = 0.0f;
                }
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
        qint8 newYHat = 0;
        if ((SDL_HAT_UP & hatPosition) > 0) newYHat = 1;
        if ((SDL_HAT_DOWN & hatPosition) > 0) newYHat = -1;
        qint8 newXHat = 0;
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
                if (isEnabled && joystickSettings[autopilotType][systemType].buttonActions.contains(i))
                {
                    emit actionTriggered(joystickSettings[autopilotType][systemType].buttonActions.value(i));
                }
                joystickButtons &= ~(1 << i);
            }
        }

        // Now signal an update for all UI together.
        if (isEnabled)
        {
            float roll = rollAxis > -1?joystickAxes[rollAxis]:numeric_limits<float>::quiet_NaN();
            float pitch = pitchAxis > -1?joystickAxes[pitchAxis]:numeric_limits<float>::quiet_NaN();
            float yaw = yawAxis > -1?joystickAxes[yawAxis]:numeric_limits<float>::quiet_NaN();
            float throttle = throttleAxis > -1?joystickAxes[throttleAxis]:numeric_limits<float>::quiet_NaN();
            emit joystickChanged(roll, pitch, yaw, throttle, xHat, yHat, joystickButtons, mode);
        }

        // Sleep, update rate of joystick is approx. 25 Hz (1000 ms / 25 = 40 ms)
        QGC::SLEEP::msleep(40);
    }
}

void JoystickInput::setActiveJoystick(int id)
{
    // If we already had a joystick, close that one before opening a new one.
    if (joystick && SDL_JoystickOpened(joystickID))
    {
        storeJoystickSettings();
        SDL_JoystickClose(joystick);
        joystick = NULL;
        joystickID = -1;
    }

    joystickID = id;
    joystick = SDL_JoystickOpen(joystickID);
    if (joystick && SDL_JoystickOpened(joystickID))
    {
        // Update joystick configuration.
        joystickName = QString(SDL_JoystickName(joystickID));
        joystickNumButtons = SDL_JoystickNumButtons(joystick);
        joystickNumAxes = SDL_JoystickNumAxes(joystick);

        // Restore saved settings for this joystick.
        loadJoystickSettings();

        // Update cached joystick axes values.
        // Also emit any signals for currently-triggering events
        joystickAxes.clear();
        for (int i = 0; i < joystickNumAxes; i++)
        {
            joystickAxes.append(numeric_limits<float>::quiet_NaN());
        }

        // Update cached joystick button values.
        // Emit signals for any button events.
        joystickButtons = 0;
    }
    else
    {
        joystickNumButtons = 0;
        joystickNumAxes = 0;
    }

    // Specify that a new joystick has been selected, so that any UI elements can update.
    emit newJoystickSelected();
    // And then trigger an update of this new UI.
    emit joystickSettingsChanged();
}

void JoystickInput::setCalibrating(bool active)
{
    if (active)
    {
        setEnabled(false);
        isCalibrating = true;

        // set range small so that limits can be re-found
        for (int i = 0; i < joystickNumAxes; i++)
        {
            joystickSettings[autopilotType][systemType].axesMinRange[i] = -10.0f;
            joystickSettings[autopilotType][systemType].axesMaxRange[i] = 10.0f;
        }

    } else {

        // store calibration values
        storeJoystickSettings();

        qDebug() << "Calibration result:";
        for (int i = 0; i < joystickNumAxes; i++)
        {
            qDebug() << i << ": " <<
                        joystickSettings[autopilotType][systemType].axesMinRange[i] <<
                        " - " <<
                        joystickSettings[autopilotType][systemType].axesMaxRange[i];
        }
        setEnabled(true);
        isCalibrating = false;
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
                joystickSettings[autopilotType][systemType].axesLimited.remove(axis);
            }
            break;
    }
    storeJoystickSettings();
}

void JoystickInput::setAxisInversion(int axis, bool inverted)
{
    if (axis < joystickNumAxes)
    {
        joystickSettings[autopilotType][systemType].axesInverted[axis] = inverted;
        storeJoystickSettings();
    }
}

void JoystickInput::setAxisRangeLimit(int axis, bool limitRange)
{
    if (axis < joystickNumAxes)
    {
        joystickSettings[autopilotType][systemType].axesLimited[axis] = limitRange;
        storeJoystickSettings();
    }
}

void JoystickInput::setAxisRangeLimitMin(int axis, float min)
{
    if (axis < joystickNumAxes)
    {
        joystickSettings[autopilotType][systemType].axesMinRange[axis] = min;
        storeJoystickSettings();
    }
}

void JoystickInput::setAxisRangeLimitMax(int axis, float max)
{
    if (axis < joystickNumAxes)
    {
        joystickSettings[autopilotType][systemType].axesMaxRange[axis] = max;
        storeJoystickSettings();
    }
}

void JoystickInput::setButtonAction(int button, int action)
{
    if (button < joystickNumButtons)
    {
        joystickSettings[autopilotType][systemType].buttonActions[button] = action;
        storeJoystickSettings();
    }
}

float JoystickInput::getCurrentValueForAxis(int axis) const
{
    if (axis < joystickNumAxes)
    {
        return joystickAxes.at(axis);
    }
    return 0.0f;
}

bool JoystickInput::getInvertedForAxis(int axis) const
{
    if (axis < joystickNumAxes)
    {
        return joystickSettings[autopilotType][systemType].axesInverted.value(axis);
    }
    return false;
}

bool JoystickInput::getRangeLimitForAxis(int axis) const
{
    if (axis < joystickNumAxes)
    {
        return joystickSettings[autopilotType][systemType].axesLimited.value(axis);
    }
    return false;
}

float JoystickInput::getAxisRangeLimitMinForAxis(int axis) const
{
    if (axis < joystickNumAxes)
    {
        return joystickSettings[autopilotType][systemType].axesMinRange.value(axis);
    }
    return sdlJoystickMin;
}

float JoystickInput::getAxisRangeLimitMaxForAxis(int axis) const
{
    if (axis < joystickNumAxes)
    {
        return joystickSettings[autopilotType][systemType].axesMaxRange.value(axis);
    }
    return sdlJoystickMax;
}

int JoystickInput::getActionForButton(int button) const
{
    if (button < joystickNumButtons && joystickSettings[autopilotType][systemType].buttonActions.contains(button))
    {
        return joystickSettings[autopilotType][systemType].buttonActions.value(button);
    }
    return -1;
}
