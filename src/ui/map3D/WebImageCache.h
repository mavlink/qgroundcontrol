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
    WebImageCache(QObject* parent, uint32_t cacheSize);

    QPair<WebImagePtr, int32_t> lookup(const QString& url);

    WebImagePtr at(int32_t index) const;

private Q_SLOTS:
    void downloadFinished(QNetworkReply* reply);

private:
    uint32_t cacheSize;

    QVector<WebImagePtr> webImages;
    uint64_t currentReference;

    QScopedPointer<QNetworkAccessManager> networkManager;
};

#endif // WEBIMAGECACHE_H
