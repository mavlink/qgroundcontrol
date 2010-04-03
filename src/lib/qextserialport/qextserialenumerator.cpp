/**
 * @file qextserialenumerator.cpp
 * @author Michał Policht
 * @see QextSerialEnumerator
 */
 
#include "qextserialenumerator.h"

#ifdef _TTY_WIN_

    #include <objbase.h>
    #include <initguid.h>


	//this is serial port GUID
	#ifndef GUID_CLASS_COMPORT
		DEFINE_GUID(GUID_CLASS_COMPORT, 0x86e0d1e0L, 0x8089, 0x11d0, 0x9c, 0xe4, 0x08, 0x00, 0x3e, 0x30, 0x1f, 0x73);
	#endif

	/* Gordon Schumacher's macros for TCHAR -> QString conversions and vice versa */	
	#ifdef UNICODE
		#define QStringToTCHAR(x)     (wchar_t*) x.utf16()
		#define PQStringToTCHAR(x)    (wchar_t*) x->utf16()
		#define TCHARToQString(x)     QString::fromUtf16((ushort*)(x))
		#define TCHARToQStringN(x,y)  QString::fromUtf16((ushort*)(x),(y))
	#else
		#define QStringToTCHAR(x)     x.local8Bit().constData()
		#define PQStringToTCHAR(x)    x->local8Bit().constData()
		#define TCHARToQString(x)     QString::fromLocal8Bit((x))
		#define TCHARToQStringN(x,y)  QString::fromLocal8Bit((x),(y))
	#endif /*UNICODE*/


	//static
	QString QextSerialEnumerator::getRegKeyValue(HKEY key, LPCTSTR property)
	{
		DWORD size = 0;
		RegQueryValueEx(key, property, NULL, NULL, NULL, & size);
		BYTE * buff = new BYTE[size];
		if (RegQueryValueEx(key, property, NULL, NULL, buff, & size) == ERROR_SUCCESS) {
			return TCHARToQStringN(buff, size);
			delete [] buff;
		} else {
			qWarning("QextSerialEnumerator::getRegKeyValue: can not obtain value from registry");
			delete [] buff;
			return QString();
		}
	}
	
	//static
	QString QextSerialEnumerator::getDeviceProperty(HDEVINFO devInfo, PSP_DEVINFO_DATA devData, DWORD property)
	{
		DWORD buffSize = 0;
		SetupDiGetDeviceRegistryProperty(devInfo, devData, property, NULL, NULL, 0, & buffSize);
		BYTE * buff = new BYTE[buffSize];
		if (!SetupDiGetDeviceRegistryProperty(devInfo, devData, property, NULL, buff, buffSize, NULL))
			qCritical("Can not obtain property: %ld from registry", property); 
		QString result = TCHARToQString(buff);
		delete [] buff;
		return result;
	}

	//static
	void QextSerialEnumerator::setupAPIScan(QList<QextPortInfo> & infoList)
	{
		HDEVINFO devInfo = INVALID_HANDLE_VALUE;
		GUID * guidDev = (GUID *) & GUID_CLASS_COMPORT;

		devInfo = SetupDiGetClassDevs(guidDev, NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
		if(devInfo == INVALID_HANDLE_VALUE) {
			qCritical("SetupDiGetClassDevs failed. Error code: %ld", GetLastError());
			return;
		}

		//enumerate the devices
		bool ok = true;
		SP_DEVICE_INTERFACE_DATA ifcData;
		ifcData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
		SP_DEVICE_INTERFACE_DETAIL_DATA * detData = NULL;
		DWORD detDataSize = 0;
		DWORD oldDetDataSize = 0;
		
		for (DWORD i = 0; ok; i++) {
			ok = SetupDiEnumDeviceInterfaces(devInfo, NULL, guidDev, i, &ifcData);
			if (ok) {
				SP_DEVINFO_DATA devData = {sizeof(SP_DEVINFO_DATA)};
				//check for required detData size
				SetupDiGetDeviceInterfaceDetail(devInfo, & ifcData, NULL, 0, & detDataSize, & devData);
				//if larger than old detData size then reallocate the buffer
				if (detDataSize > oldDetDataSize) {
					delete [] detData;
					detData = (SP_DEVICE_INTERFACE_DETAIL_DATA *) new char[detDataSize];
					detData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
					oldDetDataSize = detDataSize;
				}
				//check the details
				if (SetupDiGetDeviceInterfaceDetail(devInfo, & ifcData, detData, detDataSize, 
													NULL, & devData)) {
					// Got a device. Get the details.
					QextPortInfo info;
					info.friendName = getDeviceProperty(devInfo, & devData, SPDRP_FRIENDLYNAME);
					info.physName = getDeviceProperty(devInfo, & devData, SPDRP_PHYSICAL_DEVICE_OBJECT_NAME);
					info.enumName = getDeviceProperty(devInfo, & devData, SPDRP_ENUMERATOR_NAME);
					//anyway, to get the port name we must still open registry directly :( ??? 
					//Eh...			
					HKEY devKey = SetupDiOpenDevRegKey(devInfo, & devData, DICS_FLAG_GLOBAL, 0,
														DIREG_DEV, KEY_READ);
					info.portName = getRegKeyValue(devKey, TEXT("PortName"));
					RegCloseKey(devKey);
					infoList.append(info);
				} else {
					qCritical("SetupDiGetDeviceInterfaceDetail failed. Error code: %ld", GetLastError());
					delete [] detData;
					return;
				}
			} else {
				if (GetLastError() != ERROR_NO_MORE_ITEMS) {
					delete [] detData;
					qCritical("SetupDiEnumDeviceInterfaces failed. Error code: %ld", GetLastError());
					return;
				}
			}
		}
		delete [] detData;
	}

#endif /*_TTY_WIN_*/


//static
QList<QextPortInfo> QextSerialEnumerator::getPorts()
{
	QList<QextPortInfo> ports;

	#ifdef _TTY_WIN_
		OSVERSIONINFO vi;
		vi.dwOSVersionInfoSize = sizeof(vi);
		if (!::GetVersionEx(&vi)) {
			qCritical("Could not get OS version.");
			return ports;
		}
		// Handle windows 9x and NT4 specially
		if (vi.dwMajorVersion < 5) {
			qCritical("Enumeration for this version of Windows is not implemented yet");
/*			if (vi.dwPlatformId == VER_PLATFORM_WIN32_NT)
				EnumPortsWNt4(ports);
			else
				EnumPortsW9x(ports);*/
		} else	//w2k or later
			setupAPIScan(ports);
	#endif /*_TTY_WIN_*/
	#ifdef _TTY_POSIX_
		qCritical("Enumeration for POSIX systems is not implemented yet.");
	#endif /*_TTY_POSIX_*/
	
	return ports;
}
