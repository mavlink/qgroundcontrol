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
#include <QThread>
#include <QString>
#include <QStringList>
#include <QDebug>
#include <QSerialPort>
#include "host.h"

void printHelp()
{
  std::cout << "Usage: echo_eventmode [--help] [--port=<serial port>] [--set=<port settings>]" << std::endl;
  std::cout << std::endl;
#if defined(Q_OS_WIN)
  std::cout << "Example: echo_eventmode --port=COM3 --set=9600,8,N,1" << std::endl;
#else
  std::cout << "Example: echo_eventmode --port=/dev/ttyS0 --set=9600,8,N,1" << std::endl;
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
  using TNX::QSerialPort;

  QCoreApplication a(argc, argv);

  std::cout << "Serial port echo example with event-driven model, Copyright (c) 2010 Inbiza Systems Inc." << std::endl;
  std::cout << "Created by Inbiza Labs <labs@inbiza.com>" << std::endl;
  std::cout << std::endl;
  std::cout << "Main thread id: " << QThread::currentThreadId() << std::endl;

  QString portName;
  QString settings;

  if ( !processArgs(a.arguments(), portName, settings) )
    return 0;

  QSerialPort serport(portName, settings);
  if ( !serport.open() )
    qFatal("Cannot open serial port %s. Exiting..", qPrintable(portName));

  Host host(serport);

  return a.exec();
}
