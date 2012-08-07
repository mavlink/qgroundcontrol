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

#ifndef PROXY_H__
#define PROXY_H__

#include <QObject>

class QTcpSocket;
class QHostAddress;
class QTime;

class Proxy : public QObject
{
Q_OBJECT
public:
  Proxy(const QHostAddress &remoteHost, int tcpPort, QObject *parent = 0);
  ~Proxy();

  int start();
  int stop();

private slots:
  void onDataReceivedNetwork();
  void onConnected();
  void onDisconnected();

private:
  QTcpSocket *socket_;
  const QHostAddress &remoteHost_;
  int tcpPort_;
  QTime *timer_;
};

#endif // PROXY_H__
