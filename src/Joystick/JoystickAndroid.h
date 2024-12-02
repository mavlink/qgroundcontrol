/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "Joystick.h"
#include <QtCore/private/qandroidextras_p.h>

class JoystickAndroid : public Joystick, public QtAndroidPrivate::GenericMotionEventListener, public QtAndroidPrivate::KeyEventListener
{
public:
    JoystickAndroid(const QString& name, int axisCount, int buttonCount, int id);
    ~JoystickAndroid();

    static bool init();

    static void setNativeMethods();

    static QMap<QString, Joystick*> discover();

private:
    bool handleKeyEvent(jobject event);
    bool handleGenericMotionEvent(jobject event);
    int  _getAndroidHatAxis(int axisHatCode) const;

    bool _open          () final;
    void _close         () final;
    bool _update        () final;

    bool _getButton     (int i) const final;
    int  _getAxis       (int i) const final;
    bool _getHat        (int hat,int i) const final;

    int *btnCode;
    int *axisCode;
    bool *btnValue;
    int *axisValue;

    static int * _androidBtnList; //list of all possible android buttons
    static int _androidBtnListCount;

    static int ACTION_DOWN, ACTION_UP, AXIS_HAT_X, AXIS_HAT_Y;
    static QMutex m_mutex;

    int deviceId;
};
