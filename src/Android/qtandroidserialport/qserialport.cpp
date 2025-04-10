// Copyright (C) 2011-2012 Denis Shienkov <denis.shienkov@gmail.com>
// Copyright (C) 2011 Sergey Belyashov <Sergey.Belyashov@gmail.com>
// Copyright (C) 2012 Laszlo Papp <lpapp@kde.org>
// Copyright (C) 2012 Andre Hartmann <aha_1980@gmx.de>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qserialport.h"
#include "qserialportinfo.h"
#include "qserialportinfo_p.h"

#include "qserialport_p.h"

#include <QtCore/qdebug.h>

QT_BEGIN_NAMESPACE

QSerialPortErrorInfo::QSerialPortErrorInfo(QSerialPort::SerialPortError newErrorCode,
                                           const QString &newErrorString)
    : errorCode(newErrorCode)
    , errorString(newErrorString)
{
    if (errorString.isNull()) {
        switch (errorCode) {
        case QSerialPort::NoError:
            errorString = QSerialPort::tr("No error");
            break;
        case QSerialPort::OpenError:
            errorString = QSerialPort::tr("Device is already open");
            break;
        case QSerialPort::NotOpenError:
            errorString = QSerialPort::tr("Device is not open");
            break;
        case QSerialPort::TimeoutError:
            errorString = QSerialPort::tr("Operation timed out");
            break;
        case QSerialPort::ReadError:
            errorString = QSerialPort::tr("Error reading from device");
            break;
        case QSerialPort::WriteError:
            errorString = QSerialPort::tr("Error writing to device");
            break;
        case QSerialPort::ResourceError:
            errorString = QSerialPort::tr("Device disappeared from the system");
            break;
        default:
            // an empty string will be interpreted as "Unknown error"
            // from the QIODevice::errorString()
            break;
        }
    }
}

QSerialPortPrivate::QSerialPortPrivate()
{
    writeBufferChunkSize = QSERIALPORT_BUFFERSIZE;
    readBufferChunkSize = QSERIALPORT_BUFFERSIZE;
}

void QSerialPortPrivate::setError(const QSerialPortErrorInfo &errorInfo)
{
    Q_Q(QSerialPort);

    q->setErrorString(errorInfo.errorString);
    error.setValue(errorInfo.errorCode);
    error.notify();
    emit q->errorOccurred(error);
}

/*!
    \class QSerialPort

    \brief Provides functions to access serial ports.

    \reentrant
    \ingroup serialport-main
    \inmodule QtSerialPort
    \since 5.1

    You can get information about the available serial ports using the
    QSerialPortInfo helper class, which allows an enumeration of all the serial
    ports in the system. This is useful to obtain the correct name of the
    serial port you want to use. You can pass an object
    of the helper class as an argument to the setPort() or setPortName()
    methods to assign the desired serial device.

    After setting the port, you can open it in read-only (r/o), write-only
    (w/o), or read-write (r/w) mode using the open() method.

    \note The serial port is always opened with exclusive access
    (that is, no other process or thread can access an already opened serial port).

    Use the close() method to close the port and cancel the I/O operations.

    Having successfully opened, QSerialPort tries to determine the current
    configuration of the port and initializes itself. You can reconfigure the
    port to the desired setting using the setBaudRate(), setDataBits(),
    setParity(), setStopBits(), and setFlowControl() methods.

    There are a couple of properties to work with the pinout signals namely:
    QSerialPort::dataTerminalReady, QSerialPort::requestToSend. It is also
    possible to use the pinoutSignals() method to query the current pinout
    signals set.

    Once you know that the ports are ready to read or write, you can
    use the read() or write() methods. Alternatively the
    readLine() and readAll() convenience methods can also be invoked.
    If not all the data is read at once, the remaining data will
    be available for later as new incoming data is appended to the
    QSerialPort's internal read buffer. You can limit the size of the read
    buffer using setReadBufferSize().

    QSerialPort provides a set of functions that suspend the
    calling thread until certain signals are emitted. These functions
    can be used to implement blocking serial ports:

    \list

    \li waitForReadyRead() blocks calls until new data is available for
    reading.

    \li waitForBytesWritten() blocks calls until one payload of data has
    been written to the serial port.

    \endlist

    See the following example:

    \code
     int numRead = 0, numReadTotal = 0;
     char buffer[50];

     for (;;) {
         numRead  = serial.read(buffer, 50);

         // Do whatever with the array

         numReadTotal += numRead;
         if (numRead == 0 && !serial.waitForReadyRead())
             break;
     }
    \endcode

    If \l{QIODevice::}{waitForReadyRead()} returns \c false, the
    connection has been closed or an error has occurred.

    If an error occurs at any point in time, QSerialPort will emit the
    errorOccurred() signal. You can also call error() to find the type of
    error that occurred last.

    Programming with a blocking serial port is radically different from
    programming with a non-blocking serial port. A blocking serial port
    does not require an event loop and typically leads to simpler code.
    However, in a GUI application, blocking serial port should only be
    used in non-GUI threads, to avoid freezing the user interface.

    For more details about these approaches, refer to the
    \l {Qt Serial Port Examples}{example} applications.

    The QSerialPort class can also be used with QTextStream and QDataStream's
    stream operators (operator<<() and operator>>()). There is one issue to be
    aware of, though: make sure that enough data is available before attempting
    to read by using the operator>>() overloaded operator.

    \sa QSerialPortInfo
*/

/*!
    \enum QSerialPort::Direction

    This enum describes the possible directions of the data transmission.

    \note This enumeration is used for setting the baud rate of the device
    separately for each direction on some operating systems (for example,
    POSIX-like).

    \value Input            Input direction.
    \value Output           Output direction.
    \value AllDirections    Simultaneously in two directions.
*/

/*!
    \enum QSerialPort::BaudRate

    This enum describes the baud rate which the communication device operates
    with.

    \note Only the most common standard baud rates are listed in this enum.

    \value Baud1200     1200 baud.
    \value Baud2400     2400 baud.
    \value Baud4800     4800 baud.
    \value Baud9600     9600 baud.
    \value Baud19200    19200 baud.
    \value Baud38400    38400 baud.
    \value Baud57600    57600 baud.
    \value Baud115200   115200 baud.

    \sa QSerialPort::baudRate
*/

/*!
    \enum QSerialPort::DataBits

    This enum describes the number of data bits used.

    \value Data5            The number of data bits in each character is 5. It
                            is used for Baudot code. It generally only makes
                            sense with older equipment such as teleprinters.
    \value Data6            The number of data bits in each character is 6. It
                            is rarely used.
    \value Data7            The number of data bits in each character is 7. It
                            is used for true ASCII. It generally only makes
                            sense with older equipment such as teleprinters.
    \value Data8            The number of data bits in each character is 8. It
                            is used for most kinds of data, as this size matches
                            the size of a byte. It is almost universally used in
                            newer applications.

    \sa QSerialPort::dataBits
*/

/*!
    \enum QSerialPort::Parity

    This enum describes the parity scheme used.

    \value NoParity         No parity bit it sent. This is the most common
                            parity setting. Error detection is handled by the
                            communication protocol.
    \value EvenParity       The number of 1 bits in each character, including
                            the parity bit, is always even.
    \value OddParity        The number of 1 bits in each character, including
                            the parity bit, is always odd. It ensures that at
                            least one state transition occurs in each character.
    \value SpaceParity      Space parity. The parity bit is sent in the space
                            signal condition. It does not provide error
                            detection information.
    \value MarkParity       Mark parity. The parity bit is always set to the
                            mark signal condition (logical 1). It does not
                            provide error detection information.

    \sa QSerialPort::parity
*/

/*!
    \enum QSerialPort::StopBits

    This enum describes the number of stop bits used.

    \value OneStop          1 stop bit.
    \value OneAndHalfStop   1.5 stop bits. This is only for the Windows platform.
    \value TwoStop          2 stop bits.

    \sa QSerialPort::stopBits
*/

/*!
    \enum QSerialPort::FlowControl

    This enum describes the flow control used.

    \value NoFlowControl        No flow control.
    \value HardwareControl      Hardware flow control (RTS/CTS).
    \value SoftwareControl      Software flow control (XON/XOFF).

    \sa QSerialPort::flowControl
*/

/*!
    \enum QSerialPort::PinoutSignal

    This enum describes the possible RS-232 pinout signals.

    \value NoSignal                       No line active
    \value DataTerminalReadySignal        DTR (Data Terminal Ready).
    \value DataCarrierDetectSignal        DCD (Data Carrier Detect).
    \value DataSetReadySignal             DSR (Data Set Ready).
    \value RingIndicatorSignal            RNG (Ring Indicator).
    \value RequestToSendSignal            RTS (Request To Send).
    \value ClearToSendSignal              CTS (Clear To Send).
    \value SecondaryTransmittedDataSignal STD (Secondary Transmitted Data).
    \value SecondaryReceivedDataSignal    SRD (Secondary Received Data).

    \sa pinoutSignals(), QSerialPort::dataTerminalReady,
    QSerialPort::requestToSend
*/

/*!
    \enum QSerialPort::SerialPortError

    This enum describes the errors that may be contained by the
    QSerialPort::error property.

    \value NoError              No error occurred.

    \value DeviceNotFoundError  An error occurred while attempting to
                                open an non-existing device.

    \value PermissionError      An error occurred while attempting to
                                open an already opened device by another
                                process or a user not having enough permission
                                and credentials to open.

    \value OpenError            An error occurred while attempting to open an
                                already opened device in this object.

    \value NotOpenError         This error occurs when an operation is executed
                                that can only be successfully performed if the
                                device is open. This value was introduced in
                                QtSerialPort 5.2.

    \value WriteError           An I/O error occurred while writing the data.

    \value ReadError            An I/O error occurred while reading the data.

    \value ResourceError        An I/O error occurred when a resource becomes
                                unavailable, e.g. when the device is
                                unexpectedly removed from the system.

    \value UnsupportedOperationError The requested device operation is not
                                supported or prohibited by the running operating
                                system.

    \value TimeoutError         A timeout error occurred. This value was
                                introduced in QtSerialPort 5.2.

    \value UnknownError         An unidentified error occurred.
    \sa QSerialPort::error
*/



/*!
    Constructs a new serial port object with the given \a parent.
*/
QSerialPort::QSerialPort(QObject *parent)
    : QIODevice(*new QSerialPortPrivate, parent)
{
}

/*!
    Constructs a new serial port object with the given \a parent
    to represent the serial port with the specified \a name.

    The name should have a specific format; see the setPort() method.
*/
QSerialPort::QSerialPort(const QString &name, QObject *parent)
    : QIODevice(*new QSerialPortPrivate, parent)
{
    setPortName(name);
}

/*!
    Constructs a new serial port object with the given \a parent
    to represent the serial port with the specified helper class
    \a serialPortInfo.
*/
QSerialPort::QSerialPort(const QSerialPortInfo &serialPortInfo, QObject *parent)
    : QIODevice(*new QSerialPortPrivate, parent)
{
    setPort(serialPortInfo);
}

/*!
    Closes the serial port, if necessary, and then destroys object.
*/
QSerialPort::~QSerialPort()
{
    /**/
    if (isOpen())
        close();
}

/*!
    Sets the \a name of the serial port.

    The name of the serial port can be passed as either a short name or
    the long system location if necessary.

    \sa portName(), QSerialPortInfo
*/
void QSerialPort::setPortName(const QString &name)
{
    Q_D(QSerialPort);
    d->systemLocation = QSerialPortInfoPrivate::portNameToSystemLocation(name);
}

/*!
    Sets the port stored in the serial port info instance \a serialPortInfo.

    \sa portName(), QSerialPortInfo
*/
void QSerialPort::setPort(const QSerialPortInfo &serialPortInfo)
{
    Q_D(QSerialPort);
    d->systemLocation = serialPortInfo.systemLocation();
}

/*!
    Returns the name set by setPort() or passed to the QSerialPort constructor.
    This name is short, i.e. it is extracted and converted from the internal
    variable system location of the device. The conversion algorithm is
    platform specific:
    \table
    \header
        \li Platform
        \li Brief Description
    \row
        \li Windows
        \li Removes the prefix "\\\\.\\" or "//./" from the system location
           and returns the remainder of the string.
    \row
        \li Unix, BSD
        \li Removes the prefix "/dev/" from the system location
           and returns the remainder of the string.
    \endtable

    \sa setPort(), QSerialPortInfo::portName()
*/
QString QSerialPort::portName() const
{
    Q_D(const QSerialPort);
    return QSerialPortInfoPrivate::portNameFromSystemLocation(d->systemLocation);
}

/*!
    \reimp

    Opens the serial port using OpenMode \a mode, and then returns \c true if
    successful; otherwise returns \c false and sets an error code which can be
    obtained by calling the error() method.

    \note The method returns \c false if opening the port is successful, but could
    not set any of the port settings successfully. In that case, the port is
    closed automatically not to leave the port around with incorrect settings.

    \warning The \a mode has to be QIODeviceBase::ReadOnly, QIODeviceBase::WriteOnly,
    or QIODeviceBase::ReadWrite. Other modes are unsupported.

    \sa QIODeviceBase::OpenMode, setPort()
*/
bool QSerialPort::open(OpenMode mode)
{
    Q_D(QSerialPort);

    if (isOpen()) {
        d->setError(QSerialPortErrorInfo(QSerialPort::OpenError));
        return false;
    }

    // Define while not supported modes.
    static const OpenMode unsupportedModes = Append | Truncate | Text | Unbuffered;
    if ((mode & unsupportedModes) || mode == NotOpen) {
        d->setError(QSerialPortErrorInfo(QSerialPort::UnsupportedOperationError, tr("Unsupported open mode")));
        return false;
    }

    clearError();
    if (!d->open(mode))
        return false;

    QIODevice::open(mode);
    return true;
}

/*!
    \reimp

    \note The serial port has to be open before trying to close it; otherwise
    sets the NotOpenError error code.

    \sa QIODevice::close()
*/
void QSerialPort::close()
{
    Q_D(QSerialPort);
    if (!isOpen()) {
        d->setError(QSerialPortErrorInfo(QSerialPort::NotOpenError));
        return;
    }

    d->close();
    d->isBreakEnabled.setValue(false);
    QIODevice::close();
}

/*!
    \property QSerialPort::baudRate
    \brief the data baud rate for the desired direction

    If the setting is successful or set before opening the port, returns \c true;
    otherwise returns \c false and sets an error code which can be obtained by
    accessing the value of the QSerialPort::error property. To set the baud
    rate, use the enumeration QSerialPort::BaudRate or any positive qint32
    value.

    \note If the setting is set before opening the port, the actual serial port
    setting is done automatically in the \l{QSerialPort::open()} method right
    after that the opening of the port succeeds.

    \warning Setting the AllDirections flag is supported on all platforms.
    Windows supports only this mode.

    \warning Returns equal baud rate in any direction on Windows.

    The default value is Baud9600, i.e. 9600 bits per second.
*/
bool QSerialPort::setBaudRate(qint32 baudRate, Directions directions)
{
    Q_D(QSerialPort);

    if (!isOpen() || d->setBaudRate(baudRate, directions)) {
        if (directions & QSerialPort::Input) {
            if (d->inputBaudRate != baudRate)
                d->inputBaudRate = baudRate;
            else
                directions &= ~QSerialPort::Input;
        }

        if (directions & QSerialPort::Output) {
            if (d->outputBaudRate != baudRate)
                d->outputBaudRate = baudRate;
            else
                directions &= ~QSerialPort::Output;
        }

        if (directions)
            emit baudRateChanged(baudRate, directions);

        return true;
    }

    return false;
}

qint32 QSerialPort::baudRate(Directions directions) const
{
    Q_D(const QSerialPort);
    if (directions == QSerialPort::AllDirections)
        return d->inputBaudRate == d->outputBaudRate ?
                    d->inputBaudRate : -1;
    return directions & QSerialPort::Input ? d->inputBaudRate : d->outputBaudRate;
}

/*!
    \fn void QSerialPort::baudRateChanged(qint32 baudRate, Directions directions)

    This signal is emitted after the baud rate has been changed. The new baud
    rate is passed as \a baudRate and directions as \a directions.

    \sa QSerialPort::baudRate
*/

/*!
    \property QSerialPort::dataBits
    \brief the data bits in a frame

    If the setting is successful or set before opening the port, returns
    \c true; otherwise returns \c false and sets an error code which can be obtained
    by accessing the value of the QSerialPort::error property.

    \note If the setting is set before opening the port, the actual serial port
    setting is done automatically in the \l{QSerialPort::open()} method right
    after that the opening of the port succeeds.

    The default value is Data8, i.e. 8 data bits.
*/
bool QSerialPort::setDataBits(DataBits dataBits)
{
    Q_D(QSerialPort);
    d->dataBits.removeBindingUnlessInWrapper();
    const auto currentDataBits = d->dataBits.valueBypassingBindings();
    if (!isOpen() || d->setDataBits(dataBits)) {
        d->dataBits.setValueBypassingBindings(dataBits);
        if (currentDataBits != dataBits) {
            d->dataBits.notify();
            emit dataBitsChanged(dataBits);
        }
        return true;
    }
    return false;
}

QSerialPort::DataBits QSerialPort::dataBits() const
{
    Q_D(const QSerialPort);
    return d->dataBits;
}

QBindable<QSerialPort::DataBits> QSerialPort::bindableDataBits()
{
    return &d_func()->dataBits;
}

/*!
    \fn void QSerialPort::dataBitsChanged(DataBits dataBits)

    This signal is emitted after the data bits in a frame has been changed. The
    new data bits in a frame is passed as \a dataBits.

    \sa QSerialPort::dataBits
*/


/*!
    \property QSerialPort::parity
    \brief the parity checking mode

    If the setting is successful or set before opening the port, returns \c true;
    otherwise returns \c false and sets an error code which can be obtained by
    accessing the value of the QSerialPort::error property.

    \note If the setting is set before opening the port, the actual serial port
    setting is done automatically in the \l{QSerialPort::open()} method right
    after that the opening of the port succeeds.

    The default value is NoParity, i.e. no parity.
*/
bool QSerialPort::setParity(Parity parity)
{
    Q_D(QSerialPort);
    d->parity.removeBindingUnlessInWrapper();
    const auto currentParity = d->parity.valueBypassingBindings();
    if (!isOpen() || d->setParity(parity)) {
        d->parity.setValueBypassingBindings(parity);
        if (currentParity != parity) {
            d->parity.notify();
            emit parityChanged(parity);
        }
        return true;
    }
    return false;
}

QSerialPort::Parity QSerialPort::parity() const
{
    Q_D(const QSerialPort);
    return d->parity;
}

QBindable<QSerialPort::Parity> QSerialPort::bindableParity()
{
    return &d_func()->parity;
}

/*!
    \fn void QSerialPort::parityChanged(Parity parity)

    This signal is emitted after the parity checking mode has been changed. The
    new parity checking mode is passed as \a parity.

    \sa QSerialPort::parity
*/

/*!
    \property QSerialPort::stopBits
    \brief the number of stop bits in a frame

    If the setting is successful or set before opening the port, returns \c true;
    otherwise returns \c false and sets an error code which can be obtained by
    accessing the value of the QSerialPort::error property.

    \note If the setting is set before opening the port, the actual serial port
    setting is done automatically in the \l{QSerialPort::open()} method right
    after that the opening of the port succeeds.

    The default value is OneStop, i.e. 1 stop bit.
*/
bool QSerialPort::setStopBits(StopBits stopBits)
{
    Q_D(QSerialPort);
    d->stopBits.removeBindingUnlessInWrapper();
    const auto currentStopBits = d->stopBits.valueBypassingBindings();
    if (!isOpen() || d->setStopBits(stopBits)) {
        d->stopBits.setValueBypassingBindings(stopBits);
        if (currentStopBits != stopBits) {
            d->stopBits.notify();
            emit stopBitsChanged(stopBits);
        }
        return true;
    }
    return false;
}

QSerialPort::StopBits QSerialPort::stopBits() const
{
    Q_D(const QSerialPort);
    return d->stopBits;
}

QBindable<bool> QSerialPort::bindableStopBits()
{
    return &d_func()->stopBits;
}

/*!
    \fn void QSerialPort::stopBitsChanged(StopBits stopBits)

    This signal is emitted after the number of stop bits in a frame has been
    changed. The new number of stop bits in a frame is passed as \a stopBits.

    \sa QSerialPort::stopBits
*/

/*!
    \property QSerialPort::flowControl
    \brief the desired flow control mode

    If the setting is successful or set before opening the port, returns \c true;
    otherwise returns \c false and sets an error code which can be obtained by
    accessing the value of the QSerialPort::error property.

    \note If the setting is set before opening the port, the actual serial port
    setting is done automatically in the \l{QSerialPort::open()} method right
    after that the opening of the port succeeds.

    The default value is NoFlowControl, i.e. no flow control.
*/
bool QSerialPort::setFlowControl(FlowControl flowControl)
{
    Q_D(QSerialPort);
    d->flowControl.removeBindingUnlessInWrapper();
    const auto currentFlowControl = d->flowControl.valueBypassingBindings();
    if (!isOpen() || d->setFlowControl(flowControl)) {
        d->flowControl.setValueBypassingBindings(flowControl);
        if (currentFlowControl != flowControl) {
            d->flowControl.notify();
            emit flowControlChanged(flowControl);
        }
        return true;
    }
    return false;
}

QSerialPort::FlowControl QSerialPort::flowControl() const
{
    Q_D(const QSerialPort);
    return d->flowControl;
}

QBindable<QSerialPort::FlowControl> QSerialPort::bindableFlowControl()
{
    return &d_func()->flowControl;
}

/*!
    \fn void QSerialPort::flowControlChanged(FlowControl flow)

    This signal is emitted after the flow control mode has been changed. The
    new flow control mode is passed as \a flow.

    \sa QSerialPort::flowControl
*/

/*!
    \property QSerialPort::dataTerminalReady
    \brief the state (high or low) of the line signal DTR

    Returns \c true on success, \c false otherwise.
    If the flag is \c true then the DTR signal is set to high; otherwise low.

    \note The serial port has to be open before trying to set or get this
    property; otherwise \c false is returned and the error code is set to
    NotOpenError.

    \sa pinoutSignals()
*/
bool QSerialPort::setDataTerminalReady(bool set)
{
    Q_D(QSerialPort);

    if (!isOpen()) {
        d->setError(QSerialPortErrorInfo(QSerialPort::NotOpenError));
        qWarning("%s: device not open", Q_FUNC_INFO);
        return false;
    }

    const bool dataTerminalReady = isDataTerminalReady();
    const bool retval = d->setDataTerminalReady(set);
    if (retval && (dataTerminalReady != set))
        emit dataTerminalReadyChanged(set);

    return retval;
}

bool QSerialPort::isDataTerminalReady()
{
    Q_D(QSerialPort);
    return d->pinoutSignals() & QSerialPort::DataTerminalReadySignal;
}

/*!
    \fn void QSerialPort::dataTerminalReadyChanged(bool set)

    This signal is emitted after the state (high or low) of the line signal DTR
    has been changed. The new the state (high or low) of the line signal DTR is
    passed as \a set.

    \sa QSerialPort::dataTerminalReady
*/

/*!
    \property QSerialPort::requestToSend
    \brief the state (high or low) of the line signal RTS

    Returns \c true on success, \c false otherwise.
    If the flag is \c true then the RTS signal is set to high; otherwise low.

    \note The serial port has to be open before trying to set or get this
    property; otherwise \c false is returned and the error code is set to
    NotOpenError.

    \note An attempt to control the RTS signal in the HardwareControl mode
    will fail with error code set to UnsupportedOperationError, because
    the signal is automatically controlled by the driver.

    \sa pinoutSignals()
*/
bool QSerialPort::setRequestToSend(bool set)
{
    Q_D(QSerialPort);

    if (!isOpen()) {
        d->setError(QSerialPortErrorInfo(QSerialPort::NotOpenError));
        qWarning("%s: device not open", Q_FUNC_INFO);
        return false;
    }

    if (d->flowControl == QSerialPort::HardwareControl) {
        d->setError(QSerialPortErrorInfo(QSerialPort::UnsupportedOperationError));
        return false;
    }

    const bool requestToSend = isRequestToSend();
    const bool retval = d->setRequestToSend(set);
    if (retval && (requestToSend != set))
        emit requestToSendChanged(set);

    return retval;
}

bool QSerialPort::isRequestToSend()
{
    Q_D(QSerialPort);
    return d->pinoutSignals() & QSerialPort::RequestToSendSignal;
}

/*!
    \fn void QSerialPort::requestToSendChanged(bool set)

    This signal is emitted after the state (high or low) of the line signal RTS
    has been changed. The new the state (high or low) of the line signal RTS is
    passed as \a set.

    \sa QSerialPort::requestToSend
*/

/*!
    Returns the state of the line signals in a bitmap format.

    From this result, it is possible to allocate the state of the
    desired signal by applying a mask "AND", where the mask is
    the desired enumeration value from QSerialPort::PinoutSignals.

    \note This method performs a system call, thus ensuring that the line signal
    states are returned properly. This is necessary when the underlying
    operating systems cannot provide proper notifications about the changes.

    \note The serial port has to be open before trying to get the pinout
    signals; otherwise returns NoSignal and sets the NotOpenError error code.

    \sa QSerialPort::dataTerminalReady, QSerialPort::requestToSend
*/
QSerialPort::PinoutSignals QSerialPort::pinoutSignals()
{
    Q_D(QSerialPort);

    if (!isOpen()) {
        d->setError(QSerialPortErrorInfo(QSerialPort::NotOpenError));
        qWarning("%s: device not open", Q_FUNC_INFO);
        return QSerialPort::NoSignal;
    }

    return d->pinoutSignals();
}

/*!
    This function writes as much as possible from the internal write
    buffer to the underlying serial port without blocking. If any data
    was written, this function returns \c true; otherwise returns \c false.

    Call this function for sending the buffered data immediately to the serial
    port. The number of bytes successfully written depends on the operating
    system. In most cases, this function does not need to be called, because the
    QSerialPort class will start sending data automatically once control is
    returned to the event loop. In the absence of an event loop, call
    waitForBytesWritten() instead.

    \note The serial port has to be open before trying to flush any buffered
    data; otherwise returns \c false and sets the NotOpenError error code.

    \sa write(), waitForBytesWritten()
*/
bool QSerialPort::flush()
{
    Q_D(QSerialPort);

    if (!isOpen()) {
        d->setError(QSerialPortErrorInfo(QSerialPort::NotOpenError));
        qWarning("%s: device not open", Q_FUNC_INFO);
        return false;
    }

    return d->flush();
}

/*!
    Discards all characters from the output or input buffer, depending on
    given directions \a directions. This includes clearing the internal class buffers and
    the UART (driver) buffers. Also terminate pending read or write operations.
    If successful, returns \c true; otherwise returns \c false.

    \note The serial port has to be open before trying to clear any buffered
    data; otherwise returns \c false and sets the NotOpenError error code.
*/
bool QSerialPort::clear(Directions directions)
{
    Q_D(QSerialPort);

    if (!isOpen()) {
        d->setError(QSerialPortErrorInfo(QSerialPort::NotOpenError));
        qWarning("%s: device not open", Q_FUNC_INFO);
        return false;
    }

    if (directions & Input)
        d->buffer.clear();
    if (directions & Output)
        d->writeBuffer.clear();
    return d->clear(directions);
}

/*!
    \property QSerialPort::error
    \brief the error status of the serial port

    The I/O device status returns an error code. For example, if open()
    returns \c false, or a read/write operation returns \c -1, this property can
    be used to figure out the reason why the operation failed.

    The error code is set to the default QSerialPort::NoError after a call to
    clearError()
*/
QSerialPort::SerialPortError QSerialPort::error() const
{
    Q_D(const QSerialPort);
    return d->error;
}

void QSerialPort::clearError()
{
    Q_D(QSerialPort);
    d->setError(QSerialPortErrorInfo(QSerialPort::NoError));
}

QBindable<QSerialPort::SerialPortError> QSerialPort::bindableError() const
{
    return &d_func()->error;
}

/*!
    \fn void QSerialPort::errorOccurred(SerialPortError error)
    \since 5.8

    This signal is emitted when an error occurs in the serial port.
    The specified \a error describes the type of error that occurred.

    \sa QSerialPort::error
*/

/*!
    Returns the size of the internal read buffer. This limits the
    amount of data that the client can receive before calling the read()
    or readAll() methods.

    A read buffer size of \c 0 (the default) means that the buffer has
    no size limit, ensuring that no data is lost.

    \sa setReadBufferSize(), read()
*/
qint64 QSerialPort::readBufferSize() const
{
    Q_D(const QSerialPort);
    return d->readBufferMaxSize;
}

/*!
    Sets the size of QSerialPort's internal read buffer to be \a
    size bytes.

    If the buffer size is limited to a certain size, QSerialPort
    will not buffer more than this size of data. The special case of a buffer
    size of \c 0 means that the read buffer is unlimited and all
    incoming data is buffered. This is the default.

    This option is useful if the data is only read at certain points
    in time (for instance in a real-time streaming application) or if the serial
    port should be protected against receiving too much data, which may
    eventually cause the application to run out of memory.

    \sa readBufferSize(), read()
*/
void QSerialPort::setReadBufferSize(qint64 size)
{
    Q_D(QSerialPort);
    d->readBufferMaxSize = size;
    if (isReadable())
        d->startAsyncRead();
}

/*!
    \reimp

    Always returns \c true. The serial port is a sequential device.
*/
bool QSerialPort::isSequential() const
{
    return true;
}

/*!
    \reimp

    Returns the number of incoming bytes that are waiting to be read.

    \sa bytesToWrite(), read()
*/
qint64 QSerialPort::bytesAvailable() const
{
    return QIODevice::bytesAvailable();
}

/*!
    \reimp

    Returns the number of bytes that are waiting to be written. The
    bytes are written when control goes back to the event loop or
    when flush() is called.

    \sa bytesAvailable(), flush()
*/
qint64 QSerialPort::bytesToWrite() const
{
    qint64 pendingBytes = QIODevice::bytesToWrite();
    return pendingBytes;
}

/*!
    \reimp

    Returns \c true if a line of data can be read from the serial port;
    otherwise returns \c false.

    \sa readLine()
*/
bool QSerialPort::canReadLine() const
{
    return QIODevice::canReadLine();
}

/*!
    \reimp

    This function blocks until new data is available for reading and the
    \l{QIODevice::}{readyRead()} signal has been emitted. The function
    will timeout after \a msecs milliseconds; the default timeout is
    30000 milliseconds. If \a msecs is -1, this function will not time out.

    The function returns \c true if the readyRead() signal is emitted and
    there is new data available for reading; otherwise it returns \c false
    (if an error occurred or the operation timed out).

    \sa waitForBytesWritten()
*/
bool QSerialPort::waitForReadyRead(int msecs)
{
    Q_D(QSerialPort);
    return d->waitForReadyRead(msecs);
}

/*!
    \fn Handle QSerialPort::handle() const
    \since 5.2

    If the platform is supported and the serial port is open, returns the native
    serial port handle; otherwise returns \c -1.

    \warning This function is for expert use only; use it at your own risk.
    Furthermore, this function carries no compatibility promise between minor
    Qt releases.
*/

/*!
    \reimp

    This function blocks until at least one byte has been written to the serial
    port and the \l{QIODevice::}{bytesWritten()} signal has been emitted. The
    function will timeout after \a msecs milliseconds; the default timeout is
    30000 milliseconds. If \a msecs is -1, this function will not time out.

    The function returns \c true if the bytesWritten() signal is emitted; otherwise
    it returns \c false (if an error occurred or the operation timed out).
*/
bool QSerialPort::waitForBytesWritten(int msecs)
{
    Q_D(QSerialPort);
    return d->waitForBytesWritten(msecs);
}

/*!
    \property QSerialPort::breakEnabled
    \since 5.5
    \brief the state of the transmission line in break

    Returns \c true on success, \c false otherwise.
    If the flag is \c true then the transmission line is in break state;
    otherwise is in non-break state.

    \note The serial port has to be open before trying to set or get this
    property; otherwise returns \c false and sets the NotOpenError error code.
    This is a bit unusual as opposed to the regular Qt property settings of
    a class. However, this is a special use case since the property is set
    through the interaction with the kernel and hardware. Hence, the two
    scenarios cannot be completely compared to each other.
*/
bool QSerialPort::setBreakEnabled(bool set)
{
    Q_D(QSerialPort);
    d->isBreakEnabled.removeBindingUnlessInWrapper();
    const auto currentSet = d->isBreakEnabled.valueBypassingBindings();
    if (isOpen()) {
        if (d->setBreakEnabled(set)) {
            d->isBreakEnabled.setValueBypassingBindings(set);
            if (currentSet != set) {
                d->isBreakEnabled.notify();
                emit breakEnabledChanged(set);
            }
            return true;
        }
    } else {
        d->setError(QSerialPortErrorInfo(QSerialPort::NotOpenError));
        qWarning("%s: device not open", Q_FUNC_INFO);
    }
    return false;
}

bool QSerialPort::isBreakEnabled() const
{
    Q_D(const QSerialPort);
    return d->isBreakEnabled;
}

QBindable<bool> QSerialPort::bindableIsBreakEnabled()
{
    return &d_func()->isBreakEnabled;
}

/*!
    \reimp

    \omit
    This function does not really read anything, as we use QIODevicePrivate's
    buffer. The buffer will be read inside of QIODevice before this
    method will be called.
    \endomit
*/
qint64 QSerialPort::readData(char *data, qint64 maxSize)
{
    Q_UNUSED(data);
    Q_UNUSED(maxSize);

    // In any case we need to start the notifications if they were
    // disabled by the read handler. If enabled, next call does nothing.
    d_func()->startAsyncRead();

    // return 0 indicating there may be more data in the future
    return qint64(0);
}

/*!
    \reimp
*/
qint64 QSerialPort::readLineData(char *data, qint64 maxSize)
{
    return QIODevice::readLineData(data, maxSize);
}

/*!
    \reimp
*/
qint64 QSerialPort::writeData(const char *data, qint64 maxSize)
{
    Q_D(QSerialPort);
    return d->writeData(data, maxSize);
}

QT_END_NAMESPACE

#include "moc_qserialport.cpp"
