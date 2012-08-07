/*
 * Unofficial Qt Serial Port Library
 *
 * Copyright (c) 2010 Inbiza Systems Inc. All rights reserved.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>
 *
 * author labs@inbiza.com
 */

#include <QDebug>
#include "qserialport.h"

#ifdef TNX_POSIX_SERIAL_PORT
# include <QSocketNotifier>
# include <fcntl.h>
# include <sys/ioctl.h>
# include <errno.h>
# include "../posix/termioshelper.h"
#elif defined(TNX_WINDOWS_SERIAL_PORT)
# include "win32/commdcbhelper.h"
# include "win32/qwincommevtnotifier.h"
#endif

namespace TNX {

/*!
  Constructs a QSerialPort object with the given \a port name and \a parent.
*/
QSerialPort::QSerialPort(const QString &portName, QObject *parent)
  : QSerialPortNative(portName, parent), pendingByteCount_(0),
    doNotify_(false), readNotifyThreshold_(1)
{
}

/*!
  Constructs a QSerialPort object with the given \a port name, \a settings and \a parent.
*/
QSerialPort::QSerialPort(const QString &portName, const QPortSettings &settings, QObject *parent)
  : QSerialPortNative(portName, settings, parent), pendingByteCount_(0), doNotify_(false),
    readNotifyThreshold_(1)
{
}

/*!
  Constructs a QSerialPort object with the given \a port name, \a settings and \a parent.
*/
QSerialPort::QSerialPort(const QString &portName, const QString &settings, QObject *parent)
  : QSerialPortNative(portName, settings, parent), pendingByteCount_(0), doNotify_(false),
    readNotifyThreshold_(1)
{
}

QSerialPort::~QSerialPort()
{
  close();
}

/*!

 */
bool QSerialPort::setPortSettings(const QPortSettings &portSettings)
{
  bool result = false;
  if ( isOpen() ) {
    portHelper_->setBaudRate(portSettings.baudRate());
    portHelper_->setDataBits(portSettings.dataBits());
    portHelper_->setStopBits(portSettings.stopBits());
    portHelper_->setFlowControl(portSettings.flowControl());
    portHelper_->setParity(portSettings.parity());

    result = portHelper_->applyChanges(PortAttrOnlyAppTy);
  }
  else {
    portSettings_ = portSettings;
    result = true;
  }

  if ( !result )
    setErrorString(lastErrorText_impl());
  else
    portSettings_ = portSettings;

  return result;
}

/*! \reimp
*/
bool QSerialPort::isSequential() const
{
  return true;
}

/*!
  Opens the I/O device for the serial port.
*/
bool QSerialPort::open(OpenMode mode)
{
  if ( isOpen() )
    return true;

  // mode argument is ignored intentionally

  if ( portName_.isEmpty() ) {
    setErrorString("Port name is empty.");
    return false;
  }

  if ( !open_impl() ) {
    qCritical() << QString("QSerialPort::open(%1) failed: %2(Err #%3)")
                    .arg(portName_)
                    .arg(lastErrorText_impl())
                    .arg(lastError_impl());
    setErrorString(lastErrorText_impl());

    // we don't know exactly what state this object in before the error.
    // make sure that we don't leave any unused object hanging in the memory.
    delete portHelper_;
    delete readNotifier_;
    readNotifier_ = NULL;
    portHelper_ = NULL;

    return false;
  }

  // open QIODevice

  setOpenMode(mode | QIODevice::Unbuffered);

  qDebug() << "Serial port \"" << portName_ << "\" is successfully opened";

  // Set port attributes

  setPortSettings(portSettings_);

  // Set communication timing values

  setCommTimeouts(commTimeouts_);

  if ( !portHelper_->applyChanges() ) {
    qCritical() << QString("QSerialPort::open(%1) failed when setting up the port: %2(Err #%3)")
                    .arg(portName_)
                    .arg(lastErrorText_impl())
                    .arg(lastError_impl());
    setErrorString(lastErrorText_impl());
   }

  return true;
}

/*!
  Closes the I/O device for the serial port.

  See QIODevice::close() for a description of the actions that occur when an I/O
  device is closed.
*/
void QSerialPort::close()
{
  if ( isOpen() ) {

    delete portHelper_;
    portHelper_ = NULL;
    
    // disconnect the ReadNotifier

    if ( readNotifier_ ) {
      disconnect(readNotifier_, SIGNAL(activated(int)), this, SLOT(onDataReceived()));
      delete readNotifier_;
      readNotifier_ = NULL;
    }

    close_impl();
    
    // Close the device

    QIODevice::close();

    qDebug() << "Serial port \"" << portName_ << "\" is successfully closed";
  }
}

/*! \reimp
*/
qint64 QSerialPort::pos() const
{
  return 0;
}

/*! \reimp
*/
qint64 QSerialPort::size() const
{
  return bytesAvailable();
}

/*! \reimp
*/
bool QSerialPort::seek(qint64 /*pos*/)
{
  return false;
}

/*! \reimp
*/
bool QSerialPort::atEnd() const
{
  return true;
}

/*! \reimp
*/
qint64 QSerialPort::bytesAvailable() const
{
  qint64 nBytes = bytesAvailable_impl();
  if ( nBytes == -1LL ) {
    qDebug() << QString("QSerialPort::bytesAvailable(%1) failed: %2(Err #%3)")
                    .arg(portName_)
                    .arg(lastErrorText_impl())
                    .arg(lastError_impl());
    const_cast<QSerialPort*>(this)->setErrorString(lastErrorText_impl());
    return -1LL;
  }

  return QIODevice::bytesAvailable() + nBytes;
}

/*! \reimp
*/
qint64 QSerialPort::bytesToWrite() const
{
  return 0LL;
}

/*! \reimp
*/
bool QSerialPort::canReadLine() const
{
  return false;
}

/*! \reimp
 
 */
bool QSerialPort::waitForReadyRead(int timeout)
{
  bool ret = false;
  readNotifier_->setEnabled(false);

  int result = waitForReadyRead_impl((timeout < 0 ? -1 : timeout));
  if ( result > 0 ) {
    ret = true;
    emit readyRead();
  }
  else if ( result == 0 )
   ; // Timeout occurred. Do nothing, return false
  else {
    // Error occurred. Logs the error and return false
    qDebug() << QString("QSerialPort::waitForReadyRead(%1) failed: %2(Err #%3)")
                    .arg(portName_)
                    .arg(lastErrorText_impl())
                    .arg(lastError_impl());
    setErrorString(lastErrorText_impl());
  }

  readNotifier_->setEnabled(true);
  return ret;
}

/*! \reimp

*/
bool QSerialPort::waitForBytesWritten(int /*msecs*/)
{
  return true;
}

/*! \reimp
*/
qint64 QSerialPort::readData(char *data, qint64 maxlen)
{
  Q_CHECK_PTR(data);

  qint64 bytesRead = readData_impl(data, maxlen);

  if ( bytesRead == -1LL ) {
    qDebug() << QString("QSerialPort::readData(%1) failed: %2(Err #%3)")
                    .arg(portName_)
                    .arg(lastErrorText_impl())
                    .arg(lastError_impl());
    setErrorString(lastErrorText_impl());
  }
      
  if ( bytesRead > 0LL ) {
    doNotify_ = false;

    // the number of bytes consumed can be higher than the pending byte count
    pendingByteCount_ = (pendingByteCount_ - bytesRead) < 0LL ? 0LL :
                          (pendingByteCount_ - bytesRead);
  }

  readNotifier_->setEnabled(true);
  return bytesRead;
}

/*! \reimp
*/
qint64 QSerialPort::writeData(const char *data, qint64 len)
{
  Q_CHECK_PTR(data);

  qint64 nBytes = writeData_impl(data, len);
  if ( nBytes == -1LL ) {
    qDebug() << QString("QSerialPort::writeData(%1) failed: %2(Err #%3)")
                    .arg(portName_)
                    .arg(lastErrorText_impl())
                    .arg(lastError_impl());
    setErrorString(lastErrorText_impl());
  }

  if ( 0LL == nBytes && 0LL < len ) {
    qDebug() << QString("QSerialPort::writeData(%1) - method returns no error but " \
                          "number of bytes written is zero: %2(Err #%3)")
                .arg(portName_)
                .arg(lastErrorText_impl())
                .arg(lastError_impl());
  }

  if ( nBytes > 0LL )
    emit bytesWritten(nBytes);

  return nBytes;
}

/*!

*/
void QSerialPort::onDataReceived()
{
  qint64 nBytes = bytesAvailable_impl();

  if ( -1LL == nBytes )
    return;

  if ( nBytes > pendingByteCount_ ||
       (nBytes > 0LL && nBytes == pendingByteCount_ && !doNotify_) ) {
    pendingByteCount_ = nBytes;
    doNotify_ = true;

    if ( pendingByteCount_ >= (qint64)readNotifyThreshold_ ) {
      readNotifier_->setEnabled(false);
      emit readyRead();
    }
  }
}

/*!

 */
bool QSerialPort::flushOutBuffer()
{
  bool result = flushOutBuffer_impl();
  if ( !result ) {
    qDebug() << QString("QSerialPort::flushOutBuffer(%1) failed: %2(Err #%3)")
                    .arg(portName_)
                    .arg(lastErrorText_impl())
                    .arg(lastError_impl());
    setErrorString(lastErrorText_impl());
  }
  return result;
}

/*!

*/
bool QSerialPort::flushInBuffer()
{
  bool result = flushInBuffer_impl();
  if ( !result ) {
    qDebug() << QString("QSerialPort::flushInBuffer(%1) failed: %2(Err #%3)")
                    .arg(portName_)
                    .arg(lastErrorText_impl())
                    .arg(lastError_impl());
    setErrorString(lastErrorText_impl());
  }
  else {
    pendingByteCount_ = 0LL;
    readNotifier_->setEnabled(true);
  }
  return result;
}

/*!

*/
bool QSerialPort::sendBreak(int timeout)
{
  bool result = sendBreak_impl(timeout);
  if ( !result ) {
    qDebug() << QString("QSerialPort::sendBreak(%1) failed: %2(Err #%3)")
                    .arg(portName_)
                    .arg(lastErrorText_impl())
                    .arg(lastError_impl());
    setErrorString(lastErrorText_impl());
  }
  return result;
}


CommTimeouts QSerialPort::commTimeouts()
{
  if ( !isOpen() )
    return commTimeouts_;

  Q_CHECK_PTR(portHelper_);

  CommTimeouts commtimeouts;
  bool res = portHelper_->commTimeouts(commtimeouts);
  if ( !res ) {
    qDebug() << QString("QSerialPort::commTimeouts(%1) failed: %2(Err #%3)")
                    .arg(portName_)
                    .arg(lastErrorText_impl())
                    .arg(lastError_impl());
    setErrorString(lastErrorText_impl());
  }
  else {
    commTimeouts_ = commtimeouts;
  }

  return commTimeouts_;
}

bool QSerialPort::setCommTimeouts(const CommTimeouts commtimeouts)
{
  commTimeouts_ = commtimeouts;

  if ( !isOpen() )
    return true;

  Q_CHECK_PTR(portHelper_);

  portHelper_->setCommTimeouts(commTimeouts_);
  bool result = portHelper_->applyChanges(CommTimeoutsOnlyAppTy);
  if ( !result ) {
    qDebug() << QString("QSerialPort::setCommTimouts(%1) failed: %2(Err #%3)")
                  .arg(portName_)
                  .arg(lastErrorText_impl())
                  .arg(lastError_impl());
    setErrorString(lastErrorText_impl());
  }
  return result;
}

bool QSerialPort::setCommTimeouts(CommTimeoutSchemes scheme, int timeout)
{
  if ( scheme == CtScheme_NonBlockingRead ) {
    commTimeouts_.PosixVMIN = 0;
    commTimeouts_.PosixVTIME = 0;
    commTimeouts_.Win32ReadIntervalTimeout = CommTimeouts::NoTimeout;
    commTimeouts_.Win32ReadTotalTimeoutConstant = 0;
    commTimeouts_.Win32ReadTotalTimeoutMultiplier = 0;
  }
  else if ( scheme == CtScheme_TimedRead ) {
    commTimeouts_.PosixVMIN = 0;
    // converting ms to one-tenths-of-second (sec * 0.1)
    commTimeouts_.PosixVTIME = timeout / 100;
    commTimeouts_.Win32ReadIntervalTimeout = 0;
    commTimeouts_.Win32ReadTotalTimeoutConstant = timeout;
    commTimeouts_.Win32ReadTotalTimeoutMultiplier = 0;
  }

  if ( !isOpen() )
    return true;

  Q_CHECK_PTR(portHelper_);

  portHelper_->setCommTimeouts(commTimeouts_);
  bool result = portHelper_->applyChanges(CommTimeoutsOnlyAppTy);
  if ( !result ) {
    qDebug() << QString("QSerialPort::setCommTimouts(%1, scheme: %2) failed: %3(Err #%4)")
                  .arg(portName_)
                  .arg((int)scheme)
                  .arg(lastErrorText_impl())
                  .arg(lastError_impl());
    setErrorString(lastErrorText_impl());
  }
  return result;
}

/*!

*/
QPortSettings::BaudRate QSerialPort::baudRate() const
{
  if ( !isOpen() )
    return portSettings_.baudRate();

  Q_CHECK_PTR(portHelper_);

  return portHelper_->baudRate();
}

/*!

*/
bool QSerialPort::setBaudRate(QPortSettings::BaudRate baudRate)
{
  if ( !isOpen() ) {
    portSettings_.setBaudRate(baudRate);
    return true;
  }

  Q_CHECK_PTR(portHelper_);

  bool result = false;

  portHelper_->setBaudRate(baudRate);
  if ( (result = portHelper_->applyChanges(PortAttrOnlyAppTy)) )
    portSettings_.setBaudRate(baudRate);
  else
    setErrorString(lastErrorText_impl());

  return result;
}

/*!

*/
QPortSettings::Parity QSerialPort::parity() const
{
  if ( !isOpen() )
    return portSettings_.parity();

  Q_CHECK_PTR(portHelper_);

  return portHelper_->parity();
}

/*!

*/
bool QSerialPort::setParity(QPortSettings::Parity parity)
{
  if ( !isOpen() ) {
    portSettings_.setParity(parity);
    return true;
  }

  Q_CHECK_PTR(portHelper_);

  bool result = false;

  portHelper_->setParity(parity);
  if ( (result = portHelper_->applyChanges(PortAttrOnlyAppTy)) )
    portSettings_.setParity(parity);
  else
    setErrorString(lastErrorText_impl());

  return result;
}

/*!

*/
QPortSettings::StopBits QSerialPort::stopBits() const
{
  if ( !isOpen() )
    return portSettings_.stopBits();

  Q_CHECK_PTR(portHelper_);

  return portHelper_->stopBits();
}

/*!

*/
bool QSerialPort::setStopBits(QPortSettings::StopBits stopBits)
{
  if ( !isOpen() ) {
    portSettings_.setStopBits(stopBits);
    return true;
  }

  Q_CHECK_PTR(portHelper_);

  bool result = false;

  portHelper_->setStopBits(stopBits);
  if ( (result = portHelper_->applyChanges(PortAttrOnlyAppTy)) )
    portSettings_.setStopBits(stopBits);
  else
    setErrorString(lastErrorText_impl());

  return result;
}

/*!

*/
QPortSettings::DataBits QSerialPort::dataBits() const
{
  if ( !isOpen() )
    return portSettings_.dataBits();

  Q_CHECK_PTR(portHelper_);

  return portHelper_->dataBits();
}

/*!

*/
bool QSerialPort::setDataBits(QPortSettings::DataBits dataBits)
{
  if ( !isOpen() ) {
    portSettings_.setDataBits(dataBits);
    return true;
  }

  Q_CHECK_PTR(portHelper_);

  bool result = false;

  portHelper_->setDataBits(dataBits);
  if ( (result = portHelper_->applyChanges(PortAttrOnlyAppTy)) )
    portSettings_.setDataBits(dataBits);
  else
    setErrorString(lastErrorText_impl());

  return result;
}

/*!

*/
QPortSettings::FlowControl QSerialPort::flowControl() const
{
  if ( !isOpen() )
    return portSettings_.flowControl();

  Q_CHECK_PTR(portHelper_);

  return portHelper_->flowControl();
}

/*!

*/
bool QSerialPort::setFlowControl(QPortSettings::FlowControl flowControl)
{
  if ( !isOpen() ) {
    portSettings_.setFlowControl(flowControl);
    return true;
  }

  Q_CHECK_PTR(portHelper_);

  bool result = false;

  portHelper_->setFlowControl(flowControl);
  if ( (result = portHelper_->applyChanges(PortAttrOnlyAppTy)) )
    portSettings_.setFlowControl(flowControl);
  else
    setErrorString(lastErrorText_impl());

  return result;
}


/*!

*/
bool QSerialPort::setRts(bool on)
{
  if ( !isOpen() )
    return false;

  if ( !portHelper_->setRts(on) ) {
    setErrorString(lastErrorText_impl());
    return false;
  }
  return true;
}

/*!

*/
bool QSerialPort::setDtr(bool on)
{
  if ( !isOpen() )
    return false;

  if ( !portHelper_->setDtr(on) ) {
    setErrorString(lastErrorText_impl());
    return false;
  }
  return true;
}

/*!

*/
QSerialPort::CommSignalValues QSerialPort::cts()
{
  if ( !isOpen() )
    return Signal_Unknown;

  CommSignalValues cts;
  if ( Signal_Unknown == (cts = portHelper_->cts()) )
    setErrorString(lastErrorText_impl());

  return cts;
}

/*!

*/
QSerialPort::CommSignalValues QSerialPort::dsr()
{
  if ( !isOpen() )
    return Signal_Unknown;

  CommSignalValues dsr;
  if ( Signal_Unknown == (dsr = portHelper_->dsr()) )
    setErrorString(lastErrorText_impl());

  return dsr;
}

/*!

*/
QSerialPort::CommSignalValues QSerialPort::dcd()
{
  if ( !isOpen() )
    return Signal_Unknown;

  CommSignalValues dcd;
  if ( Signal_Unknown == (dcd = portHelper_->dcd()) )
    setErrorString(lastErrorText_impl());

  return dcd;
}

/*!

*/
QSerialPort::CommSignalValues QSerialPort::ri()
{
  if ( !isOpen() )
    return Signal_Unknown;

  CommSignalValues ri;
  if ( Signal_Unknown == (ri = portHelper_->ri()) )
    setErrorString(lastErrorText_impl());

  return ri;
}

} // namespace
