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
#include <QTimerEvent>
#include "commdcbhelper.h"
#include "qwincommevtnotifier.h"

namespace TNX {

/*!
  
*/

QWinCommEvtNotifier::QWinCommEvtNotifier(HANDLE hComm, QObject *parent)
  : QObject(parent), notifier_(NULL), commHandle_(hComm)
{
  suspendEvent_ = CreateEvent(NULL, TRUE, TRUE, NULL); // manual-reset event and initial state is signaled
  Q_ASSERT(suspendEvent_ != NULL);

  notifier_ = new NotifierThread(commHandle_, suspendEvent_, this);
  Q_CHECK_PTR(notifier_);
  notifier_->start();
}


QWinCommEvtNotifier::~QWinCommEvtNotifier()
{
  resume();
  notifier_->terminate();

  // force notifier thread to exit from WaitCommEvent() by changing the comm mask
  int allFlags = EV_RXCHAR | EV_TXEMPTY | EV_CTS | EV_DSR | EV_RLSD | EV_BREAK | EV_ERR | EV_RING | 
                   EV_PERR | EV_RX80FULL | EV_EVENT1 | EV_EVENT2;

  if ( !SetCommMask(commHandle_, allFlags) ) {
    qDebug() << QString("QWinCommEvtNotifier::dtor(hnd: %1) failed when setting comm mask for exit: %2(Err #%3)")
                .arg((quintptr)commHandle_)
                .arg(CommDCBHelper::errorText(GetLastError()))
                .arg(GetLastError());
  }

  // wait until notifier thread is terminated
  notifier_->wait();

  CloseHandle(suspendEvent_);
}


void QWinCommEvtNotifier::setEnabled(bool val) 
{
  blockSignals(!val);
  
  if ( val ) {
    DWORD errorMask = 0;
    COMSTAT comStat;

    if ( !SetCommMask(commHandle_, EV_ERR) ) {
      qDebug() << QString("QWinCommEvtNotifier::setEnabled(hnd: %1) failed when setting comm" \
                          " event mask: %2(Err #%3)")
                      .arg((quintptr)commHandle_)
                      .arg(CommDCBHelper::errorText(GetLastError()))
                      .arg(GetLastError());
    }

    // Get the number of pending bytes
    if ( !ClearCommError(commHandle_, &errorMask, &comStat) ) {
      qDebug() << QString("QWinCommEvtNotifier::setEnabled(hnd: %1) failed when requesting" \
                          " comm errors: %2(Err #%3)")
                      .arg((quintptr)commHandle_)
                      .arg(CommDCBHelper::errorText(GetLastError()))
                      .arg(GetLastError());
    }

    if ( comStat.cbInQue > 0 ) {
      // Generate a byte pending event until there is no more data in the character buffer
      bytesPending(comStat.cbInQue);
    }
    else {
      if ( !SetCommMask(commHandle_, EV_RXCHAR | EV_ERR) ) {
        qDebug() << QString("QWinCommEvtNotifier::setEnabled(hnd: %1) failed when re-setting" \
                            " comm event mask: %2(Err #%3)")
                      .arg((quintptr)commHandle_)
                      .arg(CommDCBHelper::errorText(GetLastError()))
                      .arg(GetLastError());
      }
    }//endif
  }
}

/* NotifierThread class Implementation */

void QWinCommEvtNotifier::NotifierThread::run()
{
  // store this thread's id
  threadId_ = QThread::currentThreadId();

  if ( !SetCommMask(commHandle_, EV_RXCHAR | EV_ERR) ) {
    qDebug() << QString("NotifierThread::run(hnd: %1) failed when setting comm event mask: %2(Err #%3)")
                .arg((quintptr)commHandle_)
                .arg(CommDCBHelper::errorText(GetLastError()))
                .arg(GetLastError());
  }

  DWORD dwCommEvent = 0;
  forever {
    // wait indefinitely if the event-drive model is suspended by the caller
    // using suspend() method.

    DWORD dwWaitResult = WaitForSingleObject(suspendEvent_, INFINITE);
    switch (dwWaitResult)
    {
        // Event object was signaled
        case WAIT_OBJECT_0:
            break;

        // An error occurred
        default:
            qDebug(qPrintable(QString("NotifierThread::run(hnd: %1) failed while waiting for suspend event: err #%2. ")
                                 .arg((quintptr)commHandle_).arg(GetLastError())));
    }

    if ( !WaitCommEvent(commHandle_, &dwCommEvent, 0) ) {
       qDebug(qPrintable(QString("NotifierThread::run(hnd: %1) failed in WaitCommEvent. " \
                                    "Event driven model is disabled.")
                              .arg((quintptr)commHandle_)));
       break;
    }
    else {
      if ( dwCommEvent & EV_RXCHAR )
        parent_->bytesPending(-1L);    // pending bytes count is unknown
        
      if ( dwCommEvent & EV_ERR ) {
        DWORD errorMask = 0;
        COMSTAT comStat;
        if ( !ClearCommError(commHandle_, &errorMask, &comStat) ) {
          qDebug() << QString("NotifierThread::run(hnd: %1) failed when requesting comm errors: %2(Err #%3)")
                    .arg((quintptr)commHandle_)
                    .arg(CommDCBHelper::errorText(GetLastError()))
                    .arg(GetLastError());
        }
        else {
          // Printout the error.
          if ( errorMask & CE_BREAK )
            qDebug() << QString("NotifierThread::run(hnd: %1) failed with error: " \
                                "The hardware detected a break condition.")
                          .arg((quintptr)commHandle_);
           else if ( errorMask & CE_FRAME )
            qDebug() << QString("NotifierThread::run(hnd: %1) failed with error: " \
                                "The hardware detected a framing error.")
                          .arg((quintptr)commHandle_);
          else if ( errorMask & CE_OVERRUN )
            qDebug() << QString("NotifierThread::run(hnd: %1) failed with error: " \
                                "A character-buffer overrun has occurred. The next character is lost.")
                          .arg((quintptr)commHandle_);
          else if ( errorMask & CE_RXOVER )
            qDebug() << QString("NotifierThread::run(hnd: %1) failed with error: " \
                                "An input buffer overflow has occurred. There is either no room in the input buffer, " \
                                "or a character was received after the end-of-file (EOF) character.")
                          .arg((quintptr)commHandle_);
          else if ( errorMask & CE_RXPARITY )
            qDebug() << QString("NotifierThread::run(hnd: %1) failed with error: " \
                                "The hardware detected a parity error.")
                          .arg((quintptr)commHandle_);
        }
      }
    }

    if ( doquit_ )
      break;

  }//forever
}

} // namespace


