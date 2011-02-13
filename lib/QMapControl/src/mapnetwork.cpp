/*
*
* This file is part of QMapControl,
* an open-source cross-platform map widget
*
* Copyright (C) 2007 - 2008 Kai Winter
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU Lesser General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public License
* along with QMapControl. If not, see <http://www.gnu.org/licenses/>.
*
* Contact e-mail: kaiwinter@gmx.de
* Program URL   : http://qmapcontrol.sourceforge.net/
*
*/

#include "mapnetwork.h"
#include <QWaitCondition>
namespace qmapcontrol
{
    MapNetwork::MapNetwork(ImageManager* parent)
        :parent(parent), http(new QHttp(this)), loaded(0)
    {
        connect(http, SIGNAL(requestFinished(int, bool)),
                this, SLOT(requestFinished(int, bool)));
    }

    MapNetwork::~MapNetwork()
    {
        http->clearPendingRequests();
        delete http;
    }


    void MapNetwork::loadImage(const QString& host, const QString& url)
    {
        // qDebug() << "getting: " << QString(host).append(url);
        // http->setHost(host);
        // int getId = http->get(url);

        http->setHost(host);
        QHttpRequestHeader header("GET", url);
        header.setValue("User-Agent", "Mozilla");
        header.setValue("Host", host);
        int getId = http->request(header);

        if (vectorMutex.tryLock())
        {
            loadingMap[getId] = url;
            vectorMutex.unlock();
        }
    }

    void MapNetwork::requestFinished(int id, bool error)
    {
        // sleep(1);
        qDebug() << "QMapControl: MapNetwork::requestFinished" << http->state() << ", id: " << id;
        if (error)
        {
            qDebug() << "QMapControl: network error: " << http->errorString();
            //restart query

        }
        else if (vectorMutex.tryLock())
        {
            // check if id is in map?
            if (loadingMap.contains(id))
            {

                QString url = loadingMap[id];
                loadingMap.remove(id);
                vectorMutex.unlock();
                //qDebug() << "QMapControl: request finished for id: " << id << ", belongs to: " << notifier.url << endl;
                QByteArray ax;

                if (http->bytesAvailable()>0)
                {
                    QPixmap pm;
                    ax = http->readAll();

                    qDebug() << "QMapControl: Request consisted of " << ax.size() << "bytes";

                    if (pm.loadFromData(ax))
                    {
                        loaded += pm.size().width()*pm.size().height()*pm.depth()/8/1024;
                        qDebug() << "QMapControl: Network loaded: " << (loaded);
                        parent->receivedImage(pm, url);
                    }
                    else if (pm.width() == 0 || pm.height() == 0)
                    {
                        // Silently ignore map request for a
                        // 0xn pixel map
                        qDebug() << "QMapControl: IGNORED 0x0 pixel map request, widthxheight:" << pm.width() << "x" << pm.height();
                        qDebug() << "QMapControl: HTML ERROR MESSAGE:" << ax << "at " << __FILE__ << __LINE__;
                    }
                    else
                    {
                        // QGC FIXME Error is currently undetected
                        // TODO Error is currently undetected
                        //qDebug() << "NETWORK_PIXMAP_ERROR: " << ax;
                        qDebug() << "QMapControl external library: ERROR loading map:" << "width:" << pm.width() << "heigh:" << pm.height() << "at " << __FILE__ << __LINE__;
                        qDebug() << "QMapControl: HTML ERROR MESSAGE:" << ax << "at " << __FILE__ << __LINE__;
                    }
                }

            }
            else
                vectorMutex.unlock();

        }
        if (loadingMap.size() == 0)
        {
            // qDebug () << "all loaded";
            parent->loadingQueueEmpty();
        }
    }

    void MapNetwork::abortLoading()
    {
	http->clearPendingRequests();
        if (vectorMutex.tryLock())
        {
            loadingMap.clear();
            vectorMutex.unlock();
        }
    }

    bool MapNetwork::imageIsLoading(QString url)
    {
        return loadingMap.values().contains(url);
    }

    void MapNetwork::setProxy(QString host, int port)
    {
#ifndef Q_WS_QWS
        // do not set proxy on qt/extended
        http->setProxy(host, port);
#endif
    }
}
