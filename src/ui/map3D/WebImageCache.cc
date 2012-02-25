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

#include "WebImageCache.h"

#include <cstdio>
#include <QNetworkReply>
#include <QPixmap>

WebImageCache::WebImageCache(QObject* parent, int cacheSize)
    : QObject(parent)
    , mCacheSize(cacheSize)
    , mCurrentReference(0)
    , mNetworkManager(new QNetworkAccessManager)
{
    for (int i = 0; i < mCacheSize; ++i)
    {
        WebImagePtr image(new WebImage);

        mWebImages.push_back(image);
    }

    connect(mNetworkManager.data(), SIGNAL(finished(QNetworkReply*)),
            this, SLOT(downloadFinished(QNetworkReply*)));
}

QPair<WebImagePtr, int>
WebImageCache::lookup(const QString& url)
{
    QPair<WebImagePtr, int> cacheEntry;

    for (int i = 0; i < mWebImages.size(); ++i)
    {
        WebImagePtr& image = mWebImages[i];

        if (image->getState() != WebImage::UNINITIALIZED &&
            image->getSourceURL() == url)
        {
            cacheEntry.first = image;
            cacheEntry.second = i;
            break;
        }
    }

    if (cacheEntry.first.isNull())
    {
        for (int i = 0; i < mWebImages.size(); ++i)
        {
            WebImagePtr& image = mWebImages[i];

            // get uninitialized image
            if (image->getState() == WebImage::UNINITIALIZED)
            {
                cacheEntry.first = image;
                cacheEntry.second = i;
                break;
            }
            // get oldest image
            else if (image->getState() == WebImage::READY &&
                     (cacheEntry.first.isNull() ||
                      image->getLastReference() <
                      cacheEntry.first->getLastReference()))
            {
                cacheEntry.first = image;
                cacheEntry.second = i;
            }
        }

        if (cacheEntry.first.isNull())
        {
           return qMakePair(WebImagePtr(), -1);
        }
        else
        {
            if (cacheEntry.first->getState() == WebImage::READY)
            {
                cacheEntry.first->clear();
            }
            cacheEntry.first->setSourceURL(url);
            cacheEntry.first->setLastReference(mCurrentReference);
            ++mCurrentReference;
            cacheEntry.first->setState(WebImage::REQUESTED);

            if (url.left(4).compare("http") == 0)
            {
                mNetworkManager->get(QNetworkRequest(QUrl(url)));
            }
            else
            {
                if (cacheEntry.first->setData(url))
                {
                    cacheEntry.first->setSyncFlag(true);
                    cacheEntry.first->setState(WebImage::READY);
                }
                else
                {
                    cacheEntry.first->setState(WebImage::UNINITIALIZED);
                }
            }

            return cacheEntry;
        }
    }
    else
    {
        if (cacheEntry.first->getState() == WebImage::READY)
        {
            cacheEntry.first->setLastReference(mCurrentReference);
            ++mCurrentReference;
            return cacheEntry;
        }
        else
        {
            return qMakePair(WebImagePtr(), -1);
        }
    }
}

WebImagePtr
WebImageCache::at(int index) const
{
    return mWebImages[index];
}

void
WebImageCache::downloadFinished(QNetworkReply* reply)
{
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError)
    {
        return;
    }
    QVariant attribute = reply->attribute(QNetworkRequest::RedirectionTargetAttribute);
    if (attribute.isValid())
    {
        return;
    }

    WebImagePtr image;
    foreach(image, mWebImages)
    {
        if (reply->url().toString() == image->getSourceURL())
        {
            image->setData(reply->readAll());
            image->setSyncFlag(true);
            image->setState(WebImage::READY);

            return;
        }
    }
}
