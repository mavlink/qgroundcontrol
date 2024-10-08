/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QLoggingCategory>
#include <QtCore/private/qandroidextras_p.h>

#include "Joystick.h"

Q_DECLARE_LOGGING_CATEGORY(JoystickAndroidLog)

class JoystickAndroid : public Joystick, public QtAndroidPrivate::GenericMotionEventListener, public QtAndroidPrivate::KeyEventListener
{
public:
    JoystickAndroid(const QString &name, int axisCount, int buttonCount, int id, QObject *parent = nullptr);
    ~JoystickAndroid();

    static bool init();
    static void setNativeMethods();
    static QMap<QString, Joystick*> discover();

private:
    bool _open() final { return true; }
    void _close() final {}
    bool _update() final { return true; }

    bool _getButton(int i) const final { return btnValue[i]; }
    int _getAxis(int i) const final { return axisValue[i]; }
    bool _getHat(int hat, int i) const final;

    int _getAndroidHatAxis(int axisHatCode) const;

    bool handleKeyEvent(jobject event);
    bool handleGenericMotionEvent(jobject event);

    int deviceId = 0;

    QList<int> btnCode;
    QList<int> axisCode;
    QList<bool> btnValue;
    QList<int> axisValue;

    static constexpr int _androidBtnListCount = 31;
    static QList<int> _androidBtnList;

    static int ACTION_DOWN, ACTION_UP, AXIS_HAT_X, AXIS_HAT_Y;
    static QMutex _mutex;
};
