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
 *   @author Lionel Heng <hengli@student.ethz.ch>
 *
 */

#include "WebImageCache.h"

#include <QNetworkReply>
#include <QPixmap>

WebImageCache::WebImageCache(QObject* parent, uint32_t _cacheSize)
    : QObject(parent)
    , cacheSize(_cacheSize)
    , currentReference(0)
    , networkManager(new QNetworkAccessManager)
{
    for (uint32_t i = 0; i < cacheSize; ++i)
    {
        WebImagePtr image(new WebImage);

        webImages.push_back(image);
    }

    connect(networkManager.data(), SIGNAL(finished(QNetworkReply*)),
            this, SLOT(downloadFinished(QNetworkReply*)));
}

QPair<WebImagePtr, int32_t>
WebImageCache::lookup(const QString& url, bool useHeightModel)
{
    QPair<WebImagePtr, int32_t> cacheEntry;

    for (int32_t i = 0; i < webImages.size(); ++i)
    {
        if (webImages[i]->getState() != WebImage::UNINITIALIZED &&
            webImages[i]->getSourceURL() == url &&
            webImages[i]->is3D() == useHeightModel)
        {
            cacheEntry.first = webImages[i];
            cacheEntry.second = i;
            break;
        }
    }

    if (cacheEntry.first.isNull())
    {
        for (int32_t i = 0; i < webImages.size(); ++i)
        {
            // get uninitialized image
            if (webImages[i]->getState() == WebImage::UNINITIALIZED)
            {
                cacheEntry.first = webImages[i];
                cacheEntry.second = i;
                break;
            }
            // get oldest image
            else if (webImages[i]->getState() == WebImage::READY &&
                     (cacheEntry.first.isNull() ||
                      webImages[i]->getLastReference() <
                      cacheEntry.first->getLastReference()))
            {
                cacheEntry.first = webImages[i];
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
            cacheEntry.first->setLastReference(currentReference);
            ++currentReference;
            cacheEntry.first->setState(WebImage::REQUESTED);

            if (url.left(4).compare("http") == 0)
            {
                networkManager->get(QNetworkRequest(QUrl(url)));
            }
            else
            {
                bool success;

                if (useHeightModel)
                {
                    QString heightURL = url;
                    heightURL.replace("color", "dom");
                    heightURL.replace(".jpg", ".txt");

                    success = cacheEntry.first->setData(url, heightURL);
                }
                else
                {
                    success = cacheEntry.first->setData(url);
                }

                if (success)
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
            cacheEntry.first->setLastReference(currentReference);
            ++currentReference;
            return cacheEntry;
        }
        else
        {
            return qMakePair(WebImagePtr(), -1);
        }
    }
}

WebImagePtr
WebImageCache::at(int32_t index) const
{
    return webImages[index];
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
    foreach(image, webImages)
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
