/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/// @file
/// @author Gus Grubba <mavlink@grubba.com>

#include "ScreenToolsController.h"
#include <QScreen>
#if defined(__ios__)
#include <sys/utsname.h>
#endif

ScreenToolsController::ScreenToolsController()
{

}

QString
ScreenToolsController::iOSDevice()
{
#if defined(__ios__)
    struct utsname systemInfo;
    uname(&systemInfo);
    return QString(systemInfo.machine);
#else
    return QString();
#endif
}
