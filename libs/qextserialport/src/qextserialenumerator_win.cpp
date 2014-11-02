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
#include <objbase.h>
#include <initguid.h>
#include <setupapi.h>
#include <dbt.h>
#include "qextserialport.h"

#ifdef QT_GUI_LIB
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
#include <QtGui/QWidget>
class QextSerialRegistrationWidget : public QWidget
#else
#include <QtGui/QWindow>
class QextSerialRegistrationWidget : public QWindow
#endif
{
public:
    QextSerialRegistrationWidget(QextSerialEnumeratorPrivate *qese) {
        this->qese = qese;
    }
    ~QextSerialRegistrationWidget() {}

protected:

#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
    bool winEvent(MSG *message, long *result) {
#else
    bool nativeEvent(const QByteArray & /*eventType*/, void *msg, long *result) {
        MSG *message = static_cast<MSG *>(msg);
#endif
        if (message->message == WM_DEVICECHANGE) {
            qese->onDeviceChanged(message->wParam, message->lParam);
            *result = 1;
            return true;
        }
        return false;
    }
private:
    QextSerialEnumeratorPrivate *qese;
};

#endif // QT_GUI_LIB

void QextSerialEnumeratorPrivate::platformSpecificInit()
{
#ifdef QT_GUI_LIB
    notificationWidget = 0;
#endif // QT_GUI_LIB
}

/*!
  default
*/
void QextSerialEnumeratorPrivate::platformSpecificDestruct()
{
#ifdef QT_GUI_LIB
    if (notificationWidget)
        delete notificationWidget;
#endif
}

// see http://msdn.microsoft.com/en-us/library/windows/hardware/ff553426(v=vs.85).aspx
// for list of GUID classes
const GUID deviceClassGuids[] =
{
    // Ports (COM & LPT ports), Class = Ports
    {0x4D36E978, 0xE325, 0x11CE, {0xBF, 0xC1, 0x08, 0x00, 0x2B, 0xE1, 0x03, 0x18}},
    // Modem, Class = Modem
    {0x4D36E96D, 0xE325, 0x11CE, {0xBF, 0xC1, 0x08, 0x00, 0x2B, 0xE1, 0x03, 0x18}},
    // Bluetooth Devices, Class = Bluetooth
    {0xE0CBF06C, 0xCD8B, 0x4647, {0xBB, 0x8A, 0x26, 0x3B, 0x43, 0xF0, 0xF9, 0x74}},
    // Added by Arne Kristian Jansen, for use with com0com virtual ports (See Issue 54)
    {0xDF799E12, 0x3C56, 0x421B, {0xB2, 0x98, 0xB6, 0xD3, 0x64, 0x2B, 0xC8, 0x78}}
};

/* Gordon Schumacher's macros for TCHAR -> QString conversions and vice versa */
#ifdef UNICODE
    #define QStringToTCHAR(x)     (wchar_t *) x.utf16()
    #define PQStringToTCHAR(x)    (wchar_t *) x->utf16()
    #define TCHARToQString(x)     QString::fromUtf16((ushort *)(x))
    #define TCHARToQStringN(x,y)  QString::fromUtf16((ushort *)(x),(y))
#else
    #define QStringToTCHAR(x)     x.local8Bit().constData()
    #define PQStringToTCHAR(x)    x->local8Bit().constData()
    #define TCHARToQString(x)     QString::fromLocal8Bit((char *)(x))
    #define TCHARToQStringN(x,y)  QString::fromLocal8Bit((char *)(x),(y))
#endif /*UNICODE*/

/*!
    \internal
    Get value of specified property from the registry.
        \a key handle to an open key.
        \a property property name.

        return property value.
*/
static QString getRegKeyValue(HKEY key, LPCTSTR property)
{
    DWORD size = 0;
    DWORD type;
    ::RegQueryValueEx(key, property, NULL, NULL, NULL, &size);
    BYTE *buff = new BYTE[size];
    QString result;
    if (::RegQueryValueEx(key, property, NULL, &type, buff, &size) == ERROR_SUCCESS)
        result = TCHARToQString(buff);
    ::RegCloseKey(key);
    delete [] buff;
    return result;
}

/*!
     \internal
     Get specific property from registry.
     \a devInfo pointer to the device information set that contains the interface
        and its underlying device. Returned by SetupDiGetClassDevs() function.
     \a devData pointer to an SP_DEVINFO_DATA structure that defines the device instance.
        this is returned by SetupDiGetDeviceInterfaceDetail() function.
     \a property registry property. One of defined SPDRP_* constants.

     return property string.
 */
static QString getDeviceProperty(HDEVINFO devInfo, PSP_DEVINFO_DATA devData, DWORD property)
{
    DWORD buffSize = 0;
    ::SetupDiGetDeviceRegistryProperty(devInfo, devData, property, NULL, NULL, 0, &buffSize);
    BYTE *buff = new BYTE[buffSize];
    ::SetupDiGetDeviceRegistryProperty(devInfo, devData, property, NULL, buff, buffSize, NULL);
    QString result = TCHARToQString(buff);
    delete [] buff;
    return result;
}

/*!
     \internal
*/
static bool getDeviceDetailsWin(QextPortInfo *portInfo, HDEVINFO devInfo, PSP_DEVINFO_DATA devData
                                 , WPARAM wParam = DBT_DEVICEARRIVAL)
{
    portInfo->friendName = getDeviceProperty(devInfo, devData, SPDRP_FRIENDLYNAME);
    if (wParam == DBT_DEVICEARRIVAL)
        portInfo->physName = getDeviceProperty(devInfo, devData, SPDRP_PHYSICAL_DEVICE_OBJECT_NAME);
    portInfo->enumName = getDeviceProperty(devInfo, devData, SPDRP_ENUMERATOR_NAME);
    QString hardwareIDs = getDeviceProperty(devInfo, devData, SPDRP_HARDWAREID);
    HKEY devKey = ::SetupDiOpenDevRegKey(devInfo, devData, DICS_FLAG_GLOBAL, 0, DIREG_DEV, KEY_QUERY_VALUE);
    portInfo->portName = getRegKeyValue(devKey, TEXT("PortName"));
    QRegExp idRx(QLatin1String("VID_(\\w+)&PID_(\\w+)"));
    if (hardwareIDs.toUpper().contains(idRx)) {
        bool dummy;
        portInfo->vendorID = idRx.cap(1).toInt(&dummy, 16);
        portInfo->productID = idRx.cap(2).toInt(&dummy, 16);
        //qDebug() << "got vid:" << vid << "pid:" << pid;
    }
    return true;
}

/*!
     \internal
*/
static void enumerateDevicesWin(const GUID &guid, QList<QextPortInfo> *infoList)
{
    HDEVINFO devInfo;
    if ((devInfo = ::SetupDiGetClassDevs(&guid, NULL, NULL, DIGCF_PRESENT)) != INVALID_HANDLE_VALUE) {
        SP_DEVINFO_DATA devInfoData;
        devInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
        for(int i = 0; ::SetupDiEnumDeviceInfo(devInfo, i, &devInfoData); i++) {
            QextPortInfo info;
            info.productID = info.vendorID = 0;
            getDeviceDetailsWin(&info, devInfo, &devInfoData);
            if (!info.portName.startsWith(QLatin1String("LPT"), Qt::CaseInsensitive))
                infoList->append(info);
        }
        ::SetupDiDestroyDeviceInfoList(devInfo);
    }
}


static bool lessThan(const QextPortInfo &s1, const QextPortInfo &s2)
{
    if (s1.portName.startsWith(QLatin1String("COM"))
            && s2.portName.startsWith(QLatin1String("COM"))) {
        return s1.portName.mid(3).toInt()<s2.portName.mid(3).toInt();
    }
    return s1.portName < s2.portName;
}


/*!
    Get list of ports.

    return list of ports currently available in the system.
*/
QList<QextPortInfo> QextSerialEnumeratorPrivate::getPorts_sys()
{
    QList<QextPortInfo> ports;
    const int count = sizeof(deviceClassGuids)/sizeof(deviceClassGuids[0]);
    for (int i=0; i<count; ++i)
        enumerateDevicesWin(deviceClassGuids[i], &ports);
    qSort(ports.begin(), ports.end(), lessThan);
    return ports;
}


/*
    Enable event-driven notifications of board discovery/removal.
*/
bool QextSerialEnumeratorPrivate::setUpNotifications_sys(bool setup)
{
#ifndef QT_GUI_LIB
    Q_UNUSED(setup)
    QESP_WARNING("QextSerialEnumerator: GUI not enabled - can't register for device notifications.");
    return false;
#else
    Q_Q(QextSerialEnumerator);
    if (setup && notificationWidget) //already setup
        return true;
    notificationWidget = new QextSerialRegistrationWidget(this);

    DEV_BROADCAST_DEVICEINTERFACE dbh;
    ::ZeroMemory(&dbh, sizeof(dbh));
    dbh.dbcc_size = sizeof(dbh);
    dbh.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
    // dbh.dbcc_classguid = GUID_DEVCLASS_PORTS; //Ignored in such case
    DWORD flags = DEVICE_NOTIFY_WINDOW_HANDLE|DEVICE_NOTIFY_ALL_INTERFACE_CLASSES;
    if (::RegisterDeviceNotification((HWND)notificationWidget->winId(), &dbh, flags) == NULL) {
        QESP_WARNING() << "RegisterDeviceNotification failed:" << GetLastError();
        return false;
    }
    // setting up notifications doesn't tell us about devices already connected
    // so get those manually
    foreach (QextPortInfo port, getPorts_sys())
      Q_EMIT q->deviceDiscovered(port);
    return true;
#endif // QT_GUI_LIB
}

LRESULT QextSerialEnumeratorPrivate::onDeviceChanged(WPARAM wParam, LPARAM lParam)
{
    if (DBT_DEVICEARRIVAL == wParam || DBT_DEVICEREMOVECOMPLETE == wParam) {
        PDEV_BROADCAST_HDR pHdr = (PDEV_BROADCAST_HDR)lParam;
        if (pHdr->dbch_devicetype == DBT_DEVTYP_DEVICEINTERFACE) {
            PDEV_BROADCAST_DEVICEINTERFACE pDevInf = (PDEV_BROADCAST_DEVICEINTERFACE)pHdr;
             // delimiters are different across APIs...change to backslash.  ugh.
            QString deviceID = TCHARToQString(pDevInf->dbcc_name).toUpper().replace(QLatin1String("#"), QLatin1String("\\"));

            const int count = sizeof(deviceClassGuids)/sizeof(deviceClassGuids[0]);
            for (int i=0; i<count; ++i) {
                if (matchAndDispatchChangedDevice(deviceID, deviceClassGuids[i], wParam))
                    break;
            }
        }
    }
    return 0;
}

bool QextSerialEnumeratorPrivate::matchAndDispatchChangedDevice(const QString &deviceID, const GUID &guid, WPARAM wParam)
{
    Q_Q(QextSerialEnumerator);
    bool rv = false;
    DWORD dwFlag = (DBT_DEVICEARRIVAL == wParam) ? DIGCF_PRESENT : DIGCF_ALLCLASSES;
    HDEVINFO devInfo;
    if ((devInfo = SetupDiGetClassDevs(&guid,NULL,NULL,dwFlag)) != INVALID_HANDLE_VALUE) {
        SP_DEVINFO_DATA spDevInfoData;
        spDevInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
        for(int i=0; SetupDiEnumDeviceInfo(devInfo, i, &spDevInfoData); i++) {
            DWORD nSize = 0;
            TCHAR buf[MAX_PATH];
            if (SetupDiGetDeviceInstanceId(devInfo, &spDevInfoData, buf, MAX_PATH, &nSize) &&
                    deviceID.contains(TCHARToQString(buf))) { // we found a match
                rv = true;
                QextPortInfo info;
                info.productID = info.vendorID = 0;
                getDeviceDetailsWin(&info, devInfo, &spDevInfoData, wParam);
                if (wParam == DBT_DEVICEARRIVAL)
                    Q_EMIT q->deviceDiscovered(info);
                else if (wParam == DBT_DEVICEREMOVECOMPLETE)
                    Q_EMIT q->deviceRemoved(info);
                break;
            }
        }
        SetupDiDestroyDeviceInfoList(devInfo);
    }
    return rv;
}
