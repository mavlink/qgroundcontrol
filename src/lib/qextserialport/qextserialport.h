
#ifndef _QEXTSERIALPORT_H_
#define _QEXTSERIALPORT_H_

/*POSIX CODE*/
#ifdef _TTY_POSIX_
#include "posix_qextserialport.h"
#define QextBaseType Posix_QextSerialPort

/*MS WINDOWS CODE*/
#else
#include "win_qextserialport.h"
#define QextBaseType Win_QextSerialPort
#endif

class QextSerialPort: public QextBaseType 
{
	Q_OBJECT
	
	public:
		typedef QextSerialBase::QueryMode QueryMode;

	    QextSerialPort(QueryMode mode = QextSerialPort::EventDriven);
	    QextSerialPort(const QString & name, QueryMode mode = QextSerialPort::EventDriven);
	    QextSerialPort(PortSettings const& s, QueryMode mode = QextSerialPort::EventDriven);
	    QextSerialPort(const QString & name, PortSettings const& s, QueryMode mode = QextSerialPort::EventDriven);
	    QextSerialPort(const QextSerialPort& s);
	    QextSerialPort& operator=(const QextSerialPort&);
	    virtual ~QextSerialPort();

};

#endif
