/*=====================================================================

QGroundControl Open Source Ground Control Station

(c) 2009, 2010 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>

This file is part of the QGROUNDCONTROL project

    QGROUNDCONTROL is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    QGROUNDCONTROL is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.

======================================================================*/

/**
 * @file
 *   @brief Definition of the class WebImageCache.
 *
 *   @author Lionel Heng <hengli@inf.ethz.ch>
 *
 */

#ifndef WEBIMAGECACHE_H
#define WEBIMAGECACHE_H

#include <QNetworkAccessManager>
#include <QObject>
#include <QPair>

#include "WebImage.h"

class WebImageCache : public QObject
{
    Q_OBJECT

public:
    WebImageCache(QObject* parent, int cacheSize);

    QPair<WebImagePtr, int> lookup(const QString& url);

    WebImagePtr at(int index) const;

private Q_SLOTS:
    void downloadFinished(QNetworkReply* reply);

private:
    int mCacheSize;

    QVector<WebImagePtr> mWebImages;
    quint64 mCurrentReference;

    QScopedPointer<QNetworkAccessManager> mNetworkManager;
};

#endif // WEBIMAGECACHE_H
