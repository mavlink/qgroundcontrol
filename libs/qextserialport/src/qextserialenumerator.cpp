/****************************************************************************
** Copyright (c) 2000-2003 Wayne Roth
** Copyright (c) 2004-2007 Stefan Sander
** Copyright (c) 2007 Michal Policht
** Copyright (c) 2008 Brandon Fosdick
** Copyright (c) 2009-2010 Liam Staskawicz
** Copyright (c) 2011 Debao Zhang
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

#include "qextserialenumerator.h"
#include "qextserialenumerator_p.h"

#include <QtCore/QDebug>
#include <QtCore/QMetaType>
#include <QtCore/QRegExp>

QextSerialEnumeratorPrivate::QextSerialEnumeratorPrivate(QextSerialEnumerator *enumrator)
    :q_ptr(enumrator)
{
    platformSpecificInit();
}

QextSerialEnumeratorPrivate::~QextSerialEnumeratorPrivate()
{
    platformSpecificDestruct();
}

/*!
  \class QextPortInfo

  \brief The QextPortInfo class containing port information.

  Structure containing port information.

  \code
  QString portName;   ///< Port name.
  QString physName;   ///< Physical name.
  QString friendName; ///< Friendly name.
  QString enumName;   ///< Enumerator name.
  int vendorID;       ///< Vendor ID.
  int productID;      ///< Product ID
  \endcode
 */

/*! \class QextSerialEnumerator

    \brief The QextSerialEnumerator class provides list of ports available in the system.
  
    \section1 Usage
    To poll the system for a list of connected devices, simply use getPorts().  Each
    QextPortInfo structure will populated with information about the corresponding device.
  
    \bold Example
    \code
    QList<QextPortInfo> ports = QextSerialEnumerator::getPorts();
    foreach (QextPortInfo port, ports) {
        // inspect port...
    }
    \endcode
  
    To enable event-driven notification of device connection events, first call
    setUpNotifications() and then connect to the deviceDiscovered() and deviceRemoved()
    signals.  Event-driven behavior is currently available only on Windows and OS X.
  
    \bold Example
    \code
    QextSerialEnumerator *enumerator = new QextSerialEnumerator();
    connect(enumerator, SIGNAL(deviceDiscovered(const QextPortInfo &)),
               myClass, SLOT(onDeviceDiscovered(const QextPortInfo &)));
    connect(enumerator, SIGNAL(deviceRemoved(const QextPortInfo &)),
               myClass, SLOT(onDeviceRemoved(const QextPortInfo &)));
    \endcode
  
    \section1 Credits
    Windows implementation is based on Zach Gorman's work from
    \l {http://www.codeproject.com}{The Code Project} (\l http://www.codeproject.com/system/setupdi.asp).
  
    OS X implementation, see \l http://developer.apple.com/documentation/DeviceDrivers/Conceptual/AccessingHardware/AH_Finding_Devices/chapter_4_section_2.html
  
    \bold author Michal Policht, Liam Staskawicz
*/

/*!
    \fn void QextSerialEnumerator::deviceDiscovered(const QextPortInfo &info)
    A new device has been connected to the system.
  
    setUpNotifications() must be called first to enable event-driven device notifications.
    Currently only implemented on Windows and OS X.
  
    \a info The device that has been discovered.
*/

/*!
   \fn void QextSerialEnumerator::deviceRemoved(const QextPortInfo &info);
    A device has been disconnected from the system.
  
    setUpNotifications() must be called first to enable event-driven device notifications.
    Currently only implemented on Windows and OS X.
  
    \a info The device that was disconnected.
*/

/*!
   Constructs a QextSerialEnumerator object with the given \a parent.
*/
QextSerialEnumerator::QextSerialEnumerator(QObject *parent)
    :QObject(parent), d_ptr(new QextSerialEnumeratorPrivate(this))
{
    if (!QMetaType::isRegistered(QMetaType::type("QextPortInfo")))
        qRegisterMetaType<QextPortInfo>("QextPortInfo");
}

/*!
   Destructs the QextSerialEnumerator object.
*/
QextSerialEnumerator::~QextSerialEnumerator()
{
    delete d_ptr;
}

/*!
    Get list of ports.

    return list of ports currently available in the system.
*/
QList<QextPortInfo> QextSerialEnumerator::getPorts()
{
    return QextSerialEnumeratorPrivate::getPorts_sys();
}

/*!
    Enable event-driven notifications of board discovery/removal.
*/
void QextSerialEnumerator::setUpNotifications()
{
    Q_D(QextSerialEnumerator);
    if (!d->setUpNotifications_sys(true))
        QESP_WARNING("Setup Notification Failed...");
}

#include "moc_qextserialenumerator.cpp"
