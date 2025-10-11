/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QByteArray>
#include <QtCore/QMetaType>
#include <QtCore/QString>

struct QGCCacheTile
{
    QGCCacheTile(const QString &hash_, const QByteArray &img_, const QString &format_, const QString &type_, quint64 tileSet_ = UINT64_MAX)
        : tileSet(tileSet_)
        , hash(hash_)
        , img(img_)
        , format(format_)
        , type(type_)
    {}
    QGCCacheTile(const QString &hash_, quint64 tileSet_)
        : tileSet(tileSet_)
        , hash(hash_)
    {}

    const quint64 tileSet = 0;
    const QString hash;
    const QByteArray img;
    const QString format;
    const QString type;
};
Q_DECLARE_METATYPE(QGCCacheTile)
Q_DECLARE_METATYPE(QGCCacheTile*)
