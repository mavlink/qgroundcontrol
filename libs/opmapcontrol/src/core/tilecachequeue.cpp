/**
******************************************************************************
*
* @file       tilecachequeue.cpp
* @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
* @brief      
* @see        The GNU Public License (GPL) Version 3
* @defgroup   OPMapWidget
* @{
* 
*****************************************************************************/
/* 
* This program is free software; you can redistribute it and/or modify 
* it under the terms of the GNU General Public License as published by 
* the Free Software Foundation; either version 3 of the License, or 
* (at your option) any later version.
* 
* This program is distributed in the hope that it will be useful, but 
* WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY 
* or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License 
* for more details.
* 
* You should have received a copy of the GNU General Public License along 
* with this program; if not, write to the Free Software Foundation, Inc., 
* 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
*/
#include "tilecachequeue.h"


//#define DEBUG_TILECACHEQUEUE

#ifndef DEBUG_TILECACHEQUEUE
#undef qDebug
#define qDebug QT_NO_QDEBUG_MACRO
#endif

namespace core {
TileCacheQueue::TileCacheQueue()
{

}

TileCacheQueue::~TileCacheQueue()
{
    quit();
    _waitc.wakeAll();
    wait();
    this->deleteLater();
}

void TileCacheQueue::EnqueueCacheTask(CacheItemQueue *task)
{
    qDebug()<<"DB Do I EnqueueCacheTask"<<task->GetPosition().X()<<","<<task->GetPosition().Y();
    if(!_tileCacheQueue.contains(task)) {
        qDebug()<<"EnqueueCacheTask"<<task->GetPosition().X()<<","<<task->GetPosition().Y();
        _mutex.lock();
        _tileCacheQueue.enqueue(task);
        _mutex.unlock();
        if(this->isRunning()) {
            qDebug()<<"Wake Thread";
            _waitc.wakeAll();
        } else {
            qDebug()<<"Start Thread";
            this->start(QThread::NormalPriority);
        }
    }

}
void TileCacheQueue::run()
{
    qDebug()<<"Cache Engine Start";
    while(true) {
        CacheItemQueue *task = NULL;
        qDebug() << "Cache";
        if(_tileCacheQueue.count() > 0) {
            _mutex.lock();
            task = _tileCacheQueue.dequeue();
            _mutex.unlock();
            Q_ASSERT(task);
            qDebug() << "Cache engine Put:" << task->GetPosition().X() << "," << task->GetPosition().Y();
            Cache* cache = Cache::Instance();
            if(cache) {
                cache->ImageCache.PutImageToCache(task->GetImg(), task->GetMapType(), task->GetPosition(), task->GetZoom());
            }
            usleep(44);
            delete task;
        } else {
            qDebug() << "Cache engine BEGIN WAIT";
            _waitmutex.lock();
            _waitc.wait(&_waitmutex);
            qDebug() << "Cache engine WOKE UP";
            if(!this->isRunning()) {
                _waitmutex.unlock();
                break;
            }
            _waitmutex.unlock();
        }
    }
    qDebug() << "Cache Engine Stopped";
}

}
