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
#include <QDebug>
#include <QDateTime>
#include <QTimerEvent>
#include <QStringList>
#include <QUdpSocket>
#include "receiver.h"

extern TNX::QSerialPort defaultSerPort;

Receiver::Receiver(QObject *parent)
  : QObject(parent), serialPort_(defaultSerPort), timeoutTimer_(0), readBuffer_(NULL),
    dataSetSize_(0), bufferSize_(0), doRestartTimer_(false), totalBytesRead_(0)
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

Receiver::Receiver(TNX::QSerialPort &serPort, const QString &udpAddr, int listenPort, int bufferSize,
                   QObject *parent)
  : QObject(parent), serialPort_(serPort), listenPort_(listenPort), timeoutTimer_(0), readBuffer_(NULL),
    dataSetSize_(0), bufferSize_(bufferSize), doRestartTimer_(false), totalBytesRead_(0)
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

Receiver::~Receiver()
{
}

bool Receiver::start()
{
  if ( !socket_->open(QIODevice::ReadWrite) )
    qFatal("Cannot open UDP port: \"%d\". Giving up.", udpPortNo_);

  connect(socket_, SIGNAL(readyRead()), SLOT(onControlDataReceived()));

  if ( !socket_->bind(listenPort_) )
    qFatal("Cannot bind to port: \"%d\". Giving up.", listenPort_);

  if ( serialPort_.open() ) {
    serialPort_.flushInBuffer();
    serialPort_.flushOutBuffer();
  }
  else {
    qFatal("Cannot open serial port: \"%s\". Giving up.", qPrintable(serialPort_.errorString()));
  }

  qDebug() << "Getting ready for receiving data via " << serialPort_.portName();
  qDebug() << "Control channel @ UDP port " << listenPort_;

  connect(&serialPort_, SIGNAL(readyRead()), SLOT(onDataReceived()));

  return true;
}

bool Receiver::stop()
{
  disconnect(socket_, SIGNAL(readyRead()), this, SLOT(onControlDataReceived()));
  socket_->close();

  if ( !serialPort_.isOpen() ) {
    return true;
  }
  else {
    qDebug() << "Number of bytes in serial port buffer: " << serialPort_.bytesAvailable();

    delete[] readBuffer_;
    readBuffer_ = NULL;

    disconnect(&serialPort_, SIGNAL(readyRead()), this, SLOT(onDataReceived()));
    serialPort_.close();
  }
  return true;
}

void Receiver::timerEvent(QTimerEvent *event)
{
  killTimer(event->timerId());

  // make sure that we don't leak timer
  Q_ASSERT( timeoutTimer_ == event->timerId() );

  timeoutTimer_ = 0;

  qDebug() << "Timeout occurred while waiting for more data.";
  qDebug() << "Number of pending bytes: " << serialPort_.bytesAvailable();
  qDebug() << "Number of total bytes read: " << totalBytesRead_;

  socket_->writeDatagram(QString("RECV___END@Transfer %1 - Total bytes: %2 - Received bytes: %3 - Pending bytes: %4")
                          .arg("*****failed *** TIMEOUT ****!")
                          .arg(dataSetSize_)
                          .arg(totalBytesRead_)
                          .arg(serialPort_.bytesAvailable()).toLatin1(),
                         QHostAddress(udpIp_), udpPortNo_);

  serialPort_.flushInBuffer();

  dataSetSize_ = 0;
  delete[] readBuffer_;
  readBuffer_ = NULL;
}

void Receiver::onControlDataReceived()
{
  using TNX::QPortSettings;

  QByteArray recv = socket_->read(1000);

  if ( recv.left(10) == "SEND_START" ) {
    doRestartTimer_ = true;

    delete[] readBuffer_;
    readBuffer_ = NULL;

    if ( timeoutTimer_ != 0 ) {
      killTimer(timeoutTimer_);
      serialPort_.flushInBuffer();
      qDebug() << "!!!!! Operation canceled by transmitter. Starting a new transmission.";
    }

    // change port settings

    dataSetSize_ = recv.right(recv.size() - 11).split('@').at(0).toInt();
    QString settings = recv.right(recv.size() - 11).split('@').at(1); // excluding @ as well

    std::cout << "Expecting " << dataSetSize_ << " bytes @" << qPrintable(settings)
              << " Time: " << qPrintable(QDateTime::currentDateTime().toString("hh:mm:ss.zzz"));

    // make sure that this platform support given settings
    QPortSettings ps;
    if ( !ps.set(settings) ) {
      socket_->writeDatagram("RECV_USSET", QHostAddress(udpIp_), udpPortNo_);
      std::cout << " -- unsupported settings" << std::endl;
    }
    else {
      readBuffer_ = new char[dataSetSize_];

      serialPort_.setPortSettings(ps);
      timeoutTimer_ = startTimer(5000);

      socket_->writeDatagram("RECV_READY", QHostAddress(udpIp_), udpPortNo_);
    }
  }
  else if ( recv.left(10) == "SEND___END" ) {
    // do nothing
    std::cout << "." << std::endl;
  }
  else if ( recv.left(10) == "SIG_CTS_RQ" ) {
    socket_->writeDatagram(QString("SIG_CTS___@%1").arg(sigValToString(serialPort_.cts())).toAscii(),
                           QHostAddress(udpIp_), udpPortNo_);
  }
  else if ( recv.left(10) == "SIG_DSR_RQ" ) {
    socket_->writeDatagram(QString("SIG_DSR___@%1").arg(sigValToString(serialPort_.dsr())).toAscii(),
                           QHostAddress(udpIp_), udpPortNo_);
  }
  else if ( recv.left(10) == "SIG_DCD_RQ" ) {
    socket_->writeDatagram(QString("SIG_DCD___@%1").arg(sigValToString(serialPort_.dcd())).toAscii(),
                           QHostAddress(udpIp_), udpPortNo_);
  }
  else if ( recv.left(10) == "SIG_RI__RQ" ) {
    socket_->writeDatagram(QString("SIG_RI____@%1").arg(sigValToString(serialPort_.ri())).toAscii(),
                           QHostAddress(udpIp_), udpPortNo_);
  }
  else if ( recv.left(10) == "SIG_TEST_S" ) {
    serialPort_.setRts(true);
    serialPort_.setDtr(true);
    socket_->writeDatagram("SIG_TEST_R", QHostAddress(udpIp_), udpPortNo_);
  } 
}

void Receiver::onDataReceived()
{
  using namespace TNX;

  static qint64 read = 0;
  static qint64 offset = 0;

  if ( !readBuffer_ ) {
    std::cout << "Received data when expecting nothing. Not good." << std::endl;
    return;
  }

  killTimer(timeoutTimer_);
  timeoutTimer_ = startTimer(5000);

  if ( doRestartTimer_ ) {
    doRestartTimer_ = false;
    readTimer_.restart();

    read = 0LL;       // duplication: in case that timeout occurs
    offset = 0LL;
  }

  offset = read;
  read += serialPort_.read(readBuffer_+ ((int)offset), (bufferSize_ <= 0 ? (dataSetSize_ - (int)read) : bufferSize_));
  totalBytesRead_ = read;

  if ( read == dataSetSize_ ) {
    int elapsed = readTimer_.elapsed();
    killTimer(timeoutTimer_);
    timeoutTimer_ = 0;

    int total = 0;
    for (int i=0; i < dataSetSize_; i++) {
      total += (int)readBuffer_[i];
    }

    bool correct = (total == ((dataSetSize_ / 64) * (63*64/2)));

    read = 0LL;
    offset = 0LL;

    socket_->writeDatagram(QString("RECV___END@Transfer %1 - Total bytes: %2 - Elapsed time: %3ms")
                            .arg(correct?"is completed successfully":"failed *** data corrupted!")
                            .arg(dataSetSize_)
                            .arg(elapsed).toLatin1(),
                           QHostAddress(udpIp_), udpPortNo_);

    dataSetSize_ = 0;
    delete[] readBuffer_;
    readBuffer_ = NULL;
  }
}
