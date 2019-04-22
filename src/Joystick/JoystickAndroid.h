#ifndef JOYSTICKANDROID_H
#define JOYSTICKANDROID_H

#include "Joystick.h"
#include "Vehicle.h"
#include "MultiVehicleManager.h"
#include "JoystickManager.h"
#include "InputEventReader.h"

#include <jni.h>
#include <QObject>
#include <QTimer>
#include <QSettings>
#include <QtCore/private/qjni_p.h>
#include <QtCore/private/qjnihelpers_p.h>
#include <QtAndroidExtras/QtAndroidExtras>
#include <QtAndroidExtras/QAndroidJniObject>

class JoystickAndroid : public Joystick
{
    Q_OBJECT
public:
    JoystickAndroid(const QString& name, int axisCount, int buttonCount, int id, MultiVehicleManager* multiVehicleManager, JoystickManager* joystickManager);
    ~JoystickAndroid();

    static QMap<QString, Joystick*> discover(MultiVehicleManager* _multiVehicleManager, JoystickManager* _joystickManager);

private slots:
    void _handleKeyEvent(int keycode, int action);
    void _handleGenericMotionEvent(int axiscode, float value);
    void handleLongPress();

private:
    virtual bool _open();
    virtual void _close();
    virtual bool _update();

    virtual bool _getButton(int i);
    virtual int _getAxis(int i);
    virtual uint8_t _getHat(int hat,int i);
    virtual void saveJoystickSettings();
    bool handleKeyEventInner(int keycode, int action);
    bool _handleKeyEventInner(int keycode, int action);
    int getKeyIndexByCode(int code);
    void sendChannelValue(int sbus, int ch, int value);
    bool getChannelValue(int keyCode, KeyConfiguration::KeyAction_t action, int *sbus, int *ch, int *value);

    int *btnCode;
    int *axisCode;
    bool *btnValue;
    int *axisValue;

    enum Keys {
        KEY_A = 0,
        KEY_B,
        KEY_C,
        KEY_D,
        KEY_CAM,
        KEY_MAX
    };

    typedef struct {
        int keyCode;
        quint64 startTime;
        bool isPressed;
        bool isLongPress;
        QTimer *timer;
    } KeyState_t;
    KeyState_t _keyEvents[KEY_MAX];

    static void _initStatic();
    static int * _androidBtnList; //list of all possible android buttons
    static int _androidBtnListCount;

    static int ACTION_DOWN, ACTION_UP;

    static int KEYCODE_A;//button A
    static int KEYCODE_B;//button B
    static int KEYCODE_C;//button C
    static int KEYCODE_D;//button D
    static int KEYCODE_CAM;//button CAM
    static QMutex m_mutex;

    int deviceId;
    InputEventReader *eventReader;
    QSettings *_configSaver;
};

#endif // JOYSTICKANDROID_H
