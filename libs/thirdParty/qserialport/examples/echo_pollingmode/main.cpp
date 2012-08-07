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
#include <QString>
#include <QStringList>
#include <QDateTime>
#include <QDebug>
#include <QSerialPort>

void printHelp()
{
  std::cout << "Usage: echo_pollingmode [--help] [--port=<serial port>] [--set=<port settings>]" << std::endl;
  std::cout << std::endl;
#if defined(Q_OS_WIN)
  std::cout << "Example: echo_pollingmode --port=COM3 --set=9600,8,N,1" << std::endl;
#else
  std::cout << "Example: echo_pollingmode --port=/dev/ttyS0 --set=9600,8,N,1" << std::endl;
#endif
}

bool processArgs(const QStringList &args, QString &serPort, QString &settings)
{
  QHash<QString, QString> argPairs;

  foreach( QString ar, args ) {
    if ( ar.split('=').size() == 2 ) {
      argPairs.insert(ar.split('=').at(0), ar.split('=').at(1));
    }
    else {
      if ( ar == "--help" )
        goto LPrintHelpAndExit;
    }
  }

  settings = argPairs.value("--set", "9600,8,n,1");
#if defined(Q_OS_WIN)
  serPort = argPairs.value("--port", "COM3");
#else
  serPort = argPairs.value("--port", "/dev/ttyS0");
#endif

  return true;

LPrintHelpAndExit:
  printHelp();
  return false;
}

int main(int argc, char *argv[])
{ 
  using namespace TNX;

  QCoreApplication a(argc, argv);

  std::cout << "Serial port echo example with polling model, Copyright (c) 2010 Inbiza Systems Inc." << std::endl;
  std::cout << "Created by Inbiza Labs <labs@inbiza.com>" << std::endl;

  QString portName;
  QString settings;

  if ( !processArgs(a.arguments(), portName, settings) )
    return 0;

  QSerialPort serport(portName, settings);
  if ( !serport.open() )
    qFatal("Cannot open serial port %s. Exiting..", qPrintable(portName));

  if ( !serport.setCommTimeouts(QSerialPort::CtScheme_NonBlockingRead) )
    qWarning("Cannot set communications timeout values at port %s.", qPrintable(portName));

  int byteCounter = 0;
  forever {
    if ( serport.waitForReadyRead(5000) ) { // 5sec
      QByteArray data = serport.read(512);
      byteCounter += data.size();
      std::cout << "Received data @ " << qPrintable(QDateTime::currentDateTime().toString("hh:mm:ss.zzz")) <<
          ". Echo back." << std::endl;
      serport.write(data);
      if ( byteCounter >= 4096 )
        break;
    }
    else {
      std::cout << "Timeout while waiting for incoming data @ " <<
          qPrintable(QDateTime::currentDateTime().toString("hh:mm:ss.zzz")) << std::endl;
    }
  }

  std::cout << "Polling example is terminated successfully." << std::endl;
  return 0;
}

