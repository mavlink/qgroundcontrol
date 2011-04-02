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

#include <QTcpSocket>
#include <QHostAddress>
#include <QDebug>
#include <QTime>
#include "proxy.h"

Proxy::Proxy(const QHostAddress &remoteHost, int tcpPort, QObject *parent)
  : QObject(parent), remoteHost_(remoteHost), tcpPort_(tcpPort), timer_(NULL)
{
  socket_ = new QTcpSocket(this);
}

Proxy::~Proxy()
{
  stop();
}

int Proxy::start()
{
  connect(socket_, SIGNAL(connected()), SLOT(onConnected()));
  connect(socket_, SIGNAL(disconnected()), SLOT(onDisconnected()));
  connect(socket_, SIGNAL(readyRead()), SLOT(onDataReceivedNetwork()));

  qDebug() << "Connecting to " << remoteHost_.toString() << ":" << tcpPort_;
  socket_->connectToHost(remoteHost_, tcpPort_);
  return 0;
}

int Proxy::stop()
{
  disconnect(socket_, SIGNAL(connected()), this, SLOT(onConnected()));
  disconnect(socket_, SIGNAL(disconnected()), this, SLOT(onDisconnected()));
  disconnect(socket_, SIGNAL(readyRead()), this, SLOT(onDataReceivedNetwork()));

  return 0;
}

void Proxy::onDataReceivedNetwork()
{
  QByteArray data = socket_->read(2048);
  qDebug() << data.size() << " bytes received. Echo back.";
  socket_->write(data);
}

void Proxy::onConnected()
{
  qDebug() << "Socket is connected.";
}

void Proxy::onDisconnected()
{
  qDebug() << "Socket is disconnected.";
}
