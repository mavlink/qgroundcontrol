/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#include "MobileScreenMgr.h"

#include <QtAndroidExtras/QtAndroidExtras>
#include <QtAndroidExtras/QAndroidJniObject>

static const char* kJniClassName = "org/qgroundcontrol/qgchelper/UsbDeviceJNI";

void MobileScreenMgr::setKeepScreenOn(bool keepScreenOn)
{
    if (keepScreenOn) {
        QAndroidJniObject::callStaticMethod<void>(kJniClassName, "keepScreenOn", "()V");
    } else {
        QAndroidJniObject::callStaticMethod<void>(kJniClassName, "restoreScreenOn", "()V");
    }
}
