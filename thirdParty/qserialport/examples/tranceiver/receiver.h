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

#ifndef RECEIVER_H__
#define RECEIVER_H__

#include <QObject>
#include <QTime>
#include <QSerialPort>

class QUdpSocket;
namespace TNX { class QSerialPort; }

class Receiver : public QObject
{
Q_OBJECT

public:
  explicit Receiver(QObject *parent = 0);
  Receiver(TNX::QSerialPort &serPort, const QString &udpAddr, int listenPort, int bufferSize = 0,
           QObject *parent = 0);
  ~Receiver();

  bool start();
  bool stop();

protected:
  void timerEvent(QTimerEvent *event);

//signals:

public slots:
  void onDataReceived();
  void onControlDataReceived();

private:
  inline QString sigValToString(TNX::QSerialPort::CommSignalValues sig) {
    if ( TNX::QSerialPort::Signal_On == sig )
      return "On";
    else if ( TNX::QSerialPort::Signal_Off == sig )
      return "Off";
    else
      return "Unknown";
  }

private:
  TNX::QSerialPort &serialPort_;
  QUdpSocket *socket_;
  int udpPortNo_;
  QString udpIp_;
  int listenPort_;
  int timeoutTimer_;
  char *readBuffer_;
  QTime readTimer_;
  qint32 dataSetSize_;
  int bufferSize_;
  bool doRestartTimer_;
  qint32 totalBytesRead_;
};

#endif // RECEIVER_H__
