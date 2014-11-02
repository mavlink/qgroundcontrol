/****************************************************************************
** Copyright (c) 2000-2003 Wayne Roth
** Copyright (c) 2004-2007 Stefan Sander
** Copyright (c) 2007 Michal Policht
** Copyright (c) 2008 Brandon Fosdick
** Copyright (c) 2009-2010 Liam Staskawicz
** Copyright (c) 2011 Debao Zhang
** Copyright (c) 2012 Doug Brown
** All right reserved.
** Web: http://code.google.com/p/qextserialport/
**
** Permission is hereby granted, free of charge, to any person obtaining
** a copy of this software and associated documentation files (the
** "Software"), to deal in the Software without restriction, including
** without limitation the rights to use, copy, modify, merge, publish,
** distribute, sublicense, and/or sell copies of the Software, and to
** permit persons to whom the Software is furnished to do so, subject to
** the following conditions:
**
** The above copyright notice and this permission notice shall be
** included in all copies or substantial portions of the Software.
**
** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
** EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
** MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
** NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
** LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
** OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
** WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
**
****************************************************************************/
#ifndef _QEXTSERIALENUMERATOR_P_H_
#define _QEXTSERIALENUMERATOR_P_H_

//
//  W A R N I N G
//  -------------
//
// This file is not part of the QESP API.  It exists for the convenience
// of other QESP classes.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "qextserialenumerator.h"

#ifdef Q_OS_WIN
// needed for mingw to pull in appropriate dbt business...
// probably a better way to do this
// http://mingw-users.1079350.n2.nabble.com/DEV-BROADCAST-DEVICEINTERFACE-was-not-declared-in-this-scope-td3552762.html
#  ifdef  __MINGW32__
#    define _WIN32_WINNT 0x0500
#    define _WIN32_WINDOWS 0x0500
#    define WINVER 0x0500
#  endif
#  include <QtCore/qt_windows.h>
#endif /*Q_OS_WIN*/

#ifdef Q_OS_MAC
#  include <IOKit/usb/IOUSBLib.h>
#endif /*Q_OS_MAC*/

#if defined(Q_OS_LINUX) && !defined(QESP_NO_UDEV)
#  include <QSocketNotifier>
extern "C" {
#  include <libudev.h>
}
#endif

class QextSerialRegistrationWidget;
class QextSerialEnumeratorPrivate
{
    Q_DECLARE_PUBLIC(QextSerialEnumerator)
public:
    QextSerialEnumeratorPrivate(QextSerialEnumerator *enumrator);
    ~QextSerialEnumeratorPrivate();
    void platformSpecificInit();
    void platformSpecificDestruct();

    static QList<QextPortInfo> getPorts_sys();
    bool setUpNotifications_sys(bool setup);

#ifdef Q_OS_WIN
    LRESULT onDeviceChanged(WPARAM wParam, LPARAM lParam);
    bool matchAndDispatchChangedDevice(const QString &deviceID, const GUID &guid, WPARAM wParam);
#  ifdef QT_GUI_LIB
    QextSerialRegistrationWidget *notificationWidget;
#  endif
#endif /*Q_OS_WIN*/

#ifdef Q_OS_MAC
    /*!
     * Search for serial ports using IOKit.
     *    \param infoList list with result.
     */
    static void iterateServicesOSX(io_object_t service, QList<QextPortInfo> &infoList);
    static bool getServiceDetailsOSX(io_object_t service, QextPortInfo *portInfo);
    void onDeviceDiscoveredOSX(io_object_t service);
    void onDeviceTerminatedOSX(io_object_t service);
    friend void deviceDiscoveredCallbackOSX(void *ctxt, io_iterator_t serialPortIterator);
    friend void deviceTerminatedCallbackOSX(void *ctxt, io_iterator_t serialPortIterator);

    IONotificationPortRef notificationPortRef;
#endif // Q_OS_MAC

#if defined(Q_OS_LINUX) && !defined(QESP_NO_UDEV)
    QSocketNotifier *notifier;
    int notifierFd;
    struct udev *udev;
    struct udev_monitor *monitor;

    void _q_deviceEvent();
#endif

private:
    QextSerialEnumerator *q_ptr;
};

#endif //_QEXTSERIALENUMERATOR_P_H_
