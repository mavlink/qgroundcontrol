#include "JoystickSDL.h"

#include "QGCApplication.h"

#include <QQmlEngine>
#include <QTextStream>

JoystickSDL::JoystickSDL(const QString& name, int axisCount, int buttonCount, int hatCount, int index, bool isGameController, MultiVehicleManager* multiVehicleManager)
    : Joystick(name,axisCount,buttonCount,hatCount,multiVehicleManager)
    , _isGameController(isGameController)
    , _index(index)
{
}

QMap<QString, Joystick*> JoystickSDL::discover(MultiVehicleManager* _multiVehicleManager) {
    static QMap<QString, Joystick*> ret;

    if (SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER | SDL_INIT_NOPARACHUTE) < 0) {
        qWarning() << "Couldn't initialize SimpleDirectMediaLayer:" << SDL_GetError();
        return ret;
    }

    _loadGameControllerMappings();

    // Load available joysticks

    qCDebug(JoystickLog) << "Available joysticks";

    for (int i=0; i<SDL_NumJoysticks(); i++) {
        QString name = SDL_JoystickNameForIndex(i);

        if (!ret.contains(name)) {
            int axisCount, buttonCount, hatCount;
            bool isGameController;

            SDL_Joystick* sdlJoystick = SDL_JoystickOpen(i);
            if (SDL_IsGameController(i)) {
                isGameController = true;
                axisCount = SDL_CONTROLLER_AXIS_MAX;
                buttonCount = SDL_CONTROLLER_BUTTON_MAX;
                hatCount = 0;
            } else {
                isGameController = false;
                axisCount = SDL_JoystickNumAxes(sdlJoystick);
                buttonCount = SDL_JoystickNumButtons(sdlJoystick);
                hatCount = SDL_JoystickNumHats(sdlJoystick);
            }

            SDL_JoystickClose(sdlJoystick);

            qCDebug(JoystickLog) << "\t" << name << "axes:" << axisCount << "buttons:" << buttonCount << "hats:" << hatCount << "isGC:" << isGameController;
            ret[name] = new JoystickSDL(name, qMax(0,axisCount), qMax(0,buttonCount), qMax(0,hatCount), i, isGameController, _multiVehicleManager);
        } else {
            qCDebug(JoystickLog) << "\tSkipping duplicate" << name;
        }
    }
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

    return true;
}

void JoystickSDL::_close(void) {
    SDL_JoystickClose(sdlJoystick);
}

bool JoystickSDL::_update(void)
{
    SDL_JoystickUpdate();
    SDL_GameControllerUpdate();
    return true;
}

bool JoystickSDL::_getButton(int i) {
    if ( _isGameController ) {
        return !!SDL_GameControllerGetButton(sdlController, SDL_GameControllerButton(i));
    } else {
        return !!SDL_JoystickGetButton(sdlJoystick, i);
    }
}

int JoystickSDL::_getAxis(int i) {
    if ( _isGameController ) {
        return SDL_GameControllerGetAxis(sdlController, SDL_GameControllerAxis(i));
    } else {
        return SDL_JoystickGetAxis(sdlJoystick, i);
    }
}

uint8_t JoystickSDL::_getHat(int hat,int i) {
    uint8_t hatButtons[] = {SDL_HAT_UP,SDL_HAT_DOWN,SDL_HAT_LEFT,SDL_HAT_RIGHT};

    if ( i < int(sizeof(hatButtons)) ) {
        return !!(SDL_JoystickGetHat(sdlJoystick, hat) & hatButtons[i]);
    }
    return 0;
}

