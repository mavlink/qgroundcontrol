//#include <stdio.h>
//#include <Process.h>
//#include <QCoreApplication>
//#include <QEvent>
#include <QReadWriteLock>
#include "win_qextserialport.h"


/*!
\fn Win_QextSerialPort::Win_QextSerialPort()
Default constructor.  Note that the name of the device used by a Win_QextSerialPort constructed
with this constructor will be determined by #defined constants, or lack thereof - the default
behavior is the same as _TTY_LINUX_.  Possible naming conventions and their associated constants
are:

\verbatim

Constant         Used By         Naming Convention
----------       -------------   ------------------------
_TTY_WIN_        Windows         COM1, COM2
_TTY_IRIX_       SGI/IRIX        /dev/ttyf1, /dev/ttyf2
_TTY_HPUX_       HP-UX           /dev/tty1p0, /dev/tty2p0
_TTY_SUN_        SunOS/Solaris   /dev/ttya, /dev/ttyb
_TTY_DIGITAL_    Digital UNIX    /dev/tty01, /dev/tty02
_TTY_FREEBSD_    FreeBSD         /dev/ttyd0, /dev/ttyd1
_TTY_LINUX_      Linux           /dev/ttyS0, /dev/ttyS1
<none>           Linux           /dev/ttyS0, /dev/ttyS1
\endverbatim

This constructor associates the object with the first port on the system, e.g. COM1 for Windows
platforms.  See the other constructor if you need a port other than the first.
*/
Win_QextSerialPort::Win_QextSerialPort(QextSerialBase::QueryMode mode):
	QextSerialBase() 
{
    Win_Handle=INVALID_HANDLE_VALUE;
    setQueryMode(mode);
    init();
}

/*!Win_QextSerialPort::Win_QextSerialPort(const Win_QextSerialPort&)
Copy constructor.
*/
Win_QextSerialPort::Win_QextSerialPort(const Win_QextSerialPort& s):
	QextSerialBase(s.port) 
{
    Win_Handle=INVALID_HANDLE_VALUE;
    _queryMode = s._queryMode;
    _bytesToWrite = s._bytesToWrite;
    bytesToWriteLock = new QReadWriteLock;
    overlapThread = new Win_QextSerialThread(this);
    memcpy(& overlap, & s.overlap, sizeof(OVERLAPPED));
    memcpy(& overlapWrite, & s.overlapWrite, sizeof(OVERLAPPED));
    setOpenMode(s.openMode());
    lastErr=s.lastErr;
    port = s.port;
    Settings.FlowControl=s.Settings.FlowControl;
    Settings.Parity=s.Settings.Parity;
    Settings.DataBits=s.Settings.DataBits;
    Settings.StopBits=s.Settings.StopBits;
    Settings.BaudRate=s.Settings.BaudRate;
    Win_Handle=s.Win_Handle;
    memcpy(&Win_CommConfig, &s.Win_CommConfig, sizeof(COMMCONFIG));
    memcpy(&Win_CommTimeouts, &s.Win_CommTimeouts, sizeof(COMMTIMEOUTS));
    if (s.overlapThread->isRunning())
    	overlapThread->start();
}

/*!
\fn Win_QextSerialPort::Win_QextSerialPort(const QString & name)
Constructs a serial port attached to the port specified by devName.
devName is the name of the device, which is windowsystem-specific,
e.g."COM2" or "/dev/ttyS0".
*/
Win_QextSerialPort::Win_QextSerialPort(const QString & name, QextSerialBase::QueryMode mode):
	QextSerialBase(name) 
{
    Win_Handle=INVALID_HANDLE_VALUE;
    setQueryMode(mode);
    init();
}

/*!
\fn Win_QextSerialPort::Win_QextSerialPort(const PortSettings& settings)
Constructs a port with default name and specified settings.
*/
Win_QextSerialPort::Win_QextSerialPort(const PortSettings& settings, QextSerialBase::QueryMode mode):
	QextSerialBase() 
{
    Win_Handle=INVALID_HANDLE_VALUE;
    setBaudRate(settings.BaudRate);
    setDataBits(settings.DataBits);
    setStopBits(settings.StopBits);
    setParity(settings.Parity);
    setFlowControl(settings.FlowControl);
    setTimeout(settings.Timeout_Millisec);
    setQueryMode(mode);
    init();
}

/*!
\fn Win_QextSerialPort::Win_QextSerialPort(const QString & name, const PortSettings& settings)
Constructs a port with specified name and settings.
*/
Win_QextSerialPort::Win_QextSerialPort(const QString & name, const PortSettings& settings, QextSerialBase::QueryMode mode):
	QextSerialBase(name) 
{
    Win_Handle=INVALID_HANDLE_VALUE;
    setPortName(name);
    setBaudRate(settings.BaudRate);
    setDataBits(settings.DataBits);
    setStopBits(settings.StopBits);
    setParity(settings.Parity);
    setFlowControl(settings.FlowControl);
    setTimeout(settings.Timeout_Millisec);
    setQueryMode(mode);
    init();
}

void Win_QextSerialPort::init()
{
    _bytesToWrite = 0;
    overlap.Internal = 0;
    overlap.InternalHigh = 0;
    overlap.Offset = 0;
    overlap.OffsetHigh = 0;
    overlap.hEvent = CreateEvent(NULL, true, false, NULL);
    overlapThread = new Win_QextSerialThread(this);
    bytesToWriteLock = new QReadWriteLock;
}

/*!
\fn Win_QextSerialPort::~Win_QextSerialPort()
Standard destructor.
*/
Win_QextSerialPort::~Win_QextSerialPort() {
    if (isOpen()) {
        close();
    }
    CloseHandle(overlap.hEvent);
    delete overlapThread;
    delete bytesToWriteLock; 
}

/*!
\fn Win_QextSerialPort& Win_QextSerialPort::operator=(const Win_QextSerialPort& s)
overrides the = operator
*/
Win_QextSerialPort& Win_QextSerialPort::operator=(const Win_QextSerialPort& s) {
    setOpenMode(s.openMode());
    _queryMode = s._queryMode;
    _bytesToWrite = s._bytesToWrite;
    bytesToWriteLock = new QReadWriteLock;
    overlapThread = new Win_QextSerialThread(this);
    memcpy(& overlap, & s.overlap, sizeof(OVERLAPPED));
    memcpy(& overlapWrite, & s.overlapWrite, sizeof(OVERLAPPED));
    lastErr=s.lastErr;
    port = s.port;
    Settings.FlowControl=s.Settings.FlowControl;
    Settings.Parity=s.Settings.Parity;
    Settings.DataBits=s.Settings.DataBits;
    Settings.StopBits=s.Settings.StopBits;
    Settings.BaudRate=s.Settings.BaudRate;
    Win_Handle=s.Win_Handle;
    memcpy(&Win_CommConfig, &s.Win_CommConfig, sizeof(COMMCONFIG));
    memcpy(&Win_CommTimeouts, &s.Win_CommTimeouts, sizeof(COMMTIMEOUTS));
    if (s.overlapThread->isRunning())
    	overlapThread->start();
    return *this;
}


/*!
\fn bool Win_QextSerialPort::open(OpenMode mode)
Opens a serial port.  Note that this function does not specify which device to open.  If you need
to open a device by name, see Win_QextSerialPort::open(const char*).  This function has no effect
if the port associated with the class is already open.  The port is also configured to the current
settings, as stored in the Settings structure.
*/
bool Win_QextSerialPort::open(OpenMode mode) {
    unsigned long confSize = sizeof(COMMCONFIG);
    Win_CommConfig.dwSize = confSize;
    DWORD dwFlagsAndAttributes = 0;
    if (queryMode() == QextSerialBase::EventDriven)
        dwFlagsAndAttributes += FILE_FLAG_OVERLAPPED;

    LOCK_MUTEX();
    if (mode == QIODevice::NotOpen)
        return isOpen();
    if (!isOpen()) {
        /*open the port*/
        Win_Handle=CreateFileA(port.toAscii(), GENERIC_READ|GENERIC_WRITE,
                               FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, dwFlagsAndAttributes, NULL);
        if (Win_Handle!=INVALID_HANDLE_VALUE) {
            /*configure port settings*/
            GetCommConfig(Win_Handle, &Win_CommConfig, &confSize);
            GetCommState(Win_Handle, &(Win_CommConfig.dcb));

            /*set up parameters*/
            Win_CommConfig.dcb.fBinary=TRUE;
            Win_CommConfig.dcb.fInX=FALSE;
            Win_CommConfig.dcb.fOutX=FALSE;
            Win_CommConfig.dcb.fAbortOnError=FALSE;
            Win_CommConfig.dcb.fNull=FALSE;
            setBaudRate(Settings.BaudRate);
            setDataBits(Settings.DataBits);
            setStopBits(Settings.StopBits);
            setParity(Settings.Parity);
            setFlowControl(Settings.FlowControl);
            setTimeout(Settings.Timeout_Millisec);
            SetCommConfig(Win_Handle, &Win_CommConfig, sizeof(COMMCONFIG));

            //init event driven approach
            if (queryMode() == QextSerialBase::EventDriven) {
                Win_CommTimeouts.ReadIntervalTimeout = MAXDWORD;
                Win_CommTimeouts.ReadTotalTimeoutMultiplier = 0;
                Win_CommTimeouts.ReadTotalTimeoutConstant = 0;
                Win_CommTimeouts.WriteTotalTimeoutMultiplier = 0;
                Win_CommTimeouts.WriteTotalTimeoutConstant = 0;
                SetCommTimeouts(Win_Handle, &Win_CommTimeouts);
            	if (!SetCommMask( Win_Handle, EV_TXEMPTY | EV_RXCHAR | EV_DSR)) {
                    qWarning("failed to set Comm Mask. Error code: %ld", GetLastError());
                    UNLOCK_MUTEX();
                    return false;
            	}
            	overlapThread->start();
            }
            QIODevice::open(mode);
        }
    } else {
        UNLOCK_MUTEX();
    	return false;
    }
    UNLOCK_MUTEX();
    return isOpen();
}

/*!
\fn void Win_QextSerialPort::close()
Closes a serial port.  This function has no effect if the serial port associated with the class
is not currently open.
*/
void Win_QextSerialPort::close() 
{
    LOCK_MUTEX();

    if (isOpen()) {
        flush();
        if (overlapThread->isRunning()) {
            overlapThread->stop();
            if (QThread::currentThread() != overlapThread)
                overlapThread->wait();
        }
        if (CloseHandle(Win_Handle))
            Win_Handle = INVALID_HANDLE_VALUE;
        _bytesToWrite = 0;
        QIODevice::close();
    }

    UNLOCK_MUTEX();
}

/*!
\fn void Win_QextSerialPort::flush()
Flushes all pending I/O to the serial port.  This function has no effect if the serial port
associated with the class is not currently open.
*/
void Win_QextSerialPort::flush() {
    LOCK_MUTEX();
    if (isOpen()) {
        FlushFileBuffers(Win_Handle);
    }
    UNLOCK_MUTEX();
}

/*!
\fn qint64 Win_QextSerialPort::size() const
This function will return the number of bytes waiting in the receive queue of the serial port.
It is included primarily to provide a complete QIODevice interface, and will not record errors
in the lastErr member (because it is const).  This function is also not thread-safe - in
multithreading situations, use Win_QextSerialPort::bytesAvailable() instead.
*/
qint64 Win_QextSerialPort::size() const {
    int availBytes;
    COMSTAT Win_ComStat;
    DWORD Win_ErrorMask=0;
    ClearCommError(Win_Handle, &Win_ErrorMask, &Win_ComStat);
    availBytes = Win_ComStat.cbInQue;
    return (qint64)availBytes;
}

/*!
\fn qint64 Win_QextSerialPort::bytesAvailable()
Returns the number of bytes waiting in the port's receive queue.  This function will return 0 if
the port is not currently open, or -1 on error.
*/
qint64 Win_QextSerialPort::bytesAvailable() const {
    LOCK_MUTEX();
    if (isOpen()) {
        DWORD Errors;
        COMSTAT Status;
        if (ClearCommError(Win_Handle, &Errors, &Status)) {
            UNLOCK_MUTEX();
            return Status.cbInQue + QIODevice::bytesAvailable();
        }
        UNLOCK_MUTEX();
        return (qint64)-1;
    }
    UNLOCK_MUTEX();
    return 0;
}

/*!
\fn void Win_QextSerialPort::translateError(ulong error)
Translates a system-specific error code to a QextSerialPort error code.  Used internally.
*/
void Win_QextSerialPort::translateError(ulong error) {
    if (error&CE_BREAK) {
        lastErr=E_BREAK_CONDITION;
    }
    else if (error&CE_FRAME) {
        lastErr=E_FRAMING_ERROR;
    }
    else if (error&CE_IOE) {
        lastErr=E_IO_ERROR;
    }
    else if (error&CE_MODE) {
        lastErr=E_INVALID_FD;
    }
    else if (error&CE_OVERRUN) {
        lastErr=E_BUFFER_OVERRUN;
    }
    else if (error&CE_RXPARITY) {
        lastErr=E_RECEIVE_PARITY_ERROR;
    }
    else if (error&CE_RXOVER) {
        lastErr=E_RECEIVE_OVERFLOW;
    }
    else if (error&CE_TXFULL) {
        lastErr=E_TRANSMIT_OVERFLOW;
    }
}

/*!
\fn qint64 Win_QextSerialPort::readData(char *data, qint64 maxSize)
Reads a block of data from the serial port.  This function will read at most maxlen bytes from
the serial port and place them in the buffer pointed to by data.  Return value is the number of
bytes actually read, or -1 on error.

\warning before calling this function ensure that serial port associated with this class
is currently open (use isOpen() function to check if port is open).
*/
qint64 Win_QextSerialPort::readData(char *data, qint64 maxSize)
{
    DWORD retVal;

    LOCK_MUTEX();
    
    retVal = 0;
    if (queryMode() == QextSerialBase::EventDriven) {
        OVERLAPPED overlapRead;
        overlapRead.Internal = 0;
        overlapRead.InternalHigh = 0;
        overlapRead.Offset = 0;
        overlapRead.OffsetHigh = 0;
        overlapRead.hEvent = CreateEvent(NULL, true, false, NULL);
        if (!ReadFile(Win_Handle, (void*)data, (DWORD)maxSize, & retVal, & overlapRead)) {
            if (GetLastError() == ERROR_IO_PENDING)
                GetOverlappedResult(Win_Handle, & overlapRead, & retVal, true);
            else {
                lastErr = E_READ_FAILED;
                retVal = (DWORD)-1;
            }
        }
        CloseHandle(overlapRead.hEvent);
    } else if (!ReadFile(Win_Handle, (void*)data, (DWORD)maxSize, & retVal, NULL)) {
        lastErr = E_READ_FAILED;
        retVal = (DWORD)-1;
    }

    UNLOCK_MUTEX();

    return (qint64)retVal;
}

/*!
\fn qint64 Win_QextSerialPort::writeData(const char *data, qint64 maxSize)
Writes a block of data to the serial port.  This function will write len bytes
from the buffer pointed to by data to the serial port.  Return value is the number
of bytes actually written, or -1 on error.

\warning before calling this function ensure that serial port associated with this class
is currently open (use isOpen() function to check if port is open).
*/
qint64 Win_QextSerialPort::writeData(const char *data, qint64 maxSize)
{
    DWORD retVal;

    LOCK_MUTEX();

    retVal = 0;
    if (queryMode() == QextSerialBase::EventDriven) {
    	bytesToWriteLock->lockForWrite();
    	_bytesToWrite += maxSize;
    	bytesToWriteLock->unlock();
        overlapWrite.Internal = 0;
        overlapWrite.InternalHigh = 0;
        overlapWrite.Offset = 0;
        overlapWrite.OffsetHigh = 0;
        overlapWrite.hEvent = CreateEvent(NULL, true, false, NULL);
        if (!WriteFile(Win_Handle, (void*)data, (DWORD)maxSize, & retVal, & overlapWrite)) {
            lastErr = E_WRITE_FAILED;
            retVal = (DWORD)-1;
        } else
            retVal = maxSize;
    } else if (!WriteFile(Win_Handle, (void*)data, (DWORD)maxSize, & retVal, NULL)) {
        lastErr = E_WRITE_FAILED;
        retVal = (DWORD)-1;
    }

    UNLOCK_MUTEX();

    return (qint64)retVal;
}

/*!
\fn void Win_QextSerialPort::ungetChar(char c)
This function is included to implement the full QIODevice interface, and currently has no
purpose within this class.  This function is meaningless on an unbuffered device and currently
only prints a warning message to that effect.
*/
void Win_QextSerialPort::ungetChar(char c) {

    /*meaningless on unbuffered sequential device - return error and print a warning*/
    TTY_WARNING("Win_QextSerialPort: ungetChar() called on an unbuffered sequential device - operation is meaningless");
}

/*!
\fn void Win_QextSerialPort::setFlowControl(FlowType flow)
Sets the flow control used by the port.  Possible values of flow are:
\verbatim
    FLOW_OFF            No flow control
    FLOW_HARDWARE       Hardware (RTS/CTS) flow control
    FLOW_XONXOFF        Software (XON/XOFF) flow control
\endverbatim
*/
void Win_QextSerialPort::setFlowControl(FlowType flow) {
    LOCK_MUTEX();
    if (Settings.FlowControl!=flow) {
        Settings.FlowControl=flow;
    }
    if (isOpen()) {
        switch(flow) {

            /*no flow control*/
        case FLOW_OFF:
            Win_CommConfig.dcb.fOutxCtsFlow=FALSE;
            Win_CommConfig.dcb.fRtsControl=RTS_CONTROL_DISABLE;
            Win_CommConfig.dcb.fInX=FALSE;
            Win_CommConfig.dcb.fOutX=FALSE;
            SetCommConfig(Win_Handle, &Win_CommConfig, sizeof(COMMCONFIG));
            break;

            /*software (XON/XOFF) flow control*/
        case FLOW_XONXOFF:
            Win_CommConfig.dcb.fOutxCtsFlow=FALSE;
            Win_CommConfig.dcb.fRtsControl=RTS_CONTROL_DISABLE;
            Win_CommConfig.dcb.fInX=TRUE;
            Win_CommConfig.dcb.fOutX=TRUE;
            SetCommConfig(Win_Handle, &Win_CommConfig, sizeof(COMMCONFIG));
            break;

        case FLOW_HARDWARE:
            Win_CommConfig.dcb.fOutxCtsFlow=TRUE;
            Win_CommConfig.dcb.fRtsControl=RTS_CONTROL_HANDSHAKE;
            Win_CommConfig.dcb.fInX=FALSE;
            Win_CommConfig.dcb.fOutX=FALSE;
            SetCommConfig(Win_Handle, &Win_CommConfig, sizeof(COMMCONFIG));
            break;
        }
    }
    UNLOCK_MUTEX();
}

/*!
\fn void Win_QextSerialPort::setParity(ParityType parity)
Sets the parity associated with the serial port.  The possible values of parity are:
\verbatim
    PAR_SPACE       Space Parity
    PAR_MARK        Mark Parity
    PAR_NONE        No Parity
    PAR_EVEN        Even Parity
    PAR_ODD         Odd Parity
\endverbatim
*/
void Win_QextSerialPort::setParity(ParityType parity) {
    LOCK_MUTEX();
    if (Settings.Parity!=parity) {
        Settings.Parity=parity;
    }
    if (isOpen()) {
        Win_CommConfig.dcb.Parity=(unsigned char)parity;
        switch (parity) {

            /*space parity*/
        case PAR_SPACE:
            if (Settings.DataBits==DATA_8) {
                TTY_PORTABILITY_WARNING("Win_QextSerialPort Portability Warning: Space parity with 8 data bits is not supported by POSIX systems.");
            }
            Win_CommConfig.dcb.fParity=TRUE;
            break;

            /*mark parity - WINDOWS ONLY*/
            case PAR_MARK:
            TTY_PORTABILITY_WARNING("Win_QextSerialPort Portability Warning:  Mark parity is not supported by POSIX systems");
            Win_CommConfig.dcb.fParity=TRUE;
            break;

            /*no parity*/
            case PAR_NONE:
            Win_CommConfig.dcb.fParity=FALSE;
            break;

            /*even parity*/
            case PAR_EVEN:
            Win_CommConfig.dcb.fParity=TRUE;
            break;

            /*odd parity*/
            case PAR_ODD:
            Win_CommConfig.dcb.fParity=TRUE;
            break;
        }
        SetCommConfig(Win_Handle, &Win_CommConfig, sizeof(COMMCONFIG));
    }
    UNLOCK_MUTEX();
}

/*!
\fn void Win_QextSerialPort::setDataBits(DataBitsType dataBits)
Sets the number of data bits used by the serial port.  Possible values of dataBits are:
\verbatim
    DATA_5      5 data bits
    DATA_6      6 data bits
    DATA_7      7 data bits
    DATA_8      8 data bits
\endverbatim

\note
This function is subject to the following restrictions:
\par
    5 data bits cannot be used with 2 stop bits.
\par
    1.5 stop bits can only be used with 5 data bits.
\par
    8 data bits cannot be used with space parity on POSIX systems.

*/
void Win_QextSerialPort::setDataBits(DataBitsType dataBits) {
    LOCK_MUTEX();
    if (Settings.DataBits!=dataBits) {
        if ((Settings.StopBits==STOP_2 && dataBits==DATA_5) ||
            (Settings.StopBits==STOP_1_5 && dataBits!=DATA_5)) {
        }
        else {
            Settings.DataBits=dataBits;
        }
    }
    if (isOpen()) {
        switch(dataBits) {

            /*5 data bits*/
        case DATA_5:
            if (Settings.StopBits==STOP_2) {
                TTY_WARNING("Win_QextSerialPort: 5 Data bits cannot be used with 2 stop bits.");
            }
            else {
                Win_CommConfig.dcb.ByteSize=5;
                SetCommConfig(Win_Handle, &Win_CommConfig, sizeof(COMMCONFIG));
            }
            break;

            /*6 data bits*/
            case DATA_6:
            if (Settings.StopBits==STOP_1_5) {
                TTY_WARNING("Win_QextSerialPort: 6 Data bits cannot be used with 1.5 stop bits.");
            }
            else {
                Win_CommConfig.dcb.ByteSize=6;
                SetCommConfig(Win_Handle, &Win_CommConfig, sizeof(COMMCONFIG));
            }
            break;

            /*7 data bits*/
            case DATA_7:
            if (Settings.StopBits==STOP_1_5) {
                TTY_WARNING("Win_QextSerialPort: 7 Data bits cannot be used with 1.5 stop bits.");
            }
            else {
                Win_CommConfig.dcb.ByteSize=7;
                SetCommConfig(Win_Handle, &Win_CommConfig, sizeof(COMMCONFIG));
            }
            break;

            /*8 data bits*/
            case DATA_8:
            if (Settings.StopBits==STOP_1_5) {
                TTY_WARNING("Win_QextSerialPort: 8 Data bits cannot be used with 1.5 stop bits.");
            }
            else {
                Win_CommConfig.dcb.ByteSize=8;
                SetCommConfig(Win_Handle, &Win_CommConfig, sizeof(COMMCONFIG));
            }
            break;
        }
    }
    UNLOCK_MUTEX();
}

/*!
\fn void Win_QextSerialPort::setStopBits(StopBitsType stopBits)
Sets the number of stop bits used by the serial port.  Possible values of stopBits are:
\verbatim
    STOP_1      1 stop bit
    STOP_1_5    1.5 stop bits
    STOP_2      2 stop bits
\endverbatim

\note
This function is subject to the following restrictions:
\par
    2 stop bits cannot be used with 5 data bits.
\par
    1.5 stop bits cannot be used with 6 or more data bits.
\par
    POSIX does not support 1.5 stop bits.
*/
void Win_QextSerialPort::setStopBits(StopBitsType stopBits) {
    LOCK_MUTEX();
    if (Settings.StopBits!=stopBits) {
        if ((Settings.DataBits==DATA_5 && stopBits==STOP_2) ||
            (stopBits==STOP_1_5 && Settings.DataBits!=DATA_5)) {
        }
        else {
            Settings.StopBits=stopBits;
        }
    }
    if (isOpen()) {
        switch (stopBits) {

            /*one stop bit*/
        case STOP_1:
            Win_CommConfig.dcb.StopBits=ONESTOPBIT;
            SetCommConfig(Win_Handle, &Win_CommConfig, sizeof(COMMCONFIG));
            break;

            /*1.5 stop bits*/
        case STOP_1_5:
            TTY_PORTABILITY_WARNING("Win_QextSerialPort Portability Warning: 1.5 stop bit operation is not supported by POSIX.");
            if (Settings.DataBits!=DATA_5) {
                TTY_WARNING("Win_QextSerialPort: 1.5 stop bits can only be used with 5 data bits");
            }
            else {
                Win_CommConfig.dcb.StopBits=ONE5STOPBITS;
                SetCommConfig(Win_Handle, &Win_CommConfig, sizeof(COMMCONFIG));
            }
            break;

            /*two stop bits*/
            case STOP_2:
            if (Settings.DataBits==DATA_5) {
                TTY_WARNING("Win_QextSerialPort: 2 stop bits cannot be used with 5 data bits");
            }
            else {
                Win_CommConfig.dcb.StopBits=TWOSTOPBITS;
                SetCommConfig(Win_Handle, &Win_CommConfig, sizeof(COMMCONFIG));
            }
            break;
        }
    }
    UNLOCK_MUTEX();
}

/*!
\fn void Win_QextSerialPort::setBaudRate(BaudRateType baudRate)
Sets the baud rate of the serial port.  Note that not all rates are applicable on
all platforms.  The following table shows translations of the various baud rate
constants on Windows(including NT/2000) and POSIX platforms.  Speeds marked with an *
are speeds that are usable on both Windows and POSIX.
\verbatim

  RATE          Windows Speed   POSIX Speed
  -----------   -------------   -----------
   BAUD50                 110          50
   BAUD75                 110          75
  *BAUD110                110         110
   BAUD134                110         134.5
   BAUD150                110         150
   BAUD200                110         200
  *BAUD300                300         300
  *BAUD600                600         600
  *BAUD1200              1200        1200
   BAUD1800              1200        1800
  *BAUD2400              2400        2400
  *BAUD4800              4800        4800
  *BAUD9600              9600        9600
   BAUD14400            14400        9600
  *BAUD19200            19200       19200
  *BAUD38400            38400       38400
   BAUD56000            56000       38400
  *BAUD57600            57600       57600
   BAUD76800            57600       76800
  *BAUD115200          115200      115200
   BAUD128000          128000      115200
   BAUD256000          256000      115200
   BAUD230400          230400      115200
   BAUD256000          256000      115200
   BAUD460800          460800      115200
   BAUD921600          921600      115200
\endverbatim
*/
void Win_QextSerialPort::setBaudRate(BaudRateType baudRate) {
    LOCK_MUTEX();
    if (Settings.BaudRate!=baudRate) {
        switch (baudRate) {
        case BAUD50:
        case BAUD75:
        case BAUD134:
        case BAUD150:
        case BAUD200:
            Settings.BaudRate=BAUD110;
            break;

        case BAUD1800:
            Settings.BaudRate=BAUD1200;
            break;

        case BAUD76800:
            Settings.BaudRate=BAUD57600;
            break;

        default:
            Settings.BaudRate=baudRate;
            break;
        }
    }
    if (isOpen()) {
        switch (baudRate) {

            /*50 baud*/
        case BAUD50:
            TTY_WARNING("Win_QextSerialPort: Windows does not support 50 baud operation.  Switching to 110 baud.");
            Win_CommConfig.dcb.BaudRate=CBR_110;
            break;

            /*75 baud*/
        case BAUD75:
            TTY_WARNING("Win_QextSerialPort: Windows does not support 75 baud operation.  Switching to 110 baud.");
            Win_CommConfig.dcb.BaudRate=CBR_110;
            break;

            /*110 baud*/
        case BAUD110:
            Win_CommConfig.dcb.BaudRate=CBR_110;
            break;

            /*134.5 baud*/
        case BAUD134:
            TTY_WARNING("Win_QextSerialPort: Windows does not support 134.5 baud operation.  Switching to 110 baud.");
            Win_CommConfig.dcb.BaudRate=CBR_110;
            break;

            /*150 baud*/
        case BAUD150:
            TTY_WARNING("Win_QextSerialPort: Windows does not support 150 baud operation.  Switching to 110 baud.");
            Win_CommConfig.dcb.BaudRate=CBR_110;
            break;

            /*200 baud*/
        case BAUD200:
            TTY_WARNING("Win_QextSerialPort: Windows does not support 200 baud operation.  Switching to 110 baud.");
            Win_CommConfig.dcb.BaudRate=CBR_110;
            break;

            /*300 baud*/
        case BAUD300:
            Win_CommConfig.dcb.BaudRate=CBR_300;
            break;

            /*600 baud*/
        case BAUD600:
            Win_CommConfig.dcb.BaudRate=CBR_600;
            break;

            /*1200 baud*/
        case BAUD1200:
            Win_CommConfig.dcb.BaudRate=CBR_1200;
            break;

            /*1800 baud*/
        case BAUD1800:
            TTY_WARNING("Win_QextSerialPort: Windows does not support 1800 baud operation.  Switching to 1200 baud.");
            Win_CommConfig.dcb.BaudRate=CBR_1200;
            break;

            /*2400 baud*/
        case BAUD2400:
            Win_CommConfig.dcb.BaudRate=CBR_2400;
            break;

            /*4800 baud*/
        case BAUD4800:
            Win_CommConfig.dcb.BaudRate=CBR_4800;
            break;

            /*9600 baud*/
        case BAUD9600:
            Win_CommConfig.dcb.BaudRate=CBR_9600;
            break;

            /*14400 baud*/
        case BAUD14400:
            TTY_PORTABILITY_WARNING("Win_QextSerialPort Portability Warning: POSIX does not support 14400 baud operation.");
            Win_CommConfig.dcb.BaudRate=CBR_14400;
            break;

            /*19200 baud*/
        case BAUD19200:
            Win_CommConfig.dcb.BaudRate=CBR_19200;
            break;

            /*38400 baud*/
        case BAUD38400:
            Win_CommConfig.dcb.BaudRate=CBR_38400;
            break;

            /*56000 baud*/
        case BAUD56000:
            TTY_PORTABILITY_WARNING("Win_QextSerialPort Portability Warning: POSIX does not support 56000 baud operation.");
            Win_CommConfig.dcb.BaudRate=CBR_56000;
            break;

            /*57600 baud*/
        case BAUD57600:
            Win_CommConfig.dcb.BaudRate=CBR_57600;
            break;

            /*76800 baud*/
        case BAUD76800:
            TTY_WARNING("Win_QextSerialPort: Windows does not support 76800 baud operation.  Switching to 57600 baud.");
            Win_CommConfig.dcb.BaudRate=CBR_57600;
            break;

            /*115200 baud*/
        case BAUD115200:
            Win_CommConfig.dcb.BaudRate=CBR_115200;
            break;

            /*128000 baud*/
        case BAUD128000:
            TTY_PORTABILITY_WARNING("Win_QextSerialPort Portability Warning: POSIX does not support 128000 baud operation.");
            Win_CommConfig.dcb.BaudRate=CBR_128000;
            break;

            /*256000 baud*/
        case BAUD256000:
            TTY_PORTABILITY_WARNING("Win_QextSerialPort Portability Warning: POSIX does not support 256000 baud operation.");
            Win_CommConfig.dcb.BaudRate=CBR_256000;
            break;
            /*230400 baud*/
        case BAUD230400:
            TTY_PORTABILITY_WARNING("Win_QextSerialPort Portability Warning: POSIX does not support 230400 baud operation.");
            Win_CommConfig.dcb.BaudRate=CBR_230400;
            break;
            /*460800 baud*/
        case BAUD460800:
            TTY_PORTABILITY_WARNING("Win_QextSerialPort Portability Warning: POSIX does not support 460800 baud operation.");
            Win_CommConfig.dcb.BaudRate=CBR_460800;
            break;
            /*921600 baud*/
        case BAUD921600:
            TTY_PORTABILITY_WARNING("Win_QextSerialPort Portability Warning: POSIX does not support 921600 baud operation.");
            Win_CommConfig.dcb.BaudRate=CBR_921600;
            break;
        }
        SetCommConfig(Win_Handle, &Win_CommConfig, sizeof(COMMCONFIG));
    }
    UNLOCK_MUTEX();
}

/*!
\fn void Win_QextSerialPort::setDtr(bool set)
Sets DTR line to the requested state (high by default).  This function will have no effect if
the port associated with the class is not currently open.
*/
void Win_QextSerialPort::setDtr(bool set) {
    LOCK_MUTEX();
    if (isOpen()) {
        if (set) {
            EscapeCommFunction(Win_Handle, SETDTR);
        }
        else {
            EscapeCommFunction(Win_Handle, CLRDTR);
        }
    }
    UNLOCK_MUTEX();
}

/*!
\fn void Win_QextSerialPort::setRts(bool set)
Sets RTS line to the requested state (high by default).  This function will have no effect if
the port associated with the class is not currently open.
*/
void Win_QextSerialPort::setRts(bool set) {
    LOCK_MUTEX();
    if (isOpen()) {
        if (set) {
            EscapeCommFunction(Win_Handle, SETRTS);
        }
        else {
            EscapeCommFunction(Win_Handle, CLRRTS);
        }
    }
    UNLOCK_MUTEX();
}

/*!
\fn ulong Win_QextSerialPort::lineStatus(void)
returns the line status as stored by the port function.  This function will retrieve the states
of the following lines: DCD, CTS, DSR, and RI.  On POSIX systems, the following additional lines
can be monitored: DTR, RTS, Secondary TXD, and Secondary RXD.  The value returned is an unsigned
long with specific bits indicating which lines are high.  The following constants should be used
to examine the states of individual lines:

\verbatim
Mask        Line
------      ----
LS_CTS      CTS
LS_DSR      DSR
LS_DCD      DCD
LS_RI       RI
\endverbatim

This function will return 0 if the port associated with the class is not currently open.
*/
ulong Win_QextSerialPort::lineStatus(void) {
    unsigned long Status=0, Temp=0;
    LOCK_MUTEX();
    if (isOpen()) {
        GetCommModemStatus(Win_Handle, &Temp);
        if (Temp&MS_CTS_ON) {
            Status|=LS_CTS;
        }
        if (Temp&MS_DSR_ON) {
            Status|=LS_DSR;
        }
        if (Temp&MS_RING_ON) {
            Status|=LS_RI;
        }
        if (Temp&MS_RLSD_ON) {
            Status|=LS_DCD;
        }
    }
    UNLOCK_MUTEX();
    return Status;
}

bool Win_QextSerialPort::waitForReadyRead(int msecs)
{
    //@todo implement
    return false;
}

qint64 Win_QextSerialPort::bytesToWrite() const
{
    return _bytesToWrite;
}

void Win_QextSerialPort::monitorCommEvent()
{
    DWORD eventMask = 0;

    ResetEvent(overlap.hEvent);
    if (!WaitCommEvent(Win_Handle, & eventMask, & overlap))
        if (GetLastError() != ERROR_IO_PENDING)
            qCritical("WaitCommEvent error %ld\n", GetLastError());

    if (WaitForSingleObject(overlap.hEvent, INFINITE) == WAIT_OBJECT_0) {
        //overlap event occured
        DWORD undefined;
        if (!GetOverlappedResult(Win_Handle, & overlap, & undefined, false)) {
            qWarning("CommEvent overlapped error %ld", GetLastError());
            return;
        }
        if (eventMask & EV_RXCHAR) {
            if (sender() != this)
                emit readyRead();
        }
        if (eventMask & EV_TXEMPTY) {
            DWORD numBytes;
            GetOverlappedResult(Win_Handle, & overlapWrite, & numBytes, true);
            bytesToWriteLock->lockForWrite();
            if (sender() != this)
                emit bytesWritten(bytesToWrite());
            _bytesToWrite = 0;
            bytesToWriteLock->unlock();
        }
        if (eventMask & EV_DSR){
            if (lineStatus() & LS_DSR)
                emit dsrChanged(true);
        }
        else{
            emit dsrChanged(false);
        }
    }
}

void Win_QextSerialPort::terminateCommWait()
{
    SetCommMask(Win_Handle, 0);
}


/*!
\fn void Win_QextSerialPort::setTimeout(ulong millisec);
Sets the read and write timeouts for the port to millisec milliseconds.
Setting 0 indicates that timeouts are not used for read nor write operations; 
however read() and write() functions will still block. Set -1 to provide
non-blocking behaviour (read() and write() will return immediately).

\note this function does nothing in event driven mode.
*/
void Win_QextSerialPort::setTimeout(long millisec) {
    LOCK_MUTEX();
    Settings.Timeout_Millisec = millisec;

    if (millisec == -1) {
        Win_CommTimeouts.ReadIntervalTimeout = MAXDWORD;
        Win_CommTimeouts.ReadTotalTimeoutConstant = 0;
    } else {
        Win_CommTimeouts.ReadIntervalTimeout = millisec;
        Win_CommTimeouts.ReadTotalTimeoutConstant = millisec;
    }
    Win_CommTimeouts.ReadTotalTimeoutMultiplier = 0;
    Win_CommTimeouts.WriteTotalTimeoutMultiplier = millisec;
    Win_CommTimeouts.WriteTotalTimeoutConstant = 0;
    if (queryMode() != QextSerialBase::EventDriven)
      	SetCommTimeouts(Win_Handle, &Win_CommTimeouts);

    UNLOCK_MUTEX();
}


Win_QextSerialThread::Win_QextSerialThread(Win_QextSerialPort * qesp):
	QThread()
{
    this->qesp = qesp;
    terminate = false;
}

void Win_QextSerialThread::stop()
{
    terminate = true;
    qesp->terminateCommWait();
}

void Win_QextSerialThread::run()
{
    while (!terminate)
        qesp->monitorCommEvent();
    terminate = false;
}
