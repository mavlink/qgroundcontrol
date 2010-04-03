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

#ifndef MAPNETWORK_H
#define MAPNETWORK_H

#include <QObject>
#include <QDebug>
#include <QHttp>
#include <QVector>
#include <QPixmap>
#include "imagemanager.h"
/**
        @author Kai Winter <kaiwinter@gmx.de>
 */
namespace qmapcontrol
{
    class ImageManager;
    class MapNetwork : QObject
    {
        Q_OBJECT

    public:
        MapNetwork(ImageManager* parent);
        ~MapNetwork();

        void loadImage(const QString& host, const QString& url);

        /*!
         * checks if the given url is already loading
         * @param url the url of the image
         * @return boolean, if the image is already loading
         */
        bool imageIsLoading(QString url);

        /*!
         * Aborts all current loading threads.
         * This is useful when changing the zoom-factor, though newly needed images loads faster
         */
        void abortLoading();
        void setProxy(QString host, int port);

    private:
        ImageManager* parent;
        QHttp* http;
        QMap<int, QString> loadingMap;
        qreal loaded;
        QMutex vectorMutex;
        MapNetwork& operator=(const MapNetwork& rhs);
        MapNetwork(const MapNetwork& old);

    private slots:
        void requestFinished(int id, bool error);
    };
}
#endif
