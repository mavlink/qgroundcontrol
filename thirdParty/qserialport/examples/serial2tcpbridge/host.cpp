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
#include <QTcpSocket>
#include <QTcpServer>
#include <QSerialPort>
#include "host.h"

Host::Host(const QString &serPort, int tcpPort, QObject *parent)
  : QObject(parent), tcpPort_(tcpPort), tcpSocket_(NULL)
{
  using TNX::QSerialPort;

  serialPort_ = new QSerialPort(serPort, "9600,8,N,1", this);
  tcpServer_ = new QTcpServer(this);
}

Host::~Host()
{
  stop();
}

int Host::start()
{
  // deal with serial port

  if ( serialPort_->open() ) {
    serialPort_->flushInBuffer();
    serialPort_->flushOutBuffer();
  }
  else {
    qFatal("Cannot open serial port: \"%s\". Giving up.", qPrintable(serialPort_->errorString()));
    return -1;
  }
  connect(serialPort_, SIGNAL(readyRead()), SLOT(onDataReceivedSerial()));

  // deal with tcp port

  std::cout << "Waiting for connection on tcp port " << tcpPort_ << std::endl;

  if ( tcpServer_->listen(QHostAddress::Any, tcpPort_) ) {
    std::cout << "Listening on " << qPrintable(tcpServer_->serverAddress().toString()) << ":" << tcpServer_->serverPort() << std::endl;
    connect(tcpServer_, SIGNAL(newConnection()), SLOT(onNewTcpConnection()));
  }
  else {
    qFatal("Cannot listening tcp port: \"%s\". Giving up.", qPrintable(tcpServer_->errorString()));
    return -1;
  }

  return 0;
}

int Host::stop()
{
  serialPort_->close();
  disconnect(serialPort_, SIGNAL(readyRead()), this, SLOT(onDataReceivedSerial()));

  tcpServer_->close();
  disconnect(tcpServer_, SIGNAL(newConnection()), this, SLOT(onNewTcpConnection()));

  return 0;
}

void Host::onDataReceivedNetwork()
{
  serialPort_->write(tcpSocket_->read(2048));
}

void Host::onDataReceivedSerial()
{
  if ( tcpSocket_->state() == QAbstractSocket::ConnectedState ) {
    tcpSocket_->write(serialPort_->read(2048));
  }
}

void Host::onNewTcpConnection()
{
  tcpSocket_ = tcpServer_->nextPendingConnection();

  connect(tcpSocket_, SIGNAL(readyRead()), SLOT(onDataReceivedNetwork()));

  std::cout << "Connection is established with " << qPrintable(tcpSocket_->peerAddress().toString()) << std::endl;
}
