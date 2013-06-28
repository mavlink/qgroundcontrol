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

#include <sys/param.h>

#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>

#include <IOKit/serial/IOSerialKeys.h>
#include <IOKit/storage/IOStorageDeviceCharacteristics.h> // for kIOPropertyProductNameKey
#include <IOKit/usb/USB.h>
#if defined(MAC_OS_X_VERSION_10_4) && (MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_4)
#  include <IOKit/serial/ioss.h>
#endif
#include <IOKit/IOBSD.h>

QT_BEGIN_NAMESPACE

enum { MATCHING_PROPERTIES_COUNT = 6 };

QList<QSerialPortInfo> QSerialPortInfo::availablePorts()
{
    QList<QSerialPortInfo> serialPortInfoList;

    int matchingPropertiesCounter = 0;


    ::CFMutableDictionaryRef matching = ::IOServiceMatching(kIOSerialBSDServiceValue);
    if (!matching)
        return serialPortInfoList;

    ::CFDictionaryAddValue(matching,
                           CFSTR(kIOSerialBSDTypeKey),
                           CFSTR(kIOSerialBSDAllTypes));

    io_iterator_t iter = 0;
    kern_return_t kr = ::IOServiceGetMatchingServices(kIOMasterPortDefault,
                                                      matching,
                                                      &iter);

    if (kr != kIOReturnSuccess)
        return serialPortInfoList;

    io_registry_entry_t service;

    while ((service = ::IOIteratorNext(iter))) {

        ::CFTypeRef device = 0;
        ::CFTypeRef portName = 0;
        ::CFTypeRef description = 0;
        ::CFTypeRef manufacturer = 0;
        ::CFTypeRef vendorIdentifier = 0;
        ::CFTypeRef productIdentifier = 0;

        io_registry_entry_t entry = service;

        // Find MacOSX-specific properties names.
        do {

            if (!device) {
                device =
                        ::IORegistryEntrySearchCFProperty(entry,
                                                          kIOServicePlane,
                                                          CFSTR(kIOCalloutDeviceKey),
                                                          kCFAllocatorDefault,
                                                          0);
                if (device)
                    ++matchingPropertiesCounter;
            }

            if (!portName) {
                portName =
                        ::IORegistryEntrySearchCFProperty(entry,
                                                          kIOServicePlane,
                                                          CFSTR(kIOTTYDeviceKey),
                                                          kCFAllocatorDefault,
                                                          0);
                if (portName)
                    ++matchingPropertiesCounter;
            }

            if (!description) {
                description =
                        ::IORegistryEntrySearchCFProperty(entry,
                                                          kIOServicePlane,
                                                          CFSTR(kIOPropertyProductNameKey),
                                                          kCFAllocatorDefault,
                                                          0);
                if (!description)
                    description =
                            ::IORegistryEntrySearchCFProperty(entry,
                                                              kIOServicePlane,
                                                              CFSTR(kUSBProductString),
                                                              kCFAllocatorDefault,
                                                              0);
                if (!description)
                    description =
                            ::IORegistryEntrySearchCFProperty(entry,
                                                              kIOServicePlane,
                                                              CFSTR("BTName"),
                                                              kCFAllocatorDefault,
                                                              0);

                if (description)
                    ++matchingPropertiesCounter;
            }

            if (!manufacturer) {
                manufacturer =
                        ::IORegistryEntrySearchCFProperty(entry,
                                                          kIOServicePlane,
                                                          CFSTR(kUSBVendorString),
                                                          kCFAllocatorDefault,
                                                          0);
                if (manufacturer)
                    ++matchingPropertiesCounter;

            }

            if (!vendorIdentifier) {
                vendorIdentifier =
                        ::IORegistryEntrySearchCFProperty(entry,
                                                          kIOServicePlane,
                                                          CFSTR(kUSBVendorID),
                                                          kCFAllocatorDefault,
                                                          0);
                if (vendorIdentifier)
                    ++matchingPropertiesCounter;

            }

            if (!productIdentifier) {
                productIdentifier =
                        ::IORegistryEntrySearchCFProperty(entry,
                                                          kIOServicePlane,
                                                          CFSTR(kUSBProductID),
                                                          kCFAllocatorDefault,
                                                          0);
                if (productIdentifier)
                    ++matchingPropertiesCounter;

            }

            // If all matching properties is found, then force break loop.
            if (matchingPropertiesCounter == MATCHING_PROPERTIES_COUNT)
                break;

            kr = ::IORegistryEntryGetParentEntry(entry, kIOServicePlane, &entry);

        } while (kr == kIOReturnSuccess);

        (void) ::IOObjectRelease(entry);

        // Convert from MacOSX-specific properties to Qt4-specific.
        if (matchingPropertiesCounter > 0) {

            QSerialPortInfo serialPortInfo;
            QByteArray buffer(MAXPATHLEN, 0);

            if (device) {
                if (::CFStringGetCString(CFStringRef(device),
                                         buffer.data(),
                                         buffer.size(),
                                         kCFStringEncodingUTF8)) {
                    serialPortInfo.d_ptr->device = QString::fromUtf8(buffer);
                }
                ::CFRelease(device);
            }

            if (portName) {
                if (::CFStringGetCString(CFStringRef(portName),
                                         buffer.data(),
                                         buffer.size(),
                                         kCFStringEncodingUTF8)) {
                    serialPortInfo.d_ptr->portName = QString::fromUtf8(buffer);
                }
                ::CFRelease(portName);
            }

            if (description) {
                if (::CFStringGetCString(CFStringRef(description),
                                         buffer.data(),
                                         buffer.size(),
                                         kCFStringEncodingUTF8)) {
                    serialPortInfo.d_ptr->description = QString::fromUtf8(buffer);
                }
                ::CFRelease(description);
            }

            if (manufacturer) {
                if (::CFStringGetCString(CFStringRef(manufacturer),
                                         buffer.data(),
                                         buffer.size(),
                                         kCFStringEncodingUTF8)) {
                    serialPortInfo.d_ptr->manufacturer = QString::fromUtf8(buffer);
                }
                ::CFRelease(manufacturer);
            }

            quint16 value = 0;

            if (vendorIdentifier) {
                serialPortInfo.d_ptr->hasVendorIdentifier = ::CFNumberGetValue(CFNumberRef(vendorIdentifier), kCFNumberIntType, &value);
                if (serialPortInfo.d_ptr->hasVendorIdentifier)
                    serialPortInfo.d_ptr->vendorIdentifier = value;

                ::CFRelease(vendorIdentifier);
            }

            if (productIdentifier) {
                serialPortInfo.d_ptr->hasProductIdentifier = ::CFNumberGetValue(CFNumberRef(productIdentifier), kCFNumberIntType, &value);
                if (serialPortInfo.d_ptr->hasProductIdentifier)
                    serialPortInfo.d_ptr->productIdentifier = value;

                ::CFRelease(productIdentifier);
            }

            serialPortInfoList.append(serialPortInfo);
        }

        (void) ::IOObjectRelease(service);
    }

    (void) ::IOObjectRelease(iter);

    return serialPortInfoList;
}

QT_END_NAMESPACE
