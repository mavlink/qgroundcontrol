#pragma once

#include <QtCore/QByteArray>
#include <QtCore/QString>

struct CacheTile
{
    /// \a mapId_ is the id of the provider of the tile (MapProvider::mapId())
    CacheTile(const QString& hash_, const QByteArray& img_, const QString& format_, int mapId_,
              quint64 tileSet_ = UINT64_MAX)
        : tileSet(tileSet_)
        , hash(hash_)
        , img(img_)
        , format(format_)
        , mapId(mapId_)
    {}

    quint64 tileSet;
    QString hash;
    QByteArray img;
    QString format;
    int mapId;
};
