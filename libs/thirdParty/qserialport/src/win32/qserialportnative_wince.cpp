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

// See http://msdn.microsoft.com/en-us/library/ms810467.aspx#serial_topic1
// for Win32 Serial Port programming.
// Also see the following forum http://www.codeguru.com/forum/archive/index.php/t-324660.html

#include <QDebug>
#include <QMutexLocker>
#include "qwincommevtnotifier.h"
#include "wincommevtbreaker.h"
#include "commdcbhelper.h"
#include "qserialport.h"

namespace TNX {

WinCommEvtBreaker* QSerialPortNative::waitBreakerThread_ = NULL;
QMutex* QSerialPortNative::mutex_ = new QMutex();  // this will leak once

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
  // WinCE requires ':' suffix for all comport names
  if ( portName_.right(1) != ":" )
    portName_.append(":");

  fileDescriptor_ = CreateFile(portName_.utf16(), GENERIC_READ|GENERIC_WRITE,
                                0, NULL, OPEN_EXISTING, 0, NULL );
 
  if ( INVALID_HANDLE_VALUE == fileDescriptor_ )
    return false;
  
  readNotifier_ = new QWinCommEvtNotifier(fileDescriptor_, this);

  Q_CHECK_PTR(readNotifier_);

  if ( !readNotifier_ || !connect(readNotifier_, SIGNAL(activated(int)), 
                                  this, SLOT(onDataReceived()), Qt::QueuedConnection ) ) {
    qWarning() << QString("QSerialPort::open(%1) failed when connecting to read notifier")
                    .arg(portName_);
  }
  
  // Create a dcb helper object

  portHelper_ = new CommDCBHelper(fileDescriptor_);

  Q_CHECK_PTR(portHelper_);

  return true;
}

/*!
*/
void QSerialPortNative::close_impl()
{
  // Close the serial port, and init the handle

  if ( !CloseHandle(fileDescriptor_) ) {
    qWarning() << QString("QSerialPort::close(%1) failed: %2(Err #%3)")
                    .arg(portName_)
                    .arg(lastErrorText_impl())
                    .arg(lastError_impl());
  }
  fileDescriptor_ = INVALID_HANDLE_VALUE;
}

/*!
*/
QString QSerialPortNative::lastErrorText_impl() const
{
  return CommDCBHelper::errorText(GetLastError());
}

/*!
*/
int QSerialPortNative::lastError_impl() const
{
  return GetLastError();
}

/*!
*/
qint64 QSerialPortNative::bytesAvailable_impl() const
{
  DWORD errorMask = 0;
  COMSTAT comStat;
  if ( !ClearCommError(fileDescriptor_, &errorMask, &comStat) )
    return -1LL;

  return QIODevice::bytesAvailable() + (qint64)comStat.cbInQue;
}

/*!
 * @return -1 if error, 0 if timed-out, 1 if incoming data
 */
int QSerialPortNative::waitForReadyRead_impl(int timeout)
{
  DWORD orgFlags;
  DWORD dwMask;
  int result = -1;

  readNotifier_->suspend();

  // store the current event mask.

  if ( !GetCommMask(fileDescriptor_, &orgFlags) )
    goto LExit;

  // this one is to release the notifier thread.

  if ( !SetCommMask(fileDescriptor_, 0) )
    goto LExit;


  // change the event mask to wait incoming bytes.

  if ( !SetCommMask(fileDescriptor_, EV_RXCHAR | EV_ERR) )
    goto LExit;

  // timeout == -1 means no timeout

  if ( timeout != -1 ) {
    // if it is not already done, lazily create the thread that will release
    // the WaitCommEvent() function when timeout occurs.

    if ( !waitBreakerThread_ ) {
      QMutexLocker locker(mutex_);
      if ( !waitBreakerThread_ ) {
        waitBreakerThread_ = new WinCommEvtBreaker();
        waitBreakerThread_->start();
        // wait until thread is ready to run
        waitBreakerThread_->waitUntilReady();
      }
    }

    waitBreakerThread_->startWaitTimer(fileDescriptor_, timeout);
  }
   
  if ( WaitCommEvent(fileDescriptor_, &dwMask, 0) ) {
    // Use the flags returned in dwMask to determine the reason for return
    if ( dwMask & EV_RXCHAR )
      result = 1;
    else if ( dwMask & EV_ERR )
      result = -1;
    else
      result = 0;
  }

  if ( timeout != -1 )
    waitBreakerThread_->stopWaitTimer(fileDescriptor_);

  // rollback the original event mask.

  if ( !SetCommMask(fileDescriptor_, orgFlags) )
    result = -1;

LExit:
  readNotifier_->resume();
  return result;
}

/*!
*/
bool QSerialPortNative::flushInBuffer_impl()
{
  // PURGE_RXABORT: Terminates all outstanding overlapped read operations and returns immediately, 
  // even if the read operations have not been completed.
  // PURGE_TXABORT: Terminates all outstanding overlapped write operations and returns immediately, 
  // even if the write operations have not been completed.

  // clear internal buffers
  if ( !PurgeComm(fileDescriptor_, PURGE_RXABORT | PURGE_RXCLEAR ) )
    return false;

  return true;
}

/*!
*/
bool QSerialPortNative::flushOutBuffer_impl()
{
  // PURGE_RXABORT: Terminates all outstanding overlapped read operations and returns immediately, 
  // even if the read operations have not been completed.
  // PURGE_TXABORT: Terminates all outstanding overlapped write operations and returns immediately, 
  // even if the write operations have not been completed.

  // clear internal buffers
  if ( !PurgeComm(fileDescriptor_, PURGE_TXABORT | PURGE_TXCLEAR ) ) 
    return false;

  return true;
}

/*!
*/
bool QSerialPortNative::sendBreak_impl(int timeout)
{
  // send a break for a very short time

  if ( !SetCommBreak(fileDescriptor_) )
    return false;

  Sleep(timeout); 

  // clear the break and continue

  if ( !ClearCommBreak(fileDescriptor_) )
    return false;

  return true;
}

/*!
*/
qint64 QSerialPortNative::readData_impl(char *data, qint64 maxlen)
{
  qint64 numBytes = 0LL;
  if ( !ReadFile(fileDescriptor_, (void*)data, (DWORD)maxlen, (LPDWORD)&numBytes, NULL) )
    numBytes = -1LL;
  
  return numBytes;
}

/*!
*/
qint64 QSerialPortNative::writeData_impl(const char *data, qint64 len)
{
  qint64 numBytes = 0LL;
  if ( !WriteFile(fileDescriptor_, (void*)data, (DWORD)len, (DWORD*)&numBytes, NULL) )
    numBytes = -1LL;

  return numBytes;
}

} // namespace
