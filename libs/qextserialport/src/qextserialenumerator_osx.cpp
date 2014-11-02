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
#include <IOKit/serial/IOSerialKeys.h>
#include <CoreFoundation/CFNumber.h>
#include <sys/param.h>

void QextSerialEnumeratorPrivate::platformSpecificInit()
{
}

void QextSerialEnumeratorPrivate::platformSpecificDestruct()
{
    IONotificationPortDestroy(notificationPortRef);
}

// static
QList<QextPortInfo> QextSerialEnumeratorPrivate::getPorts_sys()
{
    QList<QextPortInfo> infoList;
    io_iterator_t serialPortIterator = 0;
    kern_return_t kernResult = KERN_FAILURE;
    CFMutableDictionaryRef matchingDictionary;

    // first try to get any serialbsd devices, then try any USBCDC devices
    if (!(matchingDictionary = IOServiceMatching(kIOSerialBSDServiceValue))) {
        QESP_WARNING("IOServiceMatching returned a NULL dictionary.");
        return infoList;
    }
    CFDictionaryAddValue(matchingDictionary, CFSTR(kIOSerialBSDTypeKey), CFSTR(kIOSerialBSDAllTypes));

    // then create the iterator with all the matching devices
    if (IOServiceGetMatchingServices(kIOMasterPortDefault, matchingDictionary, &serialPortIterator) != KERN_SUCCESS) {
        qCritical() << "IOServiceGetMatchingServices failed, returned" << kernResult;
        return infoList;
    }
    iterateServicesOSX(serialPortIterator, infoList);
    IOObjectRelease(serialPortIterator);
    serialPortIterator = 0;

    if (!(matchingDictionary = IOServiceNameMatching("AppleUSBCDC"))) {
        QESP_WARNING("IOServiceNameMatching returned a NULL dictionary.");
        return infoList;
    }

    if (IOServiceGetMatchingServices(kIOMasterPortDefault, matchingDictionary, &serialPortIterator) != KERN_SUCCESS) {
        qCritical() << "IOServiceGetMatchingServices failed, returned" << kernResult;
        return infoList;
    }
    iterateServicesOSX(serialPortIterator, infoList);
    IOObjectRelease(serialPortIterator);

    return infoList;
}

void QextSerialEnumeratorPrivate::iterateServicesOSX(io_object_t service, QList<QextPortInfo> &infoList)
{
    // Iterate through all modems found.
    io_object_t usbService;
    while ((usbService = IOIteratorNext(service))) {
        QextPortInfo info;
        info.vendorID = 0;
        info.productID = 0;
        getServiceDetailsOSX(usbService, &info);
        infoList.append(info);
    }
}

bool QextSerialEnumeratorPrivate::getServiceDetailsOSX(io_object_t service, QextPortInfo *portInfo)
{
    bool retval = true;
    CFTypeRef bsdPathAsCFString = NULL;
    CFTypeRef productNameAsCFString = NULL;
    CFTypeRef vendorIdAsCFNumber = NULL;
    CFTypeRef productIdAsCFNumber = NULL;
    // check the name of the modem's callout device
    bsdPathAsCFString = IORegistryEntryCreateCFProperty(service, CFSTR(kIOCalloutDeviceKey),
                                                        kCFAllocatorDefault, 0);

    // wander up the hierarchy until we find the level that can give us the
    // vendor/product IDs and the product name, if available
    io_registry_entry_t parent;
    kern_return_t kernResult = IORegistryEntryGetParentEntry(service, kIOServicePlane, &parent);
    while (kernResult == KERN_SUCCESS && !vendorIdAsCFNumber && !productIdAsCFNumber) {
        if (!productNameAsCFString)
            productNameAsCFString = IORegistryEntrySearchCFProperty(parent,
                                                                    kIOServicePlane,
                                                                    CFSTR("Product Name"),
                                                                    kCFAllocatorDefault, 0);
        vendorIdAsCFNumber = IORegistryEntrySearchCFProperty(parent,
                                                             kIOServicePlane,
                                                             CFSTR(kUSBVendorID),
                                                             kCFAllocatorDefault, 0);
        productIdAsCFNumber = IORegistryEntrySearchCFProperty(parent,
                                                              kIOServicePlane,
                                                              CFSTR(kUSBProductID),
                                                              kCFAllocatorDefault, 0);
        io_registry_entry_t oldparent = parent;
        kernResult = IORegistryEntryGetParentEntry(parent, kIOServicePlane, &parent);
        IOObjectRelease(oldparent);
    }

    io_string_t ioPathName;
    IORegistryEntryGetPath(service, kIOServicePlane, ioPathName);
    portInfo->physName = ioPathName;

    if (bsdPathAsCFString) {
        char path[MAXPATHLEN];
        if (CFStringGetCString((CFStringRef)bsdPathAsCFString, path,
                               PATH_MAX, kCFStringEncodingUTF8))
            portInfo->portName = path;
        CFRelease(bsdPathAsCFString);
    }

    if (productNameAsCFString) {
        char productName[MAXPATHLEN];
        if (CFStringGetCString((CFStringRef)productNameAsCFString, productName,
                               PATH_MAX, kCFStringEncodingUTF8))
            portInfo->friendName = productName;
        CFRelease(productNameAsCFString);
    }

    if (vendorIdAsCFNumber) {
        SInt32 vID;
        if (CFNumberGetValue((CFNumberRef)vendorIdAsCFNumber, kCFNumberSInt32Type, &vID))
            portInfo->vendorID = vID;
        CFRelease(vendorIdAsCFNumber);
    }

    if (productIdAsCFNumber) {
        SInt32 pID;
        if (CFNumberGetValue((CFNumberRef)productIdAsCFNumber, kCFNumberSInt32Type, &pID))
            portInfo->productID = pID;
        CFRelease(productIdAsCFNumber);
    }
    IOObjectRelease(service);
    return retval;
}

// IOKit callbacks registered via setupNotifications()
void deviceDiscoveredCallbackOSX(void *ctxt, io_iterator_t serialPortIterator)
{
    QextSerialEnumeratorPrivate *d = (QextSerialEnumeratorPrivate *)ctxt;
    io_object_t serialService;
    while ((serialService = IOIteratorNext(serialPortIterator)))
        d->onDeviceDiscoveredOSX(serialService);
}

void deviceTerminatedCallbackOSX(void *ctxt, io_iterator_t serialPortIterator)
{
    QextSerialEnumeratorPrivate *d = (QextSerialEnumeratorPrivate *)ctxt;
    io_object_t serialService;
    while ((serialService = IOIteratorNext(serialPortIterator)))
        d->onDeviceTerminatedOSX(serialService);
}

/*
  A device has been discovered via IOKit.
  Create a QextPortInfo if possible, and emit the signal indicating that we've found it.
*/
void QextSerialEnumeratorPrivate::onDeviceDiscoveredOSX(io_object_t service)
{
    Q_Q(QextSerialEnumerator);
    QextPortInfo info;
    info.vendorID = 0;
    info.productID = 0;
    if (getServiceDetailsOSX(service, &info))
        Q_EMIT q->deviceDiscovered(info);
}

/*
  Notification via IOKit that a device has been removed.
  Create a QextPortInfo if possible, and emit the signal indicating that it's gone.
*/
void QextSerialEnumeratorPrivate::onDeviceTerminatedOSX(io_object_t service)
{
    Q_Q(QextSerialEnumerator);
    QextPortInfo info;
    info.vendorID = 0;
    info.productID = 0;
    if (getServiceDetailsOSX(service, &info))
        Q_EMIT q->deviceRemoved(info);
}

/*
  Create matching dictionaries for the devices we want to get notifications for,
  and add them to the current run loop.  Invoke the callbacks that will be responding
  to these notifications once to arm them, and discover any devices that
  are currently connected at the time notifications are setup.
*/
bool QextSerialEnumeratorPrivate::setUpNotifications_sys(bool /*setup*/)
{
    kern_return_t kernResult;
    mach_port_t masterPort;
    CFRunLoopSourceRef notificationRunLoopSource;
    CFMutableDictionaryRef classesToMatch;
    CFMutableDictionaryRef cdcClassesToMatch;
    io_iterator_t portIterator;

    kernResult = IOMasterPort(MACH_PORT_NULL, &masterPort);
    if (KERN_SUCCESS != kernResult) {
        qDebug() << "IOMasterPort returned:" << kernResult;
        return false;
    }

    classesToMatch = IOServiceMatching(kIOSerialBSDServiceValue);
    if (classesToMatch == NULL)
        qDebug("IOServiceMatching returned a NULL dictionary.");
    else
        CFDictionarySetValue(classesToMatch, CFSTR(kIOSerialBSDTypeKey), CFSTR(kIOSerialBSDAllTypes));

    if (!(cdcClassesToMatch = IOServiceNameMatching("AppleUSBCDC"))) {
        QESP_WARNING("couldn't create cdc matching dict");
        return false;
    }

    // Retain an additional reference since each call to IOServiceAddMatchingNotification consumes one.
    classesToMatch = (CFMutableDictionaryRef) CFRetain(classesToMatch);
    cdcClassesToMatch = (CFMutableDictionaryRef) CFRetain(cdcClassesToMatch);

    notificationPortRef = IONotificationPortCreate(masterPort);
    if (notificationPortRef == NULL) {
        qDebug("IONotificationPortCreate return a NULL IONotificationPortRef.");
        return false;
    }

    notificationRunLoopSource = IONotificationPortGetRunLoopSource(notificationPortRef);
    if (notificationRunLoopSource == NULL) {
        qDebug("IONotificationPortGetRunLoopSource returned NULL CFRunLoopSourceRef.");
        return false;
    }

    CFRunLoopAddSource(CFRunLoopGetCurrent(), notificationRunLoopSource, kCFRunLoopDefaultMode);

    kernResult = IOServiceAddMatchingNotification(notificationPortRef, kIOMatchedNotification, classesToMatch,
                                                  deviceDiscoveredCallbackOSX, this, &portIterator);
    if (kernResult != KERN_SUCCESS) {
        qDebug() << "IOServiceAddMatchingNotification return:" << kernResult;
        return false;
    }

    // arm the callback, and grab any devices that are already connected
    deviceDiscoveredCallbackOSX(this, portIterator);

    kernResult = IOServiceAddMatchingNotification(notificationPortRef, kIOMatchedNotification, cdcClassesToMatch,
                                                  deviceDiscoveredCallbackOSX, this, &portIterator);
    if (kernResult != KERN_SUCCESS) {
        qDebug() << "IOServiceAddMatchingNotification return:" << kernResult;
        return false;
    }

    // arm the callback, and grab any devices that are already connected
    deviceDiscoveredCallbackOSX(this, portIterator);

    kernResult = IOServiceAddMatchingNotification(notificationPortRef, kIOTerminatedNotification, classesToMatch,
                                                  deviceTerminatedCallbackOSX, this, &portIterator);
    if (kernResult != KERN_SUCCESS) {
        qDebug() << "IOServiceAddMatchingNotification return:" << kernResult;
        return false;
    }

    // arm the callback, and clear any devices that are terminated
    deviceTerminatedCallbackOSX(this, portIterator);

    kernResult = IOServiceAddMatchingNotification(notificationPortRef, kIOTerminatedNotification, cdcClassesToMatch,
                                                  deviceTerminatedCallbackOSX, this, &portIterator);
    if (kernResult != KERN_SUCCESS) {
        qDebug() << "IOServiceAddMatchingNotification return:" << kernResult;
        return false;
    }

    // arm the callback, and clear any devices that are terminated
    deviceTerminatedCallbackOSX(this, portIterator);
    return true;
}

