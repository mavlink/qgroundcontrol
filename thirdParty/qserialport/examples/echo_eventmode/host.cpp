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
#include <QSerialPort>
#include <QTimerEvent>
#include <QDateTime>
#include "host.h"

Host::Host(TNX::QSerialPort &serPort, QObject *parent)
  : QObject(parent), serialPort_(serPort), timerId_(0), byteCounter_(0)
{
  using TNX::QSerialPort;

  timerId_ = startTimer(5000);
  connect(&serialPort_, SIGNAL(readyRead()), SLOT(onDataReceived()));

  if ( !serialPort_.setCommTimeouts(QSerialPort::CtScheme_NonBlockingRead) )
    qWarning("Cannot set communications timeout values at port %s.", qPrintable(serialPort_.portName()));
}

Host::~Host()
{
}

void Host::onDataReceived()
{
  if ( timerId_ )
    killTimer(timerId_);

  QByteArray data = serialPort_.read(512);
  byteCounter_ += data.size();

  std::cout << "Thread id: " << QThread::currentThreadId() << " Received data @ " <<
      qPrintable(QDateTime::currentDateTime().toString("hh:mm:ss.zzz")) << ". Echo back." << std::endl;
  serialPort_.write(data);

  if ( byteCounter_ >= 4096 ) {
    std::cout << "Event-Driven example is terminated successfully." << std::endl;
    qApp->exit(0);
  }

  // create a wait timer for the next packet
  timerId_ = startTimer(5000); // 5sec
}

void Host::timerEvent(QTimerEvent *event)
{
  Q_ASSERT(timerId_ == event->timerId());

  killTimer(event->timerId());
  timerId_ = 0;

  std::cout << "Timeout occurred." << std::endl;
}
