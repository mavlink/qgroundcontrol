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

#ifndef TNX_QWINCOMMEVTNOTIFIER_H__
#define TNX_QWINCOMMEVTNOTIFIER_H__

#include <QThread>
#include <windows.h>
#include "qserialport.h"

namespace TNX {

class QWinCommEvtNotifier : public QObject
{
Q_OBJECT
  /**
   */
  class NotifierThread : public QThread
  {
  public:
    NotifierThread(HANDLE hnd, HANDLE &suspendEvent, QWinCommEvtNotifier *parent)
      : QThread(parent), parent_(parent), commHandle_(hnd), suspendEvent_(suspendEvent),
        doquit_(false)
    {}
    inline Qt::HANDLE threadId() const
      { return threadId_; }
    inline void terminate() 
      { doquit_ = true; }
  protected:
    void run();
  private:
    QWinCommEvtNotifier *parent_;
    Qt::HANDLE threadId_;
    HANDLE commHandle_;
    HANDLE &suspendEvent_;
    volatile bool doquit_;
  };

public:
  QWinCommEvtNotifier(HANDLE hComm, QObject *parent = 0);
  ~QWinCommEvtNotifier();
  void setEnabled(bool);

  inline bool suspend() {
    return ResetEvent(suspendEvent_);
  }
  inline bool resume() {
    return SetEvent(suspendEvent_);
  }

signals:
  void activated(int);

private:
  // numOfBytes is for future use
  inline void bytesPending(int /*numOfBytes*/) {
    emit activated(0);
  }
 
private:
  NotifierThread *notifier_;
  HANDLE commHandle_;
  HANDLE suspendEvent_;
 
private:  
  Q_DISABLE_COPY(QWinCommEvtNotifier)
};

} // namespace

#endif // TNX_QWINCOMMEVTNOTIFIER_H__
