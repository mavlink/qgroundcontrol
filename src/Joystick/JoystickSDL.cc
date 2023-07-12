#include "JoystickSDL.h"

#include "QGCApplication.h"

#include <QQmlEngine>
#include <QTextStream>

JoystickSDL::JoystickSDL(const QString& name, int axisCount, int buttonCount, int hatCount, int index, bool isGameController, MultiVehicleManager* multiVehicleManager)
    : Joystick(name,axisCount,buttonCount,hatCount,multiVehicleManager)
    , _isGameController(isGameController)
    , _index(index)
{
    if(_isGameController) _setDefaultCalibration();
}

bool JoystickSDL::init(void) {
    if (SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER | SDL_INIT_JOYSTICK) < 0) {
        SDL_JoystickEventState(SDL_ENABLE);
        qWarning() << "Couldn't initialize SimpleDirectMediaLayer:" << SDL_GetError();
        return false;
    }
    _loadGameControllerMappings();
    return true;
}

QMap<QString, Joystick*> JoystickSDL::discover(MultiVehicleManager* _multiVehicleManager) {
    static QMap<QString, Joystick*> ret;

    QMap<QString,Joystick*> newRet;

    // Load available joysticks

    qCDebug(JoystickLog) << "Available joysticks";

    for (int i=0; i<SDL_NumJoysticks(); i++) {
        QString name = SDL_JoystickNameForIndex(i);

        if (!ret.contains(name)) {
            int axisCount, buttonCount, hatCount;
            bool isGameController = SDL_IsGameController(i);

            if (SDL_Joystick* sdlJoystick = SDL_JoystickOpen(i)) {
                SDL_ClearError();
                axisCount = SDL_JoystickNumAxes(sdlJoystick);
                buttonCount = SDL_JoystickNumButtons(sdlJoystick);
                hatCount = SDL_JoystickNumHats(sdlJoystick);
#ifdef Q_OS_WIN
                if (name == QStringLiteral("Xbox Series X Controller") || name == QStringLiteral("Controller (Xbox One For Windows)")) {
                    hatCount = 0;
                }
#endif
                if (axisCount < 0 || buttonCount < 0 || hatCount < 0) {
                    qCWarning(JoystickLog) << "\t libsdl error parsing joystick features:" << SDL_GetError();
                }
                SDL_JoystickClose(sdlJoystick);
            } else {
                qCWarning(JoystickLog) << "\t libsdl failed opening joystick" << qPrintable(name) << "error:" << SDL_GetError();
                continue;
            }

            qCDebug(JoystickLog) << "\t" << name << "axes:" << axisCount << "buttons:" << buttonCount << "hats:" << hatCount << "isGC:" << isGameController;

            // Check for joysticks with duplicate names and differentiate the keys when necessary.
            // This is required when using an Xbox 360 wireless receiver that always identifies as
            // 4 individual joysticks, regardless of how many joysticks are actually connected to the
            // receiver. Using GUID does not help, all of these devices present the same GUID.
            QString originalName = name;
            uint8_t duplicateIdx = 1;
            while (newRet[name]) {
                name = QString("%1 %2").arg(originalName).arg(duplicateIdx++);
            }

            newRet[name] = new JoystickSDL(name, qMax(0,axisCount), qMax(0,buttonCount), qMax(0,hatCount), i, isGameController, _multiVehicleManager);
        } else {
            newRet[name] = ret[name];
            JoystickSDL *j = static_cast<JoystickSDL*>(newRet[name]);
            if (j->index() != i) {
                j->setIndex(i); // This joystick index has been remapped by SDL
            }
            // Anything left in ret after we exit the loop has been removed (unplugged) and needs to be cleaned up.
            // We will handle that in JoystickManager in case the removed joystick was in use.
            ret.remove(name);
            qCDebug(JoystickLog) << "\tSkipping duplicate" << name;
        }
    }

    if (!newRet.count()) {
        qCDebug(JoystickLog) << "\tnone found";
    }

    ret = newRet;
    return ret;
}

void JoystickSDL::_loadGameControllerMappings(void) {
    QFile file(":/db/mapping/joystick/gamecontrollerdb.txt");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        qWarning() << "Couldn't load GameController mapping database.";
        return;
    }

    QTextStream s(&file);

    while (!s.atEnd()) {
        SDL_GameControllerAddMapping(s.readLine().toStdString().c_str());
    }
}

bool JoystickSDL::_open(void) {
    if ( _isGameController ) {
        sdlController = SDL_GameControllerOpen(_index);
        sdlJoystick = SDL_GameControllerGetJoystick(sdlController);
    } else {
        sdlJoystick = SDL_JoystickOpen(_index);
    }

    if (!sdlJoystick) {
        qCWarning(JoystickLog) << "SDL_JoystickOpen failed:" << SDL_GetError();
        return false;
    }

    qCDebug(JoystickLog) << "Opened joystick at" << sdlJoystick;

    return true;
}

void JoystickSDL::_close(void) {
    if (sdlJoystick == nullptr) {
        qCDebug(JoystickLog) << "Attempt to close null joystick!";
        return;
    }

    qCDebug(JoystickLog) << "Closing" << SDL_JoystickName(sdlJoystick) << "at" << sdlJoystick;

    // We get a segfault if we try to close a joystick that has been detached
    if (SDL_JoystickGetAttached(sdlJoystick) == SDL_FALSE) {
        qCDebug(JoystickLog) << "\tJoystick is not attached!";
    } else {

        if (SDL_JoystickInstanceID(sdlJoystick) != -1) {
            qCDebug(JoystickLog) << "\tID:" << SDL_JoystickInstanceID(sdlJoystick);
            // This segfaults so often, and I've spent so much time trying to find the cause and fix it
            // I think this might be an SDL bug
            // We are much more stable just commenting this out
            //SDL_JoystickClose(sdlJoystick);
        }
    }

    sdlJoystick   = nullptr;
    sdlController = nullptr;
}

bool JoystickSDL::_update(void)
{
    SDL_JoystickUpdate();
    SDL_GameControllerUpdate();
    return true;
}

bool JoystickSDL::_getButton(int i) {
    if (_isGameController) {
        return SDL_GameControllerGetButton(sdlController, SDL_GameControllerButton(i)) == 1;
    } else {
        return SDL_JoystickGetButton(sdlJoystick, i) == 1;
    }
}

int JoystickSDL::_getAxis(int i) {
    if (_isGameController) {
        return SDL_GameControllerGetAxis(sdlController, SDL_GameControllerAxis(i));
    } else {
        return SDL_JoystickGetAxis(sdlJoystick, i);
    }
}

bool JoystickSDL::_getHat(int hat, int i) {
    uint8_t hatButtons[] = {SDL_HAT_UP,SDL_HAT_DOWN,SDL_HAT_LEFT,SDL_HAT_RIGHT};
    if (i < int(sizeof(hatButtons))) {
        return (SDL_JoystickGetHat(sdlJoystick, hat) & hatButtons[i]) != 0;
    }
    return false;
}
