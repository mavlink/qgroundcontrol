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
 *
 * @file qserialportnative_posix.cpp
 * @brief
 */

#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <QDebug>
#include <QSocketNotifier>
#include "qserialportnative.h"
#include "termioshelper.h"

namespace {
  const int kMinReadTimeout = 100;
}

namespace TNX {

/*!
  Constructs a QSerialPortNative object with the given \a port name and \a parent.
*/
QSerialPortNative::QSerialPortNative(const QString &portName, QObject *parent)
  : QIODevice(parent), portName_(portName), fileDescriptor_(0), portHelper_(NULL),
    readNotifier_(NULL)
{
}

/*!
  Constructs a QSerialPortNative object with the given \a port name, \a settings and \a parent.
*/
QSerialPortNative::QSerialPortNative(const QString &portName, const QPortSettings &settings, QObject *parent)
  : QIODevice(parent), portName_(portName), portSettings_(settings), fileDescriptor_(0), portHelper_(NULL),
    readNotifier_(NULL)
{
}

/*!
*/
bool QSerialPortNative::open_impl()
{
  // Open the serial port read/write, with no controlling terminal,
  // and don't wait for a connection.
  // The O_NONBLOCK flag also causes subsequent I/O on the device to be non-blocking.
  // See open(2) ("man 2 open") for details.

  fileDescriptor_ = ::open(qPrintable(portName_), O_RDWR | O_NOCTTY | O_NONBLOCK);
  if ( fileDescriptor_ == -1 )
    return false;

  // Note that open() follows POSIX semantics: multiple open() calls to
  // the same file will succeed unless the TIOCEXCL ioctl is issued.
  // This will prevent additional opens except by root-owned processes.
  // See tty(4) ("man 4 tty") and ioctl(2) ("man 2 ioctl") for details.

  if ( ioctl(fileDescriptor_, TIOCEXCL) == -1 )
    return false;

  // Now that the device is open, clear the O_NONBLOCK flag so
  // subsequent I/O will block.
  // See fcntl(2) ("man 2 fcntl") for details.

  if ( fcntl(fileDescriptor_, F_SETFL, 0) == -1)
    return false;

  readNotifier_ = new QSocketNotifier(fileDescriptor_, QSocketNotifier::Read, this);

   Q_CHECK_PTR(readNotifier_);

   if ( !readNotifier_ || !connect(readNotifier_, SIGNAL(activated(int)), this, SLOT(onDataReceived())) )
     qWarning() << QString("QSerialPortNative::open(%1) failed when connecting to read notifier")
                .arg(portName_);

  // create a termios helper object

  portHelper_ = new TermiosHelper(fileDescriptor_);

  Q_CHECK_PTR(portHelper_);

  return true;
}

/*!
*/
void QSerialPortNative::close_impl()
{
  ::close(fileDescriptor_);
  fileDescriptor_ = 0;
}

/*!
*/
QString QSerialPortNative::lastErrorText_impl() const
{
  return strerror(errno);
}

/*!
*/
int QSerialPortNative::lastError_impl() const
{
  return errno;
}

/*!
*/
qint64 QSerialPortNative::bytesAvailable_impl() const
{
  int nbytes;
  if ( ioctl(fileDescriptor_, FIONREAD, &nbytes) == -1 )
    return -1LL;

  return QIODevice::bytesAvailable() + (qint64)nbytes;
}

/*!
 * Blocks until new data is available for reading, or until msecs milliseconds have passed.
 * If msecs is -1, this function will not time out.
 * Returns 1 if new data is available for reading; 0 if the operation timed out or;
 * -1 if an error occurred.
 */
int QSerialPortNative::waitForReadyRead_impl(int timeout)
{
  fd_set input;
  struct timeval wait, *waitTimeout = NULL;

  FD_ZERO (&input);
  FD_SET (fileDescriptor_, &input);

  if ( timeout != -1 ) {
    wait.tv_sec = timeout/1000;
    wait.tv_usec = (timeout%1000) * 1000;
    waitTimeout = &wait;
  }

  // waitTimeout is NULL if timeout is -1, meaning no timeout

  int max_fd = fileDescriptor_ + 1;
  int num = select(max_fd, &input, NULL, NULL, waitTimeout);

  if ( num > 0 ) {
    if ( FD_ISSET(fileDescriptor_, &input) )
      return 1; // data pending.
    else {
      qWarning() << QString("QSerialPortNative::waitForReadyRead(%1): unexpected value returned from select().")
                 .arg(portName_);
      return 0; // this path is not expected
    }
  }

  return num; // error or timeout occurred.
}

/*!
*/
bool QSerialPortNative::flushInBuffer_impl()
{
  if ( tcflush(fileDescriptor_, TCIFLUSH) == -1 )
    return false;

  return true;
}

/*!
*/
bool QSerialPortNative::flushOutBuffer_impl()
{
  if ( tcflush(fileDescriptor_, TCOFLUSH) == -1 )
    return false;

  return true;
}

/*!
*/
bool QSerialPortNative::sendBreak_impl(int timeout)
{
  // Mac OSX ignores the duration parameter. Not sure what Linux does.

  if ( tcsendbreak(fileDescriptor_, (timeout*1000)) == -1 )
    return false;

  return true;
}

/*!
*/
qint64 QSerialPortNative::readData_impl(char *data, qint64 maxlen)
{
  qint64 numBytes = ::read(fileDescriptor_, data, maxlen);
  if ( numBytes == -1LL && errno != EAGAIN )
    return -1LL;

  return numBytes;
}

/*!
*/
qint64 QSerialPortNative::writeData_impl(const char *data, qint64 len)
{
  qint64 numBytes = ::write(fileDescriptor_, data, len);
  if ( numBytes == -1LL && errno != EAGAIN )
    return -1LL;

  return numBytes;
}

} // namespace
