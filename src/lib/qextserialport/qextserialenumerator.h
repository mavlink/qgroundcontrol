/*!
 * \file qextserialenumerator.h
 * \author Michal Policht
 * \see QextSerialEnumerator
 */
 
#ifndef _QEXTSERIALENUMERATOR_H_
#define _QEXTSERIALENUMERATOR_H_


#include <QString>
#include <QList>

#ifdef _TTY_WIN_
	#include <windows.h>
	#include <setupapi.h>
#endif /*_TTY_WIN_*/


/*!
 * Structure containing port information.
 */
struct QextPortInfo {
	QString portName;		///< Port name.
	QString physName;		///< Physical name.
	QString friendName;		///< Friendly name.
	QString enumName;		///< Enumerator name.
};


/*!
 * Serial port enumerator. This class provides list of ports available in the system.
 * 
 * Windows implementation is based on Zach Gorman's work from 
 * <a href="http://www.codeproject.com">The Code Project</a> (http://www.codeproject.com/system/setupdi.asp).
 */
class QextSerialEnumerator
{
	private:
		#ifdef _TTY_WIN_
			/*!
			 * Get value of specified property from the registry.
			 * 	\param key handle to an open key.
			 * 	\param property property name.
			 * 	\return property value.
			 */
			static QString getRegKeyValue(HKEY key, LPCTSTR property);

			/*!
			 * Get specific property from registry.
			 * 	\param devInfo pointer to the device information set that contains the interface 
			 * 		and its underlying device. Returned by SetupDiGetClassDevs() function.
			 * 	\param devData pointer to an SP_DEVINFO_DATA structure that defines the device instance.
			 * 		this is returned by SetupDiGetDeviceInterfaceDetail() function. 
			 * 	\param property registry property. One of defined SPDRP_* constants. 
			 * 	\return property string.
			 */
			static QString getDeviceProperty(HDEVINFO devInfo, PSP_DEVINFO_DATA devData, DWORD property);

			/*!
			 * Search for serial ports using setupapi.
			 * 	\param infoList list with result.
			 */
			static void setupAPIScan(QList<QextPortInfo> & infoList);
		#endif /*_TTY_WIN_*/

	public:
		/*!
		 * Get list of ports.
		 * 	\return list of ports currently available in the system.
		 */
		static QList<QextPortInfo> getPorts();
};

#endif /*_QEXTSERIALENUMERATOR_H_*/
