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

#include <QtCore/QCoreApplication>
#include <QStringList>
#include <QHash>
#include <QHostAddress>
#include <QDebug>
#include <iostream>
#include "host.h"
#include "proxy.h"

void printHelp()
{
  std::cout << "Usage: ser2tcpbridge [--help] [--role= host|proxy] [--tcpport=<port>] " \
               "[--serport=<serial port>] [--remotehost=<ip:port>]" << std::endl;
  std::cout << std::endl;
  std::cout << "Example: ser2tcpbridge --role=host --serport=/dev/ttySAC0 --tcpport=8765" << std::endl;
  std::cout << "Example: ser2tcpbridge --role=proxy --remotehost=192.168.0.23:8765" << std::endl;
}

bool processArgs(const QStringList &args, QString &role, QString &serPort, QString &tcpPort,
                 QString &remoteHost)
{
  QHash<QString, QString> argPairs;

  if ( args.size() < 2 )
    goto LPrintHelpAndExit;

  foreach( QString ar, args ) {
    if ( ar.split('=').size() == 2 )
      argPairs.insert(ar.split('=').at(0), ar.split('=').at(1));
    else {
      if ( ar == "--help" )
        goto LPrintHelpAndExit;
    }
  }

  role = argPairs.value("--role", "host");
  if ( role == "host" ) {
#if defined(Q_OS_WIN32) || defined(Q_OS_WINCE)
    serPort = argPairs.value("--serport", "com3");
#else
    serPort = argPairs.value("--serport", "/dev/ttyS0");
#endif
    tcpPort = argPairs.value("--tcpport", "8756");
  }
  else if ( role == "proxy" ) {
    remoteHost = argPairs.value("--remotehost", "localhost");
    tcpPort = argPairs.value("--tcpport", "8756");
  }
  else {
    // invalid role
    std::cout << "Invalid role specified." << std::endl;
    goto LPrintHelpAndExit;
  }

  return true;

LPrintHelpAndExit:
  printHelp();
  return false;
}

int main(int argc, char *argv[])
{
  QString role;
  QString serPort;
  QString tcpPort;
  QString remoteHost;
  QCoreApplication a(argc, argv);

  std::cout << "Serial port to TCP Bridge example, Copyright (c) 2010 Inbiza Systems Inc." << std::endl;
  std::cout << "Created by Inbiza Labs <labs@inbiza.com>" << std::endl;

  if ( !processArgs(a.arguments(), role, serPort, tcpPort, remoteHost) )
    return 0;

  Host *host;
  Proxy *proxy;

  if ( role == "host" ) {
    qDebug() << "role:" << role << "serport:" << serPort << "tcpport:" << tcpPort;

    host = new Host(serPort, tcpPort.toInt(), &a);
    host->start();
  }
  else {
    qDebug() << "role:" << role << "remotehost:" << remoteHost << "tcpport:" << tcpPort;

    QHostAddress addr = (remoteHost == "localhost" ? QHostAddress::LocalHost : QHostAddress(remoteHost));

    proxy = new Proxy(addr, tcpPort.toInt(), &a);
    proxy->start();
  }

  return a.exec();
}

