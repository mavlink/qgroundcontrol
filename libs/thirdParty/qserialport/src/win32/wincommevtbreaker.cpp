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

#include <QMetaType>
#include <QDebug>
#include <QMutexLocker>
#include <QTimerEvent>
#include "commdcbhelper.h"
#include "wincommevtbreaker.h"

namespace TNX {

TimeKeeper::TimeKeeper(WinCommEvtBreaker *owner, QMutex &mutex, QObject *parent)
  : QObject(parent), mutex_(mutex)
{
  if ( !owner )
    return;

  qRegisterMetaType<HANDLE>("HANDLE");

  connect(owner, SIGNAL(startTimer(HANDLE, int)), SLOT(startWaitTimer(HANDLE, int)),
          Qt::BlockingQueuedConnection);
  connect(owner, SIGNAL(stopTimer(HANDLE)), SLOT(stopWaitTimer(HANDLE)),
          Qt::BlockingQueuedConnection);
}

/**
 *
 */
void TimeKeeper::timerEvent(QTimerEvent *event)
{
  QMutexLocker locker(&mutex_);

  killTimer(event->timerId());

  if ( !handleList_.contains(event->timerId()) )
    return;

  HANDLE commHandle = handleList_.take(event->timerId());

  // force WaitCommEvent() to exit from blocked-wait by setting different event mask

  DWORD allFlags = EV_RXCHAR | EV_TXEMPTY | EV_CTS | EV_DSR | EV_RLSD | EV_BREAK | EV_ERR | EV_RING |
                   EV_PERR | EV_RX80FULL | EV_EVENT1 | EV_EVENT2;

   if ( !SetCommMask(commHandle, allFlags) ) {
    qDebug() << QString("TimeKeeper::timerEvent(hnd: %1) failed when resetting comm event mask: %2(Err #%3)")
                .arg((int)commHandle)
                .arg(CommDCBHelper::errorText(GetLastError()))
                .arg(GetLastError());
  }
}

void TimeKeeper::startWaitTimer(HANDLE commHnd, int timeout)
{
  QMutexLocker locker(&mutex_);

  int timerId = startTimer(timeout);
  handleList_.insert(timerId, commHnd);
}

void TimeKeeper::stopWaitTimer(HANDLE commHnd)
{
  QMutexLocker locker(&mutex_);

  QList<int> timerIdList = handleList_.keys(commHnd);
  foreach (int timerId, timerIdList) {
    killTimer(timerId);
    handleList_.remove(timerId);
  }
}

/**
 *
 */
void WinCommEvtBreaker::run()
{
  // store this thread's id
  threadId_ = QThread::currentThreadId();
  timeKeeper_ = new TimeKeeper(this, mutex_); // as owner, not as parent

  Q_CHECK_PTR(timeKeeper_);

  exec();

  delete timeKeeper_;
}

void WinCommEvtBreaker::startWaitTimer(HANDLE commHnd, int timeout)
{
  emit startTimer(commHnd, timeout);
}

void WinCommEvtBreaker::stopWaitTimer(HANDLE commHnd)
{
  emit stopTimer(commHnd);
}


} // namespace


