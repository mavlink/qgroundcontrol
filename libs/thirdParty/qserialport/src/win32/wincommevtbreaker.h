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

#ifndef TNX_QWINCOMMEVTBREAKER_H__
#define TNX_QWINCOMMEVTBREAKER_H__

#include <QThread>
#include <QHash>
#include <QMutex>
#include <windows.h>

namespace TNX {

class  WinCommEvtBreaker;

class TimeKeeper : public QObject
{
Q_OBJECT
public:
  TimeKeeper(WinCommEvtBreaker *owner, QMutex &mutex, QObject *parent = 0);

protected:
  void timerEvent(QTimerEvent *event);

public slots:
  void startWaitTimer(HANDLE commHnd, int timeout);
  void stopWaitTimer(HANDLE commHnd);

private:
  TimeKeeper(QObject *parent = 0);

private:
  QHash<int, HANDLE> handleList_;
  QMutex &mutex_;
};

class WinCommEvtBreaker : public QThread
{
Q_OBJECT
public:
  WinCommEvtBreaker(QObject *parent = 0)
    : QThread(parent), timeKeeper_(NULL)
  {
  }
  inline Qt::HANDLE threadId() const {
    return threadId_;
  }
  inline void waitUntilReady() const {
    while (!timeKeeper_) Sleep(0);
  }
  void startWaitTimer(HANDLE commHnd, int timeout);
  void stopWaitTimer(HANDLE commHnd);

signals:
  void startTimer(HANDLE commHnd, int timeout);
  void stopTimer(HANDLE commHnd);

protected:
  void run();

private:
  Qt::HANDLE threadId_;
  TimeKeeper *timeKeeper_;
  QMutex mutex_;
};

} // namespace

#endif // TNX_QWINCOMMEVTBREAKER_H__
