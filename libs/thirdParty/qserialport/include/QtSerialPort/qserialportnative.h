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
 * @brief Declares platform dependent attributes and methods of qserialport.
 */

#ifndef TNX_QSERIALPORTNATIVE_H__
#define TNX_QSERIALPORTNATIVE_H__

#include <QObject>
#include <QString>
#include <QIODevice>
#include <QSharedPointer>
#include "qportsettings.h"

class QSocketNotifier;
class QMutex;

namespace TNX {

class TermiosHelper;
class CommDCBHelper;
class QWinCommEvtNotifier;
class WinCommEvtBreaker;

#ifdef TNX_POSIX_SERIAL_PORT
  typedef int HND;
  typedef QSocketNotifier NativeReadNotifier;
  typedef TermiosHelper NativePortSettingsHelper;

#elif defined(TNX_WINDOWS_SERIAL_PORT)
# include <windows.h>
  typedef HANDLE HND;
  typedef QWinCommEvtNotifier NativeReadNotifier;
  typedef CommDCBHelper NativePortSettingsHelper;

#else
  #error Unsupported platform.
#endif

/**
 * Platform dependent implementation of the functionalities required by the
 * QSerialPort class.
 */
class QSerialPortNative : public QIODevice
{
Q_OBJECT

public:
  QSerialPortNative(const QString &portName, QObject *parent = 0);
  QSerialPortNative(const QString &portName, const QPortSettings &settings, QObject *parent = 0);
  virtual ~QSerialPortNative()
  {}

 // methods need to be implemented by platform dependent QSerialPortNative class
protected:
  bool open_impl();
  void close_impl();
  /** Gets the error message related to the last error occurred. */
  QString lastErrorText_impl() const;
  /** Gets the last native error. */
  int lastError_impl() const;
  qint64 bytesAvailable_impl() const;
  int waitForReadyRead_impl(int timeout);
  bool flushInBuffer_impl();
  bool flushOutBuffer_impl();
  bool sendBreak_impl(int timeout);
  qint64 readData_impl(char *data, qint64 maxlen);
  qint64 writeData_impl(const char *data, qint64 len);

protected:
  QString portName_;
  QPortSettings portSettings_;
  CommTimeouts commTimeouts_;
  HND fileDescriptor_;
  NativePortSettingsHelper *portHelper_;
  NativeReadNotifier *readNotifier_;

#if defined(Q_OS_WIN32) || defined(Q_OS_WIN) // Overlapped I/O is not supported on WinCE
  OVERLAPPED ovRead_;
  OVERLAPPED ovWrite_;
#elif defined(Q_OS_WINCE)
  static WinCommEvtBreaker *waitBreakerThread_;
  static QMutex *mutex_;
#endif


  Q_DISABLE_COPY(QSerialPortNative)
};

} // namespace

#endif // TNX_QSERIALPORTNATIVE_H__
