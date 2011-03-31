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

#ifndef HOST_H__
#define HOST_H__

#include <QObject>

class QTcpServer;
class QTcpSocket;
namespace TNX { class QSerialPort; }

class Host : public QObject
{
Q_OBJECT
public:
  Host(const QString &serPort, int tcpPort, QObject *parent = 0);
  ~Host();

  int start();
  int stop();

private slots:
  void onDataReceivedNetwork();
  void onDataReceivedSerial();
  void onNewTcpConnection();

private:
  int tcpPort_;
  TNX::QSerialPort *serialPort_;
  QTcpServer *tcpServer_;
  QTcpSocket *tcpSocket_;
};

#endif // HOST_H__
