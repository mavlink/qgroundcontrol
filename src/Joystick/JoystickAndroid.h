#ifndef JOYSTICKANDROID_H
#define JOYSTICKANDROID_H

#include "Joystick.h"
#include "Vehicle.h"
#include "MultiVehicleManager.h"

#include <jni.h>
#include <QtCore/private/qjni_p.h>
#include <QtCore/private/qjnihelpers_p.h>
#include <QtAndroidExtras/QtAndroidExtras>
#include <QtAndroidExtras/QAndroidJniObject>


class JoystickAndroid : public Joystick, public QtAndroidPrivate::GenericMotionEventListener, public QtAndroidPrivate::KeyEventListener
{
public:
    JoystickAndroid(const QString& name, int axisCount, int buttonCount, int id, MultiVehicleManager* multiVehicleManager);
    ~JoystickAndroid();

    static QMap<QString, Joystick*> discover(MultiVehicleManager* _multiVehicleManager); 

private:
    bool handleKeyEvent(jobject event);
    bool handleGenericMotionEvent(jobject event);

    bool _open() final;
    void _close() final;
    bool _update() final;

    bool _getButton(int i) final;
    int _getAxis(int i) final;
    uint8_t _getHat(int hat,int i);

    int *_btnCode;
    int *_axisCode;
    bool *_btnValue;
    int *_axisValue;

    static void _initStatic();
    static int * _androidBtnList; //list of all possible android buttons
    static int _androidBtnListCount;

    static int ACTION_DOWN, ACTION_UP;
    static QMutex _m_mutex;

    int _deviceId;
};

#endif // JOYSTICKANDROID_H
