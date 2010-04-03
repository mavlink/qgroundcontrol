#ifndef _WIN_QEXTSERIALPORT_H_
#define _WIN_QEXTSERIALPORT_H_

#include "qextserialbase.h"
#include <windows.h>
#include <QThread>


/*if all warning messages are turned off, flag portability warnings to be turned off as well*/
#ifdef _TTY_NOWARN_
#define _TTY_NOWARN_PORT_
#endif

class QReadWriteLock;
class Win_QextSerialThread;


/*!
\author Stefan Sander
\author Michal Policht

A cross-platform serial port class.
This class encapsulates the Windows portion of QextSerialPort.  The user will be notified of
errors and possible portability conflicts at run-time by default - this behavior can be turned
off by defining _TTY_NOWARN_ (to turn off all warnings) or _TTY_NOWARN_PORT_ (to turn off
portability warnings) in the project.  Note that defining _TTY_NOWARN_ also defines
_TTY_NOWARN_PORT_.

\note
On Windows NT/2000/XP this class uses Win32 serial port functions by default.  The user may
select POSIX behavior under NT, 2000, or XP ONLY by defining _TTY_POSIX_ in the project. I can
make no guarantees as to the quality of POSIX support under NT/2000 however.

\todo remove copy constructor and assign operator.
*/
class Win_QextSerialPort: public QextSerialBase 
{
	Q_OBJECT
	friend class Win_QextSerialThread;
	
	private:
		/*!
		 * This method is a part of constructor.
		 */
		void init();
		
	protected:
	    HANDLE Win_Handle;
		HANDLE threadStartEvent;
		HANDLE threadTerminateEvent;
		OVERLAPPED overlap;
	    OVERLAPPED overlapWrite;
		COMMCONFIG Win_CommConfig;
		COMMTIMEOUTS Win_CommTimeouts;
		QReadWriteLock * bytesToWriteLock;	///< @todo maybe move to QextSerialBase.
		qint64 _bytesToWrite;		///< @todo maybe move to QextSerialBase (and implement in POSIX).
		Win_QextSerialThread * overlapThread; ///< @todo maybe move to QextSerialBase (and implement in POSIX).
		 	
		void monitorCommEvent();
		void terminateCommWait();
	    virtual qint64 readData(char *data, qint64 maxSize);
	    virtual qint64 writeData(const char *data, qint64 maxSize);

	public:
	    Win_QextSerialPort(QextSerialBase::QueryMode mode);
	    Win_QextSerialPort(Win_QextSerialPort const& s);
	    Win_QextSerialPort(const QString & name, QextSerialBase::QueryMode mode);
	    Win_QextSerialPort(const PortSettings& settings, QextSerialBase::QueryMode mode);
	    Win_QextSerialPort(const QString & name, const PortSettings& settings, QextSerialBase::QueryMode mode);
	    Win_QextSerialPort& operator=(const Win_QextSerialPort& s);
	    virtual ~Win_QextSerialPort();
	    virtual bool open(OpenMode mode);
	    virtual void close();
	    virtual void flush();
	    virtual qint64 size() const;
	    virtual void ungetChar(char c);
	    virtual void setFlowControl(FlowType);
	    virtual void setParity(ParityType);
	    virtual void setDataBits(DataBitsType);
	    virtual void setStopBits(StopBitsType);
	    virtual void setBaudRate(BaudRateType);
	    virtual void setDtr(bool set=true);
	    virtual void setRts(bool set=true);
	    virtual ulong lineStatus(void);
	    virtual qint64 bytesAvailable() const;
	    virtual void translateError(ulong);
	    virtual void setTimeout(long);
	    
	    /*!
	     * Return number of bytes waiting in the buffer. Currently this shows number 
	     * of bytes queued within write() and before the TX_EMPTY event occured. TX_EMPTY
	     * event is created whenever last character in the system buffer was sent.
	     * 
	     * \return number of bytes queued within write(), before the first TX_EMPTY 
	     * event occur.
	     * 
	     * \warning this function may not give you expected results since TX_EMPTY may occur 
	     * while writing data to the buffer. Eventually some TX_EMPTY events may not be
	     * catched.
	     * 
	     * \note this function always returns 0 in polling mode.
	     * 
	     * \see flush().
	     */
		virtual qint64 bytesToWrite() const;
		
		virtual bool waitForReadyRead(int msecs);	///< @todo implement.
};

/*!
 * This thread monitors communication events.
 */
class Win_QextSerialThread: public QThread
{
	Win_QextSerialPort * qesp;
	bool terminate;

	public:
		/*!
		 * Constructor.
		 * 
		 * \param qesp valid serial port object.
		 */
		Win_QextSerialThread(Win_QextSerialPort * qesp);
		
		/*!
		 * Stop the thread.
		 */
		void stop();
	
	protected:
		//overriden
		virtual void run();
	
};

#endif
