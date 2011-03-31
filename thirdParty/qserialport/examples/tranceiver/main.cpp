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
#include <QDebug>
#include <QThread>
#include <QSerialPort>
#include <iostream>
#include "receiver.h"
#include "transmitter.h"
#include <QDateTime>

void printHelp()
{
  std::cout << "Usage: tranceiver [--help] [--role= transmit|receive] [--serport=<serial port>] " \
               "[--listen=<port>] [--remotehost=<ip:port>] [--buffersize=<number of bytes>|*]" << std::endl;
  std::cout << "==========" << std::endl;
  std::cout << "Example: tranceiver --role=transmit --serport=/dev/ttySAC0 --listen=8765 --remotehost=193.168.0.102:8765" << std::endl;
  std::cout << "Example: tranceiver --role=receive --serport=/dev/ttyUSB0 --listen=8765 --remotehost=192.168.0.23:8765 --buffersize=*" << std::endl;
  std::cout << "==========" << std::endl;
}

bool processArgs(const QStringList &args, QString &role, QString &serPort, QString &udpPort,
                 QString &remoteHost, QString &bufferSize)
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

#if defined(Q_OS_WIN32) || defined(Q_OS_WINCE)
    serPort = argPairs.value("--serport", "com3");
#else
    serPort = argPairs.value("--serport", "/dev/ttyS0");
#endif

  role = argPairs.value("--role", "transmit");

  if ( role == "transmit" ) {
    remoteHost = argPairs.value("--remotehost", "localhost:8756");
    udpPort = argPairs.value("--listen", "9766");
  }
  else if ( role == "receive" ) {
    remoteHost = argPairs.value("--remotehost", "localhost:9766");
    udpPort = argPairs.value("--listen", "8756");
    bufferSize = argPairs.value("--buffersize", "*");
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

int main(int argc, char **argv)
{
  QString role;
  QString serPort;
  QString udpPort;
  QString remoteHost;
  QString bufferSize;
  QCoreApplication a(argc, argv);

  std::cout << "Serial port transmitter/receiver example, Copyright (c) 2010 Inbiza Systems Inc." << std::endl;
  std::cout << "Created by Inbiza Labs <labs@inbiza.com>" << std::endl;
  std::cout << "==========" << std::endl;
  std::cout << "Connect two networked computers with null-modem cable, run tranceiver with \"--role=transmit\" on one, and with \"--role=receive\" on ther other." << std::endl;
  std::cout << "==========" << std::endl;
  std::cout << std::endl;

  if ( !processArgs(a.arguments(), role, serPort, udpPort, remoteHost, bufferSize) )
    return 0;

  qDebug() << "Mode:" << role << ", Serial Port:" << serPort << ", Port Settings:" << "9600,8,n,1" <<
              ", Listening at port: " << udpPort << ", Testing peer at: " << remoteHost << ", Buffer size: " << (bufferSize == "*" ? "equal to data set size" : bufferSize);

  TNX::QSerialPort serialPort(serPort, "9600,8,n,1");

  TNX::CommTimeouts commTimeouts;

  commTimeouts.PosixVMIN = 1;
  commTimeouts.PosixVTIME = 0;
  commTimeouts.Win32ReadIntervalTimeout = 75;
  commTimeouts.Win32ReadTotalTimeoutConstant = 250;
  commTimeouts.Win32ReadTotalTimeoutMultiplier = 25;
  commTimeouts.Win32WriteTotalTimeoutConstant = 250;
  commTimeouts.Win32WriteTotalTimeoutMultiplier = 75;

  if ( !serialPort.setCommTimeouts(commTimeouts) )
    qWarning("Cannot set communications timeout values at port %s.", qPrintable(serPort));

  Receiver *receiver = NULL;
  Transmitter *transmitter = NULL;
  if ( 0 == role.compare("transmit", Qt::CaseInsensitive) ) {
    transmitter = new Transmitter(serialPort, remoteHost, udpPort.toInt(), &a);
    if ( !transmitter->start() ) {
      std::cout << "Cannot start transmitter. Quitting." << std::endl;
      return 0;
    }

  }
  else if ( 0 == role.compare("receive", Qt::CaseInsensitive) ) {
    receiver = new Receiver(serialPort,  remoteHost, udpPort.toInt(), QString((bufferSize == "*" ? "0" : bufferSize)).toInt(), &a);
    if ( !receiver->start() ) {
      std::cout << "Cannot start receiver. Quitting." << std::endl;
      return 0;
    }
  }
  else {
    std::cout << "Wrong role defined. Quitting." << std::endl;
    return 0;
  }

  return a.exec();
}

