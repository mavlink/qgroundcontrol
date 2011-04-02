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
 * @brief Declares generic qserialport methods and attributes.
 */

#ifndef TNX_QSERIALPORT_H__
#define TNX_QSERIALPORT_H__

#include "qserialportnative.h"
#include "qserialport_export.h"

namespace TNX {

/**
 * Represents a serial port resource.
 */
class TONIX_EXPORT QSerialPort : public QSerialPortNative
{
Q_OBJECT

private:
  enum {
    kReadNotifyThresholdLimit = 1024,
  };

public:
  /**
    Line signal values.
   */
  enum CommSignalValues {
    Signal_Unknown,             ///< Cannot determine whether the signal is on or off. Either the port is close or error occurred while querying the signal.
    Signal_On,                  ///< Line signal ON.
    Signal_Off                  ///< Line signal OFF.
  };

  /**
    Predefined timeout schemes for serial port reading.
   */
  enum CommTimeoutSchemes {
    CtScheme_NonBlockingRead,   ///< Read operation returns immediately. Good when polling.
    CtScheme_TimedRead          ///< Read operation continues until timeout occurs.
  };

public:
  /** Initializes a new instance of the SerialPort class using the specified port name. */
  QSerialPort(const QString &portName, QObject *parent = 0);

  /** Initializes a new instance of the SerialPort class using the specified port name and settings. */
  QSerialPort(const QString &portName, const QPortSettings &settings, QObject *parent = 0);

  /** Initializes a new instance of the SerialPort class using the specified port name and settings. */
  QSerialPort(const QString &portName, const QString &settings, QObject *parent = 0);

  virtual ~QSerialPort();

  // from QIODevice

  /** @reimp */
  bool isSequential() const;

  /** @reimp */
  bool open(OpenMode mode = ReadWrite);

  /** @reimp */
  void close();

 /** @reimp */
  qint64 pos() const;

  /** @reimp */
  qint64 size() const;

  /** @reimp */
  bool seek(qint64 pos);

  /** @reimp */
  bool atEnd() const;

  /** @reimp */
  qint64 bytesAvailable() const;

  /** @reimp */
  qint64 bytesToWrite() const;

  /** @reimp */
  bool canReadLine() const;

  /** @reimp */
  bool waitForReadyRead(int timeout);

  /** @reimp */
  bool waitForBytesWritten(int timeout);

  // serial port methods

  /** Gets the serial device name. */
  inline const QString& portName() const {
    return portName_;
  }

  /** Sets the serial device name. */
  inline void setPortName(const QString &portName) {
    Q_ASSERT(!isOpen());
    portName_ = portName;
  }

  /** Gets the serial device port settings. */
  inline const QPortSettings portSettings() const {
    return portSettings_;
  }
  /** Sets the serial device port settings. If the device is not open yet, caches these values until it is open.*/
  bool setPortSettings(const QPortSettings &portSettings);

  // communication timing configuration methods

  /** Gets the communications timeout values. */
  CommTimeouts commTimeouts();
  /** Sets the communications timeout values. */
  bool setCommTimeouts(const CommTimeouts commtimeouts);
  /** Sets one of the predefined communications timeout scheme. */
  bool setCommTimeouts(CommTimeoutSchemes scheme, int timeout = 0);

  /** Gets the amount of pending bytes to trigger readyRead() event. */
  inline int readNotifyThreshold() const {
    return readNotifyThreshold_;
  }
  /** Sets the amount of pending bytes to trigger readyRead() event. */
  inline void setReadNotifyThreshold(int numOfBytes) {
    // normalize the given value
    if ( numOfBytes <= 0 )
      numOfBytes = 1;
    else if ( numOfBytes > kReadNotifyThresholdLimit )
      numOfBytes = kReadNotifyThresholdLimit;
    readNotifyThreshold_ = numOfBytes;
  }

  // port configuration methods

  /** Gets serial baud rate. */
  QPortSettings::BaudRate baudRate() const;
  /** Sets serial baud rate. */
  bool setBaudRate(QPortSettings::BaudRate baudRate);

  /** Gets parity-checking style. */
  QPortSettings::Parity parity() const;
  /** Sets parity-checking style. */
  bool setParity(QPortSettings::Parity parity);

  /** Gets the standard number of stopbits per byte. */
  QPortSettings::StopBits stopBits() const;
  /** Sets the standard number of stopbits per byte. */
  bool setStopBits(QPortSettings::StopBits stopBits);

  /** Gets the length of data bits per byte. */
  QPortSettings::DataBits dataBits() const;
  /** Sets the length of data bits per byte. */
  bool setDataBits(QPortSettings::DataBits dataBits);

  /** Gets flow control protocol. */
  QPortSettings::FlowControl flowControl() const;
  /** Sets flow control protocol. */
  bool setFlowControl(QPortSettings::FlowControl flowControl);

  // line control methods

  /** Flushes input buffer. */
  bool flushInBuffer();
  /** Flushes output buffer. */
  bool flushOutBuffer();
  /** Sends a break for the duration of the given timeout. */
  bool sendBreak(int timeout);

  // signal control methods

  /** Sets a value indicating whether the Request to Send (RTS) signal is enabled during serial communication. */
  bool setRts(bool on = true);
  /** Sets a value that enables the Data Terminal Ready (DTR) signal during serial communication. */
  bool setDtr(bool on = true);
  
  /** Gets the state of the Clear-to-Send line for the port. */
  CommSignalValues cts();
  /** Gets the state of the Data Set Ready signal. */
  CommSignalValues dsr();
  /** Gets the state of the Carrier Detect line for the port. */
  CommSignalValues dcd();
  /** Gets the state of the Ring signal. */
  CommSignalValues ri();
  
protected:
  // from QIODevice

  /** @reimp */
  qint64 readData(char *data, qint64 maxlen);
  /** @reimp */
  qint64 writeData(const char *data, qint64 len);

private slots:
  void onDataReceived();

  // properties
protected:
  volatile qint64 pendingByteCount_;    // internal counter for pending bytes in the driver character buffer.
  volatile bool doNotify_;              // internal state flag to control when to notify the user about incoming data.
  volatile int readNotifyThreshold_;    // minimum number of pending bytes required to emit readyRead() signal.

  Q_DISABLE_COPY(QSerialPort)
};

} // namespace

#endif // TNX_QSERIALPORT_H__
