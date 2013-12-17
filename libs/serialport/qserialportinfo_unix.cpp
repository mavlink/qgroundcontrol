/****************************************************************************
**
** Copyright (C) 2011-2012 Denis Shienkov <denis.shienkov@gmail.com>
** Copyright (C) 2011 Sergey Belyashov <Sergey.Belyashov@gmail.com>
** Copyright (C) 2012 Laszlo Papp <lpapp@kde.org>
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtSerialPort module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qserialportinfo.h"
#include "qserialportinfo_p.h"
#include "qttylocker_unix_p.h"
#include "qserialport_unix_p.h"
#include <QtCore/qfile.h>

#ifndef Q_OS_MAC

#if defined (Q_OS_LINUX) && defined (HAVE_LIBUDEV)
extern "C"
{
#include <libudev.h>
}
#else
#include <QtCore/qdir.h>
#include <QtCore/qstringlist.h>
#endif

#endif // Q_OS_MAC

QT_BEGIN_NAMESPACE

#ifndef Q_OS_MAC

#if !(defined (Q_OS_LINUX) && defined (HAVE_LIBUDEV))

static inline const QStringList& filtersOfDevices()
{
    static const QStringList deviceFileNameFilterList = QStringList()

#  ifdef Q_OS_LINUX
    << QLatin1String("ttyS*")    // Standart UART 8250 and etc.
    << QLatin1String("ttyUSB*")  // Usb/serial converters PL2303 and etc.
    << QLatin1String("ttyACM*")  // CDC_ACM converters (i.e. Mobile Phones).
    << QLatin1String("ttyGS*")   // Gadget serial device (i.e. Mobile Phones with gadget serial driver).
    << QLatin1String("ttyMI*")   // MOXA pci/serial converters.
    << QLatin1String("ttyAMA*")  // AMBA serial device for embedded platform on ARM (i.e. Raspberry Pi).
    << QLatin1String("rfcomm*")  // Bluetooth serial device.
    << QLatin1String("ircomm*"); // IrDA serial device.
#  elif defined (Q_OS_FREEBSD)
    << QLatin1String("cu*");
#  else
    ; // Here for other *nix OS.
#  endif

    return deviceFileNameFilterList;
}

#endif

QList<QSerialPortInfo> QSerialPortInfo::availablePorts()
{
    QList<QSerialPortInfo> serialPortInfoList;

#if defined (Q_OS_LINUX) && defined (HAVE_LIBUDEV)

    // White list for devices without a parent
    static const QString rfcommDeviceName(QLatin1String("rfcomm"));

    struct ::udev *udev = ::udev_new();
    if (udev) {

        struct ::udev_enumerate *enumerate =
                ::udev_enumerate_new(udev);

        if (enumerate) {

            ::udev_enumerate_add_match_subsystem(enumerate, "tty");
            ::udev_enumerate_scan_devices(enumerate);

            struct ::udev_list_entry *devices =
                    ::udev_enumerate_get_list_entry(enumerate);

            struct ::udev_list_entry *dev_list_entry;
            udev_list_entry_foreach(dev_list_entry, devices) {

                struct ::udev_device *dev =
                        ::udev_device_new_from_syspath(udev,
                                                       ::udev_list_entry_get_name(dev_list_entry));

                if (dev) {

                    QSerialPortInfo serialPortInfo;

                    serialPortInfo.d_ptr->device =
                            QLatin1String(::udev_device_get_devnode(dev));
                    serialPortInfo.d_ptr->portName =
                            QLatin1String(::udev_device_get_sysname(dev));

                    struct ::udev_device *parentdev = ::udev_device_get_parent(dev);

                    bool canAppendToList = true;

                    if (parentdev) {

                        QLatin1String subsys(::udev_device_get_subsystem(parentdev));

                        if (subsys == QLatin1String("usb-serial")
                                || subsys == QLatin1String("usb")) { // USB bus type
                            // Append this devices and try get additional information about them.
                            serialPortInfo.d_ptr->description = QString(
                                    QLatin1String(::udev_device_get_property_value(dev,
                                                                                   "ID_MODEL"))).replace('_', ' ');
                            serialPortInfo.d_ptr->manufacturer = QString(
                                    QLatin1String(::udev_device_get_property_value(dev,
                                                                                   "ID_VENDOR"))).replace('_', ' ');

                            serialPortInfo.d_ptr->vendorIdentifier =
                                    QString::fromLatin1(::udev_device_get_property_value(dev,
                                                "ID_VENDOR_ID")).toInt(&serialPortInfo.d_ptr->hasVendorIdentifier, 16);

                            serialPortInfo.d_ptr->productIdentifier =
                                    QString::fromLatin1(::udev_device_get_property_value(dev,
                                                "ID_MODEL_ID")).toInt(&serialPortInfo.d_ptr->hasProductIdentifier, 16);

                        } else if (subsys == QLatin1String("pnp")) { // PNP bus type
                            // Append this device.
                            // FIXME: How to get additional information about serial devices
                            // with this subsystem?
                        } else if (subsys == QLatin1String("platform")) { // Platform 'pseudo' bus for legacy device.
                            // Skip this devices because this type of subsystem does
                            // not include a real physical serial device.
                            canAppendToList = false;
                        } else { // Others types of subsystems.
                            // Append this devices because we believe that any other types of
                            // subsystems provide a real serial devices. For example, for devices
                            // such as ttyGSx, its driver provide an empty subsystem name, but it
                            // devices is a real physical serial devices.
                            // FIXME: How to get additional information about serial devices
                            // with this subsystems?
                        }
                    } else { // Devices without a parent
                        if (serialPortInfo.d_ptr->portName.startsWith(rfcommDeviceName)) { // Bluetooth device
                            bool ok;
                            // Check for an unsigned decimal integer at the end of the device name: "rfcomm0", "rfcomm15"
                            // devices with negative and invalid numbers in the name are rejected
                            int portNumber = serialPortInfo.d_ptr->portName.mid(rfcommDeviceName.length()).toInt(&ok);

                            if (!ok || (portNumber < 0) || (portNumber > 255)) {
                                canAppendToList = false;
                            }
                        } else {
                            canAppendToList = false;
                        }
                    }

                    if (canAppendToList)
                        serialPortInfoList.append(serialPortInfo);

                    ::udev_device_unref(dev);
                }

            }

            ::udev_enumerate_unref(enumerate);
        }

        ::udev_unref(udev);
    }

#elif defined (Q_OS_FREEBSD) && defined (HAVE_LIBUSB)
    // TODO: Implement me.
#else

    QDir devDir(QLatin1String("/dev"));
    if (devDir.exists()) {

        devDir.setNameFilters(filtersOfDevices());
        devDir.setFilter(QDir::Files | QDir::System | QDir::NoSymLinks);

        QStringList foundDevices; // Found devices list.

        foreach (const QFileInfo &deviceFileInfo, devDir.entryInfoList()) {
            QString deviceFilePath = deviceFileInfo.absoluteFilePath();
            if (!foundDevices.contains(deviceFilePath)) {
                foundDevices.append(deviceFilePath);

                QSerialPortInfo serialPortInfo;

                serialPortInfo.d_ptr->device = deviceFilePath;
                serialPortInfo.d_ptr->portName = QSerialPortPrivate::portNameFromSystemLocation(deviceFilePath);

                // Get description, manufacturer, vendor identifier, product
                // identifier are not supported.

                serialPortInfoList.append(serialPortInfo);

            }
        }
    }

#endif

    return serialPortInfoList;
}

#endif // Q_OS_MAC

// common part

QList<qint32> QSerialPortInfo::standardBaudRates()
{
    return QSerialPortPrivate::standardBaudRates();
}

bool QSerialPortInfo::isBusy() const
{
    bool currentPid = false;
    return QTtyLocker::isLocked(portName().toLocal8Bit().constData(), &currentPid);
}

bool QSerialPortInfo::isValid() const
{
    QFile f(systemLocation());
    return f.exists();
}

QT_END_NAMESPACE
