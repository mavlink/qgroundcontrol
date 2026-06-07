#pragma once

#include <QtCore/QByteArray>
#include <QtCore/QMetaType>
#include <QtCore/QSharedPointer>
#include <QtCore/QString>

struct QGCCacheTile
{
    QGCCacheTile(const QString& hash_, const QByteArray& img_, const QString& format_, const QString& type_,
                 quint64 tileSet_ = UINT64_MAX)
        : tileSet(tileSet_), hash(hash_), img(img_), format(format_), type(type_)
    {}

    QGCCacheTile(const QString& hash_, quint64 tileSet_) : tileSet(tileSet_), hash(hash_) {}

    quint64 tileSet;
    QString hash;
    QByteArray img;
    QString format;
    QString type;

    // HTTP cache-validation metadata. etag/lastModified drive conditional GETs
    // (If-None-Match / If-Modified-Since); expiresAt is a Unix epoch (seconds,
    // 0 = unknown) past which the tile must be revalidated before reuse.
    QByteArray etag;
    QByteArray lastModified;
    qint64 expiresAt = 0;

    // Set from response Cache-Control no-cache/must-revalidate. When true (or once
    // expiresAt has passed) the cache serves the tile stale to force a conditional GET.
    bool mustRevalidate = false;
};
Q_DECLARE_METATYPE(QGCCacheTile)
Q_DECLARE_METATYPE(QGCCacheTile*)
Q_DECLARE_METATYPE(QSharedPointer<QGCCacheTile>)
