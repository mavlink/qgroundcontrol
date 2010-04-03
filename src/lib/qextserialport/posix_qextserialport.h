#ifndef _POSIX_QEXTSERIALPORT_H_
#define _POSIX_QEXTSERIALPORT_H_

#include <stdio.h>
#include <termios.h>
#include <errno.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include "qextserialbase.h"

class Posix_QextSerialPort:public QextSerialBase 
{
	private:
	    /*!
	     * This method is a part of constructor.
	     */
	    void init();

	protected:
	    int fd;
	    struct termios Posix_CommConfig;
	    struct termios old_termios;
	    struct timeval Posix_Timeout;
	    struct timeval Posix_Copy_Timeout;
	
	    virtual qint64 readData(char * data, qint64 maxSize);
	    virtual qint64 writeData(const char * data, qint64 maxSize);

	public:
	    Posix_QextSerialPort(QextSerialBase::QueryMode mode);
	    Posix_QextSerialPort(const Posix_QextSerialPort& s);
	    Posix_QextSerialPort(const QString & name, QextSerialBase::QueryMode mode);
	    Posix_QextSerialPort(const PortSettings& settings, QextSerialBase::QueryMode mode);
	    Posix_QextSerialPort(const QString & name, const PortSettings& settings, QextSerialBase::QueryMode mode);
	    Posix_QextSerialPort& operator=(const Posix_QextSerialPort& s);
	    virtual ~Posix_QextSerialPort();
	
	    virtual void setBaudRate(BaudRateType);
	    virtual void setDataBits(DataBitsType);
	    virtual void setParity(ParityType);
	    virtual void setStopBits(StopBitsType);
	    virtual void setFlowControl(FlowType);
	    virtual void setTimeout(long);

	    virtual bool open(OpenMode mode);
	    virtual void close();
	    virtual void flush();
	
	    virtual qint64 size() const;
	    virtual qint64 bytesAvailable() const;
	
	    virtual void ungetChar(char c);
	
	    virtual void translateError(ulong error);
	
	    virtual void setDtr(bool set=true);
	    virtual void setRts(bool set=true);
	    virtual ulong lineStatus();

};

#endif
