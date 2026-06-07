// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QNETWORKDISKCACHE_P_H
#define QNETWORKDISKCACHE_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtNetwork/private/qtnetworkglobal_p.h>
#include "private/qabstractnetworkcache_p.h"

#include <qbuffer.h>
#include <qhash.h>
#include <qsavefile.h>

QT_REQUIRE_CONFIG(networkdiskcache);

QT_BEGIN_NAMESPACE

class QCacheItem
{
public:
    QCacheItem() = default;
    ~QCacheItem()
    {
        reset();
    }

    QNetworkCacheMetaData metaData;
    QBuffer data;
    QSaveFile *file = nullptr;
    inline qint64 size() const
        { return file ? file->size() : data.size(); }

    inline void reset() {
        metaData = QNetworkCacheMetaData();
        data.close();
        delete file;
        file = nullptr;
    }
    void writeHeader(QFileDevice *device) const;
    void writeCompressedData(QFileDevice *device) const;
    bool read(QFileDevice *device, bool readData);

    bool canCompress() const;
};

class QNetworkDiskCachePrivate : public QAbstractNetworkCachePrivate
{
public:
    QNetworkDiskCachePrivate()
        : QAbstractNetworkCachePrivate()
        , maximumCacheSize(1024 * 1024 * 50)
        , currentCacheSize(-1)
        {}

    static QString uniqueFileName(const QUrl &url);
    QString cacheFileName(const QUrl &url) const;
    bool removeFile(const QString &file);
    void storeItem(QCacheItem *item);
    void prepareLayout();
    static quint32 crc32(const char *data, uint len);

    mutable QCacheItem lastItem;
    QString cacheDirectory;
    QString dataDirectory;
    qint64 maximumCacheSize;
    qint64 currentCacheSize;

    QHash<QIODevice*, QCacheItem*> inserting;
    Q_DECLARE_PUBLIC(QNetworkDiskCache)
};

QT_END_NAMESPACE

#endif // QNETWORKDISKCACHE_P_H
