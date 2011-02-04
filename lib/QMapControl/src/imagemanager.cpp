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

#include "imagemanager.h"
namespace qmapcontrol
{
    ImageManager* ImageManager::m_Instance = 0;
    ImageManager::ImageManager(QObject* parent)
        :QObject(parent), emptyPixmap(QPixmap(1,1)), net(new MapNetwork(this)), doPersistentCaching(false)
    {
        emptyPixmap.fill(Qt::transparent);

        if (QPixmapCache::cacheLimit() <= 20000)
        {
            QPixmapCache::setCacheLimit(20000);	// in kb
        }
    }


    ImageManager::~ImageManager()
    {
	if (ImageManager::m_Instance != 0)
	{
	    delete ImageManager::m_Instance;
	}
        delete net;
    }

    QPixmap ImageManager::getImage(const QString& host, const QString& url)
    {
        //qDebug() << "ImageManager::getImage";
        QPixmap pm;
        //pm.fill(Qt::black);

        //is image cached (memory) or currently loading?
        if (!QPixmapCache::find(url, pm) && !net->imageIsLoading(url))
            //	if (!images.contains(url) && !net->imageIsLoading(url))
        {
            //image cached (persistent)?
            if (doPersistentCaching && tileExist(url))
            {
                loadTile(url,pm);
                QPixmapCache::insert(url.toAscii().toBase64(), pm);
            }
            else
            {
                //load from net, add empty image
                net->loadImage(host, url);
                //QPixmapCache::insert(url, emptyPixmap);
                return emptyPixmap;
            }
        }
        return pm;
    }

    QPixmap ImageManager::prefetchImage(const QString& host, const QString& url)
    {
#ifdef Q_WS_QWS
        // on mobile devices we don´t want the display resfreshing when tiles are received which are
        // prefetched... This is a performance issue, because mobile devices are very slow in
        // repainting the screen
        prefetch.append(url);
#endif
        return getImage(host, url);
    }

    void ImageManager::receivedImage(const QPixmap pixmap, const QString& url)
    {
        //qDebug() << "ImageManager::receivedImage";
        QPixmapCache::insert(url, pixmap);
        //images[url] = pixmap;

        // needed?
        if (doPersistentCaching && !tileExist(url) )
            saveTile(url,pixmap);

        //((Layer*)this->parent())->imageReceived();

        if (!prefetch.contains(url))
        {
            emit(imageReceived());
        }
        else
        {

#ifdef Q_WS_QWS
            prefetch.remove(prefetch.indexOf(url));
#endif
        }
    }

    void ImageManager::loadingQueueEmpty()
    {
        emit(loadingFinished());
        //((Layer*)this->parent())->removeZoomImage();
        //qDebug() << "size of image-map: " << images.size();
        //qDebug() << "size: " << QPixmapCache::cacheLimit();
    }

    void ImageManager::abortLoading()
    {
        net->abortLoading();
    }
    void ImageManager::setProxy(QString host, int port)
    {
        net->setProxy(host, port);
    }

    void ImageManager::setCacheDir(const QDir& path)
    {
        doPersistentCaching = true;
        cacheDir = path;
        if (!cacheDir.exists())
        {
            cacheDir.mkpath(cacheDir.absolutePath());
        }
    }

    bool ImageManager::saveTile(QString tileName,QPixmap tileData)
    {
        tileName.replace("/","-");

        QFile file(cacheDir.absolutePath() + "/" + tileName.toAscii().toBase64());

        //qDebug() << "writing: " << file.fileName();
        if (!file.open(QIODevice::ReadWrite )){
            qDebug()<<"error reading file";
            return false;
        }
        QByteArray bytes;
        QBuffer buffer(&bytes);
        buffer.open(QIODevice::WriteOnly);
        tileData.save(&buffer, "PNG");
        // FIXME This is weird - why first write in buffer and then in file?
        file.write(bytes);
        file.close();
        return true;
    }
    bool ImageManager::loadTile(QString tileName,QPixmap &tileData)
    {
        tileName.replace("/","-");
        QFile file(cacheDir.absolutePath() + "/" + tileName.toAscii().toBase64());
        if (!file.open(QIODevice::ReadOnly )) {
            return false;
        }
        tileData.loadFromData( file.readAll() );

        file.close();
        return true;
    }
    bool ImageManager::tileExist(QString tileName)
    {
        tileName.replace("/","-");
        QFile file(cacheDir.absolutePath() + "/" + tileName.toAscii().toBase64());
        if (file.exists())
            return true;
        else
            return false;
    }
}
