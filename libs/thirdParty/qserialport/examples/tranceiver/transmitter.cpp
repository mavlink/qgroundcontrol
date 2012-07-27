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

#include <iostream>
#include <QtCore/QCoreApplication>
#include <QDebug>
#include <QDateTime>
#include <QTimerEvent>
#include <QStringList>
#include <QUdpSocket>
#include "transmitter.h"

TNX::QSerialPort defaultSerPort("", "9600, 8, N, 1");

int Transmitter::DataSetSizes[] = { 4096, 4096*3, 4096*5, 4096*8 };
char Transmitter::PortSettingsToTest[][30] = { "4800,7,O,2", "38400,8,N,1", "9600,8,N,1", "19200,8,N,1", "57600,8,N,1", "115200,8,N,1",
                                               "4800,7,O,2", "38400,8,N,1", "9600,7,O,2", "19200,7,O,2", "57600,7,O,2", "115200,7,O,2",
                                               "4800,7,O,2", "38400,8,N,1", "9600,7,E,1", "19200,7,E,1", "57600,7,E,1", "115200,7,E,1",
                                               "4800,7,O,2", "38400,8,N,1", "9600,7,N,1", "19200,7,N,1", "57600,7,N,1", "115200,7,N,1",
                                               "4800,7,O,2", "38400,8,N,1", "9600,8,E,1", "19200,8,E,1", "57600,8,E,1", "115200,8,E,1"
                                            };

Transmitter::Transmitter(QObject *parent)
  : QObject(parent), serialPort_(defaultSerPort), transmitTimer_(0), commTestCounter_(0),
    dataSetCounter_(0), signalTestCounter_(0), dataSet_(NULL), test_(DataTransferTest)
{  
#if defined(Q_OS_WIN)
  serialPort_.setPortName("COM3");
#else
  serialPort_.setPortName("/dev/ttyS0");
#endif

  socket_ = new QUdpSocket(this);

  listenPort_ = 7865;
  udpPortNo_ = 7865;
  udpIp_ = "193.168.0.52";
}

Transmitter::Transmitter(TNX::QSerialPort &serPort, const QString &udpAddr, int listenPort, QObject *parent)
  : QObject(parent), serialPort_(serPort), listenPort_(listenPort), transmitTimer_(0), commTestCounter_(0),
    dataSetCounter_(0), signalTestCounter_(0), dataSet_(NULL), test_(DataTransferTest)
{
  if ( udpAddr.split(":").count() > 1 ) {
    udpIp_ = udpAddr.split(":").at(0);
    udpPortNo_ = udpAddr.split(":").at(1).toInt();
  }
  else {
    udpPortNo_ = udpAddr.toInt();
  }

  socket_ = new QUdpSocket(this);
}

Transmitter::~Transmitter()
{
}

bool Transmitter::start()
{
  test_ = DataTransferTest;

  if ( !socket_->open(QIODevice::ReadWrite) )
    qFatal("Cannot open UDP port: \"%d\". Giving up.", udpPortNo_);

  connect(socket_, SIGNAL(readyRead()), SLOT(onControlDataReceived()));

  if ( !socket_->bind(listenPort_) )
    qFatal("Cannot bind to port: \"%d\". Giving up.", listenPort_);

  if ( !serialPort_.open() )
    qFatal("Cannot open serial port: \"%s\". Giving up.", qPrintable(serialPort_.errorString()));
  else {
    serialPort_.flushInBuffer();
    serialPort_.flushOutBuffer();
  }

 qDebug() << "Getting ready for transmitting data via " << serialPort_.portName();
 qDebug() << "Control channel @ UDP port " << udpPortNo_;

 transmitTimer_ = startTimer(0);
 return true;
}

bool Transmitter::stop()
{
  disconnect(socket_, SIGNAL(readyRead()), this, SLOT(onControlDataReceived()));
  socket_->close();

  if ( !serialPort_.isOpen() ) {
    return true;
  }
  else {
    qDebug() << "Number of bytes in serial port buffer: " << serialPort_.bytesAvailable();

    delete[] dataSet_;
    dataSet_ = NULL;

    if ( transmitTimer_ != 0 ) {
      killTimer(transmitTimer_);
      transmitTimer_ = 0;
    }

    disconnect(&serialPort_, SIGNAL(readyRead()), this, SLOT(onDataReceived()));
    serialPort_.close();
  }

  return true;
}

void Transmitter::timerEvent(QTimerEvent *event)
{
  killTimer(event->timerId());

  // make sure that we don't leak timer
  Q_ASSERT( event->timerId() == transmitTimer_ );

  if ( test_ == DataTransferTest ) {
    lastDataSize_ = DataSetSizes[dataSetCounter_ % kTotalDataSetCount];

    dataSet_ = new char[lastDataSize_];

    Q_CHECK_PTR(dataSet_);

    for (int i = 0; i < lastDataSize_; i++)
      dataSet_[i] = (char)(i % 64);

    lastSettings_ = PortSettingsToTest[commTestCounter_++];

    serialPort_.setPortSettings(lastSettings_);

    socket_->writeDatagram(QString("SEND_START@%1@%2").arg(lastDataSize_).arg(lastSettings_).toLatin1(), QHostAddress(udpIp_), udpPortNo_);
  }
  else if ( test_ == SignalTest ) {
    if ( signalTestCounter_ >= kNumOfSignalsToTest ) {
      socket_->writeDatagram("TEST___END", QHostAddress(udpIp_), udpPortNo_);
      test_ = DoneTesting;
      std::cout << "..........TEST IS COMPLETED.........." << std::endl;
      qApp->exit(0);
    }
    else {
      // Signal test
      QString msg;
      switch ( signalTestCounter_++ ) {
        case 0: msg = "SIG_CTS_RQ"; break;
        case 1: msg = "SIG_DSR_RQ"; break;
        case 2: msg = "SIG_DCD_RQ"; break;
        case 3: msg = "SIG_RI__RQ"; break;
      }
      socket_->writeDatagram(msg.toAscii(), QHostAddress(udpIp_), udpPortNo_);
    }
  }
}

void Transmitter::onControlDataReceived()
{
  using TNX::QSerialPort;

  QByteArray recv = socket_->read(1000);

  if ( recv.left(10) == "RECV___END" ) {
    if ( commTestCounter_ >= kTotalPortSetTestCount ) {
      commTestCounter_ = 0;
      dataSetCounter_++;
    }

    delete[] dataSet_;

    qDebug() << recv.right(recv.size()-11) << "[" << serialPort_.portSettings().toString() << "]";

    if ( dataSetCounter_ >= kTotalDataSetCount ) {
      dataSetCounter_ = 0;
      test_ = SignalTest;

      // tell other party to get ready for signal test
      socket_->writeDatagram("SIG_TEST_S", QHostAddress(udpIp_), udpPortNo_);
    }
    else {
      // continue to test data transmission
      transmitTimer_ = startTimer(0);
    }
  }
  else if ( recv.left(10) == "RECV_USSET" ) {
    // Port settings are not supported by the receiver
    qDebug() << "Receiver doesn't support the given settings: " << lastSettings_ << ". Continuing to test with the next settings.";
    transmitTimer_ = startTimer(0);
  }
  else if ( recv.left(10) == "RECV_READY" ) {
    serialPort_.write(dataSet_, lastDataSize_);
    socket_->writeDatagram("SEND___END", QHostAddress(udpIp_), udpPortNo_);
  }
  else if ( recv.left(10) == "SIG_CTS___" ) {
    QSerialPort::CommSignalValues value = recv.right(recv.size() - 11) == "ON" ? QSerialPort::Signal_On : QSerialPort::Signal_Off;
    qDebug() << "Receiver's CTS line: " << sigValToString (value) << "| Expected value: ON";

    transmitTimer_ = startTimer(0);
  }
  else if ( recv.left(10) == "SIG_DSR___" ) {
    QSerialPort::CommSignalValues value = recv.right(recv.size() - 11) == "ON" ? QSerialPort::Signal_On : QSerialPort::Signal_Off;
    qDebug() << "Receiver's DSR line: " << sigValToString (value) << "| Expected value: ON";

    transmitTimer_ = startTimer(0);
  }
  else if ( recv.left(10) == "SIG_DCD___" ) {
    QSerialPort::CommSignalValues value = recv.right(recv.size() - 11) == "ON" ? QSerialPort::Signal_On : QSerialPort::Signal_Off;
    qDebug() << "Receiver's DCD line: " << sigValToString (value) << "| Transmitter's DCD line: " << sigValToString(serialPort_.dcd());

    transmitTimer_ = startTimer(0);
  }
  else if ( recv.left(10) == "SIG_RI____" ) {
    QSerialPort::CommSignalValues value = recv.right(recv.size() - 11) == "ON" ? QSerialPort::Signal_On : QSerialPort::Signal_Off;
    qDebug() << "Receiver's RI line: " << sigValToString (value) << "| Transmitter's RI line: " << sigValToString(serialPort_.ri());

    transmitTimer_ = startTimer(0);
  }
  // ready for signal test
  else if ( recv.left(10) == "SIG_TEST_R" ) {
    serialPort_.setRts(true);
    serialPort_.setDtr(true);

    qDebug() << "Setting transmitter's RTS and DTR lines to ON";

    transmitTimer_ = startTimer(0);
  }
}

void Transmitter::onDataReceived()
{
  std::cout << "Received data when expecting nothing. Interesting." << std::endl;
}
