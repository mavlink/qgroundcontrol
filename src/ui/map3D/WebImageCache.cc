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
WebImageCache::lookup(const QString& url)
{
    QPair<WebImagePtr, int32_t> p;
    for (int32_t i = 0; i < webImages.size(); ++i)
    {
        if (webImages[i]->getState() != WebImage::UNINITIALIZED &&
            webImages[i]->getSourceURL() == url)
        {
            p.first = webImages[i];
            p.second = i;
            break;
        }
    }

    if (p.first.isNull())
    {
        for (int32_t i = 0; i < webImages.size(); ++i)
        {
            // get uninitialized image
            if (webImages[i]->getState() == WebImage::UNINITIALIZED)
            {
                p.first = webImages[i];
                p.second = i;
                break;
            }
            // get oldest image
            else if (webImages[i]->getState() == WebImage::READY &&
                     (p.first.isNull() ||
                      p.first->getLastReference() < p.first->getLastReference()))
            {
                p.first = webImages[i];
                p.second = i;
            }
        }

        if (p.first.isNull())
        {
            return qMakePair(WebImagePtr(), -1);
        }
        else
        {
            if (p.first->getState() == WebImage::READY)
            {
                p.first->clear();
            }
            p.first->setSourceURL(url);
            p.first->setLastReference(currentReference);
            ++currentReference;
            p.first->setState(WebImage::REQUESTED);

            networkManager->get(QNetworkRequest(QUrl(url)));

            return p;
        }
    }
    else
    {
        if (p.first->getState() == WebImage::READY)
        {
            p.first->setLastReference(currentReference);
            ++currentReference;
            return p;
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
    if (reply->error() != QNetworkReply::NoError) {
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
