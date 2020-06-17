/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#include "MobileScreenMgr.h"

#include <QtAndroidExtras/QtAndroidExtras>
#include <QtAndroidExtras/QAndroidJniObject>

//static const char* kJniClassName = "org/mavlink/qgroundcontrol/QGCActivity";

void MobileScreenMgr::setKeepScreenOn(bool /*keepScreenOn*/)
{
    //-- Screen is locked on while QGC is running on Android
}
