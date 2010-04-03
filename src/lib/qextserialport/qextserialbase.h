#ifndef _QEXTSERIALBASE_H_
#define _QEXTSERIALBASE_H_

#include <QIODevice>
#include <QFile>
#include <QThread>
#include <QMutex>

/*if all warning messages are turned off, flag portability warnings to be turned off as well*/
#ifdef _TTY_NOWARN_
#define _TTY_NOWARN_PORT_
#endif

/*macros for thread support*/
#define LOCK_MUTEX() mutex->lock()
#define UNLOCK_MUTEX() mutex->unlock()

/*macros for warning and debug messages*/
#ifdef _TTY_NOWARN_PORT_
#define TTY_PORTABILITY_WARNING(s)
#else
#define TTY_PORTABILITY_WARNING(s) qWarning(s)
#endif /*_TTY_NOWARN_PORT_*/
#ifdef _TTY_NOWARN_
#define TTY_WARNING(s)
#else
#define TTY_WARNING(s) qWarning(s)
#endif /*_TTY_NOWARN_*/


/*line status constants*/
#define LS_CTS  0x01
#define LS_DSR  0x02
#define LS_DCD  0x04
#define LS_RI   0x08
#define LS_RTS  0x10
#define LS_DTR  0x20
#define LS_ST   0x40
#define LS_SR   0x80

/*error constants*/
#define E_NO_ERROR                   0
#define E_INVALID_FD                 1
#define E_NO_MEMORY                  2
#define E_CAUGHT_NON_BLOCKED_SIGNAL  3
#define E_PORT_TIMEOUT               4
#define E_INVALID_DEVICE             5
#define E_BREAK_CONDITION            6
#define E_FRAMING_ERROR              7
#define E_IO_ERROR                   8
#define E_BUFFER_OVERRUN             9
#define E_RECEIVE_OVERFLOW          10
#define E_RECEIVE_PARITY_ERROR      11
#define E_TRANSMIT_OVERFLOW         12
#define E_READ_FAILED               13
#define E_WRITE_FAILED              14

/*!
 * Enums for port settings.
 */
enum NamingConvention 
{
    WIN_NAMES,
    IRIX_NAMES,
    HPUX_NAMES,
    SUN_NAMES,
    DIGITAL_NAMES,
    FREEBSD_NAMES,
    OPENBSD_NAMES,
    LINUX_NAMES
};

enum BaudRateType 
{
    BAUD50,                //POSIX ONLY
    BAUD75,                //POSIX ONLY
    BAUD110,
    BAUD134,               //POSIX ONLY
    BAUD150,               //POSIX ONLY
    BAUD200,               //POSIX ONLY
    BAUD300,
    BAUD600,
    BAUD1200,
    BAUD1800,              //POSIX ONLY
    BAUD2400,
    BAUD4800,
    BAUD9600,
    BAUD14400,             //WINDOWS ONLY
    BAUD19200,
    BAUD38400,
    BAUD56000,             //WINDOWS ONLY
    BAUD57600,
    BAUD76800,             //POSIX ONLY
    BAUD115200,
    BAUD128000,            //WINDOWS ONLY
    BAUD256000             //WINDOWS ONLY
};

enum DataBitsType 
{
    DATA_5,
    DATA_6,
    DATA_7,
    DATA_8
};

enum ParityType 
{
    PAR_NONE,
    PAR_ODD,
    PAR_EVEN,
    PAR_MARK,               //WINDOWS ONLY
    PAR_SPACE
};

enum StopBitsType 
{
    STOP_1,
    STOP_1_5,               //WINDOWS ONLY
    STOP_2
};

enum FlowType 
{
    FLOW_OFF,
    FLOW_HARDWARE,
    FLOW_XONXOFF
};

/**
 * structure to contain port settings
 */
struct PortSettings 
{
    BaudRateType BaudRate;
    DataBitsType DataBits;
    ParityType Parity;
    StopBitsType StopBits;
    FlowType FlowControl;
    long Timeout_Millisec;
};

/*!
 * \author Stefan Sander
 * \author Michal Policht
 * 
 * A common base class for Win_QextSerialBase, Posix_QextSerialBase and QextSerialPort.
 */
class QextSerialBase : public QIODevice 
{
	Q_OBJECT

	public:
		enum QueryMode {
			Polling,
			EventDriven
		};

	protected:
	    QMutex* mutex;
	    QString port;
	    PortSettings Settings;
	    ulong lastErr;
		QextSerialBase::QueryMode _queryMode;

	    virtual qint64 readData(char * data, qint64 maxSize)=0;
	    virtual qint64 writeData(const char * data, qint64 maxSize)=0;

	public:
	    QextSerialBase();
	    QextSerialBase(const QString & name);
	    virtual ~QextSerialBase();
	    virtual void construct();
	    virtual void setPortName(const QString & name);
	    virtual QString portName() const;
	    
	    /**!
	     * Get query mode.
	     * 	\return query mode.
	     */
		inline QextSerialBase::QueryMode queryMode() const { return _queryMode; };

		/*!
		 * Set desired serial communication handling style. You may choose from polling 
		 * or event driven approach. This function does nothing when port is open; to 
		 * apply changes port must be reopened.
		 * 
		 * In event driven approach read() and write() functions are acting
		 * asynchronously. They return immediately and the operation is performed in 
		 * the background, so they doesn't freeze the calling thread.
		 * To determine when operation is finished, QextSerialPort runs separate thread 
		 * and monitors serial port events. Whenever the event occurs, adequate signal 
		 * is emitted.
		 * 
		 * When polling is set, read() and write() are acting synchronously. Signals are
		 * not working in this mode and some functions may not be available. The advantage
		 * of polling is that it generates less overhead due to lack of signals emissions
		 * and it doesn't start separate thread to monitor events.
		 * 
		 * Generally event driven approach is more capable and friendly, although some 
		 * applications may need as low overhead as possible and then polling comes.
		 * 
		 * \param mode query mode.
		 */
		virtual void setQueryMode(QueryMode mode);

	    virtual void setBaudRate(BaudRateType)=0;
	    virtual BaudRateType baudRate() const;
	    virtual void setDataBits(DataBitsType)=0;
	    virtual DataBitsType dataBits() const;
	    virtual void setParity(ParityType)=0;
	    virtual ParityType parity() const;
	    virtual void setStopBits(StopBitsType)=0;
	    virtual StopBitsType stopBits() const;
	    virtual void setFlowControl(FlowType)=0;
	    virtual FlowType flowControl() const;
	    virtual void setTimeout(long)=0;
	
	    virtual bool open(OpenMode mode)=0;
	    virtual bool isSequential() const;
	    virtual void close()=0;
	    virtual void flush()=0;
	
	    virtual qint64 size() const = 0;
	    virtual qint64 bytesAvailable() const = 0;
	    virtual bool atEnd() const;
	
	    virtual void ungetChar(char c)=0;
	    virtual qint64 readLine(char * data, qint64 maxSize);
	
	    virtual ulong lastError() const;
	    virtual void translateError(ulong error)=0;
	
	    virtual void setDtr(bool set=true)=0;
	    virtual void setRts(bool set=true)=0;
	    virtual ulong lineStatus()=0;

	signals:
		/**
		 * This signal is emitted whenever port settings are updated.
		 * 	\param valid \p true if settings are valid, \p false otherwise.
		 * 
		 * 	@todo implement.
		 */
//		void validSettings(bool valid);

		/*!
		 * This signal is emitted whenever dsr line has changed its state. You may
		 * use this signal to check if device is connected.
		 * 	\param status \p true when DSR signal is on, \p false otherwise.
		 * 
		 * 	\see lineStatus().
		 */
		void dsrChanged(bool status);
};

#endif
