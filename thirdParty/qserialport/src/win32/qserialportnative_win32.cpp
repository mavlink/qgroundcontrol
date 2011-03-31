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
#include "qwincommevtnotifier.h"
#include "commdcbhelper.h"
#include "qserialport.h"
#include "qserialportnative.h"

namespace TNX {

/*!
  Constructs a QSerialPortNative object with the given \a port name and \a parent.
*/
QSerialPortNative::QSerialPortNative(const QString &portName, QObject *parent)
  : QIODevice(parent), portName_(portName), fileDescriptor_(0), portHelper_(NULL),
    readNotifier_(NULL)
{
#ifdef __MINGW32__
  ZeroMemory(&ovRead_, sizeof(OVERLAPPED));
  ZeroMemory(&ovWrite_, sizeof(OVERLAPPED));
#else
  SecureZeroMemory(&ovRead_, sizeof(OVERLAPPED));
  SecureZeroMemory(&ovWrite_, sizeof(OVERLAPPED));
#endif
}

/*!
  Constructs a QSerialPortNative object with the given \a port name, \a settings and \a parent.
*/
QSerialPortNative::QSerialPortNative(const QString &portName, const QPortSettings &settings, QObject *parent)
  : QIODevice(parent), portName_(portName), portSettings_(settings), fileDescriptor_(0), portHelper_(NULL),
    readNotifier_(NULL)
{
#ifdef __MINGW32__
  ZeroMemory(&ovRead_, sizeof(OVERLAPPED));
  ZeroMemory(&ovWrite_, sizeof(OVERLAPPED));
#else
  SecureZeroMemory(&ovRead_, sizeof(OVERLAPPED));
  SecureZeroMemory(&ovWrite_, sizeof(OVERLAPPED));
#endif
}

/*!
*/
bool QSerialPortNative::open_impl()
{
  // This is needed if comport number
  // is higher than 9

  if ( "//./" != portName_.left(4) )
    portName_.insert(0, "//./");

  fileDescriptor_ = CreateFileA(portName_.toLatin1(), GENERIC_READ|GENERIC_WRITE,
                                0, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);

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

  // Prepare overlapped io structure for Read/Write operations

  ovRead_.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL); // manual-reset event and initial state is nonsignaled
  Q_ASSERT(ovRead_.hEvent != NULL);
  ovWrite_.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);// manual-reset event and initial state is nonsignaled
  Q_ASSERT(ovWrite_.hEvent != NULL);

  return true;
}

/*!
*/
void QSerialPortNative::close_impl()
{
  // Close Read/Write overlapped io event handles
  CloseHandle(ovRead_.hEvent);
  CloseHandle(ovWrite_.hEvent);

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
  int result = -1;
  OVERLAPPED osStatus;
  DWORD dwMask;
  DWORD orgFlags;

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

  {
#ifdef __MINGW32__
    ZeroMemory(&osStatus, sizeof(OVERLAPPED));
#else
    SecureZeroMemory(&osStatus, sizeof(OVERLAPPED));
#endif
    osStatus.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    Q_ASSERT(osStatus.hEvent != NULL);

    if ( !WaitCommEvent(fileDescriptor_, &dwMask, &osStatus) ) {
      if ( GetLastError() == ERROR_IO_PENDING ) {
        // Wait until the operation is completed
        DWORD dwRes = WaitForSingleObject(osStatus.hEvent, (timeout == -1 ? INFINITE : timeout));
        switch (dwRes)
        {
          // Event occurred
          case WAIT_OBJECT_0:
            if ( !GetOverlappedResult(fileDescriptor_, &osStatus, (LPDWORD)&dwRes, FALSE) ) {
              // An error occurred in the overlapped operation;  call GetLastError to find out what it is.
              result = -1;
            }
            else {
              if ( dwMask & EV_RXCHAR )
                result = 1; // data pending
              else if ( dwMask & EV_ERR )
                result = -1;
              else {
                qWarning() << QString("QSerialPortNative::waitForReadyRead(%1): unexpected event returned.")
                           .arg(portName_);
                result = 0; // this path is not expected
              }
            }
            break;

          case WAIT_TIMEOUT:
            result = 0;
            break;

          // Error in the WaitForSingleObject;
          // This indicates a problem with the OVERLAPPED structure's
          // event handle.
          default:
            result = -1;
        }// switch
      }
      else {
        // Error while waiting for data
        result = -1;
      }
    }
    else {
      // WaitCommEvent returned immediately.
      result = 1;
    }

    CloseHandle(osStatus.hEvent);
  }

  // reset event mask for event notifier thread
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
  ResetEvent(ovRead_.hEvent);

  qint64 numBytes = 0LL;
  if ( !ReadFile(fileDescriptor_, (void*)data, (DWORD)maxlen, (LPDWORD)&numBytes, &ovRead_) ) {
    if ( GetLastError() == ERROR_IO_PENDING ) {
      // Wait until read operation is completed
      DWORD dwRes = WaitForSingleObject(ovRead_.hEvent, INFINITE);
      switch (dwRes) 
      {
        // Event occurred
        case WAIT_OBJECT_0:
          if ( !GetOverlappedResult(fileDescriptor_, &ovRead_, (LPDWORD)&numBytes, FALSE) ) {
            // An error occurred in the overlapped operation;
            // call GetLastError to find out what it was
            numBytes = -1LL;
          }
          break;
  
        // Error in the WaitForSingleObject;
        // This indicates a problem with the OVERLAPPED structure's
        // event handle.
        default:
          numBytes = -1LL;
      }// switch
    }
    else {
      // Error while reading data
      numBytes = -1LL;
    }
  }//endif

  return numBytes;
}

/*!
*/
qint64 QSerialPortNative::writeData_impl(const char *data, qint64 len)
{
  ResetEvent(ovWrite_.hEvent);

  qint64 numBytes = 0LL;
  if ( !WriteFile(fileDescriptor_, (void*)data, (DWORD)len, (DWORD*)&numBytes, &ovWrite_) ) {
    if ( GetLastError() == ERROR_IO_PENDING ) {
      // Wait until write operation is completed
      DWORD dwRes = WaitForSingleObject(ovWrite_.hEvent, INFINITE);
      switch (dwRes) 
      {
        // Event occurred
        case WAIT_OBJECT_0:
          if ( !GetOverlappedResult(fileDescriptor_, &ovWrite_, (LPDWORD)&numBytes, FALSE) ) {
            // An error occurred in the overlapped operation;
            // call GetLastError to find out what it was
            numBytes = -1LL;
          }
          break;
  
        // Error in the WaitForSingleObject;
        // This indicates a problem with the OVERLAPPED structure's
        // event handle.
        default:
          numBytes = -1LL;
      }// switch
    }
    else {
      // Error while writing data
      numBytes = -1LL;
    }
  }
   
  return numBytes;
}

} // namespace
