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

#ifndef TRANSMITTER_H__
#define TRANSMITTER_H__

#include <QObject>
#include <QTime>
#include <QSerialPort>

class QUdpSocket;
namespace TNX { class QSerialPort; }

class Transmitter : public QObject
{
Q_OBJECT

  static const int kTotalPortSetTestCount = 30;
  static const int kTotalDataSetCount = 4;
  static const int kNumOfSignalsToTest = 4;

  static char PortSettingsToTest[kTotalPortSetTestCount][30];
  static int DataSetSizes[kTotalDataSetCount];

public:
  enum Tests {
    DataTransferTest = 0,
    SignalTest,
    DoneTesting
  };

public:
  explicit Transmitter(QObject *parent = 0);
  Transmitter(TNX::QSerialPort &serPort, const QString &udpAddr, int listenPort, QObject *parent = 0);
  ~Transmitter();

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
  int transmitTimer_;
  int commTestCounter_;
  int dataSetCounter_;
  int signalTestCounter_;
  char *dataSet_;
  Tests test_;
  QString lastSettings_;
  int lastDataSize_;
};

#endif // TRANSMITTER_H__
