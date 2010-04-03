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

#ifndef IMAGEMANAGER_H
#define IMAGEMANAGER_H

#include <QObject>
#include <QPixmapCache>
#include <QDebug>
#include <QMutex>
#include <QFile>
#include <QBuffer>
#include <QDir>
#include "mapnetwork.h"

namespace qmapcontrol
{
    class MapNetwork;
    /**
    @author Kai Winter <kaiwinter@gmx.de>
     */
    class ImageManager : public QObject
    {
        Q_OBJECT;

    public:
        static ImageManager* instance()
        {
            if(!m_Instance)
            {
                m_Instance = new ImageManager;
            }
            return m_Instance;
        }

        ~ImageManager();
        
        //! returns a QPixmap of the asked image
        /*!
         * If this component doesn´t have the image a network query gets started to load it.
         * @param host the host of the image
         * @param path the path to the image
         * @return the pixmap of the asked image
         */
        QPixmap getImage(const QString& host, const QString& path);

        QPixmap prefetchImage(const QString& host, const QString& path);

        void receivedImage(const QPixmap pixmap, const QString& url);

        /*!
         * This method is called by MapNetwork, after all images in its queue were loaded.
         * The ImageManager emits a signal, which is used in MapControl to remove the zoom image.
         * The zoom image should be removed on Tile Images with transparency.
         * Else the zoom image stay visible behind the newly loaded tiles.
         */
        void loadingQueueEmpty();

        /*!
         * Aborts all current loading threads.
         * This is useful when changing the zoom-factor, though newly needed images loads faster
         */
        void abortLoading();

        //! sets the proxy for HTTP connections
        /*!
         * This method sets the proxy for HTTP connections.
         * This is not provided by the current Qtopia version!
         * @param host the proxy´s hostname or ip
         * @param port the proxy´s port
         */
        void setProxy(QString host, int port);

        //! sets the cache directory for persistently saving map tiles
        /*!
         *
         * @param path the path where map tiles should be stored
         * @todo add maximum size
         */
        void setCacheDir(const QDir& path);

    private:
        ImageManager(QObject* parent = 0);
        ImageManager(const ImageManager&);
        ImageManager& operator=(const ImageManager&);
        QPixmap emptyPixmap;
        MapNetwork* net;
        QVector<QString> prefetch;
        QDir cacheDir;
        bool doPersistentCaching;

        static ImageManager* m_Instance;

        bool saveTile(QString tileName,QPixmap tileData);
        bool loadTile(QString tileName,QPixmap &tileData);
        bool tileExist(QString tileName);

    signals:
        void imageReceived();
        void loadingFinished();
    };
}
#endif
