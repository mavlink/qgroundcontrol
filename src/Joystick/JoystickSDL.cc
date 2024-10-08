/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "JoystickSDL.h"
#include "QGCLoggingCategory.h"

#include <QtCore/QFile>
#include <QtCore/QIODevice>
#include <QtCore/QTextStream>

#include <SDL.h>

QGC_LOGGING_CATEGORY(JoystickSDLLog, "qgc.joystick.joysticksdl")

JoystickSDL::JoystickSDL(const QString &name, int axisCount, int buttonCount, int hatCount, int index, bool isGameController, QObject *parent)
    : Joystick(name, axisCount, buttonCount, hatCount, parent)
    , _isGameController(isGameController)
    , _index(index)
{
    // qCDebug(JoystickSDLLog) << Q_FUNC_INFO << this;

    if (_isGameController) {
        _setDefaultCalibration();
    }
}

JoystickSDL::~JoystickSDL()
{
    // qCDebug(JoystickSDLLog) << Q_FUNC_INFO << this;
}

bool JoystickSDL::init()
{
    SDL_SetMainReady();
    if (SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER | SDL_INIT_JOYSTICK) < 0) {
        (void) SDL_JoystickEventState(SDL_DISABLE);
        qCWarning(JoystickSDLLog) << "Failed to initialize SDL:" << SDL_GetError();
        return false;
    }

    _loadGameControllerMappings();
    return true;
}

QMap<QString, Joystick*> JoystickSDL::discover()
{
    static QMap<QString, Joystick*> ret;

    QMap<QString, Joystick*> newRet;

    qCDebug(JoystickSDLLog) << "Discovering joysticks";

    for (int i = 0; i < SDL_NumJoysticks(); i++) {
        QString name = SDL_JoystickNameForIndex(i);
        if (ret.contains(name)) {
            newRet[name] = ret[name];
            JoystickSDL *const joystick = static_cast<JoystickSDL*>(newRet[name]);
            if (joystick->index() != i) {
                joystick->setIndex(i); // This joystick index has been remapped by SDL
            }

            // Anything left in ret after we exit the loop has been removed (unplugged) and needs to be cleaned up.
            // We will handle that in JoystickManager in case the removed joystick was in use.
            (void) ret.remove(name);

            qCDebug(JoystickSDLLog) << "Skipping duplicate" << name;
            continue;
        }

        SDL_Joystick *const sdlJoystick = SDL_JoystickOpen(i);
        if (!sdlJoystick) {
            qCWarning(JoystickSDLLog) << "SDL failed opening joystick" << qPrintable(name) << "error:" << SDL_GetError();
            continue;
        }

        SDL_ClearError();
        const int axisCount = SDL_JoystickNumAxes(sdlJoystick);
        const int buttonCount = SDL_JoystickNumButtons(sdlJoystick);
        const int hatCount = SDL_JoystickNumHats(sdlJoystick);
        if ((axisCount < 0) || (buttonCount < 0) || (hatCount < 0)) {
            qCWarning(JoystickSDLLog) << "SDL error parsing joystick features:" << SDL_GetError();
        }
        SDL_JoystickClose(sdlJoystick);

        const bool isGameController = SDL_IsGameController(i);
        qCDebug(JoystickSDLLog) << name << "axes:" << axisCount << "buttons:" << buttonCount << "hats:" << hatCount << "isGC:" << isGameController;

        // Check for joysticks with duplicate names and differentiate the keys when necessary.
        // This is required when using an Xbox 360 wireless receiver that always identifies as
        // 4 individual joysticks, regardless of how many joysticks are actually connected to the
        // receiver. Using GUID does not help, all of these devices present the same GUID.
        const QString originalName = name;
        uint8_t duplicateIdx = 1;
        while (newRet[name]) {
            name = QString("%1 %2").arg(originalName).arg(duplicateIdx++);
        }

        newRet[name] = new JoystickSDL(name, qMax(0, axisCount), qMax(0, buttonCount), qMax(0, hatCount), i, isGameController);
    }

    if (newRet.isEmpty()) {
        qCDebug(JoystickSDLLog) << "None found";
    }

    ret = newRet;
    return ret;
}

bool JoystickSDL::_open()
{
    if (_isGameController) {
        _sdlController = SDL_GameControllerOpen(_index);
        _sdlJoystick = SDL_GameControllerGetJoystick(_sdlController);
    } else {
        _sdlJoystick = SDL_JoystickOpen(_index);
    }

    if (!_sdlJoystick) {
        qCWarning(JoystickSDLLog) << "SDL_JoystickOpen failed:" << SDL_GetError();
        return false;
    }

    qCDebug(JoystickSDLLog) << "Opened" << SDL_JoystickName(_sdlJoystick) << "joystick at" << _sdlJoystick;

    return true;
}

void JoystickSDL::_close()
{
    if (!_sdlJoystick) {
        qCWarning(JoystickSDLLog) << "Attempt to close null joystick!";
        return;
    }

    qCDebug(JoystickSDLLog) << "Closing" << SDL_JoystickName(_sdlJoystick) << "joystick at" << _sdlJoystick;

    if (_isGameController) {
        SDL_GameControllerClose(_sdlController);
    } else {
        SDL_JoystickClose(_sdlJoystick);
    }

    _sdlJoystick = nullptr;
    _sdlController = nullptr;
}

bool JoystickSDL::_update()
{
    if (_isGameController) {
        SDL_GameControllerUpdate();
    } else {
        SDL_JoystickUpdate();
    }

    return true;
}

bool JoystickSDL::_getButton(int i) const
{
    int button = -1;

    if (_isGameController) {
        button = SDL_GameControllerGetButton(_sdlController, SDL_GameControllerButton(i));
    } else {
        button = SDL_JoystickGetButton(_sdlJoystick, i);
    }

    return (button == 1);
}

int JoystickSDL::_getAxis(int i) const
{
    int axis = -1;

    if (_isGameController) {
        axis = SDL_GameControllerGetAxis(_sdlController, SDL_GameControllerAxis(i));
    } else {
        axis = SDL_JoystickGetAxis(_sdlJoystick, i);
    }

    return axis;
}

bool JoystickSDL::_getHat(int hat, int i) const
{
    static constexpr uint8_t hatButtons[] = {SDL_HAT_UP, SDL_HAT_DOWN, SDL_HAT_LEFT, SDL_HAT_RIGHT};

    if (i >= std::size(hatButtons)) {
        return false;
    }

    return ((SDL_JoystickGetHat(_sdlJoystick, hat) & hatButtons[i]) != 0);
}

void JoystickSDL::_loadGameControllerMappings()
{
    QFile file(QStringLiteral(":/db/mapping/joystick/gamecontrollerdb.txt"));
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qCWarning(JoystickSDLLog) << "Couldn't load GameController mapping database.";
        return;
    }

    QTextStream stream(&file);
    while (!stream.atEnd()) {
        auto line = stream.readLine();
        if (line.startsWith('#') || line.isEmpty()) {
            continue;
        }
        if (SDL_GameControllerAddMapping(line.toStdString().c_str()) == -1) {
            qCWarning(JoystickSDLLog) << "Couldn't add GameController mapping:" << SDL_GetError();
        }
    }
}
